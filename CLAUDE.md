# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

HIME is a lightweight input method editor (IME) framework for Linux supporting multiple input methods including Cantonese (Cangjie, Jyutping), Mandarin (Zhuyin/Bopomofo, Pinyin), Vietnamese (Telex), Japanese (Anthy), Korean, and others. It provides input method modules for GTK2, GTK3, Qt5, and Qt6 applications.

## Build Commands

### Linux

```bash
# Prerequisites (Debian/Ubuntu)
sudo apt-get install -y libgtk2.0-dev libxtst-dev autoconf automake gettext

# Initial setup (required after clone or configure.ac changes)
autoreconf -i
./configure

# Build
make -j$(nproc)

# Install
make install

# Code formatting (REQUIRED before submitting PR)
make clang-format
```

If `configure` fails with "bzip2 not found", create `pkgconfig/bzip2.pc`:
```bash
mkdir -p pkgconfig
cat > pkgconfig/bzip2.pc << 'EOF'
prefix=/usr
exec_prefix=${prefix}
libdir=${exec_prefix}/lib/x86_64-linux-gnu
includedir=${prefix}/include
Name: bzip2
Description: bzip2 compression library
Version: 1.0.8
Libs: -L${libdir} -lbz2
Cflags: -I${includedir}
EOF
export PKG_CONFIG_PATH=$(pwd)/pkgconfig:$PKG_CONFIG_PATH
./configure
```

### Configure Options

- `--with-gtk=[2.0|3.0]` - Select GTK version (default: 2.0)
- `--enable-qt5-immodule` - Enable Qt5 input method module
- `--enable-qt6-immodule` - Enable Qt6 input method module (requires Qt6 moc)
- `--disable-qt6-immodule` - Disable Qt6 if moc not found
- `--enable-anthy` - Enable Japanese Anthy support
- `--enable-chewing` - Enable Chewing library support

### Windows (Cross-compile from Linux)

```bash
# Prerequisites (Debian/Ubuntu)
sudo apt-get install -y mingw-w64 cmake

# Build (Debug)
cd platform/windows
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Build (Release — no debug logging, no debug tag)
cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests (via Wine or on Windows)
./bin/test-hime-core.exe
```

Output in `platform/windows/build/bin/`: `hime-core.dll`, `hime-tsf.dll`, `test-hime-core.exe`, `data/`.

See `platform/windows/sandbox/README.md` for Windows Sandbox testing workflow.

### Android

Prerequisites:
- **JDK 17** (required — JDK 21 has `jlink` compatibility issues with AGP 8.2)
- **Android SDK** (command-line tools or Android Studio)
- **Android NDK** 25.2.9519653

```bash
# Install prerequisites (Debian/Ubuntu)
apt-get install -y openjdk-17-jdk-headless

# Install Android SDK (command-line tools)
mkdir -p ~/android-sdk && cd ~/android-sdk
curl -sL https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -o cmdline-tools.zip
unzip -qo cmdline-tools.zip
mkdir -p cmdline-tools/latest
cp -r cmdline-tools/bin cmdline-tools/lib cmdline-tools/latest/

# Accept licenses and install SDK components
printf 'y\ny\ny\ny\ny\ny\ny\ny\n' | bash ~/android-sdk/cmdline-tools/latest/bin/sdkmanager --sdk_root=~/android-sdk --licenses
bash ~/android-sdk/cmdline-tools/latest/bin/sdkmanager --sdk_root=~/android-sdk \
  "platform-tools" "platforms;android-34" "build-tools;34.0.0" "ndk;25.2.9519653"

# Set environment
export ANDROID_HOME=~/android-sdk
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

# Create local.properties (one-time)
cd /path/to/hime/platform/android
echo "sdk.dir=$ANDROID_HOME" > local.properties

# Generate gradlew if missing (requires Gradle 7+)
# Download Gradle 8.4 if system gradle is too old:
#   curl -sL https://services.gradle.org/distributions/gradle-8.4-bin.zip -o /tmp/gradle.zip
#   unzip -qo /tmp/gradle.zip -d ~/gradle
#   ~/gradle/gradle-8.4/bin/gradle wrapper
gradle wrapper 2>/dev/null || ~/gradle/gradle-8.4/bin/gradle wrapper

# Build debug APK
./gradlew assembleDebug

# Build release APK
./gradlew assembleRelease

# Install on connected device
./gradlew installDebug
```

Output: `platform/android/app/build/outputs/apk/debug/app-debug.apk`

**Known issues:**
- JDK 21 causes `jlink` errors with AGP 8.2 — use JDK 17
- `android.useAndroidX=true` must be in `gradle.properties`
- `ANDROID_SDK_ROOT` and `ANDROID_HOME` must not conflict — use `local.properties` for `sdk.dir`

### macOS (native build only)

```bash
# Requires Xcode with Input Method Kit framework
cd platform/macos
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)

# Install
sudo cp -r HIME.app /Library/Input\ Methods/
# Log out and log back in
```

### Shared Tests

```bash
cd shared/tests
make
./test-hime-core    # Runs 98 tests
```

## Code Style

- Uses clang-format-14 with GNU/GTK-based style (see `.clang-format`)
- Run `make clang-format` before committing - CI will fail otherwise
- Git pre-commit hook available in `git-hooks/pre-commit`

## Build System Structure

The project uses autoconf/automake. Key files:

- `configure.ac` - Main autoconf configuration
- `src/Makefile.am` - Main source build rules
- `src/suffixes-rule.mk` - Common pattern rules included by sub-Makefiles

### Common Object Groups (src/Makefile.am)

Object files are organized into reusable groups to reduce duplication:

```makefile
OBJS_CORE       # locale.o util.o - used by almost all programs
OBJS_HIME_CONF  # hime-common.o hime-conf.o hime-settings.o
OBJS_PHO_BASE   # pho-dbg.o pho-sym.o pho2pinyin.o pinyin.o
OBJS_PHO_UTIL   # $(OBJS_PHO_BASE) + pho-util.o table-update.o
OBJS_GTAB_UTIL  # gtab-util.o
```

When adding new tools, compose from these groups rather than listing individual objects.

## Architecture

### Daemon-Client Model

HIME runs as a server daemon (`hime`) that handles input method requests from applications via:
- **XIM protocol** - X11 Input Method protocol (IMdkit/)
- **GTK IM modules** - libhime-gtk.so (GTK2), libhime-gtk3.so (GTK3)
- **Qt IM modules** - Platform input context plugins for Qt5/Qt6

### Source Structure (src/)

| Component | Files | Purpose |
|-----------|-------|---------|
| Core daemon | hime.c, im-srv.c, im-dispatch.c | Main IME server and dispatch |
| Input context | IC.c | Input context management |
| Zhuyin input | pho.c, pho-util.c | Bopomofo/Zhuyin input method |
| Table input | gtab.c, gtab-*.c | Generic table-based input (Cangjie, Array, etc.) |
| Phrase input | tsin.c, tsin-*.c | Phrase/word input and learning |
| Configuration | hime-setup.c, hime-conf.c | GUI settings and config handling |
| GTK modules | gtk-im/, gtk3-im/ | GTK2/GTK3 input method modules |
| Qt modules | qt5-im/, qt6-im/ | Qt5/Qt6 input method modules |
| XIM protocol | IMdkit/ | X Input Method protocol implementation |
| Client library | im-client/ | Client library for applications |

### Key Binaries

- `hime` - Core IME daemon
- `hime-setup` - GUI configuration tool
- `hime-cin2gtab` / `hime-gtab2cin` - Convert between .cin and .gtab table formats
- `hime-tslearn` - Phrase learning utility
- `hime-ts-edit` - Phrase editor

## Data Files

Input method tables are in `data/` directory:
- `.cin` files - Human-readable input tables (Cangjie: cj.cin, cj5.cin; Array: ar30.cin)
- `.gtab` files - Compiled binary tables (generated by hime-cin2gtab)
- `gtab.list.in` - Configuration for available input method tables

## Scripts

Utility scripts in `scripts/`:

- `scripts/liu-tab2cin.py` - Convert Boshiamy (嘸蝦米) `liu-uni.tab` to `.cin` and `.gtab`
- `icons/gen-platform-icons.py` - Generate platform-specific icons from source PNGs

### Converting Boshiamy Tables

```bash
# Convert liu-uni.tab to .cin only
python3 scripts/liu-tab2cin.py /path/to/liu-uni.tab -o data/liu.cin

# Convert to .cin and .gtab (requires hime-cin2gtab to be built first)
python3 scripts/liu-tab2cin.py /path/to/liu-uni.tab -o data/liu.cin --gtab

# Custom table name
python3 scripts/liu-tab2cin.py /path/to/liu-uni.tab -o data/liu.cin \
    --ename liu --cname 嘸蝦米 --gtab
```

Source files are typically at `C:\Program Files\BoshiamyTIP\liu-uni.tab` on Windows.
Supports `liu-uni.tab` through `liu-uni4.tab`.

### Generating Platform Icons

```bash
# Requires: pip install Pillow
cd icons && python3 gen-platform-icons.py
```

Generates Android mipmap, iOS AppIcon, macOS iconset, and Windows .ico files
from `icons/hime.png` and `icons/blue/*.png`.

## CI/CD and Releases

### GitHub Actions Workflows

| Workflow | File | Trigger |
|----------|------|---------|
| Build (legacy) | `.github/workflows/build.yaml` | Every push and PR (format check + distro containers) |
| Build All Platforms | `.github/workflows/build.yml` | Push/PR to `master` (Linux, Windows, Android, macOS, iOS) |
| Release | `.github/workflows/release.yml` | Push of a `v*` tag |
| CodeQL | `.github/workflows/codeql-analysis.yml` | Security scanning |

### Creating a Release

The release workflow is triggered by pushing a Git tag that starts with `v`.
It builds all platforms, runs tests, packages artifacts, and creates a GitHub
Release with auto-generated release notes.

```bash
# 1. Make sure master is clean and all CI checks pass
git checkout master
git pull

# 2. Tag the release (use semver: vMAJOR.MINOR.PATCH)
git tag v0.5.0

# 3. Push the tag — this triggers the release workflow
git push origin v0.5.0
```

**What the release workflow does** (`.github/workflows/release.yml`):

1. **build-linux** — `autoreconf && configure && make`, runs shared tests, packages `hime-VERSION-linux-x86_64.tar.gz`
2. **build-windows** — Cross-compiles with MinGW, packages `hime-VERSION-windows-x86_64.zip` (includes installer/uninstaller)
3. **build-android** — Builds release APK with Gradle + NDK, produces `hime-VERSION-android.apk`
4. **build-macos** — Native CMake build + ad-hoc code sign, creates `hime-VERSION-macos.dmg`
5. **create-release** — Waits for all 4 builds, downloads artifacts, creates a GitHub Release via `softprops/action-gh-release@v2` with all binaries attached

The version string is derived from the tag name with the `v` prefix stripped
(`${GITHUB_REF_NAME#v}`).

### Monitoring a Release Build

```bash
# Watch the workflow run after pushing a tag
gh run list --workflow=release.yml --limit=5

# View a specific run
gh run view <run-id>

# Download artifacts from a run
gh run download <run-id>
```

### Deleting a Tag / Re-releasing

If a release build fails and you need to re-tag:

```bash
# Delete remote tag
git push origin :refs/tags/v0.5.0

# Delete local tag
git tag -d v0.5.0

# Fix the issue, then re-tag and push
git tag v0.5.0
git push origin v0.5.0
```

## Distribution Builds

Package scripts in `distro/`:
- `distro/debian/gen-deb` - Generate .deb package
- `distro/archlinux/makepkg.sh` - Arch Linux build
- `distro/fedora/gen-rpm` - RPM package build

## Windows Sandbox MCP Server

An MCP server enables Claude Code to interact with a running Windows Sandbox
for automated UI testing. See `platform/windows/sandbox/README.md` for details.

The server is registered in `.mcp.json` at the project root and provides 6 tools:
`sandbox_exec`, `sandbox_screenshot`, `sandbox_read_file`, `sandbox_click`,
`sandbox_type`, and `sandbox_key`.
