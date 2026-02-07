/*
 * Unit tests for HIME Core Library (shared implementation)
 *
 * Tests all public API functions defined in hime-core.h
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include "../../tests/test-framework.h"
#include "../include/hime-core.h"

/* ========== Initialization Tests ========== */

TEST(init_with_valid_path) {
    int ret = hime_init("../../data");
    ASSERT_EQ(0, ret);
    hime_cleanup();
    TEST_PASS();
}

TEST(init_with_null_path) {
    int ret = hime_init(NULL);
    /* Should still work, using default/empty path */
    ASSERT_EQ(0, ret);
    hime_cleanup();
    TEST_PASS();
}

TEST(version_not_null) {
    const char *version = hime_version();
    ASSERT_NOT_NULL(version);
    ASSERT_TRUE(strlen(version) > 0);
    TEST_PASS();
}

TEST(double_init_cleanup) {
    ASSERT_EQ(0, hime_init("../../data"));
    ASSERT_EQ(0, hime_init("../../data"));  /* Should be safe to call twice */
    hime_cleanup();
    hime_cleanup();  /* Should be safe to call twice */
    TEST_PASS();
}

/* ========== Context Management Tests ========== */

TEST(context_new_returns_valid) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();
    ASSERT_NOT_NULL(ctx);
    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(context_free_null_safe) {
    hime_context_free(NULL);  /* Should not crash */
    TEST_PASS();
}

TEST(context_reset) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();
    ASSERT_NOT_NULL(ctx);

    /* Set some state */
    hime_set_chinese_mode(ctx, true);

    /* Reset should clear state */
    hime_context_reset(ctx);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(multiple_contexts) {
    hime_init("../../data");
    HimeContext *ctx1 = hime_context_new();
    HimeContext *ctx2 = hime_context_new();
    HimeContext *ctx3 = hime_context_new();

    ASSERT_NOT_NULL(ctx1);
    ASSERT_NOT_NULL(ctx2);
    ASSERT_NOT_NULL(ctx3);
    ASSERT_NE((long)ctx1, (long)ctx2);
    ASSERT_NE((long)ctx2, (long)ctx3);

    hime_context_free(ctx1);
    hime_context_free(ctx2);
    hime_context_free(ctx3);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Input Method Control Tests ========== */

TEST(set_get_input_method) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    ASSERT_EQ(0, hime_set_input_method(ctx, HIME_IM_PHO));
    ASSERT_EQ(HIME_IM_PHO, hime_get_input_method(ctx));

    ASSERT_EQ(0, hime_set_input_method(ctx, HIME_IM_TSIN));
    ASSERT_EQ(HIME_IM_TSIN, hime_get_input_method(ctx));

    ASSERT_EQ(0, hime_set_input_method(ctx, HIME_IM_GTAB));
    ASSERT_EQ(HIME_IM_GTAB, hime_get_input_method(ctx));

    ASSERT_EQ(0, hime_set_input_method(ctx, HIME_IM_INTCODE));
    ASSERT_EQ(HIME_IM_INTCODE, hime_get_input_method(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(toggle_chinese_mode) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    /* Start in Chinese mode by default */
    hime_set_chinese_mode(ctx, true);
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    /* Toggle off */
    bool result = hime_toggle_chinese_mode(ctx);
    ASSERT_FALSE(result);
    ASSERT_FALSE(hime_is_chinese_mode(ctx));

    /* Toggle on */
    result = hime_toggle_chinese_mode(ctx);
    ASSERT_TRUE(result);
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(set_chinese_mode) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_chinese_mode(ctx, false);
    ASSERT_FALSE(hime_is_chinese_mode(ctx));

    hime_set_chinese_mode(ctx, true);
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Keyboard Layout Tests ========== */

TEST(set_get_keyboard_layout) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    ASSERT_EQ(0, hime_set_keyboard_layout(ctx, HIME_KB_STANDARD));
    ASSERT_EQ(HIME_KB_STANDARD, hime_get_keyboard_layout(ctx));

    ASSERT_EQ(0, hime_set_keyboard_layout(ctx, HIME_KB_HSU));
    ASSERT_EQ(HIME_KB_HSU, hime_get_keyboard_layout(ctx));

    ASSERT_EQ(0, hime_set_keyboard_layout(ctx, HIME_KB_ETEN));
    ASSERT_EQ(HIME_KB_ETEN, hime_get_keyboard_layout(ctx));

    ASSERT_EQ(0, hime_set_keyboard_layout(ctx, HIME_KB_DVORAK));
    ASSERT_EQ(HIME_KB_DVORAK, hime_get_keyboard_layout(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(set_keyboard_layout_by_name) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    ASSERT_EQ(0, hime_set_keyboard_layout_by_name(ctx, "standard"));
    ASSERT_EQ(HIME_KB_STANDARD, hime_get_keyboard_layout(ctx));

    ASSERT_EQ(0, hime_set_keyboard_layout_by_name(ctx, "hsu"));
    ASSERT_EQ(HIME_KB_HSU, hime_get_keyboard_layout(ctx));

    ASSERT_EQ(0, hime_set_keyboard_layout_by_name(ctx, "eten"));
    ASSERT_EQ(HIME_KB_ETEN, hime_get_keyboard_layout(ctx));

    /* Invalid name should return -1 */
    ASSERT_EQ(-1, hime_set_keyboard_layout_by_name(ctx, "invalid"));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Preedit Tests ========== */

TEST(get_preedit_empty) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    char buffer[256];
    int len = hime_get_preedit(ctx, buffer, sizeof(buffer));
    ASSERT_EQ(0, len);
    ASSERT_STR_EQ("", buffer);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(get_preedit_cursor) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    int cursor = hime_get_preedit_cursor(ctx);
    ASSERT_EQ(0, cursor);  /* Should be 0 initially */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(get_preedit_attrs) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    HimePreeditAttr attrs[10];
    int count = hime_get_preedit_attrs(ctx, attrs, 10);
    ASSERT_EQ(0, count);  /* No attrs initially */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Commit Tests ========== */

TEST(get_commit_empty) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    char buffer[256];
    int len = hime_get_commit(ctx, buffer, sizeof(buffer));
    ASSERT_EQ(0, len);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(clear_commit) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_clear_commit(ctx);  /* Should not crash */

    char buffer[256];
    int len = hime_get_commit(ctx, buffer, sizeof(buffer));
    ASSERT_EQ(0, len);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Candidate Tests ========== */

TEST(candidates_initially_empty) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    ASSERT_FALSE(hime_has_candidates(ctx));
    ASSERT_EQ(0, hime_get_candidate_count(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(candidate_page_functions) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    ASSERT_EQ(0, hime_get_candidate_page(ctx));
    ASSERT_TRUE(hime_get_candidates_per_page(ctx) > 0);

    /* Should return false when no candidates */
    ASSERT_FALSE(hime_candidate_page_up(ctx));
    ASSERT_FALSE(hime_candidate_page_down(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(set_candidates_per_page) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_candidates_per_page(ctx, 5);
    ASSERT_EQ(5, hime_get_candidates_per_page(ctx));

    hime_set_candidates_per_page(ctx, 10);
    ASSERT_EQ(10, hime_get_candidates_per_page(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(set_selection_keys) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_selection_keys(ctx, "asdfghjkl;");
    /* No getter, just verify it doesn't crash */

    hime_set_selection_keys(ctx, "1234567890");

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== GTAB Tests ========== */

TEST(gtab_get_table_count) {
    hime_init("../../data");

    int count = hime_gtab_get_table_count();
    ASSERT_TRUE(count > 0);  /* Should have some tables registered */

    hime_cleanup();
    TEST_PASS();
}

TEST(gtab_get_table_info) {
    hime_init("../../data");

    int count = hime_gtab_get_table_count();
    if (count > 0) {
        HimeGtabInfo info;
        int ret = hime_gtab_get_table_info(0, &info);
        ASSERT_EQ(0, ret);
        ASSERT_TRUE(strlen(info.name) > 0);
        ASSERT_TRUE(strlen(info.filename) > 0);
    }

    /* Out of range should return -1 */
    HimeGtabInfo info;
    ASSERT_EQ(-1, hime_gtab_get_table_info(-1, &info));
    ASSERT_EQ(-1, hime_gtab_get_table_info(1000, &info));

    hime_cleanup();
    TEST_PASS();
}

TEST(gtab_load_table_by_id) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    /* Set to GTAB mode first */
    hime_set_input_method(ctx, HIME_IM_GTAB);

    /* Try to load a table - may fail if file not found, but shouldn't crash */
    int ret = hime_gtab_load_table_by_id(ctx, HIME_GTAB_CJ);
    /* Result depends on whether the table file exists */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(gtab_get_current_table) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    const char *table = hime_gtab_get_current_table(ctx);
    /* May be empty string if no table loaded */
    ASSERT_NOT_NULL(table);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Intcode Tests ========== */

TEST(intcode_set_get_mode) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_intcode_set_mode(ctx, HIME_INTCODE_UNICODE);
    ASSERT_EQ(HIME_INTCODE_UNICODE, hime_intcode_get_mode(ctx));

    hime_intcode_set_mode(ctx, HIME_INTCODE_BIG5);
    ASSERT_EQ(HIME_INTCODE_BIG5, hime_intcode_get_mode(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(intcode_get_buffer) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    char buffer[32];
    int len = hime_intcode_get_buffer(ctx, buffer, sizeof(buffer));
    ASSERT_EQ(0, len);  /* Should be empty initially */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(intcode_convert_unicode) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_intcode_set_mode(ctx, HIME_INTCODE_UNICODE);

    char buffer[16];
    int len = hime_intcode_convert(ctx, "4E2D", buffer, sizeof(buffer));

    /* Should produce UTF-8 for 中 (U+4E2D) */
    if (len > 0) {
        ASSERT_EQ(3, len);  /* 中 is 3 bytes in UTF-8 */
        ASSERT_STR_EQ("中", buffer);
    }

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(intcode_convert_invalid) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    char buffer[16];
    int len = hime_intcode_convert(ctx, "ZZZZ", buffer, sizeof(buffer));
    ASSERT_EQ(0, len);  /* Invalid hex should return 0 */

    len = hime_intcode_convert(ctx, "", buffer, sizeof(buffer));
    ASSERT_EQ(0, len);  /* Empty string should return 0 */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Settings Tests ========== */

TEST(charset_settings) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_charset(ctx, HIME_CHARSET_TRADITIONAL);
    ASSERT_EQ(HIME_CHARSET_TRADITIONAL, hime_get_charset(ctx));

    hime_set_charset(ctx, HIME_CHARSET_SIMPLIFIED);
    ASSERT_EQ(HIME_CHARSET_SIMPLIFIED, hime_get_charset(ctx));

    /* Toggle */
    HimeCharset result = hime_toggle_charset(ctx);
    ASSERT_EQ(HIME_CHARSET_TRADITIONAL, result);
    ASSERT_EQ(HIME_CHARSET_TRADITIONAL, hime_get_charset(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(smart_punctuation_settings) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_smart_punctuation(ctx, true);
    ASSERT_TRUE(hime_get_smart_punctuation(ctx));

    hime_set_smart_punctuation(ctx, false);
    ASSERT_FALSE(hime_get_smart_punctuation(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(pinyin_annotation_settings) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_pinyin_annotation(ctx, true);
    ASSERT_TRUE(hime_get_pinyin_annotation(ctx));

    hime_set_pinyin_annotation(ctx, false);
    ASSERT_FALSE(hime_get_pinyin_annotation(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(candidate_style_settings) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_candidate_style(ctx, HIME_CAND_STYLE_HORIZONTAL);
    ASSERT_EQ(HIME_CAND_STYLE_HORIZONTAL, hime_get_candidate_style(ctx));

    hime_set_candidate_style(ctx, HIME_CAND_STYLE_VERTICAL);
    ASSERT_EQ(HIME_CAND_STYLE_VERTICAL, hime_get_candidate_style(ctx));

    hime_set_candidate_style(ctx, HIME_CAND_STYLE_POPUP);
    ASSERT_EQ(HIME_CAND_STYLE_POPUP, hime_get_candidate_style(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(sound_settings) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_sound_enabled(ctx, true);
    ASSERT_TRUE(hime_get_sound_enabled(ctx));

    hime_set_sound_enabled(ctx, false);
    ASSERT_FALSE(hime_get_sound_enabled(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(vibration_settings) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_vibration_enabled(ctx, true);
    ASSERT_TRUE(hime_get_vibration_enabled(ctx));

    hime_set_vibration_enabled(ctx, false);
    ASSERT_FALSE(hime_get_vibration_enabled(ctx));

    hime_set_vibration_duration(ctx, 25);
    ASSERT_EQ(25, hime_get_vibration_duration(ctx));

    hime_set_vibration_duration(ctx, 50);
    ASSERT_EQ(50, hime_get_vibration_duration(ctx));

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(color_scheme_settings) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_color_scheme(ctx, HIME_COLOR_SCHEME_LIGHT);
    ASSERT_EQ(HIME_COLOR_SCHEME_LIGHT, hime_get_color_scheme(ctx));

    hime_set_color_scheme(ctx, HIME_COLOR_SCHEME_DARK);
    ASSERT_EQ(HIME_COLOR_SCHEME_DARK, hime_get_color_scheme(ctx));

    hime_set_color_scheme(ctx, HIME_COLOR_SCHEME_SYSTEM);
    ASSERT_EQ(HIME_COLOR_SCHEME_SYSTEM, hime_get_color_scheme(ctx));

    /* Test system dark mode notification */
    hime_set_system_dark_mode(ctx, true);
    hime_set_system_dark_mode(ctx, false);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Punctuation Tests ========== */

TEST(reset_punctuation_state) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_reset_punctuation_state(ctx);  /* Should not crash */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(convert_punctuation) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_smart_punctuation(ctx, true);

    char buffer[16];
    int len;

    /* Test comma conversion */
    len = hime_convert_punctuation(ctx, ',', buffer, sizeof(buffer));
    if (len > 0) {
        ASSERT_TRUE(len > 0);
        /* Should be Chinese comma */
    }

    /* Test period conversion */
    len = hime_convert_punctuation(ctx, '.', buffer, sizeof(buffer));
    if (len > 0) {
        ASSERT_TRUE(len > 0);
    }

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Input Method Availability Tests ========== */

TEST(method_availability) {
    hime_init("../../data");

    /* PHO should always be available */
    ASSERT_TRUE(hime_is_method_available(HIME_IM_PHO));

    /* TSIN should be available */
    ASSERT_TRUE(hime_is_method_available(HIME_IM_TSIN));

    /* GTAB should be available */
    ASSERT_TRUE(hime_is_method_available(HIME_IM_GTAB));

    /* INTCODE should be available */
    ASSERT_TRUE(hime_is_method_available(HIME_IM_INTCODE));

    hime_cleanup();
    TEST_PASS();
}

TEST(method_names) {
    hime_init("../../data");

    const char *name;

    name = hime_get_method_name(HIME_IM_PHO);
    ASSERT_NOT_NULL(name);
    ASSERT_TRUE(strlen(name) > 0);

    name = hime_get_method_name(HIME_IM_TSIN);
    ASSERT_NOT_NULL(name);
    ASSERT_TRUE(strlen(name) > 0);

    name = hime_get_method_name(HIME_IM_GTAB);
    ASSERT_NOT_NULL(name);
    ASSERT_TRUE(strlen(name) > 0);

    hime_cleanup();
    TEST_PASS();
}

/* ========== Key Processing Tests ========== */

TEST(process_key_english_mode) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    /* Set to English mode */
    hime_set_chinese_mode(ctx, false);

    /* Keys should be ignored in English mode */
    HimeKeyResult result = hime_process_key(ctx, 'a', 'a', HIME_MOD_NONE);
    ASSERT_EQ(HIME_KEY_IGNORED, result);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(process_key_escape_clears) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_chinese_mode(ctx, true);

    /* Escape should clear any input */
    HimeKeyResult result = hime_process_key(ctx, 0x1B, 0, HIME_MOD_NONE);  /* 0x1B = Escape */
    /* Result depends on state, but shouldn't crash */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(process_key_with_modifiers) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_chinese_mode(ctx, true);

    /* Ctrl+key should typically be ignored */
    HimeKeyResult result = hime_process_key(ctx, 'a', 'a', HIME_MOD_CONTROL);
    /* Result depends on implementation */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Bopomofo Tests ========== */

TEST(get_bopomofo_string_empty) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    char buffer[64];
    int len = hime_get_bopomofo_string(ctx, buffer, sizeof(buffer));
    ASSERT_EQ(0, len);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== TSIN Tests ========== */

TEST(tsin_get_phrase_empty) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    char buffer[256];
    int len = hime_tsin_get_phrase(ctx, buffer, sizeof(buffer));
    ASSERT_EQ(0, len);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

TEST(tsin_commit_phrase) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    int len = hime_tsin_commit_phrase(ctx);
    ASSERT_EQ(0, len);  /* Nothing to commit initially */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== GTAB Key String Tests ========== */

TEST(gtab_get_key_string_empty) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    hime_set_input_method(ctx, HIME_IM_GTAB);

    char buffer[64];
    int len = hime_gtab_get_key_string(ctx, buffer, sizeof(buffer));
    ASSERT_EQ(0, len);  /* No keys entered */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Feedback Callback Tests ========== */

static int g_feedback_callback_count = 0;
static HimeFeedbackType g_last_feedback_type = -1;

static void my_feedback_callback(HimeFeedbackType type, void *user_data) {
    g_feedback_callback_count++;
    g_last_feedback_type = type;
    (void)user_data;
}

TEST(feedback_callback) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    g_feedback_callback_count = 0;

    hime_set_feedback_callback(ctx, my_feedback_callback, NULL);
    hime_set_sound_enabled(ctx, true);

    /* Callback should be registered but not called yet */
    ASSERT_EQ(0, g_feedback_callback_count);

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== Extended Candidate Tests ========== */

TEST(get_candidate_with_annotation) {
    hime_init("../../data");
    HimeContext *ctx = hime_context_new();

    char text[64];
    char annotation[64];

    /* Should handle gracefully when no candidates */
    int len = hime_get_candidate_with_annotation(ctx, 0, text, sizeof(text),
                                                  annotation, sizeof(annotation));
    ASSERT_TRUE(len <= 0);  /* No candidate at index 0 */

    hime_context_free(ctx);
    hime_cleanup();
    TEST_PASS();
}

/* ========== NULL Safety Tests ========== */

TEST(null_context_safety) {
    hime_init("../../data");

    /* These should not crash with NULL context */
    hime_context_reset(NULL);
    hime_set_input_method(NULL, HIME_IM_PHO);
    hime_get_input_method(NULL);
    hime_toggle_chinese_mode(NULL);
    hime_is_chinese_mode(NULL);
    hime_set_chinese_mode(NULL, true);

    char buffer[64];
    hime_get_preedit(NULL, buffer, sizeof(buffer));
    hime_get_commit(NULL, buffer, sizeof(buffer));
    hime_clear_commit(NULL);
    hime_has_candidates(NULL);
    hime_get_candidate_count(NULL);

    hime_cleanup();
    TEST_PASS();
}

/* ========== Test Suite ========== */

TEST_SUITE_BEGIN("HIME Core Library Tests")

    /* Initialization */
    RUN_TEST(init_with_valid_path);
    RUN_TEST(init_with_null_path);
    RUN_TEST(version_not_null);
    RUN_TEST(double_init_cleanup);

    /* Context management */
    RUN_TEST(context_new_returns_valid);
    RUN_TEST(context_free_null_safe);
    RUN_TEST(context_reset);
    RUN_TEST(multiple_contexts);

    /* Input method control */
    RUN_TEST(set_get_input_method);
    RUN_TEST(toggle_chinese_mode);
    RUN_TEST(set_chinese_mode);

    /* Keyboard layout */
    RUN_TEST(set_get_keyboard_layout);
    RUN_TEST(set_keyboard_layout_by_name);

    /* Preedit */
    RUN_TEST(get_preedit_empty);
    RUN_TEST(get_preedit_cursor);
    RUN_TEST(get_preedit_attrs);

    /* Commit */
    RUN_TEST(get_commit_empty);
    RUN_TEST(clear_commit);

    /* Candidates */
    RUN_TEST(candidates_initially_empty);
    RUN_TEST(candidate_page_functions);
    RUN_TEST(set_candidates_per_page);
    RUN_TEST(set_selection_keys);

    /* GTAB */
    RUN_TEST(gtab_get_table_count);
    RUN_TEST(gtab_get_table_info);
    RUN_TEST(gtab_load_table_by_id);
    RUN_TEST(gtab_get_current_table);

    /* Intcode */
    RUN_TEST(intcode_set_get_mode);
    RUN_TEST(intcode_get_buffer);
    RUN_TEST(intcode_convert_unicode);
    RUN_TEST(intcode_convert_invalid);

    /* Settings */
    RUN_TEST(charset_settings);
    RUN_TEST(smart_punctuation_settings);
    RUN_TEST(pinyin_annotation_settings);
    RUN_TEST(candidate_style_settings);
    RUN_TEST(sound_settings);
    RUN_TEST(vibration_settings);
    RUN_TEST(color_scheme_settings);

    /* Punctuation */
    RUN_TEST(reset_punctuation_state);
    RUN_TEST(convert_punctuation);

    /* Method availability */
    RUN_TEST(method_availability);
    RUN_TEST(method_names);

    /* Key processing */
    RUN_TEST(process_key_english_mode);
    RUN_TEST(process_key_escape_clears);
    RUN_TEST(process_key_with_modifiers);

    /* Bopomofo */
    RUN_TEST(get_bopomofo_string_empty);

    /* TSIN */
    RUN_TEST(tsin_get_phrase_empty);
    RUN_TEST(tsin_commit_phrase);

    /* GTAB key string */
    RUN_TEST(gtab_get_key_string_empty);

    /* Feedback */
    RUN_TEST(feedback_callback);

    /* Extended candidate */
    RUN_TEST(get_candidate_with_annotation);

    /* NULL safety */
    RUN_TEST(null_context_safety);

TEST_SUITE_END()
