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
#define HIME_VERSION "0.10.1-win"

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

static const KeyMapEntry ZHUYIN_KEYMAP[] = {
    /* Initials */
    {'1', 1, 0}, {'q', 2, 0}, {'a', 3, 0}, {'z', 4, 0},
    {'2', 5, 0}, {'w', 6, 0}, {'s', 7, 0}, {'x', 8, 0},
    {'e', 9, 0}, {'d', 10, 0}, {'c', 11, 0},
    {'r', 12, 0}, {'f', 13, 0}, {'v', 14, 0},
    {'5', 15, 0}, {'t', 16, 0}, {'g', 17, 0}, {'b', 18, 0},
    {'y', 19, 0}, {'h', 20, 0}, {'n', 21, 0},
    /* Medials */
    {'u', 1, 1}, {'j', 2, 1}, {'m', 3, 1},
    /* Finals */
    {'8', 1, 2}, {'i', 2, 2}, {'k', 3, 2}, {',', 4, 2},
    {'9', 5, 2}, {'o', 6, 2}, {'l', 7, 2}, {'.', 8, 2},
    {'0', 9, 2}, {'p', 10, 2}, {';', 11, 2}, {'/', 12, 2},
    {'-', 13, 2},
    /* Tones */
    {'3', 2, 3}, {'4', 3, 3}, {'6', 4, 3}, {'7', 5, 3},
    {' ', 1, 3},
    {0, 0, 0}  /* Sentinel */
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

    /* Selection keys */
    char sel_keys[16];
};

/* Global state */
static char g_data_dir[512] = "";
static PhoTable g_pho_table = {0};
static bool g_initialized = false;

/* ========== Internal Functions ========== */

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

static const KeyMapEntry *find_keymap(char key) {
    for (int i = 0; ZHUYIN_KEYMAP[i].key != 0; i++) {
        if (ZHUYIN_KEYMAP[i].key == key) {
            return &ZHUYIN_KEYMAP[i];
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
    ctx->candidates_per_page = 10;
    strcpy(ctx->sel_keys, "1234567890");

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

        const KeyMapEntry *entry = find_keymap(key);
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
                    return HIME_KEY_COMMIT;
                } else if (ctx->candidate_count > 0) {
                    return HIME_KEY_PREEDIT;
                }
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

HIME_API int hime_set_keyboard_layout(HimeContext *ctx, const char *layout_name) {
    /* TODO: Load different keyboard mappings */
    (void)ctx;
    (void)layout_name;
    return 0;
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
