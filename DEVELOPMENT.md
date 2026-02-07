# HIME Development Environment Setup

This guide helps developers set up their environment for building and contributing to HIME.

## Quick Start

```bash
# 1. Install dependencies (see platform-specific instructions below)
# 2. Clone and build
git clone https://github.com/hime-ime/hime.git
cd hime
autoreconf -i
./configure
make -j$(nproc)
```

## Platform-Specific Setup

### Debian / Ubuntu

**Minimal build (GTK2 only):**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    autoconf \
    automake \
    libtool \
    gettext \
    pkg-config \
    libgtk2.0-dev \
    libxtst-dev \
    libx11-dev
```

**Full build (all features):**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    autoconf \
    automake \
    libtool \
    gettext \
    pkg-config \
    libgtk2.0-dev \
    libgtk-3-dev \
    libxtst-dev \
    libx11-dev \
    qtbase5-private-dev \
    qt6-base-private-dev \
    libanthy-dev \
    libchewing3-dev \
    libappindicator-dev \
    libappindicator3-dev
```

**Development tools (recommended):**
```bash
sudo apt-get install -y \
    clang-format-14 \
    gdb \
    valgrind
```

### Fedora / RHEL / CentOS

**Minimal build:**
```bash
sudo dnf install -y \
    gcc \
    gcc-c++ \
    make \
    autoconf \
    automake \
    libtool \
    gettext-devel \
    pkgconfig \
    gtk2-devel \
    libXtst-devel \
    libX11-devel
```

**Full build:**
```bash
sudo dnf install -y \
    gcc \
    gcc-c++ \
    make \
    autoconf \
    automake \
    libtool \
    gettext-devel \
    pkgconfig \
    gtk2-devel \
    gtk3-devel \
    libXtst-devel \
    libX11-devel \
    qt5-qtbase-private-devel \
    qt6-qtbase-private-devel \
    anthy-devel \
    libchewing-devel \
    libappindicator-devel \
    libappindicator-gtk3-devel
```

### Arch Linux / Manjaro

**Minimal build:**
```bash
sudo pacman -S --needed \
    base-devel \
    autoconf \
    automake \
    libtool \
    gettext \
    pkgconf \
    gtk2 \
    libxtst \
    libx11
```

**Full build:**
```bash
sudo pacman -S --needed \
    base-devel \
    autoconf \
    automake \
    libtool \
    gettext \
    pkgconf \
    gtk2 \
    gtk3 \
    libxtst \
    libx11 \
    qt5-base \
    qt6-base \
    anthy \
    libchewing
```

### FreeBSD

```bash
sudo pkg install \
    autoconf \
    automake \
    libtool \
    gettext \
    pkgconf \
    gtk2 \
    libXtst \
    gmake
```

Note: Use `gmake` instead of `make` on FreeBSD.

### openSUSE

```bash
sudo zypper install -y \
    gcc \
    gcc-c++ \
    make \
    autoconf \
    automake \
    libtool \
    gettext-tools \
    pkg-config \
    gtk2-devel \
    libXtst-devel \
    libX11-devel
```

## Build Instructions

### Standard Build

```bash
# Generate configure script (required after clone or configure.ac changes)
autoreconf -i

# Configure with default options (GTK2)
./configure

# Build
make -j$(nproc)

# Optional: Install system-wide
sudo make install
```

### Configure Options

| Option | Description |
|--------|-------------|
| `--prefix=/path` | Installation prefix (default: /usr/local) |
| `--with-gtk=2.0` | Use GTK2 (default) |
| `--with-gtk=3.0` | Use GTK3 |
| `--disable-qt5-immodule` | Disable Qt5 input method module |
| `--disable-qt6-immodule` | Disable Qt6 input method module |
| `--disable-anthy` | Disable Anthy Japanese input |
| `--disable-chewing` | Disable Chewing input |
| `--enable-lib64` | Force lib64 directory |

### Example Configurations

**GTK3 with Qt5 support:**
```bash
./configure --with-gtk=3.0
```

**Minimal build (no optional features):**
```bash
./configure \
    --disable-qt5-immodule \
    --disable-qt6-immodule \
    --disable-anthy \
    --disable-chewing
```

**Custom install location:**
```bash
./configure --prefix=$HOME/.local
make
make install
```

## Development Workflow

### Code Formatting

HIME uses clang-format for consistent code style. **Always format before committing:**

```bash
# Format all source files
make clang-format

# Or format specific files
clang-format-14 -i src/your-file.c
```

The CI will reject PRs with formatting issues.

### Git Hooks

Install the pre-commit hook to auto-format:

```bash
cp git-hooks/pre-commit .git/hooks/
chmod +x .git/hooks/pre-commit
```

### Running Locally

To test without system installation:

```bash
# Build
make

# Set environment variables
export HIME_TABLE_DIR=$PWD/data
export LD_LIBRARY_PATH=$PWD/src:$LD_LIBRARY_PATH

# Run hime
./src/hime &

# Run setup tool
./src/hime-setup
```

### Debugging

**With GDB:**
```bash
gdb --args ./src/hime
```

**With Valgrind (memory checking):**
```bash
valgrind --leak-check=full ./src/hime
```

**Enable debug output:**
```bash
HIME_DEBUG=1 ./src/hime
```

## Directory Structure

```
hime/
├── configure.ac      # Autoconf configuration
├── Makefile.am       # Top-level automake file
├── src/              # Source code
│   ├── Makefile.am   # Main build rules
│   ├── hime.c        # Main daemon
│   ├── gtk-im/       # GTK2 IM module
│   ├── gtk3-im/      # GTK3 IM module
│   ├── qt5-im/       # Qt5 IM module
│   ├── qt6-im/       # Qt6 IM module
│   ├── im-client/    # Client library
│   ├── IMdkit/       # XIM protocol
│   └── modules/      # Input method modules (anthy, chewing)
├── data/             # Input method tables (.cin, .gtab)
├── icons/            # Application icons
├── po/               # Translations
├── distro/           # Distribution packaging
│   ├── debian/
│   ├── archlinux/
│   ├── fedora/
│   └── ...
└── scripts/          # Utility scripts
```

## Dependency Reference

### Required Dependencies

| Dependency | pkg-config | Purpose |
|------------|------------|---------|
| GTK+ 2 or 3 | `gtk+-2.0` / `gtk+-3.0` | GUI toolkit |
| Xtst | `xtst` | X11 test extension |
| X11 | `x11` | X Window System |
| GLib | `glib-2.0` | Core utilities (bundled with GTK) |

### Optional Dependencies

| Dependency | pkg-config | Purpose | Configure flag |
|------------|------------|---------|----------------|
| Qt5 | `Qt5Core Qt5Gui` | Qt5 IM module | `--enable-qt5-immodule` |
| Qt6 | (hardcoded) | Qt6 IM module | `--enable-qt6-immodule` |
| Anthy | `anthy` | Japanese input | `--enable-anthy` |
| Chewing | `chewing` | Zhuyin input | `--enable-chewing` |
| AppIndicator | `appindicator-0.1` | System tray | auto-detected |

### Checking Dependencies

```bash
# Check if a package is available
pkg-config --exists gtk+-2.0 && echo "GTK2 found"
pkg-config --exists gtk+-3.0 && echo "GTK3 found"
pkg-config --exists xtst && echo "Xtst found"

# Get compiler flags for a package
pkg-config --cflags --libs gtk+-2.0

# List all available packages
pkg-config --list-all | grep -i gtk
```

## Troubleshooting

### Quick Diagnosis Checklist

When the build fails, check these in order:

1. **Are autotools installed?** → `which autoreconf automake autoconf`
2. **Are dev packages installed?** → `pkg-config --exists gtk+-2.0 && echo OK`
3. **Are there line ending issues?** → `file configure` (should say "ASCII text", not "CRLF")
4. **What's the actual error?** → Check `config.log` for configure errors
5. **Missing symbols at link time?** → Check if functions are incorrectly marked `static`

### autoreconf fails

**Error:** `autoreconf: command not found`

**Solution:** Install autotools:
```bash
# Debian/Ubuntu
sudo apt-get install autoconf automake libtool

# Fedora
sudo dnf install autoconf automake libtool

# Arch
sudo pacman -S autoconf automake libtool
```

### configure fails with missing packages

**Error:** `Package 'gtk+-2.0' not found`

**Solution:** Install development packages (not just runtime):
```bash
# Debian/Ubuntu: packages end with -dev
sudo apt-get install libgtk2.0-dev

# Fedora: packages end with -devel
sudo dnf install gtk2-devel

# Arch: development files included in main package
sudo pacman -S gtk2
```

### Qt moc not found

**Error:** `Can not find moc for QT5`

**Solution:** Either disable Qt support or install Qt development tools:
```bash
# Disable Qt
./configure --disable-qt5-immodule --disable-qt6-immodule

# Or install Qt (Debian/Ubuntu)
sudo apt-get install qtbase5-private-dev

# Or specify moc path manually
./configure --with-qt5-moc-path=/usr/lib/qt5/bin/moc
```

### Permission denied during install

**Error:** `cannot create directory '/usr/local/bin'`

**Solution:** Use sudo or install to user directory:
```bash
# System-wide install
sudo make install

# Or user directory
./configure --prefix=$HOME/.local
make install
```

### hime doesn't start

1. Check if another instance is running:
   ```bash
   pkill hime
   ```

2. Check for missing libraries:
   ```bash
   ldd ./src/hime | grep "not found"
   ```

3. Run with debug output:
   ```bash
   HIME_DEBUG=1 ./src/hime
   ```

### CRLF line ending issues

**Error:** `./configure: cannot execute: required file not found` (but file exists)

**Cause:** The configure script has Windows-style line endings (CRLF).

**Diagnosis:**
```bash
file configure
# Bad: "ASCII text, with CRLF line terminators"
# Good: "POSIX shell script, ASCII text executable"
```

**Solution:**
```bash
# Fix line endings
sed -i 's/\r$//' configure

# Or use dos2unix if available
dos2unix configure
```

**Prevention:** Configure git to handle line endings:
```bash
git config --global core.autocrlf input  # On Linux/Mac
```

### Missing transitive dependencies (bzip2, etc.)

**Error:** `Package 'bzip2' not found` (when checking for GTK)

**Cause:** GTK depends on freetype2, which depends on bzip2. The bzip2 library is installed but missing its `.pc` file.

**Diagnosis:**
```bash
# Check what's requiring bzip2
pkg-config --print-requires freetype2

# Check if library exists but .pc is missing
ls /usr/lib/x86_64-linux-gnu/libbz2* && ls /usr/lib/*/pkgconfig/bzip2.pc
```

**Solution 1:** Install the development package:
```bash
# Debian/Ubuntu
sudo apt-get install libbz2-dev

# Fedora
sudo dnf install bzip2-devel
```

**Solution 2:** Create a local `.pc` file if you can't install packages:
```bash
mkdir -p ~/pkgconfig
cat > ~/pkgconfig/bzip2.pc << 'EOF'
prefix=/usr
exec_prefix=${prefix}
libdir=${exec_prefix}/lib/x86_64-linux-gnu
includedir=${prefix}/include

Name: bzip2
Description: A file compression library
Version: 1.0.8
Libs: -L${libdir} -lbz2
Cflags: -I${includedir}
EOF

# Use it
export PKG_CONFIG_PATH=~/pkgconfig:$PKG_CONFIG_PATH
./configure
```

### Linker errors: undefined reference

**Error:** `undefined reference to 'function_name'`

**Cause:** Usually one of:
1. A function is marked `static` but called from another file
2. An object file is missing from the link command
3. Library order issue

**Diagnosis:**
```bash
# Find where the function is defined
grep -rn "function_name" src/*.c src/*.h

# Check if it's marked static
grep -n "static.*function_name" src/*.c

# Check if the object file is being linked
grep "function_name" src/Makefile.am
```

**Solution:** If a function needs to be called from other files, remove `static`:
```c
// Before (wrong if called from other files)
static void my_function(void) { ... }

// After (correct)
void my_function(void) { ... }
```

### Configure succeeds but make fails

**Diagnosis workflow:**

```bash
# 1. Get the actual error (not just the last line)
make 2>&1 | head -100

# 2. For compiler errors, check the exact command
make V=1  # Verbose mode shows actual commands

# 3. For missing headers
grep -r "include.*missing_header" src/

# 4. For linking errors, check symbol visibility
nm src/problematic.o | grep function_name
# 'T' = defined, 'U' = undefined
```

### pkg-config debugging

**Check what pkg-config sees:**
```bash
# List search paths
pkg-config --variable pc_path pkg-config

# Debug a specific package
PKG_CONFIG_DEBUG_SPEW=1 pkg-config --cflags gtk+-2.0

# Check package dependencies
pkg-config --print-requires gtk+-2.0
pkg-config --print-requires-private freetype2

# Verify a package works
pkg-config --exists gtk+-2.0 && echo "OK" || echo "FAIL"
```

**Common pkg-config issues:**
| Symptom | Cause | Fix |
|---------|-------|-----|
| Package not found | Dev package not installed | Install `-dev` or `-devel` package |
| Version too old | System package outdated | Update system or build from source |
| Wrong pkg-config | Multiple pkg-config versions | Check `which pkg-config` |
| Missing .pc file | Library installed without .pc | Create .pc manually (see above) |

### Reading config.log

When `./configure` fails, check `config.log`:

```bash
# Find the actual error
grep -A5 "error:" config.log | head -20

# Find what configure tried
grep "checking for" config.log | tail -20

# Find pkg-config commands
grep "pkg-config" config.log
```

### Build in a clean environment

If you're getting strange errors, try a clean build:

```bash
# Full clean
make distclean 2>/dev/null
rm -rf autom4te.cache
rm -f config.status config.log

# Regenerate everything
autoreconf -fi
./configure
make -j$(nproc)
```

## Advanced: Understanding the Build System

### How autotools works

```
configure.ac  ──autoreconf──►  configure  ──./configure──►  Makefile
Makefile.am   ──────────────►  Makefile.in ────────────────►
```

1. `configure.ac` - Defines what to check (libraries, features)
2. `Makefile.am` - Defines what to build (sources, targets)
3. `autoreconf -i` - Generates `configure` and `Makefile.in`
4. `./configure` - Checks system and generates `Makefile`
5. `make` - Compiles using generated `Makefile`

### Key files to understand

| File | Purpose | When to modify |
|------|---------|----------------|
| `configure.ac` | Autoconf input | Adding dependencies, features |
| `src/Makefile.am` | Automake input | Adding source files, targets |
| `config.log` | Configure debug output | Debugging configure failures |
| `src/Makefile` | Generated makefile | Never (regenerated) |

### Adding a new source file

1. Edit `src/Makefile.am`
2. Add `.c` file to appropriate `_SOURCES` variable
3. Run `autoreconf -i` (or just `make` if Makefile exists)

### Adding a new dependency

1. Edit `configure.ac`
2. Add `PKG_CHECK_MODULES([NAME], [pkg-name])`
3. Use `$(NAME_CFLAGS)` and `$(NAME_LIBS)` in `Makefile.am`
4. Run `autoreconf -i && ./configure`

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b my-feature`
3. Make changes and format: `make clang-format`
4. Commit with clear message: `git commit -m "feat: add new feature"`
5. Push and create PR

See [CLAUDE.md](CLAUDE.md) for architecture details and code style guidelines.

## CI/CD

The project uses GitHub Actions for CI. Tests run on:
- Ubuntu (GTK2 and GTK3)
- Arch Linux
- Fedora

PRs must pass:
- Build compilation
- Code formatting check (`make clang-format`)

## Resources

- **Repository:** https://github.com/hime-ime/hime
- **Issues:** https://github.com/hime-ime/hime/issues
- **Wiki:** https://github.com/hime-ime/hime/wiki
