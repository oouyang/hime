# HIME Input Method for PIME (Windows)

This module provides [HIME](https://github.com/hime-ime/hime) (Huge Input Method Editor) support on Windows through the [PIME](https://github.com/nickmcmahon/PIME) framework.

## Features

- **Bopomofo (Zhuyin) Input**: Traditional Chinese phonetic input method
- **Standard Keyboard Layout**: Uses the standard Zhuyin keyboard layout
- **Candidate Selection**: Shows candidate characters with frequency-based sorting
- **Mode Toggle**: Press `Shift` to toggle between Chinese and English modes

## Installation

### Prerequisites

1. Install PIME on Windows:
   - Download from [PIME releases](https://github.com/nickmcmahon/PIME/releases)
   - Or build from source using Visual Studio

2. Python 3.x with pywin32 package

### Install HIME Module

1. Copy the `hime` directory to PIME's input methods folder:
   ```
   C:\Program Files (X86)\PIME\server\input_methods\hime\
   ```

2. Restart PIME or reboot

3. Select "HIME 輸入法" from Windows language settings

## Usage

### Keyboard Layout (Standard Zhuyin)

```
┌─────────────────────────────────────────────────────────┐
│  1   q   2   w   3   e   4   r   5   t   6   y   7   u  │
│  ㄅ  ㄆ  ㄉ  ㄊ  ˇ  ㄍ  ˋ  ㄐ  ㄓ  ㄔ  ˊ  ㄗ  ˙  ㄧ  │
├─────────────────────────────────────────────────────────┤
│  a   s   d   f   g   h   j   k   l   ;   '             │
│  ㄇ  ㄋ  ㄎ  ㄑ  ㄕ  ㄘ  ㄨ  ㄜ  ㄠ  ㄤ              │
├─────────────────────────────────────────────────────────┤
│  z   x   c   v   b   n   m   ,   .   /                 │
│  ㄈ  ㄌ  ㄏ  ㄒ  ㄖ  ㄙ  ㄩ  ㄝ  ㄡ  ㄥ              │
├─────────────────────────────────────────────────────────┤
│  8   9   0   -                                          │
│  ㄚ  ㄞ  ㄢ  ㄦ                                        │
└─────────────────────────────────────────────────────────┘
```

### Input Flow

1. Type Bopomofo symbols using the keyboard
2. Press a tone key (3, 4, 6, 7) or Space (1st tone) to complete the syllable
3. Select a character from the candidate list using number keys (1-9, 0)
4. Press `Escape` to cancel current input
5. Press `Shift` to toggle Chinese/English mode

### Special Keys

| Key | Function |
|-----|----------|
| `Shift` | Toggle Chinese/English mode |
| `Space` | First tone / Select candidate |
| `3` | Second tone (ˊ) |
| `4` | Third tone (ˇ) |
| `6` | Fourth tone (ˋ) |
| `7` | Neutral tone (˙) |
| `1-9, 0` | Select candidate |
| `Escape` | Cancel input |
| `Backspace` | Delete last phonetic component |
| `Enter` | Commit preedit string |
| `Page Up/Down` | Navigate candidate pages |

## File Structure

```
hime/
├── __init__.py          # Module initialization
├── hime_data.py         # HIME data file parsers
├── hime_engine.py       # Input engine core logic
├── hime_ime.py          # PIME TextService implementation
├── ime.json             # PIME configuration
├── icon.ico             # Candidate window icon
├── chi.ico              # Chinese mode indicator
├── eng.ico              # English mode indicator
├── README.md            # This file
└── data/
    ├── pho.tab2         # Phonetic table (from HIME)
    └── zo.kbm           # Standard Zhuyin keyboard mapping
```

## Data Files

The phonetic table (`pho.tab2`) is from the HIME project and contains:
- Bopomofo-to-character mappings
- Character usage frequencies
- Phrase data

To update the phonetic table, copy a newer version from a HIME installation:
- Linux: `/usr/share/hime/table/pho.tab2`
- Or build from source: `data/pho.tab2`

## Architecture

```
┌─────────────────────┐
│   Windows TSF       │
│   (Text Services    │
│    Framework)       │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│   PIME DLL          │
│   (PIMETextService) │
└──────────┬──────────┘
           │ Named Pipe (JSON)
┌──────────▼──────────┐
│   Python Server     │
│   (server.py)       │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│   HIME Module       │
│   ├─ hime_ime.py    │  ← TextService interface
│   ├─ hime_engine.py │  ← Input processing
│   └─ hime_data.py   │  ← Data file parsing
└─────────────────────┘
```

## Development

### Adding New Keyboard Layouts

1. Add a new `.kbm` file to `data/`
2. Update `KeyboardMapping` class in `hime_data.py` if format differs
3. Update `hime_engine.py` to load the new layout

### Adding Table-Based Input Methods

The framework supports table-based input methods (like Cangjie, Array):

1. Copy `.gtab` files to `data/`
2. Implement `TableEngine` class similar to `PhoneticEngine`
3. Update `HIMEInputEngine` to support mode switching

## License

This module is licensed under GNU LGPL v2.1, consistent with both HIME and PIME projects.

## Credits

- [HIME](https://github.com/hime-ime/hime) - Original input method engine
- [PIME](https://github.com/nickmcmahon/PIME) - Windows IME framework
- HIME Team for the phonetic tables and input algorithms
