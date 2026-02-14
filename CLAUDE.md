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
cd windows
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Build (Release — no debug logging, no debug tag)
cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests (via Wine or on Windows)
./bin/test-hime-core.exe
```

Output in `windows/build/bin/`: `hime-core.dll`, `hime-tsf.dll`, `test-hime-core.exe`, `data/`.

See `windows/sandbox/README.md` for Windows Sandbox testing workflow.

### macOS (native build only)

```bash
# Requires Xcode with Input Method Kit framework
cd macos
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

## Distribution Builds

Package scripts in `distro/`:
- `distro/debian/gen-deb` - Generate .deb package
- `distro/archlinux/makepkg.sh` - Arch Linux build
- `distro/fedora/gen-rpm` - RPM package build
