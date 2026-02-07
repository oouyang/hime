# HIME Android

Android Input Method Editor (IME) for HIME, providing Bopomofo/Zhuyin phonetic input on Android devices.

## Features

- **Bopomofo/Zhuyin Input**: Standard Taiwan phonetic keyboard layout
- **Candidate Selection**: Paginated character candidate display
- **Mode Switching**: Toggle between Chinese and English input modes
- **Native Performance**: JNI bridge to hime-core C library
- **Material Design**: Modern Android UI with dark theme

## Requirements

- Android SDK 34 (compileSdk)
- Android NDK 25.2.9519653
- Gradle 8.4+
- Java 11+
- CMake 3.22+

## Build Instructions

### Using Android Studio

1. Open the `android/` folder in Android Studio
2. Sync Gradle files
3. Build → Make Project
4. Run → Run 'app' to install on device/emulator

### Using Command Line

```bash
cd android

# Debug build
./gradlew assembleDebug

# Release build
./gradlew assembleRelease

# Install debug APK
./gradlew installDebug
```

The APK will be generated at:
- Debug: `app/build/outputs/apk/debug/app-debug.apk`
- Release: `app/build/outputs/apk/release/app-release.apk`

## Project Structure

```
android/
├── app/
│   ├── src/main/
│   │   ├── java/org/hime/android/
│   │   │   ├── core/
│   │   │   │   └── HimeEngine.java      # JNI wrapper for hime-core
│   │   │   ├── ime/
│   │   │   │   └── HimeInputMethodService.java  # IME service
│   │   │   ├── view/
│   │   │   │   ├── HimeKeyboardView.java    # Keyboard UI
│   │   │   │   └── HimeCandidateView.java   # Candidate bar
│   │   │   └── HimeSettingsActivity.java    # Settings/setup
│   │   ├── jni/
│   │   │   ├── CMakeLists.txt          # Native build config
│   │   │   ├── hime_jni.c              # JNI bridge
│   │   │   ├── hime-core.c             # Core engine (shared)
│   │   │   └── hime-core.h             # Core header (shared)
│   │   ├── res/
│   │   │   ├── xml/method.xml          # IME configuration
│   │   │   ├── values/strings.xml      # String resources
│   │   │   └── drawable/               # Icons
│   │   ├── assets/                     # Data files (pho.tab2)
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── build.gradle                        # Root build script
├── settings.gradle
└── gradle/wrapper/
```

## Installation & Setup

1. **Install the APK** on your Android device

2. **Enable the IME**:
   - Go to **Settings** → **System** → **Languages & input** → **On-screen keyboard**
   - Or: **Settings** → **General management** → **Keyboard list and default**
   - Enable **HIME 輸入法**

3. **Switch to HIME**:
   - Open any text field
   - Tap the keyboard icon in the navigation bar
   - Select **HIME**

## Keyboard Layout

The Zhuyin keyboard follows the standard Taiwan layout:

```
┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│1ㄅ│2ㄉ│3ˇ │4ˋ │5ㄓ│6ˊ │7˙ │8ㄚ│9ㄞ│0ㄢ│
├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
│qㄆ│wㄊ│eㄍ│rㄐ│tㄔ│yㄗ│uㄧ│iㄛ│oㄟ│pㄣ│
├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
│aㄇ│sㄋ│dㄎ│fㄑ│gㄕ│hㄘ│jㄨ│kㄜ│lㄠ│;ㄤ│
├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
│⇧  │zㄈ│xㄌ│cㄏ│vㄒ│bㄖ│nㄙ│mㄩ│,ㄝ│⌫  │
├───┴───┼───┴───┴───┴───┴───┼───┼───┼───┤
│MODE   │      SPACE        │ , │ . │ ↵ │
└───────┴───────────────────┴───┴───┴───┘
```

### Special Keys

| Key | Function |
|-----|----------|
| `MODE` | Toggle Chinese/English mode |
| `Space` | First tone / confirm selection |
| `3` | Second tone (ˊ) |
| `4` | Third tone (ˇ) |
| `6` | Fourth tone (ˋ) |
| `7` | Neutral tone (˙) |
| `1-9, 0` | Select candidate |
| `⌫` | Backspace / delete last symbol |
| `↵` | Return / commit preedit |
| `⇧` | Shift for uppercase (English mode) |

### Input Flow

1. Type Bopomofo symbols using the keyboard
2. Press a tone key (3, 4, 6, 7) or Space (1st tone)
3. Select a character from candidates using number keys
4. Press `MODE` to toggle Chinese/English mode

## Data Files

The following data files should be placed in `app/src/main/assets/`:

- `pho.tab2` - Phonetic table for Zhuyin-to-character conversion
- `tsin32.idx` - Phrase index (optional)
- `tsin32` - Phrase database (optional)

These files are copied from the main HIME `data/` directory.

## Architecture

```
┌─────────────────────────────────────────┐
│           Android Framework             │
│         (InputMethodService)            │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│       HimeInputMethodService.java       │
│    (Handles keyboard events & UI)       │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│          HimeEngine.java                │
│       (JNI wrapper for native)          │
└─────────────────┬───────────────────────┘
                  │ JNI
┌─────────────────▼───────────────────────┐
│           hime_jni.c                    │
│      (JNI bridge functions)             │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│           hime-core.c                   │
│    (Core Bopomofo input engine)         │
└─────────────────────────────────────────┘
```

## Differences from Other Platforms

| Feature | Linux (GTK) | Windows (TSF) | macOS (IMK) | iOS | Android |
|---------|-------------|---------------|-------------|-----|---------|
| Framework | IBus/XIM | TSF | Input Method Kit | UIKit Extension | InputMethodService |
| Language | C | C++ | Objective-C | Objective-C | Java + JNI |
| Build | autotools | MinGW CMake | CMake | Xcode | Gradle |
| UI | System | System | System | Custom | Custom |
| Candidate | System | System | System | Custom View | Custom View |

## Troubleshooting

### Build fails with NDK error
- Ensure Android NDK is installed via SDK Manager
- Check that `ndkVersion` in `build.gradle` matches installed version
- Try: `./gradlew clean && ./gradlew assembleDebug`

### IME doesn't appear in Settings
- Ensure the APK is properly installed
- Check `adb logcat` for installation errors
- Verify AndroidManifest.xml has correct service declaration

### Keyboard crashes on startup
- Check `adb logcat | grep HimeIME` for error logs
- Ensure `pho.tab2` is in the assets folder
- Verify JNI library loads correctly

### Characters not appearing
- Ensure the application supports soft keyboards
- Check that InputConnection is valid
- Some applications may not properly support IME

### JNI library not found
```bash
# Check if native library is built
ls app/build/intermediates/cmake/debug/obj/*/libhime-jni.so
```

## License

GNU Lesser General Public License v2.1

Copyright (C) 2020 The HIME team, Taiwan

## Credits

- [HIME](https://github.com/hime-ime/hime) - Original input method engine
- [TRIME](https://github.com/osfans/trime) - Reference Android IME implementation
- HIME Team for phonetic tables and algorithms
