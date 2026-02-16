/*
 * HIME iOS Unit Tests - Encryption Functions
 *
 * XCTest-based tests for the XOR cipher used for client-server
 * password exchange. Self-contained: includes local reimplementations
 * to avoid pulling in hime-protocol.h and its transitive dependencies.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <XCTest/XCTest.h>
#include <stdint.h>
#include <string.h>

/* Local definitions matching hime-protocol.h */
#define __HIME_PASSWD_N_ (31)

typedef struct {
    unsigned int seed;
    unsigned char passwd[__HIME_PASSWD_N_];
} TestHimePasswd;

/* Local copy of __hime_rand__ from hime-crypt.c */
static uint32_t test_hime_rand(uint32_t *next) {
    *next = *next * 1103515245 + 12345;
    return (*next / 65536) % 32768;
}

/* Local copy of __hime_enc_mem from hime-crypt.c */
static void test_hime_enc_mem(unsigned char *p, const int n, const TestHimePasswd *passwd, uint32_t *seed) {
    for (int i = 0; i < n; i++) {
        uint32_t v = test_hime_rand(seed) % __HIME_PASSWD_N_;
        p[i] ^= passwd->passwd[v];
    }
}

/* Helper: fill passwd with known pattern */
static void fill_passwd(TestHimePasswd *pw, unsigned char base) {
    pw->seed = 0;
    for (int i = 0; i < __HIME_PASSWD_N_; i++)
        pw->passwd[i] = (unsigned char) (base + i);
}

#pragma mark - PRNG Tests

@interface HimeCryptRandTests : XCTestCase
@end

@implementation HimeCryptRandTests

- (void)testRandDeterministicSeed0 {
    uint32_t seed = 0;
    uint32_t r = test_hime_rand(&seed);
    /* seed = 0*1103515245 + 12345 = 12345, return (12345/65536)%32768 = 0 */
    XCTAssertEqual(r, (uint32_t) 0);
}

- (void)testRandDeterministicSeed1 {
    uint32_t seed = 1;
    uint32_t r = test_hime_rand(&seed);
    /* seed = 1*1103515245 + 12345 = 1103527590, return (1103527590/65536)%32768 = 16838 */
    XCTAssertEqual(r, (uint32_t) 16838);
}

- (void)testRandSequenceReproducible {
    uint32_t seed1 = 1;
    uint32_t r1 = test_hime_rand(&seed1);
    uint32_t r2 = test_hime_rand(&seed1);
    uint32_t r3 = test_hime_rand(&seed1);

    uint32_t seed2 = 1;
    XCTAssertEqual(r1, test_hime_rand(&seed2));
    XCTAssertEqual(r2, test_hime_rand(&seed2));
    XCTAssertEqual(r3, test_hime_rand(&seed2));
}

- (void)testRandSequenceAdvances {
    uint32_t seed = 1;
    uint32_t r1 = test_hime_rand(&seed);
    uint32_t r2 = test_hime_rand(&seed);
    uint32_t r3 = test_hime_rand(&seed);

    /* Not all the same */
    XCTAssertTrue(r1 != r2 || r2 != r3, @"PRNG should produce varying values");
}

@end

#pragma mark - Encryption Tests

@interface HimeCryptEncTests : XCTestCase
@end

@implementation HimeCryptEncTests

- (void)testEncZeroLength {
    unsigned char buf[] = {0xAA, 0xBB};
    unsigned char orig[] = {0xAA, 0xBB};
    TestHimePasswd pw;
    fill_passwd(&pw, 0x10);
    uint32_t seed = 42;
    test_hime_enc_mem(buf, 0, &pw, &seed);
    XCTAssertEqual(memcmp(orig, buf, 2), 0, @"Zero-length encryption should be a no-op");
}

- (void)testEncSingleByte {
    unsigned char buf[1] = {0x42};
    TestHimePasswd pw;
    fill_passwd(&pw, 0x10);

    /* Compute expected */
    uint32_t s = 1;
    uint32_t r = test_hime_rand(&s);
    uint32_t idx = r % __HIME_PASSWD_N_;
    unsigned char expected = 0x42 ^ pw.passwd[idx];

    uint32_t seed = 1;
    test_hime_enc_mem(buf, 1, &pw, &seed);
    XCTAssertEqual(expected, buf[0]);
}

- (void)testEncKnownVector {
    unsigned char buf[4] = {0x00, 0x00, 0x00, 0x00};
    TestHimePasswd pw;
    fill_passwd(&pw, 0x10);

    unsigned char expected[4] = {0, 0, 0, 0};
    uint32_t s = 100;
    for (int i = 0; i < 4; i++) {
        uint32_t r = test_hime_rand(&s);
        expected[i] = 0x00 ^ pw.passwd[r % __HIME_PASSWD_N_];
    }

    uint32_t seed = 100;
    test_hime_enc_mem(buf, 4, &pw, &seed);
    XCTAssertEqual(memcmp(expected, buf, 4), 0);
}

- (void)testEncDecryptRoundtrip {
    unsigned char original[16];
    unsigned char buf[16];
    TestHimePasswd pw;
    fill_passwd(&pw, 0x20);

    for (int i = 0; i < 16; i++)
        original[i] = buf[i] = (unsigned char) (i * 7 + 3);

    uint32_t seed = 999;
    test_hime_enc_mem(buf, 16, &pw, &seed);

    /* XOR is self-inverse */
    seed = 999;
    test_hime_enc_mem(buf, 16, &pw, &seed);

    XCTAssertEqual(memcmp(original, buf, 16), 0, @"Roundtrip should recover original");
}

- (void)testEncDifferentSeeds {
    unsigned char buf1[8], buf2[8];
    TestHimePasswd pw;
    fill_passwd(&pw, 0x30);

    memset(buf1, 0xAA, 8);
    memset(buf2, 0xAA, 8);

    uint32_t seed1 = 1;
    uint32_t seed2 = 2;
    test_hime_enc_mem(buf1, 8, &pw, &seed1);
    test_hime_enc_mem(buf2, 8, &pw, &seed2);

    XCTAssertNotEqual(memcmp(buf1, buf2, 8), 0, @"Different seeds should produce different ciphertext");
}

- (void)testEncDifferentPasswords {
    unsigned char buf1[8], buf2[8];
    TestHimePasswd pw1, pw2;
    fill_passwd(&pw1, 0x10);
    fill_passwd(&pw2, 0x80);

    memset(buf1, 0x55, 8);
    memset(buf2, 0x55, 8);

    uint32_t seed1 = 42, seed2 = 42;
    test_hime_enc_mem(buf1, 8, &pw1, &seed1);
    test_hime_enc_mem(buf2, 8, &pw2, &seed2);

    XCTAssertNotEqual(memcmp(buf1, buf2, 8), 0, @"Different passwords should produce different ciphertext");
}

- (void)testEncAllPasswdIndices {
    int hit[__HIME_PASSWD_N_] = {0};

    uint32_t s = 12345;
    for (int i = 0; i < 256; i++) {
        uint32_t r = test_hime_rand(&s);
        hit[r % __HIME_PASSWD_N_] = 1;
    }

    for (int i = 0; i < __HIME_PASSWD_N_; i++)
        XCTAssertEqual(hit[i], 1, @"Index %d should be hit", i);
}

- (void)testEncLargeBufferRoundtrip {
    unsigned char original[1024];
    unsigned char buf[1024];
    TestHimePasswd pw;
    fill_passwd(&pw, 0x42);

    for (int i = 0; i < 1024; i++)
        original[i] = buf[i] = (unsigned char) (i & 0xFF);

    uint32_t seed = 77777;
    test_hime_enc_mem(buf, 1024, &pw, &seed);

    XCTAssertNotEqual(memcmp(original, buf, 1024), 0, @"Should differ from original");

    /* Roundtrip */
    seed = 77777;
    test_hime_enc_mem(buf, 1024, &pw, &seed);
    XCTAssertEqual(memcmp(original, buf, 1024), 0, @"Roundtrip should recover original");
}

@end
