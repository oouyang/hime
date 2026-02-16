# Recent Changes

This document summarizes the recent development changes to HIME.

## Input Method Search & UI Support (44d5fbd)

Added search functionality for input methods to support UI implementations:

### New API Functions

```c
/* Search input methods with filters */
int hime_search_methods(const HimeSearchFilter *filter,
                        HimeSearchResult *results, int max_results);

/* Search GTAB tables by name */
int hime_gtab_search_tables(const char *query,
                            HimeGtabInfo *results, int max_results);

/* Find method by exact name */
int hime_find_method_by_name(const char *name);

/* Get all available methods */
int hime_get_all_methods(HimeSearchResult *results, int max_results);
```

### Search Features
- Case-insensitive search for ASCII characters
- Exact match for Chinese characters (UTF-8)
- Match scoring with prefix bonus
- Results sorted by relevance

### Usage Example
```c
HimeSearchFilter filter = {
    .query = "倉",           /* Search for Cangjie */
    .method_type = -1,       /* All types */
    .include_disabled = false
};
HimeSearchResult results[20];
int count = hime_search_methods(&filter, results, 20);
```

---

## Simplified/Traditional Chinese Conversion (44d5fbd)

Added output variant toggle for Simplified/Traditional Chinese:

### New API Functions

```c
/* Convert between variants */
int hime_convert_simp_to_trad(const char *input, char *output, int size);
int hime_convert_trad_to_simp(const char *input, char *output, int size);

/* Output mode control */
void hime_set_output_variant(HimeContext *ctx, HimeOutputVariant variant);
HimeOutputVariant hime_get_output_variant(HimeContext *ctx);
HimeOutputVariant hime_toggle_output_variant(HimeContext *ctx);

/* Convert based on current setting */
int hime_convert_to_output_variant(HimeContext *ctx, const char *input,
                                   char *output, int size);

/* Apply conversion to all candidates */
void hime_convert_candidates_to_variant(HimeContext *ctx);
```

### Output Variants
```c
typedef enum {
    HIME_OUTPUT_TRADITIONAL = 0,  /* Traditional Chinese (default) */
    HIME_OUTPUT_SIMPLIFIED = 1,   /* Simplified Chinese */
    HIME_OUTPUT_BOTH = 2          /* Show both variants */
} HimeOutputVariant;
```

### Conversion Table
- ~600 common character conversion pairs included
- Covers most frequently used simplified ↔ traditional mappings

### Usage Example
```c
/* Toggle output mode */
HimeOutputVariant new_mode = hime_toggle_output_variant(ctx);

/* Convert string */
char output[256];
hime_convert_simp_to_trad("国家", output, sizeof(output));
/* output = "國家" */

/* Auto-convert candidates after generation */
hime_convert_candidates_to_variant(ctx);
```

---

## Code Formatting (582d30b, c842dc3)

### Multi-Language clang-format Support

Updated `.clang-format` to support multiple languages:

| Language | Base Style | Indent | Column Limit |
|----------|-----------|--------|--------------|
| C/C++ | GNU | 4 spaces | unlimited |
| Objective-C | Google | 4 spaces | 120 |
| Java | Google | 4 spaces | 120 |

### Updated format_code.py

Now formats all platform source files:

```
Platforms supported:
- src/           - Core Linux source (C)
- shared/        - Cross-platform implementation (C)
- platform/windows/  - Windows TSF (C/C++)
- platform/macos/    - macOS Input Method Kit (Objective-C)
- platform/ios/      - iOS keyboard extension (C + Objective-C)
- platform/android/  - Android IME (C + Java)
- tests/         - Unit tests (C)

File types:
- .c, .cpp, .h   - C/C++
- .m, .mm        - Objective-C
- .java          - Java
```

### Running Format
```bash
make clang-format
```

---

## Shared Implementation Consolidation (567c2d1, e2e3747)

### Architecture Change

Consolidated duplicate code across 4 platforms into a shared implementation:

**Before:**
```
windows/src/hime-core.c   (~2000 lines)
macos/src/hime-core.c     (~2000 lines)
ios/src/hime-core.c       (~2000 lines)
android/jni/hime-core.c   (~2000 lines)
Total: ~8000 lines (99.9% duplicate)
```

**After:**
```
shared/src/hime-core-impl.c           (~2600 lines) - Single implementation
platform/windows/src/hime-core.c      (13 lines)    - Platform wrapper
platform/macos/src/hime-core.c        (13 lines)    - Platform wrapper
platform/ios/src/hime-core.c          (13 lines)    - Platform wrapper
platform/android/jni/hime-core.c      (13 lines)    - Platform wrapper
```

### Platform Wrapper Pattern
```c
// platform/windows/src/hime-core.c
#define HIME_VERSION "0.10.1-win"
#include "../../../shared/src/hime-core-impl.c"

// platform/macos/src/hime-core.c
#define HIME_VERSION "0.10.1-macos"
#include "../../../shared/src/hime-core-impl.c"
```

### Benefits
- **74% code reduction** (8000 → 2600 lines)
- Single source of truth for all platforms
- Bug fixes apply to all platforms automatically
- Consistent behavior across platforms

---

## Unit Tests (4c72c7e)

### New Test Suite: shared/tests/test-hime-core.c

65 unit tests covering:

| Category | Tests |
|----------|-------|
| Initialization | 4 |
| Context Management | 4 |
| Input Method Control | 3 |
| Keyboard Layout | 2 |
| Preedit/Commit | 5 |
| Candidates | 4 |
| GTAB | 4 |
| Intcode | 4 |
| Settings | 7 |
| Punctuation | 2 |
| Method Availability | 2 |
| Key Processing | 3 |
| Bopomofo/TSIN/GTAB | 4 |
| Feedback Callback | 1 |
| NULL Safety | 1 |
| **Input Method Search** | 6 |
| **S/T Conversion** | 8 |

### Running Tests
```bash
# All tests
cd tests && make test

# Shared library tests
cd shared/tests && make && ./test-hime-core
```

### Test Results
```
Total: 171 tests
- tests/: 106 passed
- shared/tests/: 65 passed
```

---

## Complete GTAB Table Registry (5ed5832)

### Supported Input Method Tables

| Category | Tables |
|----------|--------|
| Cangjie | 倉頡, 倉頡五代, 倉頡五代三代, 倉頡標點 |
| Simplex | 簡易, 簡易標點 |
| DaYi | 大易 |
| Array | 行列30, 行列40, 行列30大字集 |
| Boshiamy | 嘸蝦米 |
| Phonetic | 拼音, 粵拼 |
| Korean | 韓文, 韓文羅馬 |
| Vietnamese | VIMS |
| Symbols | 符號, 希臘字母, 俄文, 世界語, 拉丁字母 |

### GTAB Table IDs
```c
typedef enum {
    HIME_GTAB_CJ = 0,           /* 倉頡 */
    HIME_GTAB_CJ5 = 1,          /* 倉頡五代 */
    HIME_GTAB_SIMPLEX = 10,     /* 簡易 */
    HIME_GTAB_DAYI = 20,        /* 大易 */
    HIME_GTAB_ARRAY30 = 30,     /* 行列30 */
    HIME_GTAB_BOSHIAMY = 40,    /* 嘸蝦米 */
    HIME_GTAB_PINYIN = 50,      /* 拼音 */
    HIME_GTAB_JYUTPING = 51,    /* 粵拼 */
    HIME_GTAB_HANGUL = 60,      /* 韓文 */
    HIME_GTAB_VIMS = 70,        /* 越南文 */
    HIME_GTAB_SYMBOLS = 80,     /* 符號 */
    // ... more
} HimeGtabTable;
```

---

## File Structure

```
hime/
├── shared/
│   ├── include/
│   │   └── hime-core.h        # Public API (900+ lines)
│   ├── src/
│   │   └── hime-core-impl.c   # Implementation (2600+ lines)
│   ├── tests/
│   │   ├── test-hime-core.c   # Unit tests (65 tests)
│   │   └── Makefile
│   └── README.md
├── platform/
│   ├── windows/src/hime-core.c    # Windows wrapper
│   ├── macos/src/hime-core.c      # macOS wrapper
│   ├── ios/Shared/src/hime-core.c # iOS wrapper
│   └── android/.../hime-core.c    # Android wrapper
├── docs/
│   ├── INPUT_METHODS.md       # Input methods documentation
│   └── CHANGELOG-recent.md    # This file
└── scripts/
    └── format_code.py         # Multi-language formatter
```
