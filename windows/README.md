# HIME Standalone Windows IME

This directory contains a standalone Windows Text Services Framework (TSF) implementation of HIME that works independently without requiring PIME.

## Architecture

```
┌─────────────────────────────────────────┐
│           Windows Application           │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│       Text Services Framework (TSF)     │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│     hime-tsf.dll (TSF Text Service)     │
│  - ITfTextInputProcessor                │
│  - ITfKeyEventSink                      │
│  - ITfCompositionSink                   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│    hime-core.dll (Core Engine)          │
│  - Phonetic key processing              │
│  - Candidate lookup                     │
│  - Preedit/commit management            │
└─────────────────────────────────────────┘
```

## Components

### hime-core.dll
Platform-independent core library implementing:
- Bopomofo (Zhuyin) phonetic input
- Character candidate lookup with frequency sorting
- Preedit string management
- Input method state management

### hime-tsf.dll
Windows TSF text service that:
- Registers as a Windows input method
- Handles keyboard events
- Manages composition (preedit) display
- Commits characters to applications

## Building

### Prerequisites

- Windows 10 SDK or later
- Visual Studio 2019/2022 (or CMake with MSVC)
- CMake 3.16 or later

### Build Steps

```powershell
# Clone the repository
git clone https://github.com/hime-ime/hime.git
cd hime/windows

# Create build directory
mkdir build
cd build

# Configure (64-bit)
cmake .. -G "Visual Studio 17 2022" -A x64

# Or for 32-bit
cmake .. -G "Visual Studio 17 2022" -A Win32

# Build
cmake --build . --config Release
```

### Build Output

```
build/
├── bin/
│   ├── hime-core.dll     # Core library
│   ├── hime-tsf.dll      # TSF text service
│   └── test-hime-core.exe # Test program
├── lib/
│   ├── hime-core.lib     # Import library
│   └── hime-tsf.lib      # Import library
├── register.bat          # Registration script
└── unregister.bat        # Unregistration script
```

## Installation

### Register the IME

1. Build the project as described above
2. Copy build output to a permanent location (e.g., `C:\Program Files\HIME`)
3. Copy data files:
   ```
   hime/data/pho.tab2 -> C:\Program Files\HIME\data\pho.tab2
   ```
4. Run `register.bat` as Administrator

Or manually:
```powershell
# Run as Administrator
regsvr32 "C:\Program Files\HIME\bin\hime-tsf.dll"
```

### Unregister the IME

```powershell
# Run as Administrator
regsvr32 /u "C:\Program Files\HIME\bin\hime-tsf.dll"
```

Or run `unregister.bat` as Administrator.

### Enabling the IME

After registration:
1. Open Windows Settings → Time & Language → Language
2. Click on your language → Options
3. Add a keyboard → Select "HIME 輸入法"

## Usage

### Keyboard Layout (Standard Zhuyin)

```
┌─────────────────────────────────────────────────────────┐
│  1   q   2   w   3   e   4   r   5   t   6   y   7   u  │
│  ㄅ  ㄆ  ㄉ  ㄊ  ˇ  ㄍ  ˋ  ㄐ  ㄓ  ㄔ  ˊ  ㄗ  ˙  ㄧ  │
├─────────────────────────────────────────────────────────┤
│  a   s   d   f   g   h   j   k   l   ;                  │
│  ㄇ  ㄋ  ㄎ  ㄑ  ㄕ  ㄘ  ㄨ  ㄜ  ㄠ  ㄤ                │
├─────────────────────────────────────────────────────────┤
│  z   x   c   v   b   n   m   ,   .   /                  │
│  ㄈ  ㄌ  ㄏ  ㄒ  ㄖ  ㄙ  ㄩ  ㄝ  ㄡ  ㄥ                │
├─────────────────────────────────────────────────────────┤
│  8   9   0   -                                           │
│  ㄚ  ㄞ  ㄢ  ㄦ                                          │
└─────────────────────────────────────────────────────────┘
```

### Special Keys

| Key | Function |
|-----|----------|
| `Shift` | Toggle Chinese/English mode |
| `Space` | First tone / confirm selection |
| `3` | Second tone (ˊ) |
| `4` | Third tone (ˇ) |
| `6` | Fourth tone (ˋ) |
| `7` | Neutral tone (˙) |
| `1-9, 0` | Select candidate |
| `Escape` | Cancel input |
| `Backspace` | Delete last phonetic component |

### Input Flow

1. Type Bopomofo symbols using the keyboard
2. Press a tone key (3, 4, 6, 7) or Space (1st tone)
3. Select a character from candidates using number keys
4. Press `Shift` to toggle Chinese/English mode

## Files

```
windows/
├── CMakeLists.txt       # Build configuration
├── hime-tsf.def         # DLL export definitions
├── README.md            # This file
├── include/
│   └── hime-core.h      # Core API header
├── src/
│   ├── hime-core.c      # Core implementation
│   └── hime-tsf.cpp     # TSF wrapper
└── tests/
    └── test-hime-core.c # Core library tests
```

## API Reference

See `include/hime-core.h` for the complete API documentation.

### Key Functions

```c
// Initialize the library
int hime_init(const char *data_dir);

// Create/destroy context
HimeContext *hime_context_new(void);
void hime_context_free(HimeContext *ctx);

// Process keyboard input
HimeKeyResult hime_process_key(
    HimeContext *ctx,
    uint32_t keycode,
    uint32_t charcode,
    uint32_t modifiers
);

// Get preedit/commit strings
int hime_get_preedit(HimeContext *ctx, char *buffer, int buffer_size);
int hime_get_commit(HimeContext *ctx, char *buffer, int buffer_size);

// Candidate management
int hime_get_candidate_count(HimeContext *ctx);
int hime_get_candidate(HimeContext *ctx, int index, char *buffer, int buffer_size);
```

## Differences from PIME Integration

| Feature | PIME Integration | Standalone |
|---------|-----------------|------------|
| Language | Python | C/C++ |
| Dependencies | PIME framework | Windows SDK only |
| Size | ~500KB + Python | ~200KB |
| Performance | Good | Better (native) |
| Extensibility | Easy (Python) | Requires recompile |

## Troubleshooting

### IME doesn't appear in language settings
- Ensure registration completed without errors
- Check Event Viewer for errors
- Verify DLLs are in a location accessible to all users

### IME crashes on startup
- Ensure `pho.tab2` is in the `data` subdirectory
- Check Windows Event Viewer for details
- Run `test-hime-core.exe` to verify core functionality

### Characters not appearing
- Verify the application supports TSF
- Some legacy applications may not work with TSF

## License

GNU LGPL v2.1, consistent with the main HIME project.

## Credits

- [HIME](https://github.com/hime-ime/hime) - Original input method engine
- HIME Team for phonetic tables and algorithms
