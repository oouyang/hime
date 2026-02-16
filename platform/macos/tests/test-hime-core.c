/*
 * HIME macOS Unit Tests - Core C API
 *
 * Tests the hime-core C API directly without macOS framework dependencies.
 * Links against hime-core-static and can be cross-compiled or built natively.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include "../../../tests/test-framework.h"
#include "hime-core.h"

#include <stdbool.h>

/* --- Version Tests --- */

TEST (version) {
    const char *ver = hime_version ();
    ASSERT_NOT_NULL (ver);
    ASSERT_TRUE (strlen (ver) > 0);
    TEST_PASS ();
}

/* --- Context Lifecycle --- */

TEST (context_create) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);
    hime_context_free (ctx);
    TEST_PASS ();
}

TEST (context_initial_state) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));
    ASSERT_EQ (HIME_IM_PHO, hime_get_input_method (ctx));
    ASSERT_FALSE (hime_has_candidates (ctx));
    ASSERT_EQ (0, hime_get_candidate_count (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

TEST (context_reset) {
    HimeContext *ctx = hime_context_new ();
    hime_context_reset (ctx);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Mode Control --- */

TEST (toggle_mode) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_TRUE (hime_is_chinese_mode (ctx));
    bool r = hime_toggle_chinese_mode (ctx);
    ASSERT_FALSE (r);
    ASSERT_FALSE (hime_is_chinese_mode (ctx));
    r = hime_toggle_chinese_mode (ctx);
    ASSERT_TRUE (r);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

TEST (set_chinese_mode) {
    HimeContext *ctx = hime_context_new ();
    hime_set_chinese_mode (ctx, false);
    ASSERT_FALSE (hime_is_chinese_mode (ctx));
    hime_set_chinese_mode (ctx, true);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

TEST (set_input_method) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_EQ (0, hime_set_input_method (ctx, HIME_IM_TSIN));
    ASSERT_EQ (HIME_IM_TSIN, hime_get_input_method (ctx));
    ASSERT_EQ (0, hime_set_input_method (ctx, HIME_IM_PHO));
    ASSERT_EQ (HIME_IM_PHO, hime_get_input_method (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Preedit / Commit --- */

TEST (preedit_empty) {
    HimeContext *ctx = hime_context_new ();
    char buf[256];
    int len = hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_EQ (0, len);
    ASSERT_STR_EQ ("", buf);
    hime_context_free (ctx);
    TEST_PASS ();
}

TEST (commit_empty) {
    HimeContext *ctx = hime_context_new ();
    char buf[256];
    int len = hime_get_commit (ctx, buf, sizeof (buf));
    ASSERT_EQ (0, len);
    ASSERT_STR_EQ ("", buf);
    hime_context_free (ctx);
    TEST_PASS ();
}

TEST (preedit_cursor) {
    HimeContext *ctx = hime_context_new ();
    int cursor = hime_get_preedit_cursor (ctx);
    ASSERT_TRUE (cursor >= 0);
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Candidates --- */

TEST (candidates_per_page) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_EQ (10, hime_get_candidates_per_page (ctx));
    hime_set_candidates_per_page (ctx, 5);
    ASSERT_EQ (5, hime_get_candidates_per_page (ctx));
    hime_set_candidates_per_page (ctx, 0);
    ASSERT_EQ (1, hime_get_candidates_per_page (ctx));
    hime_set_candidates_per_page (ctx, 100);
    ASSERT_EQ (10, hime_get_candidates_per_page (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Key Processing --- */

TEST (key_in_english_mode) {
    HimeContext *ctx = hime_context_new ();
    hime_set_chinese_mode (ctx, false);
    ASSERT_EQ (HIME_KEY_IGNORED, hime_process_key (ctx, 'A', 'a', 0));
    ASSERT_EQ (HIME_KEY_IGNORED, hime_process_key (ctx, ' ', ' ', 0));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Keyboard Layout --- */

TEST (keyboard_layout) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_EQ (HIME_KB_STANDARD, hime_get_keyboard_layout (ctx));
    hime_set_keyboard_layout (ctx, HIME_KB_HSU);
    ASSERT_EQ (HIME_KB_HSU, hime_get_keyboard_layout (ctx));
    hime_set_keyboard_layout (ctx, HIME_KB_STANDARD);
    ASSERT_EQ (HIME_KB_STANDARD, hime_get_keyboard_layout (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Character Set --- */

TEST (charset) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_EQ (HIME_CHARSET_TRADITIONAL, hime_get_charset (ctx));
    hime_set_charset (ctx, HIME_CHARSET_SIMPLIFIED);
    ASSERT_EQ (HIME_CHARSET_SIMPLIFIED, hime_get_charset (ctx));
    HimeCharset toggled = hime_toggle_charset (ctx);
    ASSERT_EQ (HIME_CHARSET_TRADITIONAL, toggled);
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Color Scheme --- */

TEST (color_scheme) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_EQ (HIME_COLOR_SCHEME_SYSTEM, hime_get_color_scheme (ctx));
    hime_set_color_scheme (ctx, HIME_COLOR_SCHEME_DARK);
    ASSERT_EQ (HIME_COLOR_SCHEME_DARK, hime_get_color_scheme (ctx));
    hime_set_color_scheme (ctx, HIME_COLOR_SCHEME_SYSTEM);
    ASSERT_EQ (HIME_COLOR_SCHEME_SYSTEM, hime_get_color_scheme (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Smart Punctuation --- */

TEST (smart_punctuation) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_FALSE (hime_get_smart_punctuation (ctx));
    hime_set_smart_punctuation (ctx, true);
    ASSERT_TRUE (hime_get_smart_punctuation (ctx));
    hime_set_smart_punctuation (ctx, false);
    ASSERT_FALSE (hime_get_smart_punctuation (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- Sound / Vibration --- */

TEST (sound_vibration) {
    HimeContext *ctx = hime_context_new ();
    ASSERT_FALSE (hime_get_sound_enabled (ctx));
    hime_set_sound_enabled (ctx, true);
    ASSERT_TRUE (hime_get_sound_enabled (ctx));
    ASSERT_FALSE (hime_get_vibration_enabled (ctx));
    hime_set_vibration_enabled (ctx, true);
    ASSERT_TRUE (hime_get_vibration_enabled (ctx));
    hime_set_vibration_duration (ctx, 50);
    ASSERT_EQ (50, hime_get_vibration_duration (ctx));
    hime_context_free (ctx);
    TEST_PASS ();
}

/* --- NULL Context Safety --- */

TEST (null_context_safety) {
    ASSERT_FALSE (hime_is_chinese_mode (NULL));
    ASSERT_EQ (HIME_IM_PHO, hime_get_input_method (NULL));
    ASSERT_EQ (-1, hime_set_input_method (NULL, HIME_IM_PHO));
    ASSERT_FALSE (hime_toggle_chinese_mode (NULL));

    char buf[64];
    ASSERT_EQ (-1, hime_get_preedit (NULL, buf, sizeof (buf)));
    ASSERT_EQ (-1, hime_get_commit (NULL, buf, sizeof (buf)));
    ASSERT_EQ (0, hime_get_preedit_cursor (NULL));
    ASSERT_FALSE (hime_has_candidates (NULL));
    ASSERT_EQ (0, hime_get_candidate_count (NULL));
    ASSERT_EQ (HIME_KEY_IGNORED, hime_process_key (NULL, 'A', 'a', 0));
    ASSERT_EQ (HIME_KEY_IGNORED, hime_select_candidate (NULL, 0));

    hime_context_reset (NULL);
    hime_set_chinese_mode (NULL, true);
    hime_clear_commit (NULL);
    hime_context_free (NULL);
    TEST_PASS ();
}

/* --- Multiple Contexts --- */

TEST (multiple_contexts) {
    HimeContext *ctx1 = hime_context_new ();
    HimeContext *ctx2 = hime_context_new ();
    ASSERT_NOT_NULL (ctx1);
    ASSERT_NOT_NULL (ctx2);
    hime_set_chinese_mode (ctx1, false);
    ASSERT_FALSE (hime_is_chinese_mode (ctx1));
    ASSERT_TRUE (hime_is_chinese_mode (ctx2));
    hime_context_free (ctx1);
    hime_context_free (ctx2);
    TEST_PASS ();
}

TEST_SUITE_BEGIN ("hime-core C API tests (macOS)")
RUN_TEST (version);
RUN_TEST (context_create);
RUN_TEST (context_initial_state);
RUN_TEST (context_reset);
RUN_TEST (toggle_mode);
RUN_TEST (set_chinese_mode);
RUN_TEST (set_input_method);
RUN_TEST (preedit_empty);
RUN_TEST (commit_empty);
RUN_TEST (preedit_cursor);
RUN_TEST (candidates_per_page);
RUN_TEST (key_in_english_mode);
RUN_TEST (keyboard_layout);
RUN_TEST (charset);
RUN_TEST (color_scheme);
RUN_TEST (smart_punctuation);
RUN_TEST (sound_vibration);
RUN_TEST (null_context_safety);
RUN_TEST (multiple_contexts);
TEST_SUITE_END ()
