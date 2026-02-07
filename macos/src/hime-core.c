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
#define HIME_VERSION "0.10.1-macos"

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

    return ctx;
}

HIME_API void hime_context_free(HimeContext *ctx) {
    if (ctx) {
        free(ctx);
    }
}

HIME_API void hime_context_reset(HimeContext *ctx) {
    if (!ctx) return;

    memset(ctx->typ_pho, 0, sizeof(ctx->typ_pho));
    memset(ctx->inph, 0, sizeof(ctx->inph));
    ctx->preedit[0] = '\0';
    ctx->commit[0] = '\0';
    ctx->candidate_count = 0;
    ctx->candidate_page = 0;
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

    /* Handle candidate selection */
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

    /* Handle Escape */
    if (keycode == 0x1B || charcode == 0x1B) {
        if (!typ_pho_empty(ctx->typ_pho) || ctx->candidate_count > 0) {
            hime_context_reset(ctx);
            return HIME_KEY_ABSORBED;
        }
        return HIME_KEY_IGNORED;
    }

    /* Handle Backspace */
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

    /* Handle Enter key */
    if (keycode == 0x0D || charcode == 0x0D) {
        trigger_feedback(ctx, HIME_FEEDBACK_KEY_ENTER);
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
