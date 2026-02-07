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
