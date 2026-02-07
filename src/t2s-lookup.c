/* Copyright (C) 2009 Edward Der-Hua Liu, Hsin-Chu, Taiwan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation version 2.1
 * of the License.
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

#include <stdio.h>

#include <sys/stat.h>

#include "hime.h"

#include "t2s-file.h"

/* Cached translation table for fast in-memory binary search */
typedef struct {
    T2S *table;
    int count;
} T2SCache;

static T2SCache t2s_cache = {NULL, 0};
static T2SCache s2t_cache = {NULL, 0};

/* Initial output buffer size - grows exponentially */
#define INITIAL_BUF_SIZE 256

/**
 * Load translation table into memory for fast lookups.
 * The table is cached and reused across calls.
 */
static T2SCache *load_table (const char *fname, T2SCache *cache) {
    if (cache->table != NULL) {
        return cache;
    }

    char fullname[512];
    get_sys_table_file_name (fname, fullname);

    struct stat st;
    if (stat (fullname, &st) != 0) {
        p_err ("cannot stat %s", fullname);
    }

    FILE *fp = fopen (fullname, "rb");
    if (fp == NULL) {
        p_err ("cannot open %s", fullname);
    }

    cache->count = st.st_size / sizeof (T2S);
    cache->table = (T2S *) malloc (st.st_size);
    if (cache->table == NULL) {
        fclose (fp);
        p_err ("cannot allocate memory for %s", fname);
    }

    if (fread (cache->table, sizeof (T2S), cache->count, fp) != (size_t) cache->count) {
        free (cache->table);
        cache->table = NULL;
        cache->count = 0;
        fclose (fp);
        p_err ("cannot read %s", fullname);
    }

    fclose (fp);
    return cache;
}

/**
 * Binary search lookup in cached table.
 * Returns number of bytes written to out.
 */
static int table_lookup (const T2SCache *cache, const char *s, char *out) {
    unsigned int key = 0;
    u8cpy ((char *) &key, s);

    int bot = 0;
    int top = cache->count - 1;
    const T2S *table = cache->table;

    while (bot <= top) {
        int mid = (bot + top) / 2;
        unsigned int mid_key = table[mid].a;

        if (key > mid_key) {
            bot = mid + 1;
        } else if (key < mid_key) {
            top = mid - 1;
        } else {
            return u8cpy (out, (char *) &table[mid].b);
        }
    }

    return u8cpy (out, s);
}

/**
 * Translate string using the specified translation table.
 * Uses cached table for fast lookups and exponential buffer growth.
 */
static int translate (const char *fname, T2SCache *cache, const char *str, int strN, char **out) {
    load_table (fname, cache);

    /* Allocate output buffer with exponential growth strategy */
    int buf_cap = (strN > INITIAL_BUF_SIZE) ? strN * 2 : INITIAL_BUF_SIZE;
    char *buf = (char *) malloc (buf_cap);
    if (buf == NULL) {
        p_err ("cannot allocate output buffer");
    }

    const char *p = str;
    const char *endp = str + strN;
    int out_len = 0;

    while (p < endp) {
        /* Ensure buffer has space for max UTF-8 char (4 bytes) + null */
        if (out_len + 5 > buf_cap) {
            buf_cap *= 2;
            char *new_buf = (char *) realloc (buf, buf_cap);
            if (new_buf == NULL) {
                free (buf);
                p_err ("cannot reallocate output buffer");
            }
            buf = new_buf;
        }

        out_len += table_lookup (cache, p, &buf[out_len]);
        p += utf8_sz (p);
    }

    buf[out_len] = '\0';
    *out = buf;
    return out_len;
}

int trad2sim (char *str, int strN, char **out) {
    return translate ("t2s.dat", &t2s_cache, str, strN, out);
}

int sim2trad (char *str, int strN, char **out) {
    return translate ("s2t.dat", &s2t_cache, str, strN, out);
}
