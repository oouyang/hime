# HIME iOS Keyboard Extension

This directory contains an iOS custom keyboard extension for HIME, providing Bopomofo/Zhuyin input on iPhone and iPad.

## Architecture

```
┌─────────────────────────────────────────┐
│           iOS Application               │
│         (Text Field Input)              │
└─────────────────┬───────────────────────┘
                  │ UITextDocumentProxy
┌─────────────────▼───────────────────────┐
│    HIMEKeyboardViewController           │
│    (UIInputViewController subclass)     │
│  - handleKeyPress:                      │
│  - insertText: / deleteBackward:        │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│    HIMEKeyboardView (UIView)            │
│  - Keyboard layout (Zhuyin symbols)     │
│  - Candidate bar                        │
│  - Mode indicator                       │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│    HimeEngine (Objective-C Wrapper)     │
│  - processKeyCode:character:modifiers:  │
│  - candidatesForCurrentPage             │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│    hime-core (C Library)                │
│  - Phonetic key processing              │
│  - Candidate lookup from pho.tab2       │
└─────────────────────────────────────────┘
```

## Components

### HIMEApp (Container App)
Main iOS application that:
- Displays setup instructions
- Provides keyboard enable/disable guidance
- Contains the keyboard extension

### HIMEKeyboard (App Extension)
iOS keyboard extension that:
- Implements UIInputViewController
- Displays Bopomofo keyboard layout
- Shows candidate bar with character selection
- Communicates with host app via UITextDocumentProxy

### Shared (Static Library)
Platform-independent code:
- hime-core.c - Core phonetic engine
- HimeEngine.m - Objective-C wrapper

## Building

**Requirements:**
- macOS with Xcode 12 or later
- iOS SDK 12.0 or later
- Apple Developer account (for device testing)

### Using Xcode (Recommended)

1. Open Xcode
2. Create a new workspace or project
3. Add the source files from this directory
4. Configure the targets:
   - HIMEApp (iOS Application)
   - HIMEKeyboard (App Extension)
5. Set up code signing
6. Build and run

### Using CMake (Generates Xcode Project)

```bash
cd ios
mkdir build && cd build
cmake .. -G Xcode -DCMAKE_SYSTEM_NAME=iOS
open HIME-iOS.xcodeproj
```

**Note:** The CMake-generated project may need manual configuration for:
- App extension embedding
- Code signing
- Data file bundling (pho.tab2)

## Project Structure

```
ios/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── Shared/
│   ├── include/
│   │   ├── hime-core.h         # Core API header
│   │   └── HimeEngine.h        # Objective-C wrapper
│   └── src/
│       ├── hime-core.c         # Core implementation
│       └── HimeEngine.m        # Wrapper implementation
├── HIMEApp/
│   ├── Info.plist.in           # App configuration
│   ├── main.m                  # Entry point
│   ├── AppDelegate.h/m         # App delegate
│   └── ViewController.h/m      # Setup instructions UI
└── HIMEKeyboard/
    ├── Info.plist.in           # Extension configuration
    ├── HIMEKeyboardViewController.h/m  # Input controller
    └── Views/
        └── HIMEKeyboardView.h/m        # Keyboard UI
```

## Installation (End User)

1. Install the HIME app on your iOS device
2. Open **Settings** → **General** → **Keyboard** → **Keyboards**
3. Tap **Add New Keyboard...**
4. Select **HIME** from the list
5. To use: tap 🌐 globe icon on any keyboard to switch to HIME

## Usage

### Keyboard Layout (Standard Zhuyin)

```
┌────────────────────────────────────────────────────┐
│ ㄅ  ㄉ  ˇ  ˋ  ㄓ  ˊ  ˙  ㄚ  ㄞ  ㄢ  │
│ ㄆ  ㄊ  ㄍ  ㄐ  ㄔ  ㄗ  ㄧ  ㄛ  ㄟ  ㄣ  │
│ ㄇ  ㄋ  ㄎ  ㄑ  ㄕ  ㄘ  ㄨ  ㄜ  ㄠ  ㄤ  │
│ ㄈ  ㄌ  ㄏ  ㄒ  ㄖ  ㄙ  ㄩ  ㄝ  ㄡ  ㄥ  │
│ 中/英   🌐       ␣       ⌫    ⏎   │
└────────────────────────────────────────────────────┘
```

### Special Keys

| Key | Function |
|-----|----------|
| `中/英` | Toggle Chinese/English mode |
| `🌐` | Switch to next keyboard |
| `␣` (Space) | First tone / confirm selection |
| `⌫` | Backspace / delete last symbol |
| `⏎` | Return / commit preedit |

### Candidate Selection

- Tap a candidate button to select
- Use `◀` `▶` buttons to navigate pages
- Candidates show number prefix (1., 2., etc.)

## Development Notes

### App Extension Limitations

iOS keyboard extensions have restrictions:
- No network access (unless "Allow Full Access" is enabled)
- Limited storage
- Cannot access container app data directly without App Groups
- Runs in sandboxed environment

### Data Sharing

To share data between app and extension:
1. Enable App Groups capability
2. Use shared container: `group.org.hime-ime.ios.HIME`
3. Store settings/data in shared UserDefaults

### Testing

1. Build and run on simulator or device
2. Enable keyboard in Settings
3. Switch to HIME keyboard in any text field
4. Test Zhuyin input and candidate selection

## Differences from Other Platforms

| Feature | Linux (GTK) | Windows (TSF) | macOS (IMK) | iOS |
|---------|-------------|---------------|-------------|-----|
| Framework | IBus/XIM | TSF | Input Method Kit | UIKit Extension |
| Language | C | C++ | Objective-C | Objective-C |
| UI | System | System | System | Custom |
| Candidate | System | System | System | Custom View |

## Troubleshooting

### Keyboard doesn't appear in Settings
- Ensure the app is properly installed
- Check that the extension is embedded in the app bundle
- Verify code signing is correct

### Keyboard crashes
- Check Console.app for crash logs
- Ensure pho.tab2 is bundled in extension
- Verify memory usage is within limits

### Characters not inserting
- Ensure UITextDocumentProxy is accessible
- Check that the host app supports custom keyboards

## License

GNU LGPL v2.1, consistent with the main HIME project.

## Credits

- [HIME](https://github.com/hime-ime/hime) - Original input method engine
- [OpenHeInput-iOS](https://github.com/) - Reference implementation
- HIME Team for phonetic tables and algorithms
