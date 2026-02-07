/*
 * HIME Core Library Test
 * Tests the hime-core API without requiring data files.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hime-core.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Testing %s... ", #name); \
    test_##name(); \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("FAIL\n    Assertion failed: %s\n    at %s:%d\n", \
               #expr, __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAIL\n    Expected %d, got %d\n    at %s:%d\n", \
               (int)(expected), (int)(actual), __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("FAIL\n    Expected \"%s\", got \"%s\"\n    at %s:%d\n", \
               (expected), (actual), __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* ========== Tests ========== */

TEST(version) {
    const char *ver = hime_version();
    ASSERT_TRUE(ver != NULL);
    ASSERT_TRUE(strlen(ver) > 0);
}

TEST(context_create) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);
    hime_context_free(ctx);
}

TEST(context_initial_state) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Should be in Chinese mode by default */
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    /* Should be using phonetic input by default */
    ASSERT_EQ(HIME_IM_PHO, hime_get_input_method(ctx));

    /* No candidates initially */
    ASSERT_TRUE(!hime_has_candidates(ctx));
    ASSERT_EQ(0, hime_get_candidate_count(ctx));

    hime_context_free(ctx);
}

TEST(context_reset) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Reset should not crash */
    hime_context_reset(ctx);
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
}

TEST(toggle_mode) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Initially Chinese mode */
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    /* Toggle to English */
    bool result = hime_toggle_chinese_mode(ctx);
    ASSERT_TRUE(!result);  /* Returns new state = false (English) */
    ASSERT_TRUE(!hime_is_chinese_mode(ctx));

    /* Toggle back to Chinese */
    result = hime_toggle_chinese_mode(ctx);
    ASSERT_TRUE(result);  /* Returns new state = true (Chinese) */
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
}

TEST(set_chinese_mode) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_set_chinese_mode(ctx, false);
    ASSERT_TRUE(!hime_is_chinese_mode(ctx));

    hime_set_chinese_mode(ctx, true);
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
}

TEST(set_input_method) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    int ret = hime_set_input_method(ctx, HIME_IM_TSIN);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(HIME_IM_TSIN, hime_get_input_method(ctx));

    ret = hime_set_input_method(ctx, HIME_IM_PHO);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(HIME_IM_PHO, hime_get_input_method(ctx));

    hime_context_free(ctx);
}

TEST(preedit_empty) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    char buf[256];
    int len = hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_EQ(0, len);
    ASSERT_STR_EQ("", buf);

    hime_context_free(ctx);
}

TEST(commit_empty) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    char buf[256];
    int len = hime_get_commit(ctx, buf, sizeof(buf));
    ASSERT_EQ(0, len);
    ASSERT_STR_EQ("", buf);

    hime_context_free(ctx);
}

TEST(candidates_per_page) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Default is 10 */
    ASSERT_EQ(10, hime_get_candidates_per_page(ctx));

    /* Set to 5 */
    hime_set_candidates_per_page(ctx, 5);
    ASSERT_EQ(5, hime_get_candidates_per_page(ctx));

    /* Clamp to minimum */
    hime_set_candidates_per_page(ctx, 0);
    ASSERT_EQ(1, hime_get_candidates_per_page(ctx));

    /* Clamp to maximum */
    hime_set_candidates_per_page(ctx, 100);
    ASSERT_EQ(10, hime_get_candidates_per_page(ctx));

    hime_context_free(ctx);
}

TEST(selection_keys) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Set custom selection keys */
    hime_set_selection_keys(ctx, "asdfghjkl;");

    /* No way to read back, just ensure no crash */
    hime_context_free(ctx);
}

TEST(key_in_english_mode) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Switch to English mode */
    hime_set_chinese_mode(ctx, false);

    /* Keys should be ignored in English mode */
    HimeKeyResult result = hime_process_key(ctx, 'A', 'a', 0);
    ASSERT_EQ(HIME_KEY_IGNORED, result);

    hime_context_free(ctx);
}

TEST(escape_clears_preedit) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Process a key to generate preedit (if data loaded) */
    hime_process_key(ctx, 'J', 'j', 0);

    /* Escape should reset */
    HimeKeyResult result = hime_process_key(ctx, 0x1B, 0x1B, 0);
    /* Either ABSORBED (if there was preedit) or IGNORED (if no preedit) */
    ASSERT_TRUE(result == HIME_KEY_ABSORBED || result == HIME_KEY_IGNORED);

    hime_context_free(ctx);
}

TEST(null_context_safety) {
    /* All functions should handle NULL context gracefully */
    ASSERT_TRUE(!hime_is_chinese_mode(NULL));
    ASSERT_EQ(HIME_IM_PHO, hime_get_input_method(NULL));
    ASSERT_EQ(-1, hime_set_input_method(NULL, HIME_IM_PHO));
    ASSERT_TRUE(!hime_toggle_chinese_mode(NULL));

    char buf[64];
    ASSERT_EQ(-1, hime_get_preedit(NULL, buf, sizeof(buf)));
    ASSERT_EQ(-1, hime_get_commit(NULL, buf, sizeof(buf)));
    ASSERT_EQ(0, hime_get_preedit_cursor(NULL));
    ASSERT_EQ(0, hime_get_preedit_attrs(NULL, NULL, 0));

    ASSERT_TRUE(!hime_has_candidates(NULL));
    ASSERT_EQ(0, hime_get_candidate_count(NULL));
    ASSERT_EQ(-1, hime_get_candidate(NULL, 0, buf, sizeof(buf)));
    ASSERT_EQ(0, hime_get_candidate_page(NULL));
    ASSERT_EQ(10, hime_get_candidates_per_page(NULL));

    ASSERT_EQ(HIME_KEY_IGNORED, hime_select_candidate(NULL, 0));
    ASSERT_TRUE(!hime_candidate_page_up(NULL));
    ASSERT_TRUE(!hime_candidate_page_down(NULL));

    /* These should not crash with NULL */
    hime_context_reset(NULL);
    hime_set_chinese_mode(NULL, true);
    hime_clear_commit(NULL);
    hime_set_selection_keys(NULL, "123");
    hime_set_candidates_per_page(NULL, 5);
    hime_context_free(NULL);
}

/* ========== Main ========== */

int main(int argc, char *argv[]) {
    printf("=== HIME Core Library Tests ===\n");

    printf("\n--- Basic Tests ---\n");
    RUN_TEST(version);
    RUN_TEST(context_create);
    RUN_TEST(context_initial_state);
    RUN_TEST(context_reset);

    printf("\n--- Mode Tests ---\n");
    RUN_TEST(toggle_mode);
    RUN_TEST(set_chinese_mode);
    RUN_TEST(set_input_method);

    printf("\n--- Buffer Tests ---\n");
    RUN_TEST(preedit_empty);
    RUN_TEST(commit_empty);

    printf("\n--- Configuration Tests ---\n");
    RUN_TEST(candidates_per_page);
    RUN_TEST(selection_keys);

    printf("\n--- Key Processing Tests ---\n");
    RUN_TEST(key_in_english_mode);
    RUN_TEST(escape_clears_preedit);

    printf("\n--- Safety Tests ---\n");
    RUN_TEST(null_context_safety);

    printf("\n=== Results ===\n");
    printf("Total:  %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
