/*
 * Unit tests for encryption functions (hime-crypt.c)
 *
 * Tests the XOR cipher used for client-server password exchange.
 * Self-contained: includes local implementations to avoid pulling
 * in hime-protocol.h and its transitive dependencies.
 *
 * Cross-compiled with MinGW, runs via Wine or natively on Windows.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;
static int tests_total = 0;
static const char *current_test = NULL;

#define TEST(name)                       \
    static void test_##name (void);      \
    static void run_test_##name (void) { \
        current_test = #name;            \
        tests_total++;                   \
        test_##name ();                  \
    }                                    \
    static void test_##name (void)

#define RUN_TEST(name)      \
    do {                    \
        run_test_##name (); \
    } while (0)

#define PASS()                                     \
    do {                                           \
        printf ("  PASS: %s\n", current_test);     \
        tests_passed++;                            \
    } while (0)

#define ASSERT_TRUE(expr)                                            \
    do {                                                             \
        if (!(expr)) {                                               \
            printf ("  FAIL: %s\n    %s:%d: %s\n",                   \
                    current_test, __FILE__, __LINE__, #expr);        \
            tests_failed++;                                          \
            return;                                                  \
        }                                                            \
    } while (0)

#define ASSERT_EQ(expected, actual)                                  \
    do {                                                             \
        if ((expected) != (actual)) {                                \
            printf ("  FAIL: %s\n    %s:%d: expected %ld, got %ld\n",\
                    current_test, __FILE__, __LINE__,                \
                    (long)(expected), (long)(actual));               \
            tests_failed++;                                          \
            return;                                                  \
        }                                                            \
    } while (0)

/* Local definitions matching hime-protocol.h */
#define __HIME_PASSWD_N_ (31)

typedef struct HIME_PASSWD {
    unsigned int seed;
    unsigned char passwd[__HIME_PASSWD_N_];
} HIME_PASSWD;

/* Local copy of __hime_rand__ from hime-crypt.c */
static uint32_t
__hime_rand__ (uint32_t *next)
{
    *next = *next * 1103515245 + 12345;
    return (*next / 65536) % 32768;
}

/* Local copy of __hime_enc_mem from hime-crypt.c */
static void
__hime_enc_mem (unsigned char *p,
                const int n,
                const HIME_PASSWD *passwd,
                uint32_t *seed)
{
    for (int i = 0; i < n; i++) {
        uint32_t v = __hime_rand__ (seed) % __HIME_PASSWD_N_;
        p[i] ^= passwd->passwd[v];
    }
}

/* Helper: fill passwd with known pattern */
static void
fill_passwd (HIME_PASSWD *pw, unsigned char base)
{
    pw->seed = 0;
    for (int i = 0; i < __HIME_PASSWD_N_; i++)
        pw->passwd[i] = (unsigned char) (base + i);
}

/* --- Tests --- */

TEST (enc_zero_length) {
    unsigned char buf[] = {0xAA, 0xBB};
    unsigned char orig[] = {0xAA, 0xBB};
    HIME_PASSWD pw;
    fill_passwd (&pw, 0x10);
    uint32_t seed = 42;
    __hime_enc_mem (buf, 0, &pw, &seed);
    ASSERT_TRUE (memcmp (orig, buf, 2) == 0);
    PASS ();
}

TEST (enc_single_byte) {
    unsigned char buf[1] = {0x42};
    HIME_PASSWD pw;
    fill_passwd (&pw, 0x10);
    uint32_t seed = 1;
    /* Compute expected */
    uint32_t s = 1;
    uint32_t r = __hime_rand__ (&s);
    uint32_t idx = r % __HIME_PASSWD_N_;
    unsigned char expected = 0x42 ^ pw.passwd[idx];
    __hime_enc_mem (buf, 1, &pw, &seed);
    ASSERT_EQ (expected, buf[0]);
    PASS ();
}

TEST (enc_known_vector) {
    unsigned char buf[4] = {0x00, 0x00, 0x00, 0x00};
    HIME_PASSWD pw;
    fill_passwd (&pw, 0x10);
    uint32_t seed = 100;

    unsigned char expected[4] = {0, 0, 0, 0};
    uint32_t s = 100;
    for (int i = 0; i < 4; i++) {
        uint32_t r = __hime_rand__ (&s);
        expected[i] = 0x00 ^ pw.passwd[r % __HIME_PASSWD_N_];
    }

    __hime_enc_mem (buf, 4, &pw, &seed);
    ASSERT_TRUE (memcmp (expected, buf, 4) == 0);
    PASS ();
}

TEST (enc_decrypt_roundtrip) {
    unsigned char original[16];
    unsigned char buf[16];
    HIME_PASSWD pw;
    fill_passwd (&pw, 0x20);

    for (int i = 0; i < 16; i++)
        original[i] = buf[i] = (unsigned char) (i * 7 + 3);

    uint32_t seed = 999;
    __hime_enc_mem (buf, 16, &pw, &seed);

    /* XOR is self-inverse: encrypt again with same seed to decrypt */
    seed = 999;
    __hime_enc_mem (buf, 16, &pw, &seed);

    ASSERT_TRUE (memcmp (original, buf, 16) == 0);
    PASS ();
}

TEST (enc_different_seeds) {
    unsigned char buf1[8], buf2[8];
    HIME_PASSWD pw;
    fill_passwd (&pw, 0x30);

    memset (buf1, 0xAA, 8);
    memset (buf2, 0xAA, 8);

    uint32_t seed1 = 1;
    uint32_t seed2 = 2;
    __hime_enc_mem (buf1, 8, &pw, &seed1);
    __hime_enc_mem (buf2, 8, &pw, &seed2);

    ASSERT_TRUE (memcmp (buf1, buf2, 8) != 0);
    PASS ();
}

TEST (enc_different_passwords) {
    unsigned char buf1[8], buf2[8];
    HIME_PASSWD pw1, pw2;
    fill_passwd (&pw1, 0x10);
    fill_passwd (&pw2, 0x80);

    memset (buf1, 0x55, 8);
    memset (buf2, 0x55, 8);

    uint32_t seed1 = 42, seed2 = 42;
    __hime_enc_mem (buf1, 8, &pw1, &seed1);
    __hime_enc_mem (buf2, 8, &pw2, &seed2);

    ASSERT_TRUE (memcmp (buf1, buf2, 8) != 0);
    PASS ();
}

TEST (rand_deterministic) {
    uint32_t seed = 0;
    uint32_t r = __hime_rand__ (&seed);
    /* seed = 0*1103515245 + 12345 = 12345, return (12345/65536)%32768 = 0 */
    ASSERT_EQ (0, r);
    PASS ();
}

TEST (rand_sequence) {
    uint32_t seed = 1;
    uint32_t r1 = __hime_rand__ (&seed);
    uint32_t r2 = __hime_rand__ (&seed);
    uint32_t r3 = __hime_rand__ (&seed);

    ASSERT_TRUE (r1 != r2 || r2 != r3);

    /* Verify reproducibility */
    uint32_t seed2 = 1;
    ASSERT_EQ (r1, __hime_rand__ (&seed2));
    ASSERT_EQ (r2, __hime_rand__ (&seed2));
    ASSERT_EQ (r3, __hime_rand__ (&seed2));
    PASS ();
}

TEST (enc_all_passwd_indices) {
    unsigned char buf[256];
    HIME_PASSWD pw;
    int hit[__HIME_PASSWD_N_] = {0};

    fill_passwd (&pw, 0x01);
    memset (buf, 0, 256);

    uint32_t seed = 12345;
    uint32_t s = seed;
    for (int i = 0; i < 256; i++) {
        uint32_t r = __hime_rand__ (&s);
        hit[r % __HIME_PASSWD_N_] = 1;
    }

    for (int i = 0; i < __HIME_PASSWD_N_; i++)
        ASSERT_EQ (1, hit[i]);

    __hime_enc_mem (buf, 256, &pw, &seed);
    PASS ();
}

TEST (enc_large_buffer) {
    unsigned char original[1024];
    unsigned char buf[1024];
    HIME_PASSWD pw;
    fill_passwd (&pw, 0x42);

    for (int i = 0; i < 1024; i++)
        original[i] = buf[i] = (unsigned char) (i & 0xFF);

    uint32_t seed = 77777;
    __hime_enc_mem (buf, 1024, &pw, &seed);

    ASSERT_TRUE (memcmp (original, buf, 1024) != 0);

    /* Roundtrip */
    seed = 77777;
    __hime_enc_mem (buf, 1024, &pw, &seed);
    ASSERT_TRUE (memcmp (original, buf, 1024) == 0);
    PASS ();
}

/* ========== Main ========== */

int
main (int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    printf ("\n=== HIME Encryption Tests (Windows) ===\n\n");

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

    printf ("\n=== Results ===\n");
    printf ("Total:  %d\n", tests_total);
    printf ("Passed: %d\n", tests_passed);
    printf ("Failed: %d\n", tests_failed);
    printf ("\n");

    return tests_failed > 0 ? 1 : 0;
}
