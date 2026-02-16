/*
 * HIME macOS Unit Tests - Encryption Functions
 *
 * Tests the XOR cipher used for client-server password exchange.
 * Self-contained: includes local reimplementations to avoid pulling
 * in hime-protocol.h and its transitive dependencies.
 *
 * Compiles as a standalone C executable (no macOS framework dependency).
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include "../../../tests/test-framework.h"

#include <stdint.h>

/* Local definitions matching hime-protocol.h */
#define __HIME_PASSWD_N_ (31)

typedef struct {
    unsigned int seed;
    unsigned char passwd[__HIME_PASSWD_N_];
} TestHimePasswd;

/* Local copy of __hime_rand__ from hime-crypt.c */
static uint32_t
test_hime_rand (uint32_t *next)
{
    *next = *next * 1103515245 + 12345;
    return (*next / 65536) % 32768;
}

/* Local copy of __hime_enc_mem from hime-crypt.c */
static void
test_hime_enc_mem (unsigned char *p,
                   const int n,
                   const TestHimePasswd *passwd,
                   uint32_t *seed)
{
    for (int i = 0; i < n; i++) {
        uint32_t v = test_hime_rand (seed) % __HIME_PASSWD_N_;
        p[i] ^= passwd->passwd[v];
    }
}

static void
fill_passwd (TestHimePasswd *pw, unsigned char base)
{
    pw->seed = 0;
    for (int i = 0; i < __HIME_PASSWD_N_; i++)
        pw->passwd[i] = (unsigned char) (base + i);
}

/* --- Tests --- */

TEST (enc_zero_length) {
    unsigned char buf[] = {0xAA, 0xBB};
    unsigned char orig[] = {0xAA, 0xBB};
    TestHimePasswd pw;
    fill_passwd (&pw, 0x10);
    uint32_t seed = 42;
    test_hime_enc_mem (buf, 0, &pw, &seed);
    ASSERT_MEM_EQ (orig, buf, 2);
    TEST_PASS ();
}

TEST (enc_single_byte) {
    unsigned char buf[1] = {0x42};
    TestHimePasswd pw;
    fill_passwd (&pw, 0x10);
    uint32_t s = 1;
    uint32_t r = test_hime_rand (&s);
    uint32_t idx = r % __HIME_PASSWD_N_;
    unsigned char expected = 0x42 ^ pw.passwd[idx];
    uint32_t seed = 1;
    test_hime_enc_mem (buf, 1, &pw, &seed);
    ASSERT_EQ (expected, buf[0]);
    TEST_PASS ();
}

TEST (enc_known_vector) {
    unsigned char buf[4] = {0, 0, 0, 0};
    TestHimePasswd pw;
    fill_passwd (&pw, 0x10);
    unsigned char expected[4] = {0, 0, 0, 0};
    uint32_t s = 100;
    for (int i = 0; i < 4; i++) {
        uint32_t r = test_hime_rand (&s);
        expected[i] = pw.passwd[r % __HIME_PASSWD_N_];
    }
    uint32_t seed = 100;
    test_hime_enc_mem (buf, 4, &pw, &seed);
    ASSERT_MEM_EQ (expected, buf, 4);
    TEST_PASS ();
}

TEST (enc_decrypt_roundtrip) {
    unsigned char original[16], buf[16];
    TestHimePasswd pw;
    fill_passwd (&pw, 0x20);
    for (int i = 0; i < 16; i++)
        original[i] = buf[i] = (unsigned char) (i * 7 + 3);
    uint32_t seed = 999;
    test_hime_enc_mem (buf, 16, &pw, &seed);
    seed = 999;
    test_hime_enc_mem (buf, 16, &pw, &seed);
    ASSERT_MEM_EQ (original, buf, 16);
    TEST_PASS ();
}

TEST (enc_different_seeds) {
    unsigned char buf1[8], buf2[8];
    TestHimePasswd pw;
    fill_passwd (&pw, 0x30);
    memset (buf1, 0xAA, 8);
    memset (buf2, 0xAA, 8);
    uint32_t seed1 = 1, seed2 = 2;
    test_hime_enc_mem (buf1, 8, &pw, &seed1);
    test_hime_enc_mem (buf2, 8, &pw, &seed2);
    ASSERT_TRUE (memcmp (buf1, buf2, 8) != 0);
    TEST_PASS ();
}

TEST (enc_different_passwords) {
    unsigned char buf1[8], buf2[8];
    TestHimePasswd pw1, pw2;
    fill_passwd (&pw1, 0x10);
    fill_passwd (&pw2, 0x80);
    memset (buf1, 0x55, 8);
    memset (buf2, 0x55, 8);
    uint32_t seed1 = 42, seed2 = 42;
    test_hime_enc_mem (buf1, 8, &pw1, &seed1);
    test_hime_enc_mem (buf2, 8, &pw2, &seed2);
    ASSERT_TRUE (memcmp (buf1, buf2, 8) != 0);
    TEST_PASS ();
}

TEST (rand_deterministic) {
    uint32_t seed = 0;
    uint32_t r = test_hime_rand (&seed);
    ASSERT_EQ (0, r);
    TEST_PASS ();
}

TEST (rand_sequence) {
    uint32_t seed = 1;
    uint32_t r1 = test_hime_rand (&seed);
    uint32_t r2 = test_hime_rand (&seed);
    uint32_t r3 = test_hime_rand (&seed);
    ASSERT_TRUE (r1 != r2 || r2 != r3);
    uint32_t seed2 = 1;
    ASSERT_EQ (r1, test_hime_rand (&seed2));
    ASSERT_EQ (r2, test_hime_rand (&seed2));
    ASSERT_EQ (r3, test_hime_rand (&seed2));
    TEST_PASS ();
}

TEST (enc_all_passwd_indices) {
    int hit[__HIME_PASSWD_N_] = {0};
    uint32_t s = 12345;
    for (int i = 0; i < 256; i++) {
        uint32_t r = test_hime_rand (&s);
        hit[r % __HIME_PASSWD_N_] = 1;
    }
    for (int i = 0; i < __HIME_PASSWD_N_; i++)
        ASSERT_EQ (1, hit[i]);
    TEST_PASS ();
}

TEST (enc_large_buffer) {
    unsigned char original[1024], buf[1024];
    TestHimePasswd pw;
    fill_passwd (&pw, 0x42);
    for (int i = 0; i < 1024; i++)
        original[i] = buf[i] = (unsigned char) (i & 0xFF);
    uint32_t seed = 77777;
    test_hime_enc_mem (buf, 1024, &pw, &seed);
    ASSERT_TRUE (memcmp (original, buf, 1024) != 0);
    seed = 77777;
    test_hime_enc_mem (buf, 1024, &pw, &seed);
    ASSERT_MEM_EQ (original, buf, 1024);
    TEST_PASS ();
}

TEST_SUITE_BEGIN ("hime-crypt tests (macOS)")
RUN_TEST (enc_zero_length);
RUN_TEST (enc_single_byte);
RUN_TEST (enc_known_vector);
RUN_TEST (enc_decrypt_roundtrip);
RUN_TEST (enc_different_seeds);
RUN_TEST (enc_different_passwords);
RUN_TEST (rand_deterministic);
RUN_TEST (rand_sequence);
RUN_TEST (enc_all_passwd_indices);
RUN_TEST (enc_large_buffer);
TEST_SUITE_END ()
