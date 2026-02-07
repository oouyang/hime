/*
 * Copyright (C) 2020 The HIME team, Taiwan
 * Copyright (C) 2011-2012 Huang, Kai-Chang (Solomon Huang) <kaichanh@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#if FREEBSD
#include <sys/param.h>
#include <sys/stat.h>
#endif

#include "hime.h"

#include "gtab.h"
#include "hime-endian.h"

#define MAX_FILENAME_LEN 512

/* Write UTF-8 bytes to file, up to n bytes */
static int futf8cpy_bytes (FILE *fp, const char *s, int n) {
    int tn = 0;

    while (tn < n && *s) {
        int sz = utf8_sz (s);
        fwrite (s, 1, sz, fp);
        tn += sz;
        s += sz;
    }
    return tn;
}

static int is_endkey (const struct TableHead *th, char key) {
    if (key == ' ')
        return 1;
    for (size_t i = 0; i < sizeof (th->endkey); i++) {
        if (key == th->endkey[i])
            return 1;
    }
    return 0;
}

static u_int convert_key32 (const unsigned char *key32) {
    return (u_int) key32[0] |
           ((u_int) key32[1] << 8) |
           ((u_int) key32[2] << 16) |
           ((u_int) key32[3] << 24);
}

static u_int64_t convert_key64 (const unsigned char *key64) {
    return (u_int64_t) key64[0] |
           ((u_int64_t) key64[1] << 8) |
           ((u_int64_t) key64[2] << 16) |
           ((u_int64_t) key64[3] << 24) |
           ((u_int64_t) key64[4] << 32) |
           ((u_int64_t) key64[5] << 40) |
           ((u_int64_t) key64[6] << 48) |
           ((u_int64_t) key64[7] << 56);
}

static void bot_output (int status, const struct TableHead *th) {
    if (status == 0 && th) {
        printf ("0:%d:%d:%d", th->keybits, th->MaxPress, th->DefC);
    } else {
        printf ("%d:0:0:0", status);
    }
}

static void usage (void) {
    printf (
        "hime-gtab2cin - convert gtab to cin format\n\n"
        "Usage: hime-gtab2cin -i <gtab> -o <cin>\n\n"
        "Options:\n"
        "    -h         Show this help message\n"
        "    -i FILE    Input table (.gtab) filename\n"
        "    -o FILE    Output table (.cin) filename\n"
        "    -b         Machine-readable output\n");
}

/**
 * Write a single chardef entry to the output file.
 * Handles both single characters and phrases.
 */
static void write_chardef_entry (FILE *fw, u_int64_t key, const unsigned char *ch, const char *keymap, const struct TableHead *th, int bits_per_key, const int *phridx, const char *phrbuf) {
    u_int64_t mask = ((u_int64_t) 1 << th->keybits) - 1;

    /* Output the key sequence */
    for (int key_seq = 0; key_seq < th->MaxPress; key_seq++) {
        int key_idx = (key >> (th->keybits * (bits_per_key - key_seq - 1))) & mask;
        /* Prevent leading # which would be treated as comment */
        if (key_seq == 0 && keymap[key_idx] == '#')
            fputc (' ', fw);
        fputc (keymap[key_idx], fw);
    }
    fputc (' ', fw);

    /* Output the character or phrase */
    if (ch[0] == 0) {
        /* Phrase: index stored in ch[0-2] */
        int idx = (ch[0] << 16) | (ch[1] << 8) | ch[2];
        int phr_len = phridx[idx + 2] - phridx[idx + 1];
        char phr_str[MAX_CIN_PHR + 1];

        memset (phr_str, 0, sizeof (phr_str));
        memcpy (phr_str, phrbuf + phridx[idx + 1], phr_len);
        fprintf (fw, "%s\n", phr_str);
    } else {
        /* Single character */
        futf8cpy_bytes (fw, (const char *) ch, CH_SZ);
        fputc ('\n', fw);
    }
}

int main (int argc, char **argv) {
    static const char CIN_HEADER[] = "#\n# cin file created via hime-gtab2cin\n#\n";
    FILE *fr, *fw;
    char fname_cin[MAX_FILENAME_LEN] = "";
    char fname_tab[MAX_FILENAME_LEN] = "";
    struct TableHead *th;
    char *kname;
    char *keymap;
    int quick_def = 0;
    char *gtabbuf = NULL;
    long gtablen = 0;
    QUICK_KEYS qkeys;
    int *phridx;
    char *phrbuf;
    int opt;
    int bot = 0;

    while ((opt = getopt (argc, argv, "i:o:bh")) != -1) {
        switch (opt) {
        case 'i':
            strncpy (fname_tab, optarg, sizeof (fname_tab) - 1);
            fname_tab[sizeof (fname_tab) - 1] = '\0';
            break;
        case 'o':
            strncpy (fname_cin, optarg, sizeof (fname_cin) - 1);
            fname_cin[sizeof (fname_cin) - 1] = '\0';
            break;
        case 'b':
            bot = 1;
            break;
        case 'h':
        default:
            usage ();
            exit (1);
        }
    }

    if (!fname_cin[0] || !fname_tab[0]) {
        if (bot)
            bot_output (-1, NULL);
        else
            usage ();
        exit (1);
    }

    if ((fr = fopen (fname_tab, "rb")) == NULL) {
        if (bot) {
            bot_output (1, NULL);
            exit (1);
        }
        p_err ("Cannot open %s\n", fname_tab);
    }

    /* Read entire gtab file into memory */
    fseek (fr, 0L, SEEK_END);
    gtablen = ftell (fr);
    rewind (fr);

    if (gtablen <= 0) {
        fclose (fr);
        if (bot)
            bot_output (1, NULL);
        p_err ("Invalid file size for %s\n", fname_tab);
    }

    gtabbuf = malloc (gtablen);
    if (!gtabbuf) {
        fclose (fr);
        if (bot)
            bot_output (1, NULL);
        p_err ("Cannot allocate memory\n");
    }

    if (fread (gtabbuf, 1, gtablen, fr) != (size_t) gtablen) {
        free (gtabbuf);
        fclose (fr);
        if (bot)
            bot_output (1, NULL);
        p_err ("Read %s fail\n", fname_tab);
    }
    fclose (fr);

    if ((fw = fopen (fname_cin, "wb")) == NULL) {
        free (gtabbuf);
        if (bot) {
            bot_output (2, NULL);
            exit (1);
        }
        p_err ("Cannot create %s\n", fname_cin);
    }

    th = (struct TableHead *) gtabbuf;

    /* Extract base name from input file for ename */
    const char *base_name = strrchr (fname_tab, '/');
    base_name = base_name ? base_name + 1 : fname_tab;

    /* Write CIN header */
    fprintf (fw, "%s", CIN_HEADER);
    fprintf (fw, "%%gen_inp\n");
    fprintf (fw, "%%ename %s\n", base_name);
    fprintf (fw, "%%cname %s\n", th->cname);
    fprintf (fw, "%%selkey ");
    if (th->selkey[sizeof (th->selkey) - 1] == 0) {
        fprintf (fw, "%s\n", th->selkey);
    } else {
        fwrite (th->selkey, 1, sizeof (th->selkey), fw);
        fprintf (fw, "%s\n", th->selkey2);
    }
    fprintf (fw, "%%dupsel %d\n", th->M_DUP_SEL);
    if (strlen (th->endkey))
        fprintf (fw, "%%endkey %s\n", th->endkey);
    fprintf (fw, "%%space_style %d\n", th->space_style);
    if (th->flag & FLAG_KEEP_KEY_CASE)
        fprintf (fw, "%%keep_key_case\n");
    if (th->flag & FLAG_GTAB_SYM_KBM)
        fprintf (fw, "%%symbol_kbm\n");
    if (th->flag & FLAG_PHRASE_AUTO_SKIP_ENDKEY)
        fprintf (fw, "%%phase_auto_skip_endkey\n");
    if (th->flag & FLAG_AUTO_SELECT_BY_PHRASE)
        fprintf (fw, "%%flag_auto_select_by_phrase\n");
    if (th->flag & FLAG_GTAB_DISP_PARTIAL_MATCH)
        fprintf (fw, "%%flag_disp_partial_match\n");
    if (th->flag & FLAG_GTAB_DISP_FULL_MATCH)
        fprintf (fw, "%%flag_disp_full_match\n");
    if (th->flag & FLAG_GTAB_VERTICAL_SELECTION)
        fprintf (fw, "%%flag_vertical_selection\n");
    if (th->flag & FLAG_GTAB_PRESS_FULL_AUTO_SEND)
        fprintf (fw, "%%flag_press_full_auto_send\n");
    if (th->flag & FLAG_GTAB_UNIQUE_AUTO_SEND)
        fprintf (fw, "%%flag_unique_auto_send\n");

    keymap = gtabbuf + sizeof (struct TableHead);
    kname = keymap + th->KeyS;
    fprintf (fw, "%%keyname begin\n");
    for (int key_idx = 1; key_idx < th->KeyS; key_idx++) {
        /* prevent leading # */
        if (keymap[key_idx] == '#')
            fputc (' ', fw);
        fprintf (fw, "%c ", keymap[key_idx]);
        futf8cpy_bytes (fw, kname + CH_SZ * key_idx, CH_SZ);
        fputc ('\n', fw);
    }
    fprintf (fw, "%%keyname end\n");

    /* check quick def */
    memset (&qkeys, 0, sizeof (qkeys));
    if (0 != memcmp (&qkeys, &th->qkeys, sizeof (qkeys)))
        quick_def = 1;
    if (quick_def) {
        fprintf (fw, "%%quick begin\n");
        for (int key_idx = 0; key_idx < th->KeyS; key_idx++) {
            if (is_endkey (th, keymap[key_idx + 1]))
                continue;
            fprintf (fw, "%c ", keymap[key_idx + 1]);
            for (int i = 0; i < 10; i++)
                futf8cpy_bytes (fw, th->qkeys.quick1[key_idx][i], CH_SZ);
            fputc ('\n', fw);
        }
        for (int key_idx = 0; key_idx < th->KeyS; key_idx++) {
            for (int key_idx2 = 0; key_idx2 < th->KeyS; key_idx2++) {
                if (is_endkey (th, keymap[key_idx + 1]))
                    continue;
                if (is_endkey (th, keymap[key_idx2 + 1]))
                    continue;
                fprintf (fw, "%c%c ", keymap[key_idx + 1], keymap[key_idx2 + 1]);
                for (int i = 0; i < 10; i++) {
                    if (0 == futf8cpy_bytes (fw, th->qkeys.quick2[key_idx][key_idx2][i], CH_SZ))
                        futf8cpy_bytes (fw, "□", CH_SZ);
                }
                fputc ('\n', fw);
            }
        }
        fprintf (fw, "%%quick end\n");
    }

    fprintf (fw, "%%chardef begin\n");

    /* older gtab */
    if (th->keybits == 0) {
        if (th->MaxPress <= 5)
            th->keybits = 6;
        else
            th->keybits = 7;
    }

    if (th->keybits * th->MaxPress <= 32) {
        ITEM *item = (ITEM *) (kname + (CH_SZ * th->KeyS) + (sizeof (gtab_idx1_t) * (th->KeyS + 1)));
        phridx = (int *) (item + th->DefC);
        phrbuf = (char *) (phridx + *phridx + 1);
        for (int i = 0; i < th->DefC; i++) {
            u_int key = convert_key32 ((unsigned char *) item->key);
            write_chardef_entry (fw, key, item->ch, keymap, th, 32 / th->keybits, phridx, phrbuf);
            item++;
        }
    } else if (th->keybits * th->MaxPress <= 64) {
        ITEM64 *item = (ITEM64 *) (kname + (CH_SZ * th->KeyS) + (sizeof (gtab_idx1_t) * (th->KeyS + 1)));
        phridx = (int *) (item + th->DefC);
        phrbuf = (char *) (phridx + *phridx + 1);
        for (int i = 0; i < th->DefC; i++) {
            u_int64_t key = convert_key64 ((unsigned char *) item->key);
            write_chardef_entry (fw, key, item->ch, keymap, th, 64 / th->keybits, phridx, phrbuf);
            item++;
        }
    } else
        fprintf (fw, "# Unknown chardef\n");
    fprintf (fw, "%%chardef end\n");

    fprintf (fw, "#\n");
    fprintf (fw, "# Gtab version: %d\n", th->version);
    fprintf (fw, "# flags: %#x\n", th->flag);
    fprintf (fw, "# keybits: %d\n", th->keybits);
    fprintf (fw, "# MaxPress: %d\n", th->MaxPress);
    fprintf (fw, "# Defined Characters : %d\n", th->DefC);
    fprintf (fw, "#\n");
    if (bot)
        bot_output (0, th);
    else
        printf ("hime-gtab2cin done\n");
    free (gtabbuf);
    fclose (fw);
    return 0;
}
