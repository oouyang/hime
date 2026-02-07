/*
 * Unit tests for phonetic functions (pho.c)
 *
 * These tests verify the bit manipulation algorithms used for phonetic key
 * encoding/decoding. We include local implementations to avoid dependencies
 * on the full HIME system.
 */

#include "test-framework.h"

#include <glib.h>
#include <string.h>

/* Types from pho.h */
typedef unsigned short phokey_t;
typedef unsigned char u_char;

/* Constants from pho.h */
#define BACK_QUOTE_NO 24

/* Local implementations of functions from pho.c */
static char typ_pho_len[] = {5, 2, 4, 3};

phokey_t pho2key(char typ_pho[]) {
    phokey_t key = typ_pho[0];
    int i;

    if (key == BACK_QUOTE_NO)
        return (BACK_QUOTE_NO << 9) | typ_pho[1];

    for (i = 1; i < 4; i++) {
        key = typ_pho[i] | (key << typ_pho_len[i]);
    }

    return key;
}

void key_typ_pho(phokey_t phokey, u_char rtyp_pho[]) {
    rtyp_pho[3] = phokey & 7;
    phokey >>= 3;
    rtyp_pho[2] = phokey & 0xf;
    phokey >>= 4;
    rtyp_pho[1] = phokey & 0x3;
    phokey >>= 2;
    rtyp_pho[0] = phokey;
}

/* ============ pho2key tests ============ */

/*
 * pho2key encoding:
 * typ_pho_len[] = {5, 2, 4, 3} for positions 0-3
 * Total = 14 bits, fits in u_short (16 bits)
 *
 * Layout: [typ_pho[0]:5][typ_pho[1]:2][typ_pho[2]:4][typ_pho[3]:3]
 * key = (typ_pho[0] << 9) | (typ_pho[1] << 7) | (typ_pho[2] << 3) | typ_pho[3]
 */

TEST(pho2key_zero) {
    char typ_pho[4] = {0, 0, 0, 0};
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(0, key);
    TEST_PASS();
}

TEST(pho2key_position0_only) {
    /* typ_pho[0] = 1, shifted left 9 bits = 512 */
    char typ_pho[4] = {1, 0, 0, 0};
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(512, key);  /* 1 << 9 = 512 */
    TEST_PASS();
}

TEST(pho2key_position1_only) {
    /* typ_pho[1] = 1, shifted left 7 bits = 128 */
    char typ_pho[4] = {0, 1, 0, 0};
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(128, key);  /* 1 << 7 = 128 */
    TEST_PASS();
}

TEST(pho2key_position2_only) {
    /* typ_pho[2] = 1, shifted left 3 bits = 8 */
    char typ_pho[4] = {0, 0, 1, 0};
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(8, key);  /* 1 << 3 = 8 */
    TEST_PASS();
}

TEST(pho2key_position3_only) {
    /* typ_pho[3] = 1, no shift = 1 */
    char typ_pho[4] = {0, 0, 0, 1};
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(1, key);
    TEST_PASS();
}

TEST(pho2key_all_ones) {
    /* All positions = 1 */
    /* 512 + 128 + 8 + 1 = 649 */
    char typ_pho[4] = {1, 1, 1, 1};
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(649, key);
    TEST_PASS();
}

TEST(pho2key_max_values) {
    /*
     * Max values for each position:
     * pos0: 5 bits, max = 31
     * pos1: 2 bits, max = 3
     * pos2: 4 bits, max = 15
     * pos3: 3 bits, max = 7
     *
     * key = (31 << 9) | (3 << 7) | (15 << 3) | 7
     *     = 15872 + 384 + 120 + 7
     *     = 16383 (0x3FFF)
     */
    char typ_pho[4] = {31, 3, 15, 7};
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(16383, key);
    TEST_PASS();
}

TEST(pho2key_typical_values) {
    /* Typical phonetic input */
    char typ_pho[4] = {5, 2, 8, 3};
    /* 5 << 9 = 2560
     * 2 << 7 = 256
     * 8 << 3 = 64
     * 3 = 3
     * Total = 2883
     */
    phokey_t key = pho2key(typ_pho);
    ASSERT_EQ(2883, key);
    TEST_PASS();
}

TEST(pho2key_back_quote_special) {
    /*
     * BACK_QUOTE_NO (24) is a special case:
     * Returns (BACK_QUOTE_NO << 9) | typ_pho[1]
     */
    char typ_pho[4] = {BACK_QUOTE_NO, 65, 0, 0};  /* 65 = 'A' */
    phokey_t key = pho2key(typ_pho);
    /* (24 << 9) | 65 = 12288 + 65 = 12353 */
    ASSERT_EQ(12353, key);
    TEST_PASS();
}

/* ============ key_typ_pho tests ============ */

TEST(key_typ_pho_zero) {
    u_char rtyp_pho[4] = {0xFF, 0xFF, 0xFF, 0xFF};  /* Initialize with garbage */
    key_typ_pho(0, rtyp_pho);
    ASSERT_EQ(0, rtyp_pho[0]);
    ASSERT_EQ(0, rtyp_pho[1]);
    ASSERT_EQ(0, rtyp_pho[2]);
    ASSERT_EQ(0, rtyp_pho[3]);
    TEST_PASS();
}

TEST(key_typ_pho_position0_only) {
    u_char rtyp_pho[4];
    key_typ_pho(512, rtyp_pho);  /* 1 << 9 */
    ASSERT_EQ(1, rtyp_pho[0]);
    ASSERT_EQ(0, rtyp_pho[1]);
    ASSERT_EQ(0, rtyp_pho[2]);
    ASSERT_EQ(0, rtyp_pho[3]);
    TEST_PASS();
}

TEST(key_typ_pho_position1_only) {
    u_char rtyp_pho[4];
    key_typ_pho(128, rtyp_pho);  /* 1 << 7 */
    ASSERT_EQ(0, rtyp_pho[0]);
    ASSERT_EQ(1, rtyp_pho[1]);
    ASSERT_EQ(0, rtyp_pho[2]);
    ASSERT_EQ(0, rtyp_pho[3]);
    TEST_PASS();
}

TEST(key_typ_pho_position2_only) {
    u_char rtyp_pho[4];
    key_typ_pho(8, rtyp_pho);  /* 1 << 3 */
    ASSERT_EQ(0, rtyp_pho[0]);
    ASSERT_EQ(0, rtyp_pho[1]);
    ASSERT_EQ(1, rtyp_pho[2]);
    ASSERT_EQ(0, rtyp_pho[3]);
    TEST_PASS();
}

TEST(key_typ_pho_position3_only) {
    u_char rtyp_pho[4];
    key_typ_pho(1, rtyp_pho);
    ASSERT_EQ(0, rtyp_pho[0]);
    ASSERT_EQ(0, rtyp_pho[1]);
    ASSERT_EQ(0, rtyp_pho[2]);
    ASSERT_EQ(1, rtyp_pho[3]);
    TEST_PASS();
}

TEST(key_typ_pho_all_ones) {
    u_char rtyp_pho[4];
    key_typ_pho(649, rtyp_pho);  /* All positions = 1 */
    ASSERT_EQ(1, rtyp_pho[0]);
    ASSERT_EQ(1, rtyp_pho[1]);
    ASSERT_EQ(1, rtyp_pho[2]);
    ASSERT_EQ(1, rtyp_pho[3]);
    TEST_PASS();
}

TEST(key_typ_pho_max_values) {
    u_char rtyp_pho[4];
    key_typ_pho(16383, rtyp_pho);  /* 0x3FFF - all bits set */
    ASSERT_EQ(31, rtyp_pho[0]);  /* 5 bits max */
    ASSERT_EQ(3, rtyp_pho[1]);   /* 2 bits max */
    ASSERT_EQ(15, rtyp_pho[2]);  /* 4 bits max */
    ASSERT_EQ(7, rtyp_pho[3]);   /* 3 bits max */
    TEST_PASS();
}

TEST(key_typ_pho_typical_values) {
    u_char rtyp_pho[4];
    key_typ_pho(2883, rtyp_pho);
    ASSERT_EQ(5, rtyp_pho[0]);
    ASSERT_EQ(2, rtyp_pho[1]);
    ASSERT_EQ(8, rtyp_pho[2]);
    ASSERT_EQ(3, rtyp_pho[3]);
    TEST_PASS();
}

/* ============ Roundtrip tests ============ */

TEST(pho2key_key_typ_pho_roundtrip) {
    /* Test that pho2key and key_typ_pho are inverses */
    char typ_pho[4] = {10, 2, 5, 4};
    phokey_t key = pho2key(typ_pho);
    u_char rtyp_pho[4];
    key_typ_pho(key, rtyp_pho);

    ASSERT_EQ(typ_pho[0], rtyp_pho[0]);
    ASSERT_EQ(typ_pho[1], rtyp_pho[1]);
    ASSERT_EQ(typ_pho[2], rtyp_pho[2]);
    ASSERT_EQ(typ_pho[3], rtyp_pho[3]);
    TEST_PASS();
}

TEST(key_typ_pho_pho2key_roundtrip) {
    /* Test that key_typ_pho and pho2key are inverses */
    phokey_t original_key = 8765;  /* Arbitrary value within range */
    u_char rtyp_pho[4];
    key_typ_pho(original_key, rtyp_pho);

    /* Convert rtyp_pho to char array for pho2key */
    char typ_pho[4];
    typ_pho[0] = rtyp_pho[0];
    typ_pho[1] = rtyp_pho[1];
    typ_pho[2] = rtyp_pho[2];
    typ_pho[3] = rtyp_pho[3];

    phokey_t result_key = pho2key(typ_pho);
    ASSERT_EQ(original_key, result_key);
    TEST_PASS();
}

TEST(roundtrip_multiple_values) {
    /* Test multiple values for roundtrip */
    phokey_t test_keys[] = {0, 1, 255, 1000, 5000, 10000, 16383};
    int num_tests = sizeof(test_keys) / sizeof(test_keys[0]);

    for (int i = 0; i < num_tests; i++) {
        phokey_t key = test_keys[i];
        u_char rtyp_pho[4];
        char typ_pho[4];

        key_typ_pho(key, rtyp_pho);
        typ_pho[0] = rtyp_pho[0];
        typ_pho[1] = rtyp_pho[1];
        typ_pho[2] = rtyp_pho[2];
        typ_pho[3] = rtyp_pho[3];

        phokey_t result = pho2key(typ_pho);
        if (result != key) {
            fprintf(stderr, "Roundtrip failed for key %d\n", key);
            ASSERT_EQ(key, result);
        }
    }
    TEST_PASS();
}

/* ============ Test Suite ============ */

TEST_SUITE_BEGIN("Phonetic Functions")
    /* pho2key tests */
    RUN_TEST(pho2key_zero);
    RUN_TEST(pho2key_position0_only);
    RUN_TEST(pho2key_position1_only);
    RUN_TEST(pho2key_position2_only);
    RUN_TEST(pho2key_position3_only);
    RUN_TEST(pho2key_all_ones);
    RUN_TEST(pho2key_max_values);
    RUN_TEST(pho2key_typical_values);
    RUN_TEST(pho2key_back_quote_special);

    /* key_typ_pho tests */
    RUN_TEST(key_typ_pho_zero);
    RUN_TEST(key_typ_pho_position0_only);
    RUN_TEST(key_typ_pho_position1_only);
    RUN_TEST(key_typ_pho_position2_only);
    RUN_TEST(key_typ_pho_position3_only);
    RUN_TEST(key_typ_pho_all_ones);
    RUN_TEST(key_typ_pho_max_values);
    RUN_TEST(key_typ_pho_typical_values);

    /* Roundtrip tests */
    RUN_TEST(pho2key_key_typ_pho_roundtrip);
    RUN_TEST(key_typ_pho_pho2key_roundtrip);
    RUN_TEST(roundtrip_multiple_values);
TEST_SUITE_END()
