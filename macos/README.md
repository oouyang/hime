# HIME macOS Input Method

This directory contains a macOS Input Method implementation of HIME using Apple's Input Method Kit (IMK) framework.

## Architecture

```
┌─────────────────────────────────────────┐
│         macOS Application               │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│       Input Method Kit (IMK)            │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│    HimeInputController (IMKController)  │
│  - handleEvent:client:                  │
│  - candidates:                          │
│  - candidateSelected:                   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│    HimeEngine (Objective-C Wrapper)     │
│  - processKeyCode:character:modifiers:  │
│  - preeditString / commitString         │
│  - candidatesForCurrentPage             │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│    hime-core (C Library)                │
│  - Phonetic key processing              │
│  - Candidate lookup                     │
│  - Preedit/commit management            │
└─────────────────────────────────────────┘
```

## Components

### hime-core (C)
Platform-independent core library implementing:
- Bopomofo (Zhuyin) phonetic input
- Character candidate lookup with frequency sorting
- Preedit string management
- Input method state management

### HimeEngine (Objective-C)
Wrapper class that bridges hime-core to Cocoa:
- Manages HimeContext lifecycle
- Converts between C strings and NSString
- Provides Cocoa-friendly candidate API

### HimeInputController (Objective-C)
macOS Input Method Kit controller:
- Handles keyboard events from macOS
- Manages composition (marked text) display
- Displays candidate window via IMKCandidates
- Commits selected characters to applications

## Building

**Requirements:**
- macOS 10.13 or later
- Xcode Command Line Tools or Xcode
- CMake 3.16 or later

### Build Steps

```bash
cd macos
mkdir build && cd build
cmake ..
make
```

### Build Output

```
build/
└── HIME.app/
    └── Contents/
        ├── Info.plist
        ├── MacOS/
        │   └── HIME
        └── Resources/
            └── pho.tab2
```

## Installation

### Install the Input Method

```bash
# Copy to system Input Methods folder
sudo cp -r HIME.app /Library/Input\ Methods/

# Or install for current user only
cp -r HIME.app ~/Library/Input\ Methods/
```

### Enable the Input Method

1. Open **System Preferences** → **Keyboard** → **Input Sources**
2. Click the **+** button
3. Select **Chinese, Traditional** → **HIME Zhuyin**
4. Click **Add**

**Note:** You may need to log out and log back in for the input method to appear.

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
| `Return` | Commit preedit as-is |
| `Page Up/Down` | Navigate candidate pages |

### Input Flow

1. Type Bopomofo symbols using the keyboard
2. Press a tone key (3, 4, 6, 7) or Space (1st tone)
3. Select a character from candidates using number keys
4. Press `Shift` to toggle Chinese/English mode

## Files

```
macos/
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── include/
│   ├── hime-core.h          # Core API header (shared)
│   └── HimeEngine.h         # Objective-C wrapper header
├── src/
│   ├── hime-core.c          # Core implementation (shared)
│   ├── HimeEngine.m         # Objective-C wrapper
│   ├── HimeInputController.h
│   ├── HimeInputController.m # IMK controller
│   └── main.m               # Application entry point
└── Resources/
    └── Info.plist.in        # Bundle configuration template
```

## Differences from Other Platforms

| Feature | Linux (GTK) | Windows (TSF) | macOS (IMK) |
|---------|-------------|---------------|-------------|
| Framework | IBus/XIM | TSF | Input Method Kit |
| Language | C | C++ | Objective-C |
| Build | autotools | MinGW CMake | CMake |
| Integration | System-wide | Per-user | Per-user or System |

## Troubleshooting

### Input method doesn't appear
- Log out and log back in after installation
- Check Console.app for error messages
- Verify the bundle is properly signed (for macOS Catalina+)

### Crashes on startup
- Ensure `pho.tab2` is in the Resources folder
- Check Console.app for crash logs
- Try reinstalling to `/Library/Input Methods/`

### Candidates not appearing
- Verify the application supports Input Method Kit
- Some applications may not properly support IMK

### Building fails
- Ensure Xcode Command Line Tools are installed: `xcode-select --install`
- Check that CMake is version 3.16 or later

## Code Signing (for Distribution)

For distribution outside the App Store, the input method should be signed:

```bash
codesign --force --deep --sign "Developer ID Application: Your Name" HIME.app
```

For local development and testing, you can run unsigned.

## License

GNU LGPL v2.1, consistent with the main HIME project.

## Credits

- [HIME](https://github.com/hime-ime/hime) - Original input method engine
- [OpenHeInput-MacOS](https://github.com/nickmcmahon/OpenHeInput-MacOS) - Reference implementation
- HIME Team for phonetic tables and algorithms
