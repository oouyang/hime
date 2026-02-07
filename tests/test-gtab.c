/*
 * Unit tests for generic table functions (gtab-util.c)
 */

#include <stdint.h>
#include <string.h>

#include <glib.h>

#include "test-framework.h"

/* Types from gtab.h */
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef uint64_t u_int64_t;
typedef unsigned int gtab_idx1_t;

#define CH_SZ 4

typedef struct {
    u_char key[4];
    u_char ch[CH_SZ];
} ITEM;

typedef struct {
    u_char key[8];
    u_char ch[CH_SZ];
} ITEM64;

/* Simplified INMD structure for testing */
typedef struct {
    ITEM *tbl;
    ITEM64 *tbl64;
    int DefChars;
    char *keyname;
    char *keyname_lookup;
    gboolean key64;
    u_char kmask, keybits;
} INMD;

/* Macros from gtab.h */
#define KeyBits1(inm) (inm->keybits)
#define MAX_TAB_KEY_NUM1(inm) (32 / KeyBits1 (inm))
#define MAX_TAB_KEY_NUM641(inm) (64 / KeyBits1 (inm))
#define Max_tab_key_num1(inm) (inm->key64 ? MAX_TAB_KEY_NUM641 (inm) : MAX_TAB_KEY_NUM1 (inm))

/* Function declarations - we'll implement local versions for testing */
int utf8_sz (char *s);

/* Local implementation of utf8_sz for testing */
int utf8_sz (char *s) {
    unsigned char c = *s;
    if (c < 0x80)
        return 1;
    if ((c & 0xe0) == 0xc0)
        return 2;
    if ((c & 0xf0) == 0xe0)
        return 3;
    if ((c & 0xf8) == 0xf0)
        return 4;
    return 1;
}

/* Local implementation of CONVT2 (same as gtab-util.c) */
u_int64_t CONVT2 (INMD *inmd, int i) {
    u_int64_t kk;

    if (i >= inmd->DefChars || i < 0) {
        return 0;
    }

    if (inmd->key64) {
        memcpy (&kk, inmd->tbl64[i].key, sizeof (u_int64_t));
    } else {
        u_int tt;
        memcpy (&tt, inmd->tbl[i].key, sizeof (u_int));
        kk = tt;
    }

    return kk;
}

/* Local implementation of gtab_key2name (same as gtab-util.c) */
int gtab_key2name (INMD *tinmd, u_int64_t key, char *t, int *rtlen) {
    int tlen = 0, klen = 0;

    int j;
    for (j = Max_tab_key_num1 (tinmd) - 1; j >= 0; j--) {
        int sh = j * KeyBits1 (tinmd);
        int k = (key >> sh) & tinmd->kmask;

        if (!k)
            break;
        int len;
        char *keyname;

        if (tinmd->keyname_lookup) {
            len = 1;
            keyname = (char *) &tinmd->keyname_lookup[k];
        } else {
            keyname = (char *) &tinmd->keyname[k * CH_SZ];
            len = (*keyname & 0x80) ? utf8_sz (keyname) : strlen (keyname);
        }
        memcpy (&t[tlen], keyname, len);
        tlen += len;
        klen++;
    }

    t[tlen] = 0;
    *rtlen = tlen;
    return klen;
}

/* ============ CONVT2 tests ============ */

TEST (CONVT2_empty) {
    INMD inmd = {0};
    inmd.DefChars = 0;

    u_int64_t result = CONVT2 (&inmd, 0);
    ASSERT_EQ (0, result);
    TEST_PASS ();
}

TEST (CONVT2_negative_index) {
    INMD inmd = {0};
    inmd.DefChars = 10;

    u_int64_t result = CONVT2 (&inmd, -1);
    ASSERT_EQ (0, result);
    TEST_PASS ();
}

TEST (CONVT2_index_out_of_bounds) {
    INMD inmd = {0};
    inmd.DefChars = 10;

    u_int64_t result = CONVT2 (&inmd, 10);
    ASSERT_EQ (0, result);

    result = CONVT2 (&inmd, 100);
    ASSERT_EQ (0, result);
    TEST_PASS ();
}

TEST (CONVT2_32bit_key) {
    ITEM items[3];
    memset (items, 0, sizeof (items));

    /* Set key for item[1] to 0x12345678 */
    u_int key1 = 0x12345678;
    memcpy (items[1].key, &key1, sizeof (u_int));

    INMD inmd = {0};
    inmd.tbl = items;
    inmd.DefChars = 3;
    inmd.key64 = FALSE;

    u_int64_t result = CONVT2 (&inmd, 1);
    ASSERT_EQ (0x12345678, result);
    TEST_PASS ();
}

TEST (CONVT2_64bit_key) {
    ITEM64 items64[3];
    memset (items64, 0, sizeof (items64));

    /* Set key for item[1] to 0x123456789ABCDEF0 */
    u_int64_t key1 = 0x123456789ABCDEF0ULL;
    memcpy (items64[1].key, &key1, sizeof (u_int64_t));

    INMD inmd = {0};
    inmd.tbl64 = items64;
    inmd.DefChars = 3;
    inmd.key64 = TRUE;

    u_int64_t result = CONVT2 (&inmd, 1);
    ASSERT_EQ (0x123456789ABCDEF0ULL, result);
    TEST_PASS ();
}

TEST (CONVT2_boundary_valid) {
    ITEM items[5];
    memset (items, 0, sizeof (items));

    u_int key4 = 0xDEADBEEF;
    memcpy (items[4].key, &key4, sizeof (u_int));

    INMD inmd = {0};
    inmd.tbl = items;
    inmd.DefChars = 5;
    inmd.key64 = FALSE;

    /* Test last valid index */
    u_int64_t result = CONVT2 (&inmd, 4);
    ASSERT_EQ (0xDEADBEEF, result);
    TEST_PASS ();
}

/* ============ gtab_key2name tests ============ */

TEST (gtab_key2name_empty_key) {
    /* 5-bit keybits, ASCII keynames */
    char keynames[32 * CH_SZ];
    memset (keynames, 0, sizeof (keynames));
    /* Set up keynames: index 1='a', 2='b', etc. */
    keynames[1 * CH_SZ] = 'a';
    keynames[2 * CH_SZ] = 'b';
    keynames[3 * CH_SZ] = 'c';

    INMD inmd = {0};
    inmd.keyname = keynames;
    inmd.keyname_lookup = NULL;
    inmd.keybits = 5;
    inmd.kmask = (1 << 5) - 1; /* 0x1F */
    inmd.key64 = FALSE;

    char result[32] = {0};
    int tlen;
    int klen = gtab_key2name (&inmd, 0, result, &tlen);

    ASSERT_EQ (0, klen);
    ASSERT_EQ (0, tlen);
    ASSERT_STR_EQ ("", result);
    TEST_PASS ();
}

TEST (gtab_key2name_single_key) {
    char keynames[32 * CH_SZ];
    memset (keynames, 0, sizeof (keynames));
    keynames[1 * CH_SZ] = 'a';
    keynames[2 * CH_SZ] = 'b';
    keynames[3 * CH_SZ] = 'c';

    INMD inmd = {0};
    inmd.keyname = keynames;
    inmd.keyname_lookup = NULL;
    inmd.keybits = 5;
    inmd.kmask = 0x1F;
    inmd.key64 = FALSE;

    char result[32] = {0};
    int tlen;

    /* With 5-bit keybits, 32/5 = 6 keys max
     * Key 1 in position 5 (highest) = 1 << (5*5) = 1 << 25 */
    u_int64_t key = 1ULL << 25; /* Key 'a' (index 1) in highest position */
    int klen = gtab_key2name (&inmd, key, result, &tlen);

    ASSERT_EQ (1, klen);
    ASSERT_EQ (1, tlen);
    ASSERT_STR_EQ ("a", result);
    TEST_PASS ();
}

TEST (gtab_key2name_multiple_keys) {
    char keynames[32 * CH_SZ];
    memset (keynames, 0, sizeof (keynames));
    keynames[1 * CH_SZ] = 'a';
    keynames[2 * CH_SZ] = 'b';
    keynames[3 * CH_SZ] = 'c';

    INMD inmd = {0};
    inmd.keyname = keynames;
    inmd.keyname_lookup = NULL;
    inmd.keybits = 5;
    inmd.kmask = 0x1F;
    inmd.key64 = FALSE;

    char result[32] = {0};
    int tlen;

    /* Encode "abc" = key[5]=1, key[4]=2, key[3]=3
     * (1 << 25) | (2 << 20) | (3 << 15) */
    u_int64_t key = (1ULL << 25) | (2ULL << 20) | (3ULL << 15);
    int klen = gtab_key2name (&inmd, key, result, &tlen);

    ASSERT_EQ (3, klen);
    ASSERT_EQ (3, tlen);
    ASSERT_STR_EQ ("abc", result);
    TEST_PASS ();
}

TEST (gtab_key2name_with_lookup) {
    /* Use keyname_lookup (single-byte lookup table) */
    char lookup[32];
    memset (lookup, 0, sizeof (lookup));
    lookup[1] = 'x';
    lookup[2] = 'y';
    lookup[3] = 'z';

    INMD inmd = {0};
    inmd.keyname = NULL;
    inmd.keyname_lookup = lookup;
    inmd.keybits = 5;
    inmd.kmask = 0x1F;
    inmd.key64 = FALSE;

    char result[32] = {0};
    int tlen;

    /* Encode "xy" = key[5]=1, key[4]=2 */
    u_int64_t key = (1ULL << 25) | (2ULL << 20);
    int klen = gtab_key2name (&inmd, key, result, &tlen);

    ASSERT_EQ (2, klen);
    ASSERT_EQ (2, tlen);
    ASSERT_STR_EQ ("xy", result);
    TEST_PASS ();
}

TEST (gtab_key2name_64bit) {
    char keynames[32 * CH_SZ];
    memset (keynames, 0, sizeof (keynames));
    keynames[1 * CH_SZ] = 'a';
    keynames[2 * CH_SZ] = 'b';

    INMD inmd = {0};
    inmd.keyname = keynames;
    inmd.keyname_lookup = NULL;
    inmd.keybits = 5;
    inmd.kmask = 0x1F;
    inmd.key64 = TRUE; /* 64-bit mode: 64/5 = 12 keys max */

    char result[32] = {0};
    int tlen;

    /* In 64-bit mode, highest position is 11 (0-11) = 11*5 = 55 bits */
    u_int64_t key = (1ULL << 55) | (2ULL << 50);
    int klen = gtab_key2name (&inmd, key, result, &tlen);

    ASSERT_EQ (2, klen);
    ASSERT_EQ (2, tlen);
    ASSERT_STR_EQ ("ab", result);
    TEST_PASS ();
}

TEST (gtab_key2name_utf8_keyname) {
    /* Test with UTF-8 keynames (CJK characters) */
    char keynames[32 * CH_SZ];
    memset (keynames, 0, sizeof (keynames));
    /* 中 = E4 B8 AD in UTF-8 */
    keynames[1 * CH_SZ + 0] = (char) 0xE4;
    keynames[1 * CH_SZ + 1] = (char) 0xB8;
    keynames[1 * CH_SZ + 2] = (char) 0xAD;

    INMD inmd = {0};
    inmd.keyname = keynames;
    inmd.keyname_lookup = NULL;
    inmd.keybits = 5;
    inmd.kmask = 0x1F;
    inmd.key64 = FALSE;

    char result[32] = {0};
    int tlen;

    u_int64_t key = 1ULL << 25; /* Single key */
    int klen = gtab_key2name (&inmd, key, result, &tlen);

    ASSERT_EQ (1, klen); /* 1 key */
    ASSERT_EQ (3, tlen); /* 3 bytes for UTF-8 char */
    ASSERT_STR_EQ ("中", result);
    TEST_PASS ();
}

/* ============ Test Suite ============ */

TEST_SUITE_BEGIN ("Generic Table Functions")
/* CONVT2 tests */
RUN_TEST (CONVT2_empty);
RUN_TEST (CONVT2_negative_index);
RUN_TEST (CONVT2_index_out_of_bounds);
RUN_TEST (CONVT2_32bit_key);
RUN_TEST (CONVT2_64bit_key);
RUN_TEST (CONVT2_boundary_valid);

/* gtab_key2name tests */
RUN_TEST (gtab_key2name_empty_key);
RUN_TEST (gtab_key2name_single_key);
RUN_TEST (gtab_key2name_multiple_keys);
RUN_TEST (gtab_key2name_with_lookup);
RUN_TEST (gtab_key2name_64bit);
RUN_TEST (gtab_key2name_utf8_keyname);
TEST_SUITE_END ()
