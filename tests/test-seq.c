/*
 * Unit tests for sequence comparison functions (tsin-util.c)
 *
 * These functions are static inline in tsin-util.c, so we test
 * equivalent implementations here to verify the algorithms.
 */

#include <stdint.h>
#include <string.h>

#include <glib.h>

#include "test-framework.h"

/* Types */
typedef unsigned short phokey_t;
typedef unsigned int u_int;
typedef uint64_t u_int64_t;
typedef unsigned char u_char;

/* Copy of the sequence comparison functions from tsin-util.c */
static inline int phokey_t_seq16 (phokey_t *a, phokey_t *b, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (a[i] > b[i])
            return 1;
        else if (a[i] < b[i])
            return -1;
    }

    return 0;
}

static inline int phokey_t_seq32 (u_int *a, u_int *b, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (a[i] > b[i])
            return 1;
        else if (a[i] < b[i])
            return -1;
    }

    return 0;
}

static inline int phokey_t_seq64 (u_int64_t *a, u_int64_t *b, int len) {
    int i;

    for (i = 0; i < len; i++) {
        if (a[i] > b[i])
            return 1;
        else if (a[i] < b[i])
            return -1;
    }

    return 0;
}

/* Dispatcher function */
static int ph_key_sz = 2; /* Default to 16-bit */

static int phokey_t_seq (void *a, void *b, int len) {
    if (ph_key_sz == 2)
        return phokey_t_seq16 ((phokey_t *) a, (phokey_t *) b, len);
    if (ph_key_sz == 4)
        return phokey_t_seq32 ((u_int *) a, (u_int *) b, len);
    if (ph_key_sz == 8)
        return phokey_t_seq64 ((u_int64_t *) a, (u_int64_t *) b, len);
    return 0;
}

/* mask_tone function from tsin-util.c */
void mask_tone (phokey_t *pho, int plen, char *tone_mask) {
    int i;
    for (i = 0; i < plen; i++) {
        if (tone_mask[i])
            pho[i] &= ~7; /* Mask out lowest 3 bits (tone) */
    }
}

/* ============ phokey_t_seq16 tests ============ */

TEST (seq16_equal) {
    phokey_t a[] = {100, 200, 300};
    phokey_t b[] = {100, 200, 300};
    ASSERT_EQ (0, phokey_t_seq16 (a, b, 3));
    TEST_PASS ();
}

TEST (seq16_a_greater_first) {
    phokey_t a[] = {200, 100, 100};
    phokey_t b[] = {100, 200, 300};
    ASSERT_EQ (1, phokey_t_seq16 (a, b, 3));
    TEST_PASS ();
}

TEST (seq16_b_greater_first) {
    phokey_t a[] = {100, 200, 300};
    phokey_t b[] = {200, 100, 100};
    ASSERT_EQ (-1, phokey_t_seq16 (a, b, 3));
    TEST_PASS ();
}

TEST (seq16_a_greater_middle) {
    phokey_t a[] = {100, 300, 100};
    phokey_t b[] = {100, 200, 300};
    ASSERT_EQ (1, phokey_t_seq16 (a, b, 3));
    TEST_PASS ();
}

TEST (seq16_b_greater_middle) {
    phokey_t a[] = {100, 200, 300};
    phokey_t b[] = {100, 300, 100};
    ASSERT_EQ (-1, phokey_t_seq16 (a, b, 3));
    TEST_PASS ();
}

TEST (seq16_a_greater_last) {
    phokey_t a[] = {100, 200, 400};
    phokey_t b[] = {100, 200, 300};
    ASSERT_EQ (1, phokey_t_seq16 (a, b, 3));
    TEST_PASS ();
}

TEST (seq16_b_greater_last) {
    phokey_t a[] = {100, 200, 300};
    phokey_t b[] = {100, 200, 400};
    ASSERT_EQ (-1, phokey_t_seq16 (a, b, 3));
    TEST_PASS ();
}

TEST (seq16_empty) {
    phokey_t a[] = {100};
    phokey_t b[] = {200};
    ASSERT_EQ (0, phokey_t_seq16 (a, b, 0)); /* Length 0 = equal */
    TEST_PASS ();
}

TEST (seq16_single_element) {
    phokey_t a[] = {100};
    phokey_t b[] = {100};
    ASSERT_EQ (0, phokey_t_seq16 (a, b, 1));

    phokey_t c[] = {200};
    ASSERT_EQ (-1, phokey_t_seq16 (a, c, 1));
    ASSERT_EQ (1, phokey_t_seq16 (c, a, 1));
    TEST_PASS ();
}

TEST (seq16_max_values) {
    phokey_t a[] = {0xFFFF, 0xFFFF};
    phokey_t b[] = {0xFFFF, 0xFFFF};
    ASSERT_EQ (0, phokey_t_seq16 (a, b, 2));

    phokey_t c[] = {0xFFFF, 0xFFFE};
    ASSERT_EQ (1, phokey_t_seq16 (a, c, 2));
    ASSERT_EQ (-1, phokey_t_seq16 (c, a, 2));
    TEST_PASS ();
}

/* ============ phokey_t_seq32 tests ============ */

TEST (seq32_equal) {
    u_int a[] = {1000000, 2000000, 3000000};
    u_int b[] = {1000000, 2000000, 3000000};
    ASSERT_EQ (0, phokey_t_seq32 (a, b, 3));
    TEST_PASS ();
}

TEST (seq32_a_greater) {
    u_int a[] = {2000000, 1000000, 1000000};
    u_int b[] = {1000000, 2000000, 3000000};
    ASSERT_EQ (1, phokey_t_seq32 (a, b, 3));
    TEST_PASS ();
}

TEST (seq32_b_greater) {
    u_int a[] = {1000000, 2000000, 3000000};
    u_int b[] = {2000000, 1000000, 1000000};
    ASSERT_EQ (-1, phokey_t_seq32 (a, b, 3));
    TEST_PASS ();
}

TEST (seq32_max_values) {
    u_int a[] = {0xFFFFFFFF, 0xFFFFFFFF};
    u_int b[] = {0xFFFFFFFF, 0xFFFFFFFF};
    ASSERT_EQ (0, phokey_t_seq32 (a, b, 2));

    u_int c[] = {0xFFFFFFFF, 0xFFFFFFFE};
    ASSERT_EQ (1, phokey_t_seq32 (a, c, 2));
    TEST_PASS ();
}

/* ============ phokey_t_seq64 tests ============ */

TEST (seq64_equal) {
    u_int64_t a[] = {0x100000000ULL, 0x200000000ULL};
    u_int64_t b[] = {0x100000000ULL, 0x200000000ULL};
    ASSERT_EQ (0, phokey_t_seq64 (a, b, 2));
    TEST_PASS ();
}

TEST (seq64_a_greater) {
    u_int64_t a[] = {0x200000000ULL, 0x100000000ULL};
    u_int64_t b[] = {0x100000000ULL, 0x200000000ULL};
    ASSERT_EQ (1, phokey_t_seq64 (a, b, 2));
    TEST_PASS ();
}

TEST (seq64_b_greater) {
    u_int64_t a[] = {0x100000000ULL, 0x200000000ULL};
    u_int64_t b[] = {0x200000000ULL, 0x100000000ULL};
    ASSERT_EQ (-1, phokey_t_seq64 (a, b, 2));
    TEST_PASS ();
}

TEST (seq64_max_values) {
    u_int64_t a[] = {0xFFFFFFFFFFFFFFFFULL};
    u_int64_t b[] = {0xFFFFFFFFFFFFFFFFULL};
    ASSERT_EQ (0, phokey_t_seq64 (a, b, 1));

    u_int64_t c[] = {0xFFFFFFFFFFFFFFFEULL};
    ASSERT_EQ (1, phokey_t_seq64 (a, c, 1));
    ASSERT_EQ (-1, phokey_t_seq64 (c, a, 1));
    TEST_PASS ();
}

/* ============ phokey_t_seq dispatcher tests ============ */

TEST (seq_dispatcher_16bit) {
    ph_key_sz = 2;
    phokey_t a[] = {100, 200};
    phokey_t b[] = {100, 200};
    ASSERT_EQ (0, phokey_t_seq (a, b, 2));

    phokey_t c[] = {100, 300};
    ASSERT_EQ (-1, phokey_t_seq (a, c, 2));
    ASSERT_EQ (1, phokey_t_seq (c, a, 2));
    TEST_PASS ();
}

TEST (seq_dispatcher_32bit) {
    ph_key_sz = 4;
    u_int a[] = {100000, 200000};
    u_int b[] = {100000, 200000};
    ASSERT_EQ (0, phokey_t_seq (a, b, 2));

    u_int c[] = {100000, 300000};
    ASSERT_EQ (-1, phokey_t_seq (a, c, 2));
    TEST_PASS ();
}

TEST (seq_dispatcher_64bit) {
    ph_key_sz = 8;
    u_int64_t a[] = {0x100000000ULL};
    u_int64_t b[] = {0x100000000ULL};
    ASSERT_EQ (0, phokey_t_seq (a, b, 1));

    u_int64_t c[] = {0x200000000ULL};
    ASSERT_EQ (-1, phokey_t_seq (a, c, 1));
    TEST_PASS ();
}

TEST (seq_dispatcher_invalid_size) {
    ph_key_sz = 3; /* Invalid size */
    u_int a[] = {100, 200};
    u_int b[] = {100, 300};
    /* Should return 0 for invalid size */
    ASSERT_EQ (0, phokey_t_seq (a, b, 2));
    ph_key_sz = 2; /* Reset */
    TEST_PASS ();
}

/* ============ mask_tone tests ============ */

TEST (mask_tone_no_mask) {
    phokey_t pho[] = {0x1234, 0x5678, 0x9ABC};
    char tone_mask[] = {0, 0, 0};
    phokey_t original[] = {0x1234, 0x5678, 0x9ABC};

    mask_tone (pho, 3, tone_mask);

    /* Should be unchanged */
    ASSERT_EQ (original[0], pho[0]);
    ASSERT_EQ (original[1], pho[1]);
    ASSERT_EQ (original[2], pho[2]);
    TEST_PASS ();
}

TEST (mask_tone_all_masked) {
    phokey_t pho[] = {0x1237, 0x5675, 0x9AB3}; /* Last 3 bits: 7, 5, 3 */
    char tone_mask[] = {1, 1, 1};

    mask_tone (pho, 3, tone_mask);

    /* Last 3 bits should be cleared */
    ASSERT_EQ (0x1230, pho[0]); /* 0x1237 & ~7 = 0x1230 */
    ASSERT_EQ (0x5670, pho[1]); /* 0x5675 & ~7 = 0x5670 */
    ASSERT_EQ (0x9AB0, pho[2]); /* 0x9AB3 & ~7 = 0x9AB0 */
    TEST_PASS ();
}

TEST (mask_tone_partial_mask) {
    phokey_t pho[] = {0x1237, 0x5675, 0x9AB3};
    char tone_mask[] = {1, 0, 1}; /* Mask first and third only */

    mask_tone (pho, 3, tone_mask);

    ASSERT_EQ (0x1230, pho[0]); /* Masked */
    ASSERT_EQ (0x5675, pho[1]); /* Unchanged */
    ASSERT_EQ (0x9AB0, pho[2]); /* Masked */
    TEST_PASS ();
}

TEST (mask_tone_zero_tones) {
    phokey_t pho[] = {0x1230, 0x5670}; /* Already have 0 in last 3 bits */
    char tone_mask[] = {1, 1};

    mask_tone (pho, 2, tone_mask);

    /* Should remain unchanged */
    ASSERT_EQ (0x1230, pho[0]);
    ASSERT_EQ (0x5670, pho[1]);
    TEST_PASS ();
}

TEST (mask_tone_single_element) {
    phokey_t pho[] = {0xFFFF}; /* All bits set */
    char tone_mask[] = {1};

    mask_tone (pho, 1, tone_mask);

    /* Last 3 bits cleared: 0xFFFF & ~7 = 0xFFF8 */
    ASSERT_EQ (0xFFF8, pho[0]);
    TEST_PASS ();
}

TEST (mask_tone_empty) {
    phokey_t pho[] = {0x1234};
    char tone_mask[] = {1};

    mask_tone (pho, 0, tone_mask);

    /* Nothing should happen with length 0 */
    ASSERT_EQ (0x1234, pho[0]);
    TEST_PASS ();
}

/* ============ Test Suite ============ */

TEST_SUITE_BEGIN ("Sequence Comparison Functions")
/* phokey_t_seq16 tests */
RUN_TEST (seq16_equal);
RUN_TEST (seq16_a_greater_first);
RUN_TEST (seq16_b_greater_first);
RUN_TEST (seq16_a_greater_middle);
RUN_TEST (seq16_b_greater_middle);
RUN_TEST (seq16_a_greater_last);
RUN_TEST (seq16_b_greater_last);
RUN_TEST (seq16_empty);
RUN_TEST (seq16_single_element);
RUN_TEST (seq16_max_values);

/* phokey_t_seq32 tests */
RUN_TEST (seq32_equal);
RUN_TEST (seq32_a_greater);
RUN_TEST (seq32_b_greater);
RUN_TEST (seq32_max_values);

/* phokey_t_seq64 tests */
RUN_TEST (seq64_equal);
RUN_TEST (seq64_a_greater);
RUN_TEST (seq64_b_greater);
RUN_TEST (seq64_max_values);

/* Dispatcher tests */
RUN_TEST (seq_dispatcher_16bit);
RUN_TEST (seq_dispatcher_32bit);
RUN_TEST (seq_dispatcher_64bit);
RUN_TEST (seq_dispatcher_invalid_size);

/* mask_tone tests */
RUN_TEST (mask_tone_no_mask);
RUN_TEST (mask_tone_all_masked);
RUN_TEST (mask_tone_partial_mask);
RUN_TEST (mask_tone_zero_tones);
RUN_TEST (mask_tone_single_element);
RUN_TEST (mask_tone_empty);
TEST_SUITE_END ()
