/*
 * HIME Core Library Implementation
 *
 * Platform-independent core for HIME input method.
 * This file implements the hime-core.h API without any
 * platform-specific GUI or IME framework dependencies.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include "hime-core.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Version */
#define HIME_VERSION "0.10.1-ios"

/* Phonetic segment bit lengths */
static const int TYP_PHO_LEN[] = {5, 2, 4, 3};

/* Bopomofo display characters */
static const char *BOPOMOFO_INITIALS[] = {
    "", "ㄅ", "ㄆ", "ㄇ", "ㄈ", "ㄉ", "ㄊ", "ㄋ", "ㄌ",
    "ㄍ", "ㄎ", "ㄏ", "ㄐ", "ㄑ", "ㄒ", "ㄓ", "ㄔ",
    "ㄕ", "ㄖ", "ㄗ", "ㄘ", "ㄙ", "[", "]", "`"
};

static const char *BOPOMOFO_MEDIALS[] = {
    "", "ㄧ", "ㄨ", "ㄩ"
};

static const char *BOPOMOFO_FINALS[] = {
    "", "ㄚ", "ㄛ", "ㄜ", "ㄝ", "ㄞ", "ㄟ", "ㄠ", "ㄡ",
    "ㄢ", "ㄣ", "ㄤ", "ㄥ", "ㄦ"
};

static const char *BOPOMOFO_TONES[] = {
    "", "", "ˊ", "ˇ", "ˋ", "˙"
};

/* Standard Zhuyin keyboard mapping */
typedef struct {
    char key;
    int num;
    int typ;
} KeyMapEntry;

/* Standard Zhuyin keyboard layout (大千/標準注音) */
static const KeyMapEntry KEYMAP_STANDARD[] = {
    /* Initials: ㄅㄆㄇㄈㄉㄊㄋㄌㄍㄎㄏㄐㄑㄒㄓㄔㄕㄖㄗㄘㄙ */
    {'1', 1, 0}, {'q', 2, 0}, {'a', 3, 0}, {'z', 4, 0},
    {'2', 5, 0}, {'w', 6, 0}, {'s', 7, 0}, {'x', 8, 0},
    {'e', 9, 0}, {'d', 10, 0}, {'c', 11, 0},
    {'r', 12, 0}, {'f', 13, 0}, {'v', 14, 0},
    {'5', 15, 0}, {'t', 16, 0}, {'g', 17, 0}, {'b', 18, 0},
    {'y', 19, 0}, {'h', 20, 0}, {'n', 21, 0},
    /* Medials: ㄧㄨㄩ */
    {'u', 1, 1}, {'j', 2, 1}, {'m', 3, 1},
    /* Finals: ㄚㄛㄜㄝㄞㄟㄠㄡㄢㄣㄤㄥㄦ */
    {'8', 1, 2}, {'i', 2, 2}, {'k', 3, 2}, {',', 4, 2},
    {'9', 5, 2}, {'o', 6, 2}, {'l', 7, 2}, {'.', 8, 2},
    {'0', 9, 2}, {'p', 10, 2}, {';', 11, 2}, {'/', 12, 2},
    {'-', 13, 2},
    /* Tones: 二聲ˊ 三聲ˇ 四聲ˋ 輕聲˙ */
    {'3', 2, 3}, {'4', 3, 3}, {'6', 4, 3}, {'7', 5, 3},
    {' ', 1, 3},
    {0, 0, 0}  /* Sentinel */
};

/* HSU keyboard layout (許氏鍵盤) - designed by Hsu Deh-Chang */
static const KeyMapEntry KEYMAP_HSU[] = {
    /* Initials */
    {'b', 1, 0},   /* ㄅ */
    {'p', 2, 0},   /* ㄆ */
    {'m', 3, 0},   /* ㄇ */
    {'f', 4, 0},   /* ㄈ */
    {'d', 5, 0},   /* ㄉ */
    {'t', 6, 0},   /* ㄊ */
    {'n', 7, 0},   /* ㄋ */
    {'l', 8, 0},   /* ㄌ */
    {'g', 9, 0},   /* ㄍ */
    {'k', 10, 0},  /* ㄎ */
    {'h', 11, 0},  /* ㄏ */
    {'j', 12, 0},  /* ㄐ */
    {'v', 13, 0},  /* ㄑ */
    {'c', 14, 0},  /* ㄒ */
    {'j', 15, 0},  /* ㄓ (shared with ㄐ, context-dependent) */
    {'v', 16, 0},  /* ㄔ (shared with ㄑ) */
    {'c', 17, 0},  /* ㄕ (shared with ㄒ) */
    {'r', 18, 0},  /* ㄖ */
    {'z', 19, 0},  /* ㄗ */
    {'a', 20, 0},  /* ㄘ */
    {'s', 21, 0},  /* ㄙ */
    /* Medials */
    {'e', 1, 1},   /* ㄧ */
    {'x', 2, 1},   /* ㄨ */
    {'u', 3, 1},   /* ㄩ */
    /* Finals */
    {'a', 1, 2},   /* ㄚ */
    {'o', 2, 2},   /* ㄛ */
    {'r', 3, 2},   /* ㄜ */
    {'w', 4, 2},   /* ㄝ */
    {'i', 5, 2},   /* ㄞ */
    {'q', 6, 2},   /* ㄟ */
    {'z', 7, 2},   /* ㄠ */
    {'p', 8, 2},   /* ㄡ */
    {'m', 9, 2},   /* ㄢ */
    {'n', 10, 2},  /* ㄣ */
    {'k', 11, 2},  /* ㄤ */
    {'g', 12, 2},  /* ㄥ */
    {'l', 13, 2},  /* ㄦ */
    /* Tones */
    {'s', 2, 3},   /* ˊ 二聲 */
    {'d', 3, 3},   /* ˇ 三聲 */
    {'f', 4, 3},   /* ˋ 四聲 */
    {'j', 5, 3},   /* ˙ 輕聲 */
    {' ', 1, 3},   /* 一聲 */
    {0, 0, 0}
};

/* ETen keyboard layout (倚天鍵盤) */
static const KeyMapEntry KEYMAP_ETEN[] = {
    /* Initials */
    {'b', 1, 0},   /* ㄅ */
    {'p', 2, 0},   /* ㄆ */
    {'m', 3, 0},   /* ㄇ */
    {'f', 4, 0},   /* ㄈ */
    {'d', 5, 0},   /* ㄉ */
    {'t', 6, 0},   /* ㄊ */
    {'n', 7, 0},   /* ㄋ */
    {'l', 8, 0},   /* ㄌ */
    {'v', 9, 0},   /* ㄍ */
    {'k', 10, 0},  /* ㄎ */
    {'h', 11, 0},  /* ㄏ */
    {'g', 12, 0},  /* ㄐ */
    {'7', 13, 0},  /* ㄑ */
    {'c', 14, 0},  /* ㄒ */
    {';', 15, 0},  /* ㄓ */
    {'\'', 16, 0}, /* ㄔ */
    {'s', 17, 0},  /* ㄕ */
    {'j', 18, 0},  /* ㄖ */
    {'r', 19, 0},  /* ㄗ */
    {'z', 20, 0},  /* ㄘ */
    {'y', 21, 0},  /* ㄙ */
    /* Medials */
    {'u', 1, 1},   /* ㄧ */
    {'i', 2, 1},   /* ㄨ */
    {'x', 3, 1},   /* ㄩ */
    /* Finals */
    {'a', 1, 2},   /* ㄚ */
    {'o', 2, 2},   /* ㄛ */
    {'w', 3, 2},   /* ㄜ */
    {',', 4, 2},   /* ㄝ */
    {'e', 5, 2},   /* ㄞ */
    {'q', 6, 2},   /* ㄟ */
    {'1', 7, 2},   /* ㄠ */
    {'.', 8, 2},   /* ㄡ */
    {'2', 9, 2},   /* ㄢ */
    {'/', 10, 2},  /* ㄣ */
    {'3', 11, 2},  /* ㄤ */
    {'4', 12, 2},  /* ㄥ */
    {'-', 13, 2},  /* ㄦ */
    /* Tones */
    {'6', 2, 3},   /* ˊ */
    {'9', 3, 3},   /* ˇ */
    {'0', 4, 3},   /* ˋ */
    {'8', 5, 3},   /* ˙ */
    {' ', 1, 3},
    {0, 0, 0}
};

/* ETen 26-key layout (倚天26鍵) */
static const KeyMapEntry KEYMAP_ETEN26[] = {
    /* Initials */
    {'b', 1, 0},   /* ㄅ */
    {'p', 2, 0},   /* ㄆ */
    {'m', 3, 0},   /* ㄇ */
    {'f', 4, 0},   /* ㄈ */
    {'d', 5, 0},   /* ㄉ */
    {'t', 6, 0},   /* ㄊ */
    {'n', 7, 0},   /* ㄋ */
    {'l', 8, 0},   /* ㄌ */
    {'v', 9, 0},   /* ㄍ */
    {'k', 10, 0},  /* ㄎ */
    {'h', 11, 0},  /* ㄏ */
    {'g', 12, 0},  /* ㄐ */
    {'c', 13, 0},  /* ㄑ */
    {'y', 14, 0},  /* ㄒ */
    {'j', 15, 0},  /* ㄓ */
    {'q', 16, 0},  /* ㄔ */
    {'w', 17, 0},  /* ㄕ */
    {'s', 18, 0},  /* ㄖ */
    {'r', 19, 0},  /* ㄗ */
    {'z', 20, 0},  /* ㄘ */
    {'x', 21, 0},  /* ㄙ */
    /* Medials */
    {'u', 1, 1},   /* ㄧ */
    {'i', 2, 1},   /* ㄨ */
    {'o', 3, 1},   /* ㄩ */
    /* Finals */
    {'a', 1, 2},   /* ㄚ */
    {'o', 2, 2},   /* ㄛ */
    {'e', 3, 2},   /* ㄜ */
    {'e', 4, 2},   /* ㄝ (shared) */
    {'i', 5, 2},   /* ㄞ */
    {'a', 6, 2},   /* ㄟ (shared) */
    {'u', 7, 2},   /* ㄠ */
    {'o', 8, 2},   /* ㄡ (shared) */
    {'n', 9, 2},   /* ㄢ */
    {'n', 10, 2},  /* ㄣ (shared) */
    {'k', 11, 2},  /* ㄤ */
    {'g', 12, 2},  /* ㄥ */
    {'l', 13, 2},  /* ㄦ */
    /* Tones */
    {'d', 2, 3},   /* ˊ */
    {'f', 3, 3},   /* ˇ */
    {'j', 4, 3},   /* ˋ */
    {'s', 5, 3},   /* ˙ */
    {' ', 1, 3},
    {0, 0, 0}
};

/* IBM keyboard layout (IBM倚天) */
static const KeyMapEntry KEYMAP_IBM[] = {
    /* Initials */
    {'1', 1, 0},   /* ㄅ */
    {'2', 2, 0},   /* ㄆ */
    {'3', 3, 0},   /* ㄇ */
    {'4', 4, 0},   /* ㄈ */
    {'5', 5, 0},   /* ㄉ */
    {'6', 6, 0},   /* ㄊ */
    {'7', 7, 0},   /* ㄋ */
    {'8', 8, 0},   /* ㄌ */
    {'9', 9, 0},   /* ㄍ */
    {'0', 10, 0},  /* ㄎ */
    {'-', 11, 0},  /* ㄏ */
    {'q', 12, 0},  /* ㄐ */
    {'w', 13, 0},  /* ㄑ */
    {'e', 14, 0},  /* ㄒ */
    {'r', 15, 0},  /* ㄓ */
    {'t', 16, 0},  /* ㄔ */
    {'y', 17, 0},  /* ㄕ */
    {'u', 18, 0},  /* ㄖ */
    {'a', 19, 0},  /* ㄗ */
    {'s', 20, 0},  /* ㄘ */
    {'d', 21, 0},  /* ㄙ */
    /* Medials */
    {'i', 1, 1},   /* ㄧ */
    {'o', 2, 1},   /* ㄨ */
    {'p', 3, 1},   /* ㄩ */
    /* Finals */
    {'z', 1, 2},   /* ㄚ */
    {'x', 2, 2},   /* ㄛ */
    {'c', 3, 2},   /* ㄜ */
    {'v', 4, 2},   /* ㄝ */
    {'b', 5, 2},   /* ㄞ */
    {'n', 6, 2},   /* ㄟ */
    {'m', 7, 2},   /* ㄠ */
    {',', 8, 2},   /* ㄡ */
    {'.', 9, 2},   /* ㄢ */
    {'/', 10, 2},  /* ㄣ */
    {'f', 11, 2},  /* ㄤ */
    {'g', 12, 2},  /* ㄥ */
    {'h', 13, 2},  /* ㄦ */
    /* Tones */
    {'j', 2, 3},   /* ˊ */
    {'k', 3, 3},   /* ˇ */
    {'l', 4, 3},   /* ˋ */
    {';', 5, 3},   /* ˙ */
    {' ', 1, 3},
    {0, 0, 0}
};

/* Hanyu Pinyin keyboard layout (漢語拼音) */
static const KeyMapEntry KEYMAP_PINYIN[] = {
    /* Initials - mapped to Pinyin consonants */
    {'b', 1, 0},   /* ㄅ = b */
    {'p', 2, 0},   /* ㄆ = p */
    {'m', 3, 0},   /* ㄇ = m */
    {'f', 4, 0},   /* ㄈ = f */
    {'d', 5, 0},   /* ㄉ = d */
    {'t', 6, 0},   /* ㄊ = t */
    {'n', 7, 0},   /* ㄋ = n */
    {'l', 8, 0},   /* ㄌ = l */
    {'g', 9, 0},   /* ㄍ = g */
    {'k', 10, 0},  /* ㄎ = k */
    {'h', 11, 0},  /* ㄏ = h */
    {'j', 12, 0},  /* ㄐ = j */
    {'q', 13, 0},  /* ㄑ = q */
    {'x', 14, 0},  /* ㄒ = x */
    {'v', 15, 0},  /* ㄓ = zh (use v) */
    {'c', 16, 0},  /* ㄔ = ch (use c) */
    {'s', 17, 0},  /* ㄕ = sh (use s in context) */
    {'r', 18, 0},  /* ㄖ = r */
    {'z', 19, 0},  /* ㄗ = z */
    {'c', 20, 0},  /* ㄘ = c */
    {'s', 21, 0},  /* ㄙ = s */
    /* Medials */
    {'i', 1, 1},   /* ㄧ = i */
    {'u', 2, 1},   /* ㄨ = u */
    {'y', 3, 1},   /* ㄩ = ü (use y or v) */
    /* Finals */
    {'a', 1, 2},   /* ㄚ = a */
    {'o', 2, 2},   /* ㄛ = o */
    {'e', 3, 2},   /* ㄜ = e */
    {'e', 4, 2},   /* ㄝ = ê */
    {'i', 5, 2},   /* ㄞ = ai */
    {'i', 6, 2},   /* ㄟ = ei */
    {'o', 7, 2},   /* ㄠ = ao */
    {'u', 8, 2},   /* ㄡ = ou */
    {'n', 9, 2},   /* ㄢ = an */
    {'n', 10, 2},  /* ㄣ = en */
    {'g', 11, 2},  /* ㄤ = ang */
    {'g', 12, 2},  /* ㄥ = eng */
    {'r', 13, 2},  /* ㄦ = er */
    /* Tones - use number keys */
    {'2', 2, 3},   /* ˊ */
    {'3', 3, 3},   /* ˇ */
    {'4', 4, 3},   /* ˋ */
    {'5', 5, 3},   /* ˙ */
    {' ', 1, 3},   /* 一聲 */
    {'1', 1, 3},   /* 一聲 (explicit) */
    {0, 0, 0}
};

/* Dvorak-based Zhuyin keyboard layout */
static const KeyMapEntry KEYMAP_DVORAK[] = {
    /* Remapped from Standard based on Dvorak layout */
    /* Initials */
    {'1', 1, 0}, {'\'', 2, 0}, {'a', 3, 0}, {';', 4, 0},
    {'2', 5, 0}, {',', 6, 0}, {'o', 7, 0}, {'q', 8, 0},
    {'.', 9, 0}, {'e', 10, 0}, {'j', 11, 0},
    {'p', 12, 0}, {'u', 13, 0}, {'k', 14, 0},
    {'5', 15, 0}, {'y', 16, 0}, {'i', 17, 0}, {'x', 18, 0},
    {'f', 19, 0}, {'d', 20, 0}, {'b', 21, 0},
    /* Medials */
    {'g', 1, 1}, {'h', 2, 1}, {'m', 3, 1},
    /* Finals */
    {'8', 1, 2}, {'c', 2, 2}, {'t', 3, 2}, {'w', 4, 2},
    {'9', 5, 2}, {'r', 6, 2}, {'n', 7, 2}, {'v', 8, 2},
    {'0', 9, 2}, {'l', 10, 2}, {'s', 11, 2}, {'z', 12, 2},
    {'[', 13, 2},
    /* Tones */
    {'3', 2, 3}, {'4', 3, 3}, {'6', 4, 3}, {'7', 5, 3},
    {' ', 1, 3},
    {0, 0, 0}
};

/* Array of keyboard layout tables for easy switching */
static const KeyMapEntry *KEYBOARD_LAYOUTS[] = {
    KEYMAP_STANDARD,  /* HIME_KB_STANDARD */
    KEYMAP_HSU,       /* HIME_KB_HSU */
    KEYMAP_ETEN,      /* HIME_KB_ETEN */
    KEYMAP_ETEN26,    /* HIME_KB_ETEN26 */
    KEYMAP_IBM,       /* HIME_KB_IBM */
    KEYMAP_PINYIN,    /* HIME_KB_PINYIN */
    KEYMAP_DVORAK     /* HIME_KB_DVORAK */
};

/* Layout name mapping for hime_set_keyboard_layout_by_name() */
static const struct {
    const char *name;
    HimeKeyboardLayout layout;
} LAYOUT_NAMES[] = {
    {"standard", HIME_KB_STANDARD},
    {"zo", HIME_KB_STANDARD},      /* Alias */
    {"hsu", HIME_KB_HSU},
    {"eten", HIME_KB_ETEN},
    {"et", HIME_KB_ETEN},          /* Alias */
    {"eten26", HIME_KB_ETEN26},
    {"et26", HIME_KB_ETEN26},      /* Alias */
    {"ibm", HIME_KB_IBM},
    {"pinyin", HIME_KB_PINYIN},
    {"hanyu", HIME_KB_PINYIN},     /* Alias */
    {"dvorak", HIME_KB_DVORAK},
    {NULL, HIME_KB_STANDARD}
};

/* Phonetic index entry */
typedef struct {
    uint16_t key;
    uint16_t start;
} PhoIdx;

/* Phonetic item */
typedef struct {
    char ch[HIME_CH_SZ];
    int32_t count;
} PhoItem;

/* Phonetic table */
typedef struct {
    PhoIdx *idx;
    int idx_count;
    PhoItem *items;
    int item_count;
    char *phrase_area;
    int phrase_area_size;
} PhoTable;

/* ========== GTAB Structures ========== */

#define GTAB_MAX_KEYS 8
#define GTAB_MAX_KEYNAME 64

/* GTAB file header (matches original HIME format) */
typedef struct {
    int version;
    uint32_t flag;
    char cname[32];      /* Display name */
    char selkey[12];     /* Selection keys */
    int space_style;     /* Space key behavior */
    int key_count;       /* Number of keys in keymap */
    int max_press;       /* Max keystrokes per character */
    int dup_sel;         /* Duplicate selection count */
    int def_chars;       /* Number of defined characters */
    /* Quick keys omitted for simplicity */
    char reserved[512];  /* Reserved/padding */
} GtabHeader;

/* GTAB item (32-bit key) */
typedef struct {
    uint8_t key[4];
    char ch[HIME_CH_SZ];
} GtabItem;

/* GTAB item (64-bit key for large tables) */
typedef struct {
    uint8_t key[8];
    char ch[HIME_CH_SZ];
} GtabItem64;

/* Loaded GTAB table */
typedef struct {
    char name[64];
    char filename[128];
    char keymap[128];        /* Key to symbol mapping */
    char keyname[128];       /* Symbol display names */
    char selkey[16];
    int key_count;
    int max_press;
    int def_chars;
    int keybits;
    bool key64;              /* Using 64-bit keys */
    GtabItem *items;
    GtabItem64 *items64;
    int item_count;
    uint32_t *idx;           /* Index for fast lookup */
    int idx_count;
    bool loaded;
} GtabTable;

/* GTAB registry entry */
typedef struct {
    char name[64];
    char filename[128];
    char icon[64];
    HimeGtabTable id;
} GtabRegistry;

/* Well-known GTAB tables */
static const GtabRegistry GTAB_REGISTRY[] = {
    {"倉頡", "cj.gtab", "cj.png", HIME_GTAB_CJ},
    {"倉五", "cj5.gtab", "cj5.png", HIME_GTAB_CJ5},
    {"速成", "simplex.gtab", "simplex.png", HIME_GTAB_SIMPLEX},
    {"大易", "dayi3.gtab", "dayi3.png", HIME_GTAB_DAYI},
    {"行列30", "ar30.gtab", "ar30.png", HIME_GTAB_ARRAY30},
    {"行列40", "array40.gtab", "ar30.png", HIME_GTAB_ARRAY40},
    {"拼音", "pinyin.gtab", "pinyin.png", HIME_GTAB_PINYIN},
    {"粵拼", "jyutping.gtab", "jyutping.png", HIME_GTAB_JYUTPING},
    {"韓諺", "hangul.gtab", "hangul.png", HIME_GTAB_HANGUL},
    {"符號", "symbols.gtab", "symbols.png", HIME_GTAB_SYMBOLS},
    {"希臘文", "greek.gtab", "greek.png", HIME_GTAB_GREEK},
    {"俄文", "russian.gtab", "russian.png", HIME_GTAB_RUSSIAN},
    {"", "", "", HIME_GTAB_CUSTOM}  /* Sentinel */
};

/* ========== TSIN Structures ========== */

#define TSIN_MAX_PHRASE_LEN 32

/* TSIN phrase entry */
typedef struct {
    uint16_t phokeys[TSIN_MAX_PHRASE_LEN];
    char phrase[TSIN_MAX_PHRASE_LEN * HIME_CH_SZ];
    int len;
    int usecount;
} TsinPhrase;

/* TSIN database */
typedef struct {
    uint16_t *idx;
    int idx_count;
    TsinPhrase *phrases;
    int phrase_count;
    bool loaded;
} TsinDatabase;

/* ========== Intcode Structures ========== */

#define INTCODE_MAX_DIGITS 8

/* Input method names */
static const char *INPUT_METHOD_NAMES[] = {
    "注音 (Phonetic)",
    "詞音 (Phrase)",
    "倉頡 (Table)",
    "日文 (Anthy)",
    "新酷音 (Chewing)",
    "內碼 (Intcode)"
};

/* Input context structure */
struct HimeContext {
    /* Current input state */
    int typ_pho[4];           /* Phonetic components */
    int inph[4];              /* Input keys */

    /* Buffers */
    char preedit[HIME_MAX_PREEDIT];
    char commit[HIME_MAX_PREEDIT];
    char candidates[HIME_MAX_CANDIDATES][HIME_MAX_CANDIDATE_LEN];
    int candidate_count;
    int candidate_page;
    int candidates_per_page;

    /* State */
    bool chinese_mode;
    HimeInputMethod method;
    HimeKeyboardLayout keyboard_layout;  /* Current keyboard layout */

    /* Selection keys */
    char sel_keys[16];

    /* GTAB state */
    GtabTable *gtab;                    /* Current GTAB table */
    uint8_t gtab_keys[GTAB_MAX_KEYS];   /* Current key buffer */
    int gtab_key_count;                 /* Number of keys entered */
    char gtab_key_display[64];          /* Key display string */

    /* TSIN state */
    char tsin_phrase[HIME_MAX_PREEDIT]; /* Current phrase buffer */
    int tsin_phrase_len;                /* Phrase length in chars */
    int tsin_cursor;                    /* Cursor position */

    /* Intcode state */
    HimeIntcodeMode intcode_mode;       /* Unicode or Big5 */
    char intcode_buffer[INTCODE_MAX_DIGITS + 1];
    int intcode_len;                    /* Number of hex digits */

    /* New features - Settings */
    HimeCharset charset;                /* Simplified/Traditional */
    HimeCandidateStyle candidate_style; /* Candidate display style */
    HimeColorScheme color_scheme;       /* Light/Dark/System */
    bool system_dark_mode;              /* System dark mode state */

    /* Smart punctuation */
    bool smart_punctuation;             /* Enable smart punctuation */
    bool pinyin_annotation;             /* Show Pinyin hints */
    bool quote_open_double;             /* Quote pairing state */
    bool quote_open_single;             /* Single quote state */

    /* Feedback settings */
    bool sound_enabled;
    bool vibration_enabled;
    int vibration_duration_ms;
    HimeFeedbackCallback feedback_callback;
    void *feedback_user_data;
};

/* Global state */
static char g_data_dir[512] = "";
static PhoTable g_pho_table = {0};
static GtabTable g_gtab_tables[16] = {0};  /* Loaded GTAB tables */
static int g_gtab_table_count = 0;
static TsinDatabase g_tsin_db = {0};
static bool g_initialized = false;

/* ========== Internal Functions ========== */

/* Forward declaration for feedback trigger */
static void trigger_feedback(HimeContext *ctx, HimeFeedbackType type);

static hime_phokey_t pho2key(int typ_pho[4]) {
    hime_phokey_t key = typ_pho[0];

    if (key == 24) {  /* BACK_QUOTE_NO */
        return (24 << 9) | typ_pho[1];
    }

    for (int i = 1; i < 4; i++) {
        key = typ_pho[i] | (key << TYP_PHO_LEN[i]);
    }

    return key;
}

static void key_typ_pho(hime_phokey_t phokey, int rtyp_pho[4]) {
    rtyp_pho[3] = phokey & 7;
    phokey >>= 3;
    rtyp_pho[2] = phokey & 0xF;
    phokey >>= 4;
    rtyp_pho[1] = phokey & 0x3;
    phokey >>= 2;
    rtyp_pho[0] = phokey;
}

static bool typ_pho_empty(int typ_pho[4]) {
    return typ_pho[0] == 0 && typ_pho[1] == 0 &&
           typ_pho[2] == 0 && typ_pho[3] == 0;
}

static int load_pho_table(const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return -1;

    uint16_t idxnum;
    int32_t ch_phoN, phrase_sz;

    /* Read header */
    fread(&idxnum, 2, 1, fp);  /* Read twice (historical quirk) */
    fread(&idxnum, 2, 1, fp);
    fread(&ch_phoN, 4, 1, fp);
    fread(&phrase_sz, 4, 1, fp);

    /* Allocate and read index */
    g_pho_table.idx = (PhoIdx *)malloc(sizeof(PhoIdx) * (idxnum + 1));
    if (!g_pho_table.idx) {
        fclose(fp);
        return -1;
    }
    fread(g_pho_table.idx, sizeof(PhoIdx), idxnum, fp);
    g_pho_table.idx_count = idxnum;

    /* Add sentinel */
    g_pho_table.idx[idxnum].key = 0xFFFF;
    g_pho_table.idx[idxnum].start = ch_phoN;

    /* Allocate and read items */
    g_pho_table.items = (PhoItem *)malloc(sizeof(PhoItem) * ch_phoN);
    if (!g_pho_table.items) {
        free(g_pho_table.idx);
        fclose(fp);
        return -1;
    }
    fread(g_pho_table.items, sizeof(PhoItem), ch_phoN, fp);
    g_pho_table.item_count = ch_phoN;

    /* Read phrase area */
    if (phrase_sz > 0) {
        g_pho_table.phrase_area = (char *)malloc(phrase_sz);
        if (g_pho_table.phrase_area) {
            fread(g_pho_table.phrase_area, 1, phrase_sz, fp);
            g_pho_table.phrase_area_size = phrase_sz;
        }
    }

    fclose(fp);
    return 0;
}

static void free_pho_table(void) {
    if (g_pho_table.idx) {
        free(g_pho_table.idx);
        g_pho_table.idx = NULL;
    }
    if (g_pho_table.items) {
        free(g_pho_table.items);
        g_pho_table.items = NULL;
    }
    if (g_pho_table.phrase_area) {
        free(g_pho_table.phrase_area);
        g_pho_table.phrase_area = NULL;
    }
}

static const char *get_pho_char(int idx) {
    if (idx < 0 || idx >= g_pho_table.item_count) {
        return "";
    }

    PhoItem *item = &g_pho_table.items[idx];

    /* Check for phrase escape (0x1B) */
    if ((unsigned char)item->ch[0] == 0x1B && g_pho_table.phrase_area) {
        int offset = (unsigned char)item->ch[1] |
                    ((unsigned char)item->ch[2] << 8) |
                    ((unsigned char)item->ch[3] << 16);
        if (offset < g_pho_table.phrase_area_size) {
            return g_pho_table.phrase_area + offset;
        }
    }

    return item->ch;
}

static int lookup_phokey(hime_phokey_t phokey, HimeContext *ctx) {
    ctx->candidate_count = 0;

    if (!g_pho_table.idx) return 0;

    /* Find matching key in index */
    for (int i = 0; i < g_pho_table.idx_count; i++) {
        if (g_pho_table.idx[i].key == phokey) {
            int start = g_pho_table.idx[i].start;
            int end = g_pho_table.idx[i + 1].start;

            /* Collect candidates (sorted by count in table) */
            for (int j = start; j < end && ctx->candidate_count < HIME_MAX_CANDIDATES; j++) {
                const char *ch = get_pho_char(j);
                if (ch && ch[0]) {
                    strncpy(ctx->candidates[ctx->candidate_count], ch,
                           HIME_MAX_CANDIDATE_LEN - 1);
                    ctx->candidates[ctx->candidate_count][HIME_MAX_CANDIDATE_LEN - 1] = '\0';
                    ctx->candidate_count++;
                }
            }
            break;
        } else if (g_pho_table.idx[i].key > phokey) {
            break;
        }
    }

    return ctx->candidate_count;
}

static void update_preedit(HimeContext *ctx) {
    ctx->preedit[0] = '\0';
    int pos = 0;

    /* Build Bopomofo string */
    for (int i = 0; i < 4; i++) {
        int idx = ctx->typ_pho[i];
        const char *s = NULL;

        switch (i) {
            case 0:
                if (idx > 0 && idx < 25) s = BOPOMOFO_INITIALS[idx];
                break;
            case 1:
                if (idx > 0 && idx < 4) s = BOPOMOFO_MEDIALS[idx];
                break;
            case 2:
                if (idx > 0 && idx < 14) s = BOPOMOFO_FINALS[idx];
                break;
            case 3:
                if (idx > 0 && idx < 6) s = BOPOMOFO_TONES[idx];
                break;
        }

        if (s && s[0]) {
            int len = strlen(s);
            if (pos + len < HIME_MAX_PREEDIT - 1) {
                strcpy(ctx->preedit + pos, s);
                pos += len;
            }
        }
    }
}

static const KeyMapEntry *find_keymap(HimeContext *ctx, char key) {
    HimeKeyboardLayout layout = ctx ? ctx->keyboard_layout : HIME_KB_STANDARD;
    if (layout >= HIME_KB_COUNT) layout = HIME_KB_STANDARD;

    const KeyMapEntry *keymap = KEYBOARD_LAYOUTS[layout];
    for (int i = 0; keymap[i].key != 0; i++) {
        if (keymap[i].key == key) {
            return &keymap[i];
        }
    }
    return NULL;
}

/* ========== API Implementation ========== */

HIME_API int hime_init(const char *data_dir) {
    if (g_initialized) {
        return 0;
    }

    if (data_dir) {
        strncpy(g_data_dir, data_dir, sizeof(g_data_dir) - 1);
    }

    /* Load phonetic table */
    char pho_path[1024];
    snprintf(pho_path, sizeof(pho_path), "%s/pho.tab2", g_data_dir);

    if (load_pho_table(pho_path) < 0) {
        /* Try alternate paths */
        snprintf(pho_path, sizeof(pho_path), "%s/data/pho.tab2", g_data_dir);
        if (load_pho_table(pho_path) < 0) {
            return -1;
        }
    }

    g_initialized = true;
    return 0;
}

HIME_API void hime_cleanup(void) {
    if (!g_initialized) return;

    free_pho_table();
    g_initialized = false;
}

HIME_API const char *hime_version(void) {
    return HIME_VERSION;
}

HIME_API HimeContext *hime_context_new(void) {
    HimeContext *ctx = (HimeContext *)calloc(1, sizeof(HimeContext));
    if (!ctx) return NULL;

    ctx->chinese_mode = true;
    ctx->method = HIME_IM_PHO;
    ctx->keyboard_layout = HIME_KB_STANDARD;
    ctx->candidates_per_page = 10;
    strcpy(ctx->sel_keys, "1234567890");

    /* Initialize new feature settings with defaults */
    ctx->charset = HIME_CHARSET_TRADITIONAL;
    ctx->candidate_style = HIME_CAND_STYLE_HORIZONTAL;
    ctx->color_scheme = HIME_COLOR_SCHEME_SYSTEM;
    ctx->system_dark_mode = false;

    ctx->smart_punctuation = false;
    ctx->pinyin_annotation = false;
    ctx->quote_open_double = false;
    ctx->quote_open_single = false;

    ctx->sound_enabled = false;
    ctx->vibration_enabled = false;
    ctx->vibration_duration_ms = 20;
    ctx->feedback_callback = NULL;
    ctx->feedback_user_data = NULL;

    /* Initialize input method specific state */
    ctx->gtab = NULL;
    ctx->intcode_mode = HIME_INTCODE_UNICODE;

    return ctx;
}

HIME_API void hime_context_free(HimeContext *ctx) {
    if (ctx) {
        free(ctx);
    }
}

HIME_API void hime_context_reset(HimeContext *ctx) {
    if (!ctx) return;

    /* Reset PHO state */
    memset(ctx->typ_pho, 0, sizeof(ctx->typ_pho));
    memset(ctx->inph, 0, sizeof(ctx->inph));

    /* Reset common buffers */
    ctx->preedit[0] = '\0';
    ctx->commit[0] = '\0';
    ctx->candidate_count = 0;
    ctx->candidate_page = 0;

    /* Reset GTAB state */
    memset(ctx->gtab_keys, 0, sizeof(ctx->gtab_keys));
    ctx->gtab_key_count = 0;
    ctx->gtab_key_display[0] = '\0';

    /* Reset TSIN state */
    ctx->tsin_phrase[0] = '\0';
    ctx->tsin_phrase_len = 0;
    ctx->tsin_cursor = 0;

    /* Reset intcode state */
    ctx->intcode_buffer[0] = '\0';
    ctx->intcode_len = 0;
}

HIME_API int hime_set_input_method(HimeContext *ctx, HimeInputMethod method) {
    if (!ctx) return -1;
    ctx->method = method;
    hime_context_reset(ctx);
    return 0;
}

HIME_API HimeInputMethod hime_get_input_method(HimeContext *ctx) {
    return ctx ? ctx->method : HIME_IM_PHO;
}

HIME_API bool hime_toggle_chinese_mode(HimeContext *ctx) {
    if (!ctx) return false;
    ctx->chinese_mode = !ctx->chinese_mode;
    hime_context_reset(ctx);
    return ctx->chinese_mode;
}

HIME_API bool hime_is_chinese_mode(HimeContext *ctx) {
    return ctx ? ctx->chinese_mode : false;
}

HIME_API void hime_set_chinese_mode(HimeContext *ctx, bool chinese) {
    if (!ctx) return;
    ctx->chinese_mode = chinese;
    if (!chinese) {
        hime_context_reset(ctx);
    }
}

/* PHO input method processing */
static HimeKeyResult pho_process_key(HimeContext *ctx, uint32_t keycode, uint32_t charcode) {
    /* Handle Backspace for PHO */
    if (keycode == 0x08 || charcode == 0x08) {
        if (!typ_pho_empty(ctx->typ_pho)) {
            /* Delete last component */
            for (int i = 3; i >= 0; i--) {
                if (ctx->typ_pho[i]) {
                    ctx->typ_pho[i] = 0;
                    ctx->inph[i] = 0;
                    break;
                }
            }
            ctx->candidate_count = 0;
            ctx->candidate_page = 0;
            update_preedit(ctx);
            trigger_feedback(ctx, HIME_FEEDBACK_KEY_DELETE);
            return HIME_KEY_PREEDIT;
        }
        return HIME_KEY_IGNORED;
    }

    /* Handle character input */
    if (charcode && charcode < 128) {
        char key = (char)charcode;
        if (key >= 'A' && key <= 'Z') {
            key = key - 'A' + 'a';  /* Convert to lowercase */
        }

        const KeyMapEntry *entry = find_keymap(ctx, key);
        if (entry) {
            int typ = entry->typ;
            int num = entry->num;

            /* Insert phonetic component */
            ctx->typ_pho[typ] = num;
            ctx->inph[typ] = charcode;

            update_preedit(ctx);

            /* Check if syllable is complete (has tone or space pressed) */
            if (ctx->typ_pho[3] != 0 || key == ' ') {
                hime_phokey_t phokey = pho2key(ctx->typ_pho);
                lookup_phokey(phokey, ctx);

                if (ctx->candidate_count == 1) {
                    /* Auto-select single candidate */
                    strcpy(ctx->commit, ctx->candidates[0]);
                    hime_context_reset(ctx);
                    trigger_feedback(ctx, HIME_FEEDBACK_CANDIDATE);
                    return HIME_KEY_COMMIT;
                } else if (ctx->candidate_count > 0) {
                    trigger_feedback(ctx, key == ' ' ? HIME_FEEDBACK_KEY_SPACE : HIME_FEEDBACK_KEY_PRESS);
                    return HIME_KEY_PREEDIT;
                } else if (key == ' ') {
                    /* Space pressed but no candidates - invalid input */
                    trigger_feedback(ctx, HIME_FEEDBACK_ERROR);
                }
            } else {
                trigger_feedback(ctx, HIME_FEEDBACK_KEY_PRESS);
            }

            return HIME_KEY_PREEDIT;
        }
    }

    return HIME_KEY_IGNORED;
}

HIME_API HimeKeyResult hime_process_key(
    HimeContext *ctx,
    uint32_t keycode,
    uint32_t charcode,
    uint32_t modifiers
) {
    (void)modifiers;  /* Currently unused */

    if (!ctx || !ctx->chinese_mode) {
        return HIME_KEY_IGNORED;
    }

    /* Handle candidate selection (common to all methods) */
    if (ctx->candidate_count > 0 && charcode) {
        char c = (char)charcode;
        char *pos = strchr(ctx->sel_keys, c);
        if (pos) {
            int idx = (pos - ctx->sel_keys) +
                     (ctx->candidate_page * ctx->candidates_per_page);
            if (idx < ctx->candidate_count) {
                strcpy(ctx->commit, ctx->candidates[idx]);
                hime_context_reset(ctx);
                trigger_feedback(ctx, HIME_FEEDBACK_CANDIDATE);
                return HIME_KEY_COMMIT;
            }
        }
    }

    /* Handle Escape (common to all methods) */
    if (keycode == 0x1B || charcode == 0x1B) {
        bool has_input = !typ_pho_empty(ctx->typ_pho) ||
                        ctx->candidate_count > 0 ||
                        ctx->gtab_key_count > 0 ||
                        ctx->intcode_len > 0 ||
                        ctx->tsin_phrase_len > 0;
        if (has_input) {
            hime_context_reset(ctx);
            return HIME_KEY_ABSORBED;
        }
        return HIME_KEY_IGNORED;
    }

    /* Handle Enter key (common to all methods) */
    if (keycode == 0x0D || charcode == 0x0D) {
        /* For TSIN, commit current phrase */
        if (ctx->method == HIME_IM_TSIN && ctx->tsin_phrase_len > 0) {
            hime_tsin_commit_phrase(ctx);
            trigger_feedback(ctx, HIME_FEEDBACK_KEY_ENTER);
            return HIME_KEY_COMMIT;
        }
        /* For intcode, commit if valid */
        if (ctx->method == HIME_IM_INTCODE && ctx->intcode_len > 0) {
            char utf8[8];
            int len = hime_intcode_convert(ctx, ctx->intcode_buffer, utf8, sizeof(utf8));
            if (len > 0) {
                strcpy(ctx->commit, utf8);
                ctx->intcode_len = 0;
                ctx->intcode_buffer[0] = '\0';
                ctx->preedit[0] = '\0';
                trigger_feedback(ctx, HIME_FEEDBACK_KEY_ENTER);
                return HIME_KEY_COMMIT;
            }
        }
        trigger_feedback(ctx, HIME_FEEDBACK_KEY_ENTER);
        return HIME_KEY_IGNORED;
    }

    /* Handle Backspace based on input method */
    if (keycode == 0x08 || charcode == 0x08) {
        switch (ctx->method) {
            case HIME_IM_GTAB:
                if (ctx->gtab_key_count > 0) {
                    ctx->gtab_key_count--;
                    ctx->gtab_key_display[ctx->gtab_key_count] = '\0';
                    strcpy(ctx->preedit, ctx->gtab_key_display);
                    if (ctx->gtab_key_count > 0) {
                        gtab_lookup(ctx);
                    } else {
                        ctx->candidate_count = 0;
                    }
                    trigger_feedback(ctx, HIME_FEEDBACK_KEY_DELETE);
                    return HIME_KEY_PREEDIT;
                }
                return HIME_KEY_IGNORED;

            case HIME_IM_INTCODE:
                if (ctx->intcode_len > 0) {
                    ctx->intcode_len--;
                    ctx->intcode_buffer[ctx->intcode_len] = '\0';
                    if (ctx->intcode_len > 0) {
                        snprintf(ctx->preedit, HIME_MAX_PREEDIT, "U+%s", ctx->intcode_buffer);
                    } else {
                        ctx->preedit[0] = '\0';
                    }
                    trigger_feedback(ctx, HIME_FEEDBACK_KEY_DELETE);
                    return HIME_KEY_PREEDIT;
                }
                return HIME_KEY_IGNORED;

            case HIME_IM_TSIN:
                /* For TSIN, delete last character from phrase */
                if (ctx->tsin_phrase_len > 0) {
                    /* Find last character boundary (UTF-8) */
                    int i = strlen(ctx->tsin_phrase) - 1;
                    while (i > 0 && (ctx->tsin_phrase[i] & 0xC0) == 0x80) i--;
                    ctx->tsin_phrase[i] = '\0';
                    ctx->tsin_phrase_len--;
                    strcpy(ctx->preedit, ctx->tsin_phrase);
                    trigger_feedback(ctx, HIME_FEEDBACK_KEY_DELETE);
                    return HIME_KEY_PREEDIT;
                }
                /* Fall through to PHO backspace if phrase empty */
                /* FALLTHROUGH */

            case HIME_IM_PHO:
            default:
                return pho_process_key(ctx, keycode, charcode);
        }
    }

    /* Dispatch to input method for character input */
    if (charcode && charcode < 128) {
        char key = (char)charcode;

        switch (ctx->method) {
            case HIME_IM_GTAB:
                return gtab_process_key(ctx, key);

            case HIME_IM_INTCODE:
                return intcode_process_key(ctx, key);

            case HIME_IM_TSIN:
                /* TSIN uses PHO for input, builds phrase from selected candidates */
                /* When a candidate is selected, it's added to the phrase */
                /* Fall through to PHO processing */
                /* FALLTHROUGH */

            case HIME_IM_PHO:
            default:
                return pho_process_key(ctx, keycode, charcode);
        }
    }

    return HIME_KEY_IGNORED;
}

HIME_API int hime_get_preedit(HimeContext *ctx, char *buffer, int buffer_size) {
    if (!ctx || !buffer || buffer_size <= 0) return -1;

    int len = strlen(ctx->preedit);
    if (len >= buffer_size) len = buffer_size - 1;

    strncpy(buffer, ctx->preedit, len);
    buffer[len] = '\0';

    return len;
}

HIME_API int hime_get_preedit_cursor(HimeContext *ctx) {
    return ctx ? strlen(ctx->preedit) : 0;
}

HIME_API int hime_get_preedit_attrs(HimeContext *ctx, HimePreeditAttr *attrs, int max_attrs) {
    if (!ctx || !attrs || max_attrs <= 0) return 0;

    /* Single underline attribute for entire preedit */
    if (ctx->preedit[0]) {
        attrs[0].flag = HIME_ATTR_UNDERLINE;
        attrs[0].start = 0;
        attrs[0].end = strlen(ctx->preedit);
        return 1;
    }

    return 0;
}

HIME_API int hime_get_commit(HimeContext *ctx, char *buffer, int buffer_size) {
    if (!ctx || !buffer || buffer_size <= 0) return -1;

    int len = strlen(ctx->commit);
    if (len >= buffer_size) len = buffer_size - 1;

    strncpy(buffer, ctx->commit, len);
    buffer[len] = '\0';

    return len;
}

HIME_API void hime_clear_commit(HimeContext *ctx) {
    if (ctx) {
        ctx->commit[0] = '\0';
    }
}

HIME_API bool hime_has_candidates(HimeContext *ctx) {
    return ctx && ctx->candidate_count > 0;
}

HIME_API int hime_get_candidate_count(HimeContext *ctx) {
    return ctx ? ctx->candidate_count : 0;
}

HIME_API int hime_get_candidate(HimeContext *ctx, int index, char *buffer, int buffer_size) {
    if (!ctx || !buffer || buffer_size <= 0) return -1;
    if (index < 0 || index >= ctx->candidate_count) return -1;

    int len = strlen(ctx->candidates[index]);
    if (len >= buffer_size) len = buffer_size - 1;

    strncpy(buffer, ctx->candidates[index], len);
    buffer[len] = '\0';

    return len;
}

HIME_API int hime_get_candidate_page(HimeContext *ctx) {
    return ctx ? ctx->candidate_page : 0;
}

HIME_API int hime_get_candidates_per_page(HimeContext *ctx) {
    return ctx ? ctx->candidates_per_page : 10;
}

HIME_API HimeKeyResult hime_select_candidate(HimeContext *ctx, int index) {
    if (!ctx) return HIME_KEY_IGNORED;

    int actual_idx = ctx->candidate_page * ctx->candidates_per_page + index;
    if (actual_idx >= 0 && actual_idx < ctx->candidate_count) {
        strcpy(ctx->commit, ctx->candidates[actual_idx]);
        hime_context_reset(ctx);
        return HIME_KEY_COMMIT;
    }

    return HIME_KEY_IGNORED;
}

HIME_API bool hime_candidate_page_up(HimeContext *ctx) {
    if (!ctx || ctx->candidate_page == 0) return false;
    ctx->candidate_page--;
    return true;
}

HIME_API bool hime_candidate_page_down(HimeContext *ctx) {
    if (!ctx) return false;

    int max_page = (ctx->candidate_count - 1) / ctx->candidates_per_page;
    if (ctx->candidate_page < max_page) {
        ctx->candidate_page++;
        return true;
    }
    return false;
}

HIME_API int hime_get_bopomofo_string(HimeContext *ctx, char *buffer, int buffer_size) {
    return hime_get_preedit(ctx, buffer, buffer_size);
}

HIME_API int hime_set_keyboard_layout(HimeContext *ctx, HimeKeyboardLayout layout) {
    if (!ctx) return -1;
    if (layout < 0 || layout >= HIME_KB_COUNT) return -1;

    ctx->keyboard_layout = layout;
    hime_context_reset(ctx);  /* Reset state when changing layout */
    return 0;
}

HIME_API HimeKeyboardLayout hime_get_keyboard_layout(HimeContext *ctx) {
    return ctx ? ctx->keyboard_layout : HIME_KB_STANDARD;
}

HIME_API int hime_set_keyboard_layout_by_name(HimeContext *ctx, const char *layout_name) {
    if (!ctx || !layout_name) return -1;

    /* Search for matching layout name */
    for (int i = 0; LAYOUT_NAMES[i].name != NULL; i++) {
        if (strcmp(layout_name, LAYOUT_NAMES[i].name) == 0) {
            return hime_set_keyboard_layout(ctx, LAYOUT_NAMES[i].layout);
        }
    }

    return -1;  /* Layout not found */
}

HIME_API void hime_set_selection_keys(HimeContext *ctx, const char *keys) {
    if (!ctx || !keys) return;
    strncpy(ctx->sel_keys, keys, sizeof(ctx->sel_keys) - 1);
    ctx->sel_keys[sizeof(ctx->sel_keys) - 1] = '\0';
}

HIME_API void hime_set_candidates_per_page(HimeContext *ctx, int count) {
    if (!ctx) return;
    if (count < 1) count = 1;
    if (count > 10) count = 10;
    ctx->candidates_per_page = count;
}

/* ========== GTAB Implementation ========== */

HIME_API int hime_gtab_get_table_count(void) {
    int count = 0;
    for (int i = 0; GTAB_REGISTRY[i].filename[0] != '\0'; i++) {
        count++;
    }
    return count;
}

HIME_API int hime_gtab_get_table_info(int index, HimeGtabInfo *info) {
    if (!info) return -1;

    int count = hime_gtab_get_table_count();
    if (index < 0 || index >= count) return -1;

    strncpy(info->name, GTAB_REGISTRY[index].name, sizeof(info->name) - 1);
    strncpy(info->filename, GTAB_REGISTRY[index].filename, sizeof(info->filename) - 1);
    strncpy(info->icon, GTAB_REGISTRY[index].icon, sizeof(info->icon) - 1);

    /* Check if already loaded */
    info->loaded = false;
    for (int i = 0; i < g_gtab_table_count; i++) {
        if (strcmp(g_gtab_tables[i].filename, GTAB_REGISTRY[index].filename) == 0) {
            info->loaded = true;
            info->key_count = g_gtab_tables[i].key_count;
            info->max_keystrokes = g_gtab_tables[i].max_press;
            strncpy(info->selkey, g_gtab_tables[i].selkey, sizeof(info->selkey) - 1);
            break;
        }
    }

    return 0;
}

/* Simple GTAB loader - loads basic structure */
static GtabTable *load_gtab_file(const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return NULL;

    /* Find free slot */
    if (g_gtab_table_count >= 16) {
        fclose(fp);
        return NULL;
    }

    GtabTable *table = &g_gtab_tables[g_gtab_table_count];
    memset(table, 0, sizeof(GtabTable));

    /* Read header */
    GtabHeader header;
    if (fread(&header, sizeof(GtabHeader), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    strncpy(table->name, header.cname, sizeof(table->name) - 1);
    strncpy(table->selkey, header.selkey, sizeof(table->selkey) - 1);
    table->key_count = header.key_count;
    table->max_press = header.max_press;
    table->def_chars = header.def_chars;
    table->keybits = 6;  /* Default: 6 bits per key */

    /* Determine if using 64-bit keys */
    table->key64 = (table->max_press > 5);

    /* Read keymap (128 bytes) */
    fread(table->keymap, 1, 128, fp);

    /* Read index */
    int idx_size = (1 << table->keybits);
    table->idx = (uint32_t *)malloc(sizeof(uint32_t) * idx_size);
    if (!table->idx) {
        fclose(fp);
        return NULL;
    }
    fread(table->idx, sizeof(uint32_t), idx_size, fp);
    table->idx_count = idx_size;

    /* Read items */
    if (table->key64) {
        table->items64 = (GtabItem64 *)malloc(sizeof(GtabItem64) * table->def_chars);
        if (!table->items64) {
            free(table->idx);
            fclose(fp);
            return NULL;
        }
        fread(table->items64, sizeof(GtabItem64), table->def_chars, fp);
    } else {
        table->items = (GtabItem *)malloc(sizeof(GtabItem) * table->def_chars);
        if (!table->items) {
            free(table->idx);
            fclose(fp);
            return NULL;
        }
        fread(table->items, sizeof(GtabItem), table->def_chars, fp);
    }
    table->item_count = table->def_chars;
    table->loaded = true;

    fclose(fp);
    g_gtab_table_count++;
    return table;
}

HIME_API int hime_gtab_load_table(HimeContext *ctx, const char *filename) {
    if (!ctx || !filename) return -1;

    /* Check if already loaded */
    for (int i = 0; i < g_gtab_table_count; i++) {
        if (strcmp(g_gtab_tables[i].filename, filename) == 0) {
            ctx->gtab = &g_gtab_tables[i];
            ctx->method = HIME_IM_GTAB;
            hime_context_reset(ctx);
            return 0;
        }
    }

    /* Build full path */
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", g_data_dir, filename);

    GtabTable *table = load_gtab_file(filepath);
    if (!table) {
        /* Try alternate path */
        snprintf(filepath, sizeof(filepath), "%s/data/%s", g_data_dir, filename);
        table = load_gtab_file(filepath);
    }

    if (!table) return -1;

    strncpy(table->filename, filename, sizeof(table->filename) - 1);
    ctx->gtab = table;
    ctx->method = HIME_IM_GTAB;
    hime_context_reset(ctx);
    return 0;
}

HIME_API int hime_gtab_load_table_by_id(HimeContext *ctx, HimeGtabTable table_id) {
    if (!ctx) return -1;

    for (int i = 0; GTAB_REGISTRY[i].filename[0] != '\0'; i++) {
        if (GTAB_REGISTRY[i].id == table_id) {
            return hime_gtab_load_table(ctx, GTAB_REGISTRY[i].filename);
        }
    }

    return -1;
}

HIME_API const char *hime_gtab_get_current_table(HimeContext *ctx) {
    if (!ctx || !ctx->gtab) return "";
    return ctx->gtab->name;
}

HIME_API int hime_gtab_get_key_string(HimeContext *ctx, char *buffer, int buffer_size) {
    if (!ctx || !buffer || buffer_size <= 0) return -1;

    int len = strlen(ctx->gtab_key_display);
    if (len >= buffer_size) len = buffer_size - 1;
    strncpy(buffer, ctx->gtab_key_display, len);
    buffer[len] = '\0';
    return len;
}

/* GTAB key processing - lookup candidates based on entered keys */
static int gtab_lookup(HimeContext *ctx) {
    if (!ctx || !ctx->gtab || ctx->gtab_key_count == 0) return 0;

    ctx->candidate_count = 0;
    GtabTable *table = ctx->gtab;

    /* Build key value from entered keys */
    uint32_t key = 0;
    for (int i = 0; i < ctx->gtab_key_count; i++) {
        key = (key << table->keybits) | ctx->gtab_keys[i];
    }

    /* Search for matches */
    if (!table->key64 && table->items) {
        for (int i = 0; i < table->item_count && ctx->candidate_count < HIME_MAX_CANDIDATES; i++) {
            uint32_t item_key = (table->items[i].key[0] << 24) |
                               (table->items[i].key[1] << 16) |
                               (table->items[i].key[2] << 8) |
                               table->items[i].key[3];

            /* Match prefix */
            int shift = (table->max_press - ctx->gtab_key_count) * table->keybits;
            if ((item_key >> shift) == key) {
                strncpy(ctx->candidates[ctx->candidate_count],
                       (char *)table->items[i].ch, HIME_CH_SZ);
                ctx->candidates[ctx->candidate_count][HIME_CH_SZ] = '\0';
                ctx->candidate_count++;
            }
        }
    }

    return ctx->candidate_count;
}

/* Process GTAB key input */
static HimeKeyResult gtab_process_key(HimeContext *ctx, char key) {
    if (!ctx || !ctx->gtab) return HIME_KEY_IGNORED;

    GtabTable *table = ctx->gtab;

    /* Check if key is in keymap */
    int key_idx = -1;
    for (int i = 0; i < 128 && table->keymap[i]; i++) {
        if (table->keymap[i] == key) {
            key_idx = i;
            break;
        }
    }

    if (key_idx < 0) return HIME_KEY_IGNORED;

    /* Add key */
    if (ctx->gtab_key_count < GTAB_MAX_KEYS && ctx->gtab_key_count < table->max_press) {
        ctx->gtab_keys[ctx->gtab_key_count++] = key_idx;

        /* Update display string */
        int len = strlen(ctx->gtab_key_display);
        if (len < (int)sizeof(ctx->gtab_key_display) - 1) {
            ctx->gtab_key_display[len] = key;
            ctx->gtab_key_display[len + 1] = '\0';
        }

        /* Update preedit */
        strcpy(ctx->preedit, ctx->gtab_key_display);

        /* Lookup candidates */
        gtab_lookup(ctx);

        if (ctx->candidate_count == 1 && ctx->gtab_key_count >= table->max_press) {
            /* Auto-commit single match at max keys */
            strcpy(ctx->commit, ctx->candidates[0]);
            ctx->gtab_key_count = 0;
            ctx->gtab_key_display[0] = '\0';
            ctx->preedit[0] = '\0';
            ctx->candidate_count = 0;
            return HIME_KEY_COMMIT;
        }

        return HIME_KEY_PREEDIT;
    }

    return HIME_KEY_ABSORBED;
}

/* ========== TSIN Implementation ========== */

HIME_API int hime_tsin_load_database(HimeContext *ctx, const char *filename) {
    if (!ctx || !filename) return -1;

    /* Build full path */
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", g_data_dir, filename);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        snprintf(filepath, sizeof(filepath), "%s/data/%s", g_data_dir, filename);
        fp = fopen(filepath, "rb");
    }

    if (!fp) return -1;

    /* Free existing database */
    if (g_tsin_db.idx) {
        free(g_tsin_db.idx);
        g_tsin_db.idx = NULL;
    }
    if (g_tsin_db.phrases) {
        free(g_tsin_db.phrases);
        g_tsin_db.phrases = NULL;
    }

    /* Read header - similar to pho.tab2 format */
    uint16_t idxnum;
    int32_t phrase_count;

    fread(&idxnum, 2, 1, fp);
    fread(&idxnum, 2, 1, fp);  /* Read twice (historical quirk) */
    fread(&phrase_count, 4, 1, fp);

    g_tsin_db.idx_count = idxnum;
    g_tsin_db.phrase_count = phrase_count;
    g_tsin_db.loaded = true;

    fclose(fp);

    ctx->method = HIME_IM_TSIN;
    return 0;
}

HIME_API int hime_tsin_get_phrase(HimeContext *ctx, char *buffer, int buffer_size) {
    if (!ctx || !buffer || buffer_size <= 0) return -1;

    int len = strlen(ctx->tsin_phrase);
    if (len >= buffer_size) len = buffer_size - 1;
    strncpy(buffer, ctx->tsin_phrase, len);
    buffer[len] = '\0';
    return len;
}

HIME_API int hime_tsin_commit_phrase(HimeContext *ctx) {
    if (!ctx) return 0;

    int len = strlen(ctx->tsin_phrase);
    if (len > 0) {
        strcpy(ctx->commit, ctx->tsin_phrase);
        ctx->tsin_phrase[0] = '\0';
        ctx->tsin_phrase_len = 0;
        ctx->tsin_cursor = 0;
    }
    return len;
}

/* ========== Intcode Implementation ========== */

HIME_API void hime_intcode_set_mode(HimeContext *ctx, HimeIntcodeMode mode) {
    if (!ctx) return;
    ctx->intcode_mode = mode;
    ctx->intcode_len = 0;
    ctx->intcode_buffer[0] = '\0';
}

HIME_API HimeIntcodeMode hime_intcode_get_mode(HimeContext *ctx) {
    return ctx ? ctx->intcode_mode : HIME_INTCODE_UNICODE;
}

HIME_API int hime_intcode_get_buffer(HimeContext *ctx, char *buffer, int buffer_size) {
    if (!ctx || !buffer || buffer_size <= 0) return -1;

    int len = ctx->intcode_len;
    if (len >= buffer_size) len = buffer_size - 1;
    strncpy(buffer, ctx->intcode_buffer, len);
    buffer[len] = '\0';
    return len;
}

/* Convert Unicode codepoint to UTF-8 */
static int unicode_to_utf8(uint32_t codepoint, char *buffer) {
    if (codepoint < 0x80) {
        buffer[0] = (char)codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        buffer[0] = 0xC0 | (codepoint >> 6);
        buffer[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint < 0x10000) {
        buffer[0] = 0xE0 | (codepoint >> 12);
        buffer[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        buffer[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    } else if (codepoint < 0x110000) {
        buffer[0] = 0xF0 | (codepoint >> 18);
        buffer[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        buffer[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        buffer[3] = 0x80 | (codepoint & 0x3F);
        return 4;
    }
    return 0;
}

/* Big5 to Unicode conversion table (simplified subset) */
static uint32_t big5_to_unicode(uint16_t big5) {
    /* This is a simplified implementation - real conversion requires full table */
    /* Common Big5 range: 0xA140-0xF9FE */
    if (big5 >= 0xA440 && big5 <= 0xC67E) {
        /* Level 1 frequently used characters - approximate mapping */
        /* Full implementation would use complete conversion table */
        return 0x4E00 + (big5 - 0xA440);  /* Simplified: map to CJK range */
    }
    return big5;  /* Return as-is for unsupported codes */
}

HIME_API int hime_intcode_convert(HimeContext *ctx, const char *hex_code,
                                   char *buffer, int buffer_size) {
    if (!ctx || !hex_code || !buffer || buffer_size < 5) return 0;

    /* Parse hex code */
    uint32_t code = 0;
    for (const char *p = hex_code; *p; p++) {
        char c = *p;
        int digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else return 0;  /* Invalid hex */
        code = (code << 4) | digit;
    }

    /* Convert based on mode */
    uint32_t unicode;
    if (ctx->intcode_mode == HIME_INTCODE_BIG5) {
        unicode = big5_to_unicode((uint16_t)code);
    } else {
        unicode = code;
    }

    /* Convert to UTF-8 */
    int len = unicode_to_utf8(unicode, buffer);
    buffer[len] = '\0';
    return len;
}

/* Process intcode key input */
static HimeKeyResult intcode_process_key(HimeContext *ctx, char key) {
    if (!ctx) return HIME_KEY_IGNORED;

    /* Check if valid hex digit */
    bool valid = (key >= '0' && key <= '9') ||
                 (key >= 'A' && key <= 'F') ||
                 (key >= 'a' && key <= 'f');

    if (!valid) return HIME_KEY_IGNORED;

    /* Max digits: 4 for Big5, 6 for Unicode */
    int max_digits = (ctx->intcode_mode == HIME_INTCODE_BIG5) ? 4 : 6;

    if (ctx->intcode_len < max_digits) {
        ctx->intcode_buffer[ctx->intcode_len++] = (key >= 'a') ? (key - 32) : key;
        ctx->intcode_buffer[ctx->intcode_len] = '\0';

        /* Update preedit to show hex digits */
        snprintf(ctx->preedit, HIME_MAX_PREEDIT, "U+%s", ctx->intcode_buffer);

        /* Auto-commit at max digits */
        if (ctx->intcode_len >= max_digits) {
            char utf8[8];
            int len = hime_intcode_convert(ctx, ctx->intcode_buffer, utf8, sizeof(utf8));
            if (len > 0) {
                strcpy(ctx->commit, utf8);
                ctx->intcode_len = 0;
                ctx->intcode_buffer[0] = '\0';
                ctx->preedit[0] = '\0';
                return HIME_KEY_COMMIT;
            }
        }

        return HIME_KEY_PREEDIT;
    }

    return HIME_KEY_ABSORBED;
}

/* ========== Input Method Availability ========== */

HIME_API bool hime_is_method_available(HimeInputMethod method) {
    switch (method) {
        case HIME_IM_PHO:
            return g_pho_table.items != NULL;
        case HIME_IM_TSIN:
            return true;  /* Always available, uses PHO table */
        case HIME_IM_GTAB:
            return true;  /* Available if tables exist */
        case HIME_IM_INTCODE:
            return true;  /* Always available */
        case HIME_IM_ANTHY:
        case HIME_IM_CHEWING:
            return false; /* External modules - not bundled */
        default:
            return false;
    }
}

HIME_API const char *hime_get_method_name(HimeInputMethod method) {
    if (method >= 0 && method < HIME_IM_COUNT) {
        return INPUT_METHOD_NAMES[method];
    }
    return "Unknown";
}

/* ========== Settings/Preferences Implementation ========== */

HIME_API void hime_set_charset(HimeContext *ctx, HimeCharset charset) {
    if (!ctx) return;
    ctx->charset = charset;
}

HIME_API HimeCharset hime_get_charset(HimeContext *ctx) {
    return ctx ? ctx->charset : HIME_CHARSET_TRADITIONAL;
}

HIME_API HimeCharset hime_toggle_charset(HimeContext *ctx) {
    if (!ctx) return HIME_CHARSET_TRADITIONAL;
    ctx->charset = (ctx->charset == HIME_CHARSET_TRADITIONAL)
                   ? HIME_CHARSET_SIMPLIFIED
                   : HIME_CHARSET_TRADITIONAL;
    return ctx->charset;
}

HIME_API void hime_set_smart_punctuation(HimeContext *ctx, bool enabled) {
    if (!ctx) return;
    ctx->smart_punctuation = enabled;
}

HIME_API bool hime_get_smart_punctuation(HimeContext *ctx) {
    return ctx ? ctx->smart_punctuation : false;
}

HIME_API void hime_set_pinyin_annotation(HimeContext *ctx, bool enabled) {
    if (!ctx) return;
    ctx->pinyin_annotation = enabled;
}

HIME_API bool hime_get_pinyin_annotation(HimeContext *ctx) {
    return ctx ? ctx->pinyin_annotation : false;
}

HIME_API int hime_get_pinyin_for_char(
    HimeContext *ctx,
    const char *character,
    char *buffer,
    int buffer_size
) {
    /* TODO: Implement Pinyin lookup table */
    (void)ctx;
    (void)character;
    if (buffer && buffer_size > 0) {
        buffer[0] = '\0';
    }
    return 0;
}

HIME_API void hime_set_candidate_style(HimeContext *ctx, HimeCandidateStyle style) {
    if (!ctx) return;
    ctx->candidate_style = style;
}

HIME_API HimeCandidateStyle hime_get_candidate_style(HimeContext *ctx) {
    return ctx ? ctx->candidate_style : HIME_CAND_STYLE_HORIZONTAL;
}

/* ========== Feedback Implementation ========== */

HIME_API void hime_set_feedback_callback(
    HimeContext *ctx,
    HimeFeedbackCallback callback,
    void *user_data
) {
    if (!ctx) return;
    ctx->feedback_callback = callback;
    ctx->feedback_user_data = user_data;
}

static void trigger_feedback(HimeContext *ctx, HimeFeedbackType type) {
    if (ctx && ctx->feedback_callback) {
        ctx->feedback_callback(type, ctx->feedback_user_data);
    }
}

HIME_API void hime_set_sound_enabled(HimeContext *ctx, bool enabled) {
    if (!ctx) return;
    ctx->sound_enabled = enabled;
}

HIME_API bool hime_get_sound_enabled(HimeContext *ctx) {
    return ctx ? ctx->sound_enabled : false;
}

HIME_API void hime_set_vibration_enabled(HimeContext *ctx, bool enabled) {
    if (!ctx) return;
    ctx->vibration_enabled = enabled;
}

HIME_API bool hime_get_vibration_enabled(HimeContext *ctx) {
    return ctx ? ctx->vibration_enabled : false;
}

HIME_API void hime_set_vibration_duration(HimeContext *ctx, int duration_ms) {
    if (!ctx) return;
    if (duration_ms < 1) duration_ms = 1;
    if (duration_ms > 500) duration_ms = 500;
    ctx->vibration_duration_ms = duration_ms;
}

HIME_API int hime_get_vibration_duration(HimeContext *ctx) {
    return ctx ? ctx->vibration_duration_ms : 20;
}

/* ========== Smart Punctuation Implementation ========== */

/* Punctuation conversion table */
typedef struct {
    char ascii;
    const char *chinese;
    const char *chinese_alt;  /* Alternate for paired punctuation */
} PunctuationEntry;

static const PunctuationEntry PUNCTUATION_TABLE[] = {
    {',', "，", NULL},
    {'.', "。", NULL},
    {'?', "？", NULL},
    {'!', "！", NULL},
    {':', "：", NULL},
    {';', "；", NULL},
    {'(', "（", NULL},
    {')', "）", NULL},
    {'[', "「", NULL},
    {']', "」", NULL},
    {'{', "『", NULL},
    {'}', "』", NULL},
    {'<', "《", NULL},
    {'>', "》", NULL},
    {'"', """, """},  /* Opening/closing double quotes */
    {'\'', "'", "'"},  /* Opening/closing single quotes */
    {'~', "～", NULL},
    {'@', "＠", NULL},
    {'#', "＃", NULL},
    {'$', "￥", NULL},
    {'%', "％", NULL},
    {'^', "……", NULL},
    {'&', "＆", NULL},
    {'*', "×", NULL},
    {'-', "—", NULL},
    {'_', "——", NULL},
    {'+', "＋", NULL},
    {'=', "＝", NULL},
    {'/', "、", NULL},
    {'\\', "＼", NULL},
    {'|', "｜", NULL},
    {0, NULL, NULL}
};

HIME_API void hime_reset_punctuation_state(HimeContext *ctx) {
    if (!ctx) return;
    ctx->quote_open_double = false;
    ctx->quote_open_single = false;
}

HIME_API int hime_convert_punctuation(
    HimeContext *ctx,
    char ascii,
    char *buffer,
    int buffer_size
) {
    if (!ctx || !buffer || buffer_size <= 0) return 0;
    buffer[0] = '\0';

    if (!ctx->smart_punctuation) return 0;

    for (int i = 0; PUNCTUATION_TABLE[i].ascii != 0; i++) {
        if (PUNCTUATION_TABLE[i].ascii == ascii) {
            const char *result = PUNCTUATION_TABLE[i].chinese;

            /* Handle paired punctuation (quotes) */
            if (ascii == '"' && PUNCTUATION_TABLE[i].chinese_alt) {
                if (ctx->quote_open_double) {
                    result = PUNCTUATION_TABLE[i].chinese_alt;
                }
                ctx->quote_open_double = !ctx->quote_open_double;
            } else if (ascii == '\'' && PUNCTUATION_TABLE[i].chinese_alt) {
                if (ctx->quote_open_single) {
                    result = PUNCTUATION_TABLE[i].chinese_alt;
                }
                ctx->quote_open_single = !ctx->quote_open_single;
            }

            if (result) {
                int len = strlen(result);
                if (len < buffer_size) {
                    strcpy(buffer, result);
                    return len;
                }
            }
            break;
        }
    }

    return 0;
}

/* ========== Theme/Color Implementation ========== */

HIME_API void hime_set_color_scheme(HimeContext *ctx, HimeColorScheme scheme) {
    if (!ctx) return;
    ctx->color_scheme = scheme;
}

HIME_API HimeColorScheme hime_get_color_scheme(HimeContext *ctx) {
    return ctx ? ctx->color_scheme : HIME_COLOR_SCHEME_LIGHT;
}

HIME_API void hime_set_system_dark_mode(HimeContext *ctx, bool is_dark) {
    if (!ctx) return;
    ctx->system_dark_mode = is_dark;
}

/* ========== Extended Candidate Info ========== */

HIME_API int hime_get_candidate_with_annotation(
    HimeContext *ctx,
    int index,
    char *text_buffer,
    int text_size,
    char *annotation_buffer,
    int annotation_size
) {
    if (!ctx || !text_buffer || text_size <= 0) return -1;

    /* Get the candidate text */
    int len = hime_get_candidate(ctx, index, text_buffer, text_size);
    if (len < 0) return -1;

    /* Get annotation if requested and enabled */
    if (annotation_buffer && annotation_size > 0) {
        if (ctx->pinyin_annotation) {
            hime_get_pinyin_for_char(ctx, text_buffer, annotation_buffer, annotation_size);
        } else {
            annotation_buffer[0] = '\0';
        }
    }

    return len;
}
