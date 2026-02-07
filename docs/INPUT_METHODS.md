# HIME Input Methods Reference

This document provides a comprehensive guide to all input methods supported by HIME across all platforms (Linux, Windows, macOS, iOS, Android).

## Table of Contents

- [Overview](#overview)
- [Input Method Types](#input-method-types)
- [Supported Input Methods](#supported-input-methods)
  - [Phonetic Input (注音)](#phonetic-input-注音)
  - [Phrase Input (詞音)](#phrase-input-詞音)
  - [Table-based Input (GTAB)](#table-based-input-gtab)
  - [Internal Code Input (內碼)](#internal-code-input-內碼)
  - [External Modules](#external-modules)
- [GTAB Tables Reference](#gtab-tables-reference)
- [Keyboard Layouts](#keyboard-layouts)
- [API Reference](#api-reference)
- [Platform-Specific Notes](#platform-specific-notes)

---

## Overview

HIME (姫) is a lightweight, cross-platform input method editor supporting multiple input methods for Chinese, Japanese, Korean, Vietnamese, and other languages. The core engine (`hime-core`) is written in portable C and can be embedded in any platform's IME framework.

### Architecture

```
┌─────────────────────────────────────────────────────┐
│              Platform IME Framework                  │
│  (TSF/GTK/Qt/IMKit/UIKit/InputMethodService)        │
└─────────────────────────┬───────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────┐
│                    hime-core                         │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │
│  │   PHO   │ │  TSIN   │ │  GTAB   │ │ INTCODE │   │
│  │ 注音輸入│ │ 詞音輸入│ │ 表格輸入│ │ 內碼輸入│   │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘   │
└─────────────────────────────────────────────────────┘
```

---

## Input Method Types

HIME supports 6 input method types:

| Type | Enum Value | Description | Languages |
|------|------------|-------------|-----------|
| **PHO** | `HIME_IM_PHO` | Phonetic (Bopomofo/Zhuyin) | Mandarin Chinese |
| **TSIN** | `HIME_IM_TSIN` | Phrase input with learning | Mandarin Chinese |
| **GTAB** | `HIME_IM_GTAB` | Table-based (generic) | Multiple |
| **ANTHY** | `HIME_IM_ANTHY` | Japanese Anthy module | Japanese |
| **CHEWING** | `HIME_IM_CHEWING` | Chewing library module | Mandarin Chinese |
| **INTCODE** | `HIME_IM_INTCODE` | Unicode/Big5 code input | Any |

---

## Supported Input Methods

### Phonetic Input (注音)

**Type:** `HIME_IM_PHO`

The phonetic input method uses Bopomofo (Zhuyin/注音符號) symbols to input Chinese characters. Users type phonetic symbols representing the pronunciation, then select from candidate characters.

#### Features
- Multiple keyboard layouts (Standard, HSU, ETen, IBM, Pinyin, Dvorak)
- Tone input support (4 tones + neutral)
- Frequency-based candidate sorting
- Full-width/half-width punctuation

#### Keyboard Layouts

| Layout | Enum Value | Description |
|--------|------------|-------------|
| Standard | `HIME_KB_STANDARD` | Traditional Zhuyin layout (default) |
| HSU | `HIME_KB_HSU` | 許氏鍵盤 - compact layout |
| ETen | `HIME_KB_ETEN` | 倚天鍵盤 - ETen layout |
| ETen 26 | `HIME_KB_ETEN26` | ETen 26-key compact layout |
| IBM | `HIME_KB_IBM` | IBM keyboard layout |
| Pinyin | `HIME_KB_PINYIN` | Hanyu Pinyin romanization |
| Dvorak | `HIME_KB_DVORAK` | Dvorak-based Zhuyin |

#### Standard Zhuyin Layout

```
┌─────────────────────────────────────────────────────────┐
│  1ㄅ  qㄆ  2ㄉ  wㄊ  3ˇ  eㄍ  4ˋ  rㄐ  5ㄓ  tㄔ  6ˊ  yㄗ  7˙  uㄧ │
├─────────────────────────────────────────────────────────┤
│  aㄇ  sㄋ  dㄎ  fㄑ  gㄕ  hㄘ  jㄨ  kㄜ  lㄠ  ;ㄤ          │
├─────────────────────────────────────────────────────────┤
│  zㄈ  xㄌ  cㄏ  vㄒ  bㄖ  nㄙ  mㄩ  ,ㄝ  .ㄡ  /ㄥ          │
├─────────────────────────────────────────────────────────┤
│  8ㄚ  9ㄞ  0ㄢ  -ㄦ  =ㄆ                                   │
└─────────────────────────────────────────────────────────┘
```

---

### Phrase Input (詞音)

**Type:** `HIME_IM_TSIN`

The phrase input method extends phonetic input with intelligent phrase composition. It allows users to input multiple syllables and automatically suggests common phrases.

#### Features
- Phrase database with frequency learning
- Auto-completion of common phrases
- User phrase addition
- Phrase buffer editing (vi-style)

#### Usage
1. Type multiple Bopomofo syllables
2. System suggests matching phrases
3. Select phrase or continue typing
4. Press Enter to commit

---

### Table-based Input (GTAB)

**Type:** `HIME_IM_GTAB`

GTAB (Generic Table) is a flexible table-based input system supporting many different input methods. Each method uses a `.gtab` file containing character mappings.

#### Features
- Support for 21+ input method tables
- Wildcard matching (* and ?)
- Duplicate character selection
- Custom table support

---

### Internal Code Input (內碼)

**Type:** `HIME_IM_INTCODE`

Direct character input using Unicode or Big5 hex codes.

#### Modes

| Mode | Enum Value | Format | Example |
|------|------------|--------|---------|
| Unicode | `HIME_INTCODE_UNICODE` | 4-digit hex | `4E2D` → 中 |
| Big5 | `HIME_INTCODE_BIG5` | 4-digit hex | `A4A4` → 中 |

#### Usage
1. Switch to intcode mode (`Ctrl+Alt+0`)
2. Type 4 hex digits
3. Character is automatically committed

---

### External Modules

#### Anthy (日文)

**Type:** `HIME_IM_ANTHY`

Japanese input using the Anthy library. Requires external `anthy-module.so`.

#### Chewing (新酷音)

**Type:** `HIME_IM_CHEWING`

Alternative phonetic input using the Chewing library. Requires external `chewing-module.so`.

---

## GTAB Tables Reference

### Chinese - Cangjie Family (倉頡系列)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 倉頡 | `HIME_GTAB_CJ` | `cj.gtab` | `Ctrl+Alt+1` | Standard Cangjie |
| 倉五 | `HIME_GTAB_CJ5` | `cj5.gtab` | `Ctrl+Alt+2` | Cangjie Version 5 |
| 五四三倉頡 | `HIME_GTAB_CJ543` | `cj543.gtab` | - | Cangjie 543 variant |
| 標點倉頡 | `HIME_GTAB_CJ_PUNC` | `cj-punc.gtab` | - | Cangjie with punctuation |

**Cangjie Basics:**
Cangjie is a shape-based input method that decomposes characters into basic components (radicals). Each component is assigned a letter key.

Example: 明 = 日 (A) + 月 (B) → Type `AB`

### Chinese - Simplex Family (速成/簡易系列)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 速成 | `HIME_GTAB_SIMPLEX` | `simplex.gtab` | `Ctrl+Alt+-` | Simplex (Quick) - first/last keys only |
| 標點簡易 | `HIME_GTAB_SIMPLEX_PUNC` | `simplex-punc.gtab` | - | Simplex with punctuation |

**Simplex Basics:**
Simplex uses only the first and last Cangjie codes, making it faster but with more candidates.

Example: 明 = 日 (A) + 月 (B) → Type `AB` (same as Cangjie for 2-code chars)

### Chinese - DaYi (大易)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 大易 | `HIME_GTAB_DAYI` | `dayi3.gtab` | `Ctrl+Alt+7` | DaYi 3-code input |

**DaYi Basics:**
DaYi is a shape-based method using 40 basic components mapped to keyboard keys. Most characters require only 2-3 keystrokes.

### Chinese - Array Family (行列系列)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 行列30 | `HIME_GTAB_ARRAY30` | `ar30.gtab` | `Ctrl+Alt+8` | Array 30 - 30 key positions |
| 行列40 | `HIME_GTAB_ARRAY40` | `array40.gtab` | `Ctrl+Alt+8` | Array 40 - extended version |
| 行列大字集 | `HIME_GTAB_ARRAY30_BIG` | `ar30-big.gtab` | - | Array 30 extended charset |

**Array Basics:**
Array is a systematic shape-based method where each key represents a position (row + column). Characters are decomposed into components based on their position in a grid.

Key layout: Each key has 3 positions (top, middle, bottom) × 10 columns

### Chinese - Boshiamy (嘸蝦米)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 嘸蝦米 | `HIME_GTAB_BOSHIAMY` | `noseeing.gtab` | `Ctrl+Alt+9` | Boshiamy input method |

**Boshiamy Basics:**
Boshiamy uses shape decomposition with mnemonic English letter associations. It's known for fast typing speed and low code collision rate.

### Chinese - Phonetic-based (拼音系列)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 拼音 | `HIME_GTAB_PINYIN` | `pinyin.gtab` | - | Hanyu Pinyin romanization |
| 粵拼 | `HIME_GTAB_JYUTPING` | `jyutping.gtab` | `Ctrl+Alt+]` | Cantonese Jyutping |

**Pinyin Basics:**
Romanization-based input using standard Hanyu Pinyin. Type the pronunciation in Latin letters.

Example: 中 → Type `zhong`

**Jyutping Basics:**
Cantonese romanization system. Type Cantonese pronunciation.

Example: 中 → Type `zung1`

### Korean (韓文)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 韓諺 | `HIME_GTAB_HANGUL` | `hangul.gtab` | `Ctrl+Alt+.` | Korean Hangul direct input |
| 韓羅 | `HIME_GTAB_HANGUL_ROMAN` | `hangul-roman.gtab` | `Ctrl+Alt+/` | Korean via romanization |

### Vietnamese (越南文)

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 越南文 | `HIME_GTAB_VIMS` | `vims.gtab` | `Ctrl+Alt+'` | Vietnamese VIMS input |

### Symbols and Special Characters

| Table | ID | File | Hotkey | Description |
|-------|-----|------|--------|-------------|
| 符號 | `HIME_GTAB_SYMBOLS` | `symbols.gtab` | `Ctrl+Alt+,` | Special symbols input |
| 希臘文 | `HIME_GTAB_GREEK` | `greek.gtab` | `Ctrl+Alt+`` | Greek letters |
| 俄文 | `HIME_GTAB_RUSSIAN` | `russian.gtab` | `Ctrl+Alt+?` | Russian/Cyrillic |
| 世界文 | `HIME_GTAB_ESPERANTO` | `esperanto.gtab` | `Ctrl+Alt+w` | Esperanto |
| 拉丁字母 | `HIME_GTAB_LATIN` | `latin-letters.gtab` | `Ctrl+Alt+a` | Latin letters with diacritics |

---

## Keyboard Layouts

### Bopomofo Layouts Comparison

| Key | Standard | HSU | ETen |
|-----|----------|-----|------|
| A | ㄇ | ㄇ | ㄚ |
| B | ㄖ | ㄅ | ㄅ |
| C | ㄏ | ㄕ | ㄒ |
| D | ㄎ | ㄉ | ㄉ |
| ... | ... | ... | ... |

(See platform-specific README files for complete layout charts)

---

## API Reference

### Initialization

```c
#include "hime-core.h"

// Initialize library with data directory
int ret = hime_init("/path/to/hime/data");

// Create input context
HimeContext *ctx = hime_context_new();
```

### Input Method Selection

```c
// Set input method type
hime_set_input_method(ctx, HIME_IM_GTAB);

// Load specific GTAB table
hime_gtab_load_table_by_id(ctx, HIME_GTAB_CJ);
// Or by filename
hime_gtab_load_table(ctx, "cj.gtab");

// Set keyboard layout for phonetic input
hime_set_keyboard_layout(ctx, HIME_KB_HSU);
```

### Key Processing

```c
// Process a key event
HimeKeyResult result = hime_process_key(ctx, keycode, charcode, modifiers);

switch (result) {
    case HIME_KEY_IGNORED:
        // Pass through to application
        break;
    case HIME_KEY_ABSORBED:
        // Key consumed, no output
        break;
    case HIME_KEY_PREEDIT:
        // Update preedit display
        char preedit[256];
        hime_get_preedit(ctx, preedit, sizeof(preedit));
        break;
    case HIME_KEY_COMMIT:
        // Commit string to application
        char commit[256];
        hime_get_commit(ctx, commit, sizeof(commit));
        hime_clear_commit(ctx);
        break;
}
```

### Candidate Handling

```c
if (hime_has_candidates(ctx)) {
    int count = hime_get_candidate_count(ctx);
    for (int i = 0; i < count; i++) {
        char cand[64];
        hime_get_candidate(ctx, i, cand, sizeof(cand));
        // Display candidate
    }

    // Select candidate by index
    hime_select_candidate(ctx, 0);

    // Or page through candidates
    hime_candidate_page_down(ctx);
}
```

### GTAB Table Information

```c
// Get number of available tables
int table_count = hime_gtab_get_table_count();

// Get table information
for (int i = 0; i < table_count; i++) {
    HimeGtabInfo info;
    hime_gtab_get_table_info(i, &info);
    printf("Table: %s (%s)\n", info.name, info.filename);
}
```

### Configuration

```c
// Character set (Traditional/Simplified)
hime_set_charset(ctx, HIME_CHARSET_TRADITIONAL);

// Smart punctuation
hime_set_smart_punctuation(ctx, true);

// Candidate display style
hime_set_candidate_style(ctx, HIME_CAND_STYLE_VERTICAL);

// Color scheme
hime_set_color_scheme(ctx, HIME_COLOR_SCHEME_SYSTEM);

// Feedback (for mobile platforms)
hime_set_sound_enabled(ctx, true);
hime_set_vibration_enabled(ctx, true);
hime_set_vibration_duration(ctx, 20);  // milliseconds
```

### Cleanup

```c
// Free context
hime_context_free(ctx);

// Cleanup library
hime_cleanup();
```

---

## Platform-Specific Notes

### Linux
- Native support via GTK2, GTK3, Qt5, Qt6 IM modules
- XIM protocol support for legacy applications
- See [DEVELOPMENT.md](../DEVELOPMENT.md) for build instructions

### Windows
- Uses Text Services Framework (TSF)
- Cross-compile from Linux using MinGW-w64
- See [windows/README.md](../windows/README.md)

### macOS
- Uses Input Method Kit framework
- Native Cocoa application
- See [macos/README.md](../macos/README.md)

### iOS
- Keyboard extension using UIInputViewController
- Custom keyboard UI
- See [ios/README.md](../ios/README.md)

### Android
- InputMethodService implementation
- JNI bridge to hime-core
- See [android/README.md](../android/README.md)

---

## Quick Reference Card

### Common Hotkeys (Linux)

| Hotkey | Action |
|--------|--------|
| `Ctrl+Space` | Toggle IME on/off |
| `Ctrl+Shift` | Cycle through input methods |
| `Shift` | Toggle Chinese/English |
| `Ctrl+Alt+0` | Switch to Intcode |
| `Ctrl+Alt+1` | Switch to Cangjie |
| `Ctrl+Alt+3` | Switch to Zhuyin |
| `Ctrl+Alt+6` | Switch to Phrase (TSIN) |
| `Ctrl+Alt+7` | Switch to DaYi |
| `Ctrl+Alt+8` | Switch to Array |
| `Ctrl+Alt+9` | Switch to Boshiamy |
| `Ctrl+Alt+=` | Switch to Anthy |

### Selection Keys

Default selection keys: `1234567890`

Candidates are selected by pressing the corresponding number key.

### Special Keys in Input

| Key | Function |
|-----|----------|
| `Space` | Confirm selection / First tone |
| `Enter` | Commit buffer |
| `Escape` | Cancel / Clear input |
| `Backspace` | Delete last component |
| `PageUp/Down` | Navigate candidate pages |
| `*` | Wildcard (any characters) in GTAB |
| `?` | Wildcard (single character) in GTAB |

---

## Version History

- **0.10.1** - Added complete GTAB table registry with 21+ input methods
- **0.10.0** - Cross-platform hime-core library with PHO, TSIN, GTAB, INTCODE support

---

## License

GNU LGPL v2.1

Copyright (C) 2020 The HIME team, Taiwan
