/*
 * HIME Android Unit Tests - Encryption Functions
 *
 * Java port of the XOR cipher tests from tests/test-hime-crypt.c.
 * Tests the LCG PRNG and XOR encryption algorithm used for
 * client-server password exchange.
 *
 * These tests run on the JVM without Android framework.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android;

import static org.junit.Assert.*;

import org.junit.Test;

/**
 * Tests for the HIME encryption algorithm (pure Java reimplementation).
 * Verifies that the XOR cipher + LCG PRNG produces correct results,
 * matching the C implementation in hime-crypt.c.
 */
public class HimeCryptTest {
    private static final int HIME_PASSWD_N = 31;

    /**
     * LCG PRNG matching __hime_rand__ in hime-crypt.c.
     * Uses a long[] as mutable seed holder (Java has no pointer).
     */
    private static int himeRand(long[] seed) {
        seed[0] = (seed[0] * 1103515245L + 12345L) & 0xFFFFFFFFL;
        return (int) ((seed[0] / 65536L) % 32768L);
    }

    /**
     * XOR cipher matching __hime_enc_mem in hime-crypt.c.
     */
    private static void himeEncMem(byte[] data, int offset, int length,
                                    byte[] passwd, long[] seed) {
        for (int i = 0; i < length; i++) {
            int v = himeRand(seed) % HIME_PASSWD_N;
            data[offset + i] ^= passwd[v];
        }
    }

    /** Fill passwd with known pattern */
    private static byte[] makePasswd(int base) {
        byte[] passwd = new byte[HIME_PASSWD_N];
        for (int i = 0; i < HIME_PASSWD_N; i++) {
            passwd[i] = (byte) ((base + i) & 0xFF);
        }
        return passwd;
    }

    /* ========== PRNG Tests ========== */

    @Test
    public void rand_deterministic_seed0() {
        long[] seed = {0};
        int r = himeRand(seed);
        /* seed = 0*1103515245 + 12345 = 12345, return (12345/65536)%32768 = 0 */
        assertEquals(0, r);
    }

    @Test
    public void rand_sequence_reproducible() {
        long[] seed1 = {1};
        int r1 = himeRand(seed1);
        int r2 = himeRand(seed1);
        int r3 = himeRand(seed1);

        /* Verify reproducibility */
        long[] seed2 = {1};
        assertEquals(r1, himeRand(seed2));
        assertEquals(r2, himeRand(seed2));
        assertEquals(r3, himeRand(seed2));
    }

    @Test
    public void rand_sequence_advances() {
        long[] seed = {1};
        int r1 = himeRand(seed);
        int r2 = himeRand(seed);
        int r3 = himeRand(seed);

        /* Not all the same */
        assertTrue(r1 != r2 || r2 != r3);
    }

    /* ========== Encryption Tests ========== */

    @Test
    public void enc_zero_length() {
        byte[] buf = {(byte) 0xAA, (byte) 0xBB};
        byte[] orig = {(byte) 0xAA, (byte) 0xBB};
        byte[] passwd = makePasswd(0x10);
        long[] seed = {42};
        himeEncMem(buf, 0, 0, passwd, seed);
        assertArrayEquals(orig, buf);
    }

    @Test
    public void enc_single_byte() {
        byte[] buf = {0x42};
        byte[] passwd = makePasswd(0x10);

        /* Compute expected */
        long[] s = {1};
        int r = himeRand(s);
        int idx = r % HIME_PASSWD_N;
        byte expected = (byte) (0x42 ^ passwd[idx]);

        long[] seed = {1};
        himeEncMem(buf, 0, 1, passwd, seed);
        assertEquals(expected, buf[0]);
    }

    @Test
    public void enc_known_vector() {
        byte[] buf = {0, 0, 0, 0};
        byte[] passwd = makePasswd(0x10);

        /* Compute expected manually */
        byte[] expected = new byte[4];
        long[] s = {100};
        for (int i = 0; i < 4; i++) {
            int r = himeRand(s);
            expected[i] = (byte) (0x00 ^ passwd[r % HIME_PASSWD_N]);
        }

        long[] seed = {100};
        himeEncMem(buf, 0, 4, passwd, seed);
        assertArrayEquals(expected, buf);
    }

    @Test
    public void enc_decrypt_roundtrip() {
        byte[] original = new byte[16];
        byte[] buf = new byte[16];
        for (int i = 0; i < 16; i++) {
            original[i] = buf[i] = (byte) (i * 7 + 3);
        }
        byte[] passwd = makePasswd(0x20);

        long[] seed = {999};
        himeEncMem(buf, 0, 16, passwd, seed);

        /* XOR is self-inverse */
        seed[0] = 999;
        himeEncMem(buf, 0, 16, passwd, seed);

        assertArrayEquals(original, buf);
    }

    @Test
    public void enc_different_seeds() {
        byte[] buf1 = new byte[8];
        byte[] buf2 = new byte[8];
        java.util.Arrays.fill(buf1, (byte) 0xAA);
        java.util.Arrays.fill(buf2, (byte) 0xAA);
        byte[] passwd = makePasswd(0x30);

        himeEncMem(buf1, 0, 8, passwd, new long[]{1});
        himeEncMem(buf2, 0, 8, passwd, new long[]{2});

        assertFalse(java.util.Arrays.equals(buf1, buf2));
    }

    @Test
    public void enc_different_passwords() {
        byte[] buf1 = new byte[8];
        byte[] buf2 = new byte[8];
        java.util.Arrays.fill(buf1, (byte) 0x55);
        java.util.Arrays.fill(buf2, (byte) 0x55);

        himeEncMem(buf1, 0, 8, makePasswd(0x10), new long[]{42});
        himeEncMem(buf2, 0, 8, makePasswd(0x80), new long[]{42});

        assertFalse(java.util.Arrays.equals(buf1, buf2));
    }

    @Test
    public void enc_all_passwd_indices() {
        boolean[] hit = new boolean[HIME_PASSWD_N];
        long[] s = {12345};
        for (int i = 0; i < 256; i++) {
            int r = himeRand(s);
            hit[r % HIME_PASSWD_N] = true;
        }
        for (int i = 0; i < HIME_PASSWD_N; i++) {
            assertTrue("Index " + i + " should be hit", hit[i]);
        }
    }

    @Test
    public void enc_large_buffer_roundtrip() {
        byte[] original = new byte[1024];
        byte[] buf = new byte[1024];
        for (int i = 0; i < 1024; i++) {
            original[i] = buf[i] = (byte) (i & 0xFF);
        }
        byte[] passwd = makePasswd(0x42);

        long[] seed = {77777};
        himeEncMem(buf, 0, 1024, passwd, seed);

        /* Should differ from original */
        assertFalse(java.util.Arrays.equals(original, buf));

        /* Roundtrip */
        seed[0] = 77777;
        himeEncMem(buf, 0, 1024, passwd, seed);
        assertArrayEquals(original, buf);
    }

    /* ========== Cross-platform Verification ========== */

    @Test
    public void rand_matches_c_implementation() {
        /*
         * Verify the Java PRNG produces the same sequence as the C version.
         * seed=1: C computes next = 1*1103515245 + 12345 = 1103527590
         *         return (1103527590 / 65536) % 32768 = 16838
         */
        long[] seed = {1};
        assertEquals(16838, himeRand(seed));
        /* seed is now 1103527590; next: 1103527590*1103515245 + 12345 (mod 2^32) */
    }
}
