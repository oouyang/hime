/*
 * cin2gtab — Convert .cin table files to GTAB v2 compact binary format
 *
 * Reads a .cin input table (used by Cangjie, Array, etc.) and writes
 * a .gtab v2 file with a 72-byte header and pre-sorted items for
 * binary search lookup (O(log n) per keystroke).
 *
 * Usage: cin2gtab input.cin output.gtab
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hime-core.h"

/* V2 magic and header (must match hime-core-impl.c) */
#define GTAB_V2_MAGIC 0x48475432

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    char cname[32];
    char selkey[12];
    uint8_t space_style;
    uint8_t key_count;
    uint8_t max_press;
    uint8_t keybits;
    uint32_t item_count;
    uint32_t keymap_offset;
    uint32_t keyname_offset;
    uint32_t items_offset;
} GtabHeaderV2; /* 72 bytes */

/* GTAB item (32-bit key) */
typedef struct {
    uint8_t key[4];
    char ch[HIME_CH_SZ];
} GtabItem;

/* GTAB item (64-bit key) */
typedef struct {
    uint8_t key[8];
    char ch[HIME_CH_SZ];
} GtabItem64;

#define MAX_KEYS 128
#define MAX_ITEMS 200000
#define LINE_MAX_LEN 1024

/* Parse state */
static char g_cname[32];
static char g_selkey[12] = "1234567890";
static int g_space_style = 0;
static char g_keymap[MAX_KEYS];                /* index → ASCII char */
static char g_keyname[MAX_KEYS * HIME_CH_SZ];  /* index → radical name */
static int g_key_count = 0;
static int g_max_press = 0;

/* Reverse map: ASCII char → key index */
static int g_char_to_idx[128];

/* Items */
static GtabItem *g_items;
static GtabItem64 *g_items64;
static int g_item_count = 0;
static int g_keybits = 0;
static int g_key64 = 0;

static void
strip_newline (char *s)
{
    char *p = strchr (s, '\n');
    if (p)
        *p = '\0';
    p = strchr (s, '\r');
    if (p)
        *p = '\0';
}

/* Store key string and UTF-8 char temporarily for two-pass packing */
typedef struct {
    char keystr[16]; /* ASCII key sequence, e.g. "hqmol" */
    char ch[HIME_CH_SZ];
} RawEntry;

static RawEntry *g_raw_entries;
static int g_raw_count = 0;

static int
compare_items (const void *a, const void *b)
{
    const GtabItem *ia = (const GtabItem *) a;
    const GtabItem *ib = (const GtabItem *) b;
    return memcmp (ia->key, ib->key, 4);
}

static int
compare_items64 (const void *a, const void *b)
{
    const GtabItem64 *ia = (const GtabItem64 *) a;
    const GtabItem64 *ib = (const GtabItem64 *) b;
    return memcmp (ia->key, ib->key, 8);
}

static int
parse_cin (const char *path)
{
    FILE *fp = fopen (path, "r");
    if (!fp) {
        fprintf (stderr, "Error: cannot open %s\n", path);
        return -1;
    }

    memset (g_char_to_idx, -1, sizeof (g_char_to_idx));
    g_raw_entries = (RawEntry *) malloc (sizeof (RawEntry) * MAX_ITEMS);
    if (!g_raw_entries) {
        fclose (fp);
        return -1;
    }

    char line[LINE_MAX_LEN];
    int in_keyname = 0, in_chardef = 0;
    int keyname_idx = 0;

    while (fgets (line, sizeof (line), fp)) {
        strip_newline (line);

        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\0')
            continue;

        /* Parse directives */
        if (strncmp (line, "%cname ", 7) == 0) {
            strncpy (g_cname, line + 7, sizeof (g_cname) - 1);
            continue;
        }
        if (strncmp (line, "%selkey ", 8) == 0) {
            strncpy (g_selkey, line + 8, sizeof (g_selkey) - 1);
            continue;
        }
        if (strncmp (line, "%space_style ", 13) == 0) {
            g_space_style = atoi (line + 13);
            continue;
        }
        if (strcmp (line, "%keyname begin") == 0) {
            in_keyname = 1;
            keyname_idx = 0;
            continue;
        }
        if (strcmp (line, "%keyname end") == 0) {
            in_keyname = 0;
            continue;
        }
        if (strcmp (line, "%chardef begin") == 0) {
            in_chardef = 1;
            continue;
        }
        if (strcmp (line, "%chardef end") == 0) {
            in_chardef = 0;
            continue;
        }

        /* Skip other % directives */
        if (line[0] == '%')
            continue;

        if (in_keyname) {
            /* Format: "a 日" — ASCII key followed by radical name */
            char key_ch = line[0];
            char *radical = line + 1;
            while (*radical == ' ' || *radical == '\t')
                radical++;

            if (key_ch >= '!' && key_ch <= '~' && keyname_idx < MAX_KEYS) {
                g_keymap[keyname_idx] = key_ch;
                g_char_to_idx[(unsigned char) key_ch] = keyname_idx;

                /* Copy radical name (up to HIME_CH_SZ bytes) */
                memset (&g_keyname[keyname_idx * HIME_CH_SZ], 0, HIME_CH_SZ);
                int rlen = (int) strlen (radical);
                if (rlen > HIME_CH_SZ)
                    rlen = HIME_CH_SZ;
                memcpy (&g_keyname[keyname_idx * HIME_CH_SZ], radical, rlen);

                keyname_idx++;
            }
            continue;
        }

        if (in_chardef) {
            /* Format: "hqmol 嗨" — key sequence + char */
            if (g_raw_count >= MAX_ITEMS)
                continue;

            char keystr[16] = {0};
            char ch[32] = {0};
            if (sscanf (line, "%15s %31s", keystr, ch) != 2)
                continue;

            strncpy (g_raw_entries[g_raw_count].keystr, keystr,
                     sizeof (g_raw_entries[g_raw_count].keystr) - 1);

            /* Copy UTF-8 char (up to HIME_CH_SZ bytes) */
            memset (g_raw_entries[g_raw_count].ch, 0, HIME_CH_SZ);
            int clen = (int) strlen (ch);
            if (clen > HIME_CH_SZ)
                clen = HIME_CH_SZ;
            memcpy (g_raw_entries[g_raw_count].ch, ch, clen);

            /* Track max key length */
            int klen = (int) strlen (keystr);
            if (klen > g_max_press)
                g_max_press = klen;

            g_raw_count++;
            continue;
        }
    }

    g_key_count = keyname_idx;
    fclose (fp);
    return 0;
}

static int
build_items (void)
{
    /* Compute keybits: ceil(log2(key_count + 1)) */
    int n = g_key_count + 1;
    g_keybits = 1;
    while ((1 << g_keybits) < n)
        g_keybits++;

    /* Determine if 64-bit keys needed */
    g_key64 = (g_max_press * g_keybits > 32);

    g_item_count = g_raw_count;

    if (!g_key64) {
        g_items = (GtabItem *) calloc (g_item_count, sizeof (GtabItem));
        if (!g_items)
            return -1;

        for (int i = 0; i < g_item_count; i++) {
            const char *ks = g_raw_entries[i].keystr;
            int len = (int) strlen (ks);

            /* Pack key bits left-aligned in max_press * keybits field */
            uint32_t val = 0;
            for (int j = 0; j < len; j++) {
                int idx = g_char_to_idx[(unsigned char) ks[j]];
                if (idx < 0)
                    idx = 0;
                val = (val << g_keybits) | (uint32_t) idx;
            }
            /* Left-align: shift by remaining key positions */
            val <<= (g_max_press - len) * g_keybits;

            g_items[i].key[0] = (uint8_t) (val >> 24);
            g_items[i].key[1] = (uint8_t) (val >> 16);
            g_items[i].key[2] = (uint8_t) (val >> 8);
            g_items[i].key[3] = (uint8_t) val;
            memcpy (g_items[i].ch, g_raw_entries[i].ch, HIME_CH_SZ);
        }

        /* Sort items by key (ascending) for binary search */
        qsort (g_items, g_item_count, sizeof (GtabItem), compare_items);
    } else {
        g_items64 = (GtabItem64 *) calloc (g_item_count, sizeof (GtabItem64));
        if (!g_items64)
            return -1;

        for (int i = 0; i < g_item_count; i++) {
            const char *ks = g_raw_entries[i].keystr;
            int len = (int) strlen (ks);

            uint64_t val = 0;
            for (int j = 0; j < len; j++) {
                int idx = g_char_to_idx[(unsigned char) ks[j]];
                if (idx < 0)
                    idx = 0;
                val = (val << g_keybits) | (uint64_t) idx;
            }
            val <<= (g_max_press - len) * g_keybits;

            g_items64[i].key[0] = (uint8_t) (val >> 56);
            g_items64[i].key[1] = (uint8_t) (val >> 48);
            g_items64[i].key[2] = (uint8_t) (val >> 40);
            g_items64[i].key[3] = (uint8_t) (val >> 32);
            g_items64[i].key[4] = (uint8_t) (val >> 24);
            g_items64[i].key[5] = (uint8_t) (val >> 16);
            g_items64[i].key[6] = (uint8_t) (val >> 8);
            g_items64[i].key[7] = (uint8_t) val;
            memcpy (g_items64[i].ch, g_raw_entries[i].ch, HIME_CH_SZ);
        }

        qsort (g_items64, g_item_count, sizeof (GtabItem64), compare_items64);
    }

    return 0;
}

static int
write_v2 (const char *path)
{
    FILE *fp = fopen (path, "wb");
    if (!fp) {
        fprintf (stderr, "Error: cannot write %s\n", path);
        return -1;
    }

    /* Compute section offsets */
    uint32_t hdr_size = (uint32_t) sizeof (GtabHeaderV2); /* 72 */
    uint32_t keymap_off = hdr_size;
    uint32_t keyname_off = keymap_off + (uint32_t) g_key_count;
    uint32_t items_off = keyname_off + (uint32_t) (g_key_count * HIME_CH_SZ);

    /* Build header */
    GtabHeaderV2 hdr;
    memset (&hdr, 0, sizeof (hdr));
    hdr.magic = GTAB_V2_MAGIC;
    hdr.version = 0x0002;
    hdr.flags = g_key64 ? 1 : 0;
    strncpy (hdr.cname, g_cname, sizeof (hdr.cname) - 1);
    strncpy (hdr.selkey, g_selkey, sizeof (hdr.selkey) - 1);
    hdr.space_style = (uint8_t) g_space_style;
    hdr.key_count = (uint8_t) g_key_count;
    hdr.max_press = (uint8_t) g_max_press;
    hdr.keybits = (uint8_t) g_keybits;
    hdr.item_count = (uint32_t) g_item_count;
    hdr.keymap_offset = keymap_off;
    hdr.keyname_offset = keyname_off;
    hdr.items_offset = items_off;

    /* Write header */
    fwrite (&hdr, sizeof (hdr), 1, fp);

    /* Write keymap */
    fwrite (g_keymap, 1, g_key_count, fp);

    /* Write keyname */
    fwrite (g_keyname, HIME_CH_SZ, g_key_count, fp);

    /* Write items */
    if (g_key64) {
        fwrite (g_items64, sizeof (GtabItem64), g_item_count, fp);
    } else {
        fwrite (g_items, sizeof (GtabItem), g_item_count, fp);
    }

    fclose (fp);
    return 0;
}

int
main (int argc, char **argv)
{
    if (argc != 3) {
        fprintf (stderr, "Usage: %s input.cin output.gtab\n", argv[0]);
        return 1;
    }

    if (parse_cin (argv[1]) != 0)
        return 1;

    fprintf (stderr, "  cname:      %s\n", g_cname);
    fprintf (stderr, "  key_count:  %d\n", g_key_count);
    fprintf (stderr, "  entries:    %d\n", g_raw_count);
    fprintf (stderr, "  max_press:  %d\n", g_max_press);

    if (build_items () != 0) {
        fprintf (stderr, "Error: failed to build items\n");
        return 1;
    }

    fprintf (stderr, "  keybits:    %d\n", g_keybits);
    fprintf (stderr, "  key64:      %s\n", g_key64 ? "yes" : "no");

    if (write_v2 (argv[2]) != 0)
        return 1;

    fprintf (stderr, "  wrote:      %s\n", argv[2]);

    /* Cleanup */
    free (g_raw_entries);
    free (g_items);
    free (g_items64);

    return 0;
}
