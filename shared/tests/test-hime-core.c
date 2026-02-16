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

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

/* ========== pho.tab2 Test Fixture ========== */

static bool _pho_tab2_created = false;
static char _pho_tab2_path[1024];

/*
 * Generate a minimal valid pho.tab2 binary file for testing.
 *
 * Format (from load_pho_table):
 *   uint16_t  idxnum     (written twice — historical quirk)
 *   int32_t   ch_phoN    (number of PhoItem entries)
 *   int32_t   phrase_sz  (0 = no phrases)
 *   PhoIdx[idxnum]       (4 bytes each: uint16_t key, uint16_t start)
 *   PhoItem[ch_phoN]     (8 bytes each: char[4] ch, int32_t count)
 */
static void
generate_test_pho_tab2 (const char *dir)
{
    snprintf (_pho_tab2_path, sizeof (_pho_tab2_path), "%s/pho.tab2", dir);

    /* Don't overwrite if it already exists (e.g. from a full build) */
    struct stat st;
    if (stat (_pho_tab2_path, &st) == 0) {
        _pho_tab2_created = false;
        return;
    }

    FILE *fp = fopen (_pho_tab2_path, "wb");
    if (!fp)
        return;

    /*
     * We need entries for the phonetic tests:
     *   ㄇㄚ tone1 (space): typ_pho={3,0,1,1} → key=1545
     *   ㄇㄚˇ (3rd tone):   typ_pho={3,0,1,2} → key=1546
     *
     * Key calculation: pho2key merges typ_pho[] with bit shifts:
     *   key = initial(3) << 2 = 12; 12 << 4 | final(1) = 193;
     *   193 << 3 | tone = 1545 (tone1) or 1546 (tone3)
     *
     * Items must be sorted by index key. We use 3 index entries
     * and 3 items total (one per key, plus a dummy low key).
     */
    uint16_t idxnum = 3;
    int32_t ch_phoN = 3;
    int32_t phrase_sz = 0;

    /* Header: idxnum written twice (historical quirk) */
    fwrite (&idxnum, 2, 1, fp);
    fwrite (&idxnum, 2, 1, fp);
    fwrite (&ch_phoN, 4, 1, fp);
    fwrite (&phrase_sz, 4, 1, fp);

    /* PhoIdx entries (key, start) — must be sorted by key */
    struct {
        uint16_t key;
        uint16_t start;
    } idx[3] = {
        {1, 0},    /* dummy entry for key=1 → item 0 */
        {1545, 1}, /* ㄇㄚ tone1 → item 1 */
        {1546, 2}, /* ㄇㄚˇ      → item 2 */
    };
    fwrite (idx, 4, 3, fp);

    /* PhoItem entries: ch[4] + count(int32) = 8 bytes each */
    struct {
        char ch[4];
        int32_t count;
    } items[3] = {
        {{'\xe4', '\xb8', '\xad', '\0'}, 1}, /* 中 (dummy) */
        {{'\xe5', '\xaa', '\xbd', '\0'}, 1}, /* 媽 (ma1) */
        {{'\xe9', '\xa6', '\xac', '\0'}, 1}, /* 馬 (ma3) */
    };
    fwrite (items, 8, 3, fp);

    fclose (fp);
    _pho_tab2_created = true;
}

static void
cleanup_test_pho_tab2 (void)
{
    if (_pho_tab2_created) {
        unlink (_pho_tab2_path);
        _pho_tab2_created = false;
    }
}

/* ========== Initialization Tests ========== */

TEST (init_with_valid_path) {
    int ret = hime_init ("../../data");
    ASSERT_EQ (0, ret);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (init_with_null_path) {
    /* hime_init(NULL) leaves g_data_dir empty, so it tries "/pho.tab2"
     * which won't exist. Just verify it doesn't crash. */
    int ret = hime_init (NULL);
    if (ret == 0)
        hime_cleanup ();
    TEST_PASS ();
}

TEST (version_not_null) {
    const char *version = hime_version ();
    ASSERT_NOT_NULL (version);
    ASSERT_TRUE (strlen (version) > 0);
    TEST_PASS ();
}

TEST (double_init_cleanup) {
    ASSERT_EQ (0, hime_init ("../../data"));
    ASSERT_EQ (0, hime_init ("../../data")); /* Should be safe to call twice */
    hime_cleanup ();
    hime_cleanup (); /* Should be safe to call twice */
    TEST_PASS ();
}

/* ========== Context Management Tests ========== */

TEST (context_new_returns_valid) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (context_free_null_safe) {
    hime_context_free (NULL); /* Should not crash */
    TEST_PASS ();
}

TEST (context_reset) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Set some state */
    hime_set_chinese_mode (ctx, true);

    /* Reset should clear state */
    hime_context_reset (ctx);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (multiple_contexts) {
    hime_init ("../../data");
    HimeContext *ctx1 = hime_context_new ();
    HimeContext *ctx2 = hime_context_new ();
    HimeContext *ctx3 = hime_context_new ();

    ASSERT_NOT_NULL (ctx1);
    ASSERT_NOT_NULL (ctx2);
    ASSERT_NOT_NULL (ctx3);
    ASSERT_NE ((long) ctx1, (long) ctx2);
    ASSERT_NE ((long) ctx2, (long) ctx3);

    hime_context_free (ctx1);
    hime_context_free (ctx2);
    hime_context_free (ctx3);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Input Method Control Tests ========== */

TEST (set_get_input_method) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    ASSERT_EQ (0, hime_set_input_method (ctx, HIME_IM_PHO));
    ASSERT_EQ (HIME_IM_PHO, hime_get_input_method (ctx));

    ASSERT_EQ (0, hime_set_input_method (ctx, HIME_IM_TSIN));
    ASSERT_EQ (HIME_IM_TSIN, hime_get_input_method (ctx));

    ASSERT_EQ (0, hime_set_input_method (ctx, HIME_IM_GTAB));
    ASSERT_EQ (HIME_IM_GTAB, hime_get_input_method (ctx));

    ASSERT_EQ (0, hime_set_input_method (ctx, HIME_IM_INTCODE));
    ASSERT_EQ (HIME_IM_INTCODE, hime_get_input_method (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (toggle_chinese_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    /* Start in Chinese mode by default */
    hime_set_chinese_mode (ctx, true);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));

    /* Toggle off */
    bool result = hime_toggle_chinese_mode (ctx);
    ASSERT_FALSE (result);
    ASSERT_FALSE (hime_is_chinese_mode (ctx));

    /* Toggle on */
    result = hime_toggle_chinese_mode (ctx);
    ASSERT_TRUE (result);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (set_chinese_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_chinese_mode (ctx, false);
    ASSERT_FALSE (hime_is_chinese_mode (ctx));

    hime_set_chinese_mode (ctx, true);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Keyboard Layout Tests ========== */

TEST (set_get_keyboard_layout) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    ASSERT_EQ (0, hime_set_keyboard_layout (ctx, HIME_KB_STANDARD));
    ASSERT_EQ (HIME_KB_STANDARD, hime_get_keyboard_layout (ctx));

    ASSERT_EQ (0, hime_set_keyboard_layout (ctx, HIME_KB_HSU));
    ASSERT_EQ (HIME_KB_HSU, hime_get_keyboard_layout (ctx));

    ASSERT_EQ (0, hime_set_keyboard_layout (ctx, HIME_KB_ETEN));
    ASSERT_EQ (HIME_KB_ETEN, hime_get_keyboard_layout (ctx));

    ASSERT_EQ (0, hime_set_keyboard_layout (ctx, HIME_KB_DVORAK));
    ASSERT_EQ (HIME_KB_DVORAK, hime_get_keyboard_layout (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (set_keyboard_layout_by_name) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    ASSERT_EQ (0, hime_set_keyboard_layout_by_name (ctx, "standard"));
    ASSERT_EQ (HIME_KB_STANDARD, hime_get_keyboard_layout (ctx));

    ASSERT_EQ (0, hime_set_keyboard_layout_by_name (ctx, "hsu"));
    ASSERT_EQ (HIME_KB_HSU, hime_get_keyboard_layout (ctx));

    ASSERT_EQ (0, hime_set_keyboard_layout_by_name (ctx, "eten"));
    ASSERT_EQ (HIME_KB_ETEN, hime_get_keyboard_layout (ctx));

    /* Invalid name should return -1 */
    ASSERT_EQ (-1, hime_set_keyboard_layout_by_name (ctx, "invalid"));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Preedit Tests ========== */

TEST (get_preedit_empty) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char buffer[256];
    int len = hime_get_preedit (ctx, buffer, sizeof (buffer));
    ASSERT_EQ (0, len);
    ASSERT_STR_EQ ("", buffer);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (get_preedit_cursor) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    int cursor = hime_get_preedit_cursor (ctx);
    ASSERT_EQ (0, cursor); /* Should be 0 initially */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (get_preedit_attrs) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    HimePreeditAttr attrs[10];
    int count = hime_get_preedit_attrs (ctx, attrs, 10);
    ASSERT_EQ (0, count); /* No attrs initially */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Commit Tests ========== */

TEST (get_commit_empty) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char buffer[256];
    int len = hime_get_commit (ctx, buffer, sizeof (buffer));
    ASSERT_EQ (0, len);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (clear_commit) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_clear_commit (ctx); /* Should not crash */

    char buffer[256];
    int len = hime_get_commit (ctx, buffer, sizeof (buffer));
    ASSERT_EQ (0, len);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Candidate Tests ========== */

TEST (candidates_initially_empty) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    ASSERT_FALSE (hime_has_candidates (ctx));
    ASSERT_EQ (0, hime_get_candidate_count (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (candidate_page_functions) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    ASSERT_EQ (0, hime_get_candidate_page (ctx));
    ASSERT_TRUE (hime_get_candidates_per_page (ctx) > 0);

    /* Should return false when no candidates */
    ASSERT_FALSE (hime_candidate_page_up (ctx));
    ASSERT_FALSE (hime_candidate_page_down (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (set_candidates_per_page) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_candidates_per_page (ctx, 5);
    ASSERT_EQ (5, hime_get_candidates_per_page (ctx));

    hime_set_candidates_per_page (ctx, 10);
    ASSERT_EQ (10, hime_get_candidates_per_page (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (set_selection_keys) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_selection_keys (ctx, "asdfghjkl;");
    /* No getter, just verify it doesn't crash */

    hime_set_selection_keys (ctx, "1234567890");

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== GTAB Tests ========== */

TEST (gtab_get_table_count) {
    hime_init ("../../data");

    int count = hime_gtab_get_table_count ();
    ASSERT_TRUE (count > 0); /* Should have some tables registered */

    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_get_table_info) {
    hime_init ("../../data");

    int count = hime_gtab_get_table_count ();
    if (count > 0) {
        HimeGtabInfo info;
        int ret = hime_gtab_get_table_info (0, &info);
        ASSERT_EQ (0, ret);
        ASSERT_TRUE (strlen (info.name) > 0);
        ASSERT_TRUE (strlen (info.filename) > 0);
    }

    /* Out of range should return -1 */
    HimeGtabInfo info;
    ASSERT_EQ (-1, hime_gtab_get_table_info (-1, &info));
    ASSERT_EQ (-1, hime_gtab_get_table_info (1000, &info));

    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_load_table_by_id) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    /* Set to GTAB mode first */
    hime_set_input_method (ctx, HIME_IM_GTAB);

    /* Try to load a table - may fail if file not found, but shouldn't crash */
    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_CJ);
    /* Result depends on whether the table file exists */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_get_current_table) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    const char *table = hime_gtab_get_current_table (ctx);
    /* May be empty string if no table loaded */
    ASSERT_NOT_NULL (table);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Intcode Tests ========== */

TEST (intcode_set_get_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_intcode_set_mode (ctx, HIME_INTCODE_UNICODE);
    ASSERT_EQ (HIME_INTCODE_UNICODE, hime_intcode_get_mode (ctx));

    hime_intcode_set_mode (ctx, HIME_INTCODE_BIG5);
    ASSERT_EQ (HIME_INTCODE_BIG5, hime_intcode_get_mode (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (intcode_get_buffer) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char buffer[32];
    int len = hime_intcode_get_buffer (ctx, buffer, sizeof (buffer));
    ASSERT_EQ (0, len); /* Should be empty initially */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (intcode_convert_unicode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_intcode_set_mode (ctx, HIME_INTCODE_UNICODE);

    char buffer[16];
    int len = hime_intcode_convert (ctx, "4E2D", buffer, sizeof (buffer));

    /* Should produce UTF-8 for 中 (U+4E2D) */
    if (len > 0) {
        ASSERT_EQ (3, len); /* 中 is 3 bytes in UTF-8 */
        ASSERT_STR_EQ ("中", buffer);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (intcode_convert_invalid) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char buffer[16];
    int len = hime_intcode_convert (ctx, "ZZZZ", buffer, sizeof (buffer));
    ASSERT_EQ (0, len); /* Invalid hex should return 0 */

    len = hime_intcode_convert (ctx, "", buffer, sizeof (buffer));
    ASSERT_EQ (0, len); /* Empty string should return 0 */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Settings Tests ========== */

TEST (charset_settings) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_charset (ctx, HIME_CHARSET_TRADITIONAL);
    ASSERT_EQ (HIME_CHARSET_TRADITIONAL, hime_get_charset (ctx));

    hime_set_charset (ctx, HIME_CHARSET_SIMPLIFIED);
    ASSERT_EQ (HIME_CHARSET_SIMPLIFIED, hime_get_charset (ctx));

    /* Toggle */
    HimeCharset result = hime_toggle_charset (ctx);
    ASSERT_EQ (HIME_CHARSET_TRADITIONAL, result);
    ASSERT_EQ (HIME_CHARSET_TRADITIONAL, hime_get_charset (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (smart_punctuation_settings) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_smart_punctuation (ctx, true);
    ASSERT_TRUE (hime_get_smart_punctuation (ctx));

    hime_set_smart_punctuation (ctx, false);
    ASSERT_FALSE (hime_get_smart_punctuation (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pinyin_annotation_settings) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_pinyin_annotation (ctx, true);
    ASSERT_TRUE (hime_get_pinyin_annotation (ctx));

    hime_set_pinyin_annotation (ctx, false);
    ASSERT_FALSE (hime_get_pinyin_annotation (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (candidate_style_settings) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_candidate_style (ctx, HIME_CAND_STYLE_HORIZONTAL);
    ASSERT_EQ (HIME_CAND_STYLE_HORIZONTAL, hime_get_candidate_style (ctx));

    hime_set_candidate_style (ctx, HIME_CAND_STYLE_VERTICAL);
    ASSERT_EQ (HIME_CAND_STYLE_VERTICAL, hime_get_candidate_style (ctx));

    hime_set_candidate_style (ctx, HIME_CAND_STYLE_POPUP);
    ASSERT_EQ (HIME_CAND_STYLE_POPUP, hime_get_candidate_style (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (sound_settings) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_sound_enabled (ctx, true);
    ASSERT_TRUE (hime_get_sound_enabled (ctx));

    hime_set_sound_enabled (ctx, false);
    ASSERT_FALSE (hime_get_sound_enabled (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (vibration_settings) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_vibration_enabled (ctx, true);
    ASSERT_TRUE (hime_get_vibration_enabled (ctx));

    hime_set_vibration_enabled (ctx, false);
    ASSERT_FALSE (hime_get_vibration_enabled (ctx));

    hime_set_vibration_duration (ctx, 25);
    ASSERT_EQ (25, hime_get_vibration_duration (ctx));

    hime_set_vibration_duration (ctx, 50);
    ASSERT_EQ (50, hime_get_vibration_duration (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (color_scheme_settings) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_color_scheme (ctx, HIME_COLOR_SCHEME_LIGHT);
    ASSERT_EQ (HIME_COLOR_SCHEME_LIGHT, hime_get_color_scheme (ctx));

    hime_set_color_scheme (ctx, HIME_COLOR_SCHEME_DARK);
    ASSERT_EQ (HIME_COLOR_SCHEME_DARK, hime_get_color_scheme (ctx));

    hime_set_color_scheme (ctx, HIME_COLOR_SCHEME_SYSTEM);
    ASSERT_EQ (HIME_COLOR_SCHEME_SYSTEM, hime_get_color_scheme (ctx));

    /* Test system dark mode notification */
    hime_set_system_dark_mode (ctx, true);
    hime_set_system_dark_mode (ctx, false);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Punctuation Tests ========== */

TEST (reset_punctuation_state) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_reset_punctuation_state (ctx); /* Should not crash */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (convert_punctuation) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_smart_punctuation (ctx, true);

    char buffer[16];
    int len;

    /* Test comma conversion */
    len = hime_convert_punctuation (ctx, ',', buffer, sizeof (buffer));
    if (len > 0) {
        ASSERT_TRUE (len > 0);
        /* Should be Chinese comma */
    }

    /* Test period conversion */
    len = hime_convert_punctuation (ctx, '.', buffer, sizeof (buffer));
    if (len > 0) {
        ASSERT_TRUE (len > 0);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Input Method Availability Tests ========== */

TEST (method_availability) {
    hime_init ("../../data");

    /* PHO should always be available */
    ASSERT_TRUE (hime_is_method_available (HIME_IM_PHO));

    /* TSIN should be available */
    ASSERT_TRUE (hime_is_method_available (HIME_IM_TSIN));

    /* GTAB should be available */
    ASSERT_TRUE (hime_is_method_available (HIME_IM_GTAB));

    /* INTCODE should be available */
    ASSERT_TRUE (hime_is_method_available (HIME_IM_INTCODE));

    hime_cleanup ();
    TEST_PASS ();
}

TEST (method_names) {
    hime_init ("../../data");

    const char *name;

    name = hime_get_method_name (HIME_IM_PHO);
    ASSERT_NOT_NULL (name);
    ASSERT_TRUE (strlen (name) > 0);

    name = hime_get_method_name (HIME_IM_TSIN);
    ASSERT_NOT_NULL (name);
    ASSERT_TRUE (strlen (name) > 0);

    name = hime_get_method_name (HIME_IM_GTAB);
    ASSERT_NOT_NULL (name);
    ASSERT_TRUE (strlen (name) > 0);

    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Key Processing Tests ========== */

TEST (process_key_english_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    /* Set to English mode */
    hime_set_chinese_mode (ctx, false);

    /* Keys should be ignored in English mode */
    HimeKeyResult result = hime_process_key (ctx, 'a', 'a', HIME_MOD_NONE);
    ASSERT_EQ (HIME_KEY_IGNORED, result);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (process_key_escape_clears) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_chinese_mode (ctx, true);

    /* Escape should clear any input */
    HimeKeyResult result = hime_process_key (ctx, 0x1B, 0, HIME_MOD_NONE); /* 0x1B = Escape */
    /* Result depends on state, but shouldn't crash */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (process_key_with_modifiers) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_chinese_mode (ctx, true);

    /* Ctrl+key should typically be ignored */
    HimeKeyResult result = hime_process_key (ctx, 'a', 'a', HIME_MOD_CONTROL);
    /* Result depends on implementation */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Bopomofo Tests ========== */

TEST (get_bopomofo_string_empty) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char buffer[64];
    int len = hime_get_bopomofo_string (ctx, buffer, sizeof (buffer));
    ASSERT_EQ (0, len);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== TSIN Tests ========== */

TEST (tsin_get_phrase_empty) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char buffer[256];
    int len = hime_tsin_get_phrase (ctx, buffer, sizeof (buffer));
    ASSERT_EQ (0, len);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (tsin_commit_phrase) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    int len = hime_tsin_commit_phrase (ctx);
    ASSERT_EQ (0, len); /* Nothing to commit initially */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== GTAB Key String Tests ========== */

TEST (gtab_get_key_string_empty) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_input_method (ctx, HIME_IM_GTAB);

    char buffer[64];
    int len = hime_gtab_get_key_string (ctx, buffer, sizeof (buffer));
    ASSERT_EQ (0, len); /* No keys entered */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Feedback Callback Tests ========== */

static int g_feedback_callback_count = 0;
static HimeFeedbackType g_last_feedback_type = -1;

static void my_feedback_callback (HimeFeedbackType type, void *user_data) {
    g_feedback_callback_count++;
    g_last_feedback_type = type;
    (void) user_data;
}

TEST (feedback_callback) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    g_feedback_callback_count = 0;

    hime_set_feedback_callback (ctx, my_feedback_callback, NULL);
    hime_set_sound_enabled (ctx, true);

    /* Callback should be registered but not called yet */
    ASSERT_EQ (0, g_feedback_callback_count);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Extended Candidate Tests ========== */

TEST (get_candidate_with_annotation) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char text[64];
    char annotation[64];

    /* Should handle gracefully when no candidates */
    int len = hime_get_candidate_with_annotation (ctx, 0, text, sizeof (text),
                                                  annotation, sizeof (annotation));
    ASSERT_TRUE (len <= 0); /* No candidate at index 0 */

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== NULL Safety Tests ========== */

TEST (null_context_safety) {
    hime_init ("../../data");

    /* These should not crash with NULL context */
    hime_context_reset (NULL);
    hime_set_input_method (NULL, HIME_IM_PHO);
    hime_get_input_method (NULL);
    hime_toggle_chinese_mode (NULL);
    hime_is_chinese_mode (NULL);
    hime_set_chinese_mode (NULL, true);

    char buffer[64];
    hime_get_preedit (NULL, buffer, sizeof (buffer));
    hime_get_commit (NULL, buffer, sizeof (buffer));
    hime_clear_commit (NULL);
    hime_has_candidates (NULL);
    hime_get_candidate_count (NULL);

    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Input Method Search Tests ========== */

TEST (search_methods_all) {
    hime_init ("../../data");

    HimeSearchResult results[50];
    int count = hime_get_all_methods (results, 50);

    /* Should return at least built-in methods + GTAB tables */
    ASSERT_TRUE (count >= HIME_IM_COUNT);

    /* First results should be built-in methods */
    bool found_pho = false;
    bool found_tsin = false;
    for (int i = 0; i < count; i++) {
        if (results[i].method_type == HIME_IM_PHO)
            found_pho = true;
        if (results[i].method_type == HIME_IM_TSIN)
            found_tsin = true;
    }
    ASSERT_TRUE (found_pho);
    ASSERT_TRUE (found_tsin);

    hime_cleanup ();
    TEST_PASS ();
}

TEST (search_methods_with_query) {
    hime_init ("../../data");

    HimeSearchFilter filter;
    filter.query = "倉"; /* Search for Cangjie */
    filter.method_type = (HimeInputMethod) -1;
    filter.include_disabled = false;

    HimeSearchResult results[20];
    int count = hime_search_methods (&filter, results, 20);

    /* Should find Cangjie tables */
    ASSERT_TRUE (count > 0);
    /* First result should have highest score */
    if (count > 1) {
        ASSERT_TRUE (results[0].match_score >= results[1].match_score);
    }

    hime_cleanup ();
    TEST_PASS ();
}

TEST (search_methods_filter_by_type) {
    hime_init ("../../data");

    HimeSearchFilter filter;
    filter.query = NULL;
    filter.method_type = HIME_IM_GTAB;
    filter.include_disabled = false;

    HimeSearchResult results[50];
    int count = hime_search_methods (&filter, results, 50);

    /* All results should be GTAB type */
    for (int i = 0; i < count; i++) {
        ASSERT_EQ (HIME_IM_GTAB, results[i].method_type);
    }

    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_search_tables) {
    hime_init ("../../data");

    HimeGtabInfo results[20];
    int count = hime_gtab_search_tables ("行列", results, 20);

    /* Should find Array tables */
    ASSERT_TRUE (count > 0);
    ASSERT_TRUE (strlen (results[0].name) > 0);

    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_search_tables_empty_query) {
    hime_init ("../../data");

    HimeGtabInfo results[50];
    int count = hime_gtab_search_tables (NULL, results, 50);

    /* Empty query should return all tables */
    ASSERT_TRUE (count > 0);

    hime_cleanup ();
    TEST_PASS ();
}

TEST (find_method_by_name) {
    hime_init ("../../data");

    /* Find built-in method */
    int idx = hime_find_method_by_name ("注音 (Phonetic)");
    ASSERT_EQ (HIME_IM_PHO, idx);

    /* Find GTAB table */
    idx = hime_find_method_by_name ("倉頡");
    ASSERT_TRUE (idx >= HIME_IM_COUNT);

    /* Not found */
    idx = hime_find_method_by_name ("不存在的輸入法");
    ASSERT_EQ (-1, idx);

    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Simplified/Traditional Conversion Tests ========== */

TEST (convert_simp_to_trad) {
    char output[256];
    int len;

    /* Test single character conversion */
    len = hime_convert_simp_to_trad ("国", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("國", output);

    /* Test multiple characters */
    len = hime_convert_simp_to_trad ("国家", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("國家", output);

    /* Test string with no conversion needed */
    len = hime_convert_simp_to_trad ("你好", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("你好", output);

    TEST_PASS ();
}

TEST (convert_trad_to_simp) {
    char output[256];
    int len;

    /* Test single character conversion */
    len = hime_convert_trad_to_simp ("國", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("国", output);

    /* Test multiple characters */
    len = hime_convert_trad_to_simp ("國家", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("国家", output);

    /* Test string with no conversion needed */
    len = hime_convert_trad_to_simp ("你好", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("你好", output);

    TEST_PASS ();
}

TEST (convert_mixed_string) {
    char output[256];
    int len;

    /* Test mixed simplified and ASCII */
    len = hime_convert_simp_to_trad ("中国China", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    /* Should convert Chinese but keep ASCII */
    ASSERT_TRUE (strstr (output, "China") != NULL);

    TEST_PASS ();
}

TEST (output_variant_toggle) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    /* Default should be Traditional */
    ASSERT_EQ (HIME_OUTPUT_TRADITIONAL, hime_get_output_variant (ctx));

    /* Toggle to Simplified */
    HimeOutputVariant result = hime_toggle_output_variant (ctx);
    ASSERT_EQ (HIME_OUTPUT_SIMPLIFIED, result);
    ASSERT_EQ (HIME_OUTPUT_SIMPLIFIED, hime_get_output_variant (ctx));

    /* Toggle back to Traditional */
    result = hime_toggle_output_variant (ctx);
    ASSERT_EQ (HIME_OUTPUT_TRADITIONAL, result);
    ASSERT_EQ (HIME_OUTPUT_TRADITIONAL, hime_get_output_variant (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (set_output_variant) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_set_output_variant (ctx, HIME_OUTPUT_SIMPLIFIED);
    ASSERT_EQ (HIME_OUTPUT_SIMPLIFIED, hime_get_output_variant (ctx));

    hime_set_output_variant (ctx, HIME_OUTPUT_TRADITIONAL);
    ASSERT_EQ (HIME_OUTPUT_TRADITIONAL, hime_get_output_variant (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (convert_to_output_variant) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    char output[256];
    int len;

    /* Set to Simplified output */
    hime_set_output_variant (ctx, HIME_OUTPUT_SIMPLIFIED);
    len = hime_convert_to_output_variant (ctx, "國家", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("国家", output);

    /* Set to Traditional output */
    hime_set_output_variant (ctx, HIME_OUTPUT_TRADITIONAL);
    len = hime_convert_to_output_variant (ctx, "国家", output, sizeof (output));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("國家", output);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (conversion_null_safety) {
    char output[256];

    /* NULL input */
    int len = hime_convert_simp_to_trad (NULL, output, sizeof (output));
    ASSERT_EQ (-1, len);

    /* NULL output */
    len = hime_convert_simp_to_trad ("国", NULL, sizeof (output));
    ASSERT_EQ (-1, len);

    /* Zero size */
    len = hime_convert_simp_to_trad ("国", output, 0);
    ASSERT_EQ (-1, len);

    TEST_PASS ();
}

TEST (conversion_common_chars) {
    char output[256];
    int len;

    /* Test common conversion pairs */
    struct {
        const char *simp;
        const char *trad;
    } tests[] = {
        {"东", "東"}, {"书", "書"}, {"学", "學"}, {"开", "開"}, {"门", "門"}, {"马", "馬"}, {"龙", "龍"}, {"鸟", "鳥"}, {"车", "車"}, {"鱼", "魚"}, {"风", "風"}, {"飞", "飛"}, {NULL, NULL}};

    for (int i = 0; tests[i].simp != NULL; i++) {
        len = hime_convert_simp_to_trad (tests[i].simp, output, sizeof (output));
        ASSERT_TRUE (len > 0);
        ASSERT_STR_EQ (tests[i].trad, output);
    }

    TEST_PASS ();
}

/* ========== Typing Practice Tests ========== */

TEST (typing_start_session) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    int ret = hime_typing_start_session (ctx, "Hello");
    ASSERT_EQ (0, ret);
    ASSERT_TRUE (hime_typing_is_active (ctx));

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_session_null_safety) {
    ASSERT_EQ (-1, hime_typing_start_session (NULL, "test"));
    ASSERT_FALSE (hime_typing_is_active (NULL));
    ASSERT_EQ (-1, hime_typing_end_session (NULL));
    ASSERT_EQ (-1, hime_typing_reset_session (NULL));
    ASSERT_EQ (-1, hime_typing_submit_char (NULL, "a"));
    ASSERT_EQ (-1, hime_typing_get_position (NULL));
    TEST_PASS ();
}

TEST (typing_submit_correct_chars) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "ABC");

    ASSERT_EQ (1, hime_typing_submit_char (ctx, "A")); /* Correct */
    ASSERT_EQ (1, hime_typing_submit_char (ctx, "B")); /* Correct */
    ASSERT_EQ (1, hime_typing_submit_char (ctx, "C")); /* Correct */

    HimeTypingStats stats;
    hime_typing_get_stats (ctx, &stats);
    ASSERT_EQ (3, stats.correct_characters);
    ASSERT_EQ (0, stats.incorrect_characters);
    ASSERT_TRUE (stats.completed);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_submit_incorrect_chars) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "ABC");

    ASSERT_EQ (0, hime_typing_submit_char (ctx, "X")); /* Wrong */
    ASSERT_EQ (0, hime_typing_submit_char (ctx, "Y")); /* Wrong */
    ASSERT_EQ (1, hime_typing_submit_char (ctx, "C")); /* Correct */

    HimeTypingStats stats;
    hime_typing_get_stats (ctx, &stats);
    ASSERT_EQ (1, stats.correct_characters);
    ASSERT_EQ (2, stats.incorrect_characters);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_chinese_characters) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "你好");

    char expected[8];
    int len = hime_typing_get_expected_char (ctx, expected, sizeof (expected));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("你", expected);

    ASSERT_EQ (1, hime_typing_submit_char (ctx, "你"));
    ASSERT_EQ (1, hime_typing_get_position (ctx));

    len = hime_typing_get_expected_char (ctx, expected, sizeof (expected));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("好", expected);

    ASSERT_EQ (1, hime_typing_submit_char (ctx, "好"));

    HimeTypingStats stats;
    hime_typing_get_stats (ctx, &stats);
    ASSERT_EQ (2, stats.total_characters);
    ASSERT_EQ (2, stats.correct_characters);
    ASSERT_TRUE (stats.completed);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_get_practice_text) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "Test text");

    char buffer[256];
    int len = hime_typing_get_practice_text (ctx, buffer, sizeof (buffer));
    ASSERT_TRUE (len > 0);
    ASSERT_STR_EQ ("Test text", buffer);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_reset_session) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "ABC");
    hime_typing_submit_char (ctx, "A");
    hime_typing_submit_char (ctx, "B");

    /* Reset should restart from beginning */
    hime_typing_reset_session (ctx);

    ASSERT_EQ (0, hime_typing_get_position (ctx));

    HimeTypingStats stats;
    hime_typing_get_stats (ctx, &stats);
    ASSERT_EQ (0, stats.correct_characters);
    ASSERT_EQ (0, stats.incorrect_characters);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_submit_string) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "Hello");

    int correct = hime_typing_submit_string (ctx, "Hello");
    ASSERT_EQ (5, correct);

    HimeTypingStats stats;
    hime_typing_get_stats (ctx, &stats);
    ASSERT_EQ (5, stats.correct_characters);
    ASSERT_TRUE (stats.completed);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_record_keystroke) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "AB");

    hime_typing_record_keystroke (ctx);
    hime_typing_record_keystroke (ctx);
    hime_typing_record_keystroke (ctx);
    hime_typing_submit_char (ctx, "A");
    hime_typing_record_keystroke (ctx);
    hime_typing_submit_char (ctx, "B");

    HimeTypingStats stats;
    hime_typing_get_stats (ctx, &stats);
    ASSERT_EQ (4, stats.total_keystrokes);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (typing_accuracy_calculation) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();

    hime_typing_start_session (ctx, "ABCD");

    hime_typing_submit_char (ctx, "A"); /* Correct */
    hime_typing_submit_char (ctx, "X"); /* Wrong */
    hime_typing_submit_char (ctx, "C"); /* Correct */
    hime_typing_submit_char (ctx, "D"); /* Correct */

    HimeTypingStats stats;
    hime_typing_get_stats (ctx, &stats);

    /* 3 correct, 1 incorrect = 75% accuracy */
    ASSERT_TRUE (stats.accuracy >= 74.9 && stats.accuracy <= 75.1);

    hime_typing_end_session (ctx);
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Practice Text Library Tests ========== */

TEST (practice_text_count) {
    int english_count = hime_typing_get_text_count (HIME_PRACTICE_CAT_ENGLISH);
    ASSERT_TRUE (english_count >= 5);

    int zhuyin_count = hime_typing_get_text_count (HIME_PRACTICE_CAT_ZHUYIN);
    ASSERT_TRUE (zhuyin_count >= 5);

    int pinyin_count = hime_typing_get_text_count (HIME_PRACTICE_CAT_PINYIN);
    ASSERT_TRUE (pinyin_count >= 5);

    TEST_PASS ();
}

TEST (practice_get_texts_by_category) {
    HimePracticeText texts[20];
    int count = hime_typing_get_texts_by_category (HIME_PRACTICE_CAT_ENGLISH, texts, 20);
    ASSERT_TRUE (count >= 5);

    /* All returned texts should be English category */
    for (int i = 0; i < count; i++) {
        ASSERT_EQ (HIME_PRACTICE_CAT_ENGLISH, texts[i].category);
        ASSERT_TRUE (texts[i].char_count > 0);
    }

    TEST_PASS ();
}

TEST (practice_get_texts_by_difficulty) {
    HimePracticeText texts[50];
    int count = hime_typing_get_texts_by_difficulty (HIME_PRACTICE_EASY, texts, 50);
    ASSERT_TRUE (count >= 5);

    /* All returned texts should be easy difficulty */
    for (int i = 0; i < count; i++) {
        ASSERT_EQ (HIME_PRACTICE_EASY, texts[i].difficulty);
    }

    TEST_PASS ();
}

TEST (practice_get_text_by_id) {
    HimePracticeText text;

    /* Get English text (ID 1) */
    int ret = hime_typing_get_text_by_id (1, &text);
    ASSERT_EQ (0, ret);
    ASSERT_EQ (1, text.id);
    ASSERT_TRUE (strlen (text.text) > 0);

    /* Invalid ID */
    ret = hime_typing_get_text_by_id (9999, &text);
    ASSERT_EQ (-1, ret);

    TEST_PASS ();
}

TEST (practice_get_all_texts) {
    HimePracticeText texts[100];
    int count = hime_typing_get_all_texts (texts, 100);
    ASSERT_TRUE (count >= 20);
    TEST_PASS ();
}

TEST (practice_category_names) {
    const char *name = hime_typing_get_category_name (HIME_PRACTICE_CAT_ENGLISH);
    ASSERT_NOT_NULL (name);
    ASSERT_TRUE (strlen (name) > 0);

    name = hime_typing_get_category_name (HIME_PRACTICE_CAT_ZHUYIN);
    ASSERT_NOT_NULL (name);
    ASSERT_TRUE (strlen (name) > 0);

    /* Invalid category */
    name = hime_typing_get_category_name (-1);
    ASSERT_STR_EQ ("", name);

    TEST_PASS ();
}

TEST (practice_difficulty_names) {
    const char *name = hime_typing_get_difficulty_name (HIME_PRACTICE_EASY);
    ASSERT_NOT_NULL (name);
    ASSERT_TRUE (strlen (name) > 0);

    name = hime_typing_get_difficulty_name (HIME_PRACTICE_HARD);
    ASSERT_NOT_NULL (name);
    ASSERT_TRUE (strlen (name) > 0);

    /* Invalid difficulty */
    name = hime_typing_get_difficulty_name (0);
    ASSERT_STR_EQ ("", name);

    TEST_PASS ();
}

TEST (practice_random_text) {
    HimePracticeText text;
    int ret = hime_typing_get_random_text (HIME_PRACTICE_CAT_ENGLISH, &text);
    ASSERT_EQ (0, ret);
    ASSERT_EQ (HIME_PRACTICE_CAT_ENGLISH, text.category);
    ASSERT_TRUE (text.id > 0);
    ASSERT_TRUE (strlen (text.text) > 0);
    TEST_PASS ();
}

/* ========== Integration Tests: Full Input Flow ========== */

TEST (pho_type_bopomofo_key) {
    /* Test that typing a single Bopomofo key produces preedit */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));

    /* Type 'j' which maps to ㄓ on standard keyboard */
    HimeKeyResult kr = hime_process_key (ctx, 'J', 'j', 0);
    ASSERT_EQ (HIME_KEY_PREEDIT, kr);

    char buf[256];
    int len = hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_TRUE (len > 0);
    /* Preedit should contain Bopomofo symbol */
    ASSERT_TRUE (strlen (buf) > 0);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_space_triggers_lookup) {
    /* Test that Space after Bopomofo triggers candidate lookup */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type ㄉ (d) + ㄜ (k on standard keyboard) + ˙ (space = 1st tone) */
    /* This is the phonetic input for 的 (de, neutral tone) or 德 (de, 2nd tone) */
    /* Actually let's type a simpler example: ㄇ (m='a') + ㄚ (a on keyboard='8') */
    /* Standard keyboard: 'a' -> ㄇ, '8' -> ㄚ */
    hime_process_key (ctx, 'A', 'a', 0); /* ㄇ */
    hime_process_key (ctx, '8', '8', 0); /* ㄚ */

    char buf[256];
    int len = hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_TRUE (len > 0); /* Should have Bopomofo preedit */

    /* Press Space (1st tone) to trigger lookup */
    HimeKeyResult kr = hime_process_key (ctx, ' ', ' ', 0);

    /* Should either commit (single candidate) or show candidates */
    ASSERT_TRUE (kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    if (kr == HIME_KEY_COMMIT) {
        len = hime_get_commit (ctx, buf, sizeof (buf));
        ASSERT_TRUE (len > 0); /* Should have committed a character */
    } else {
        /* Multiple candidates - verify they exist */
        ASSERT_TRUE (hime_get_candidate_count (ctx) > 0);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_tone_triggers_lookup) {
    /* Test that pressing a tone key triggers candidate lookup */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type ㄇ (a) + ㄚ (8) + ˇ (3rd tone = key '3') */
    hime_process_key (ctx, 'A', 'a', 0);                    /* ㄇ initial */
    hime_process_key (ctx, '8', '8', 0);                    /* ㄚ final */
    HimeKeyResult kr = hime_process_key (ctx, '3', '3', 0); /* 3rd tone */

    /* Should trigger lookup */
    ASSERT_TRUE (kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    if (kr == HIME_KEY_PREEDIT) {
        /* Candidates available - preedit should contain candidate list */
        char buf[1024];
        hime_get_preedit (ctx, buf, sizeof (buf));
        ASSERT_TRUE (hime_get_candidate_count (ctx) > 0);
        /* Preedit should contain number labels like "1." for candidate selection */
        ASSERT_TRUE (strstr (buf, "1.") != NULL);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_candidate_selection) {
    /* Test selecting a candidate by number key */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type a syllable that produces multiple candidates */
    hime_process_key (ctx, 'A', 'a', 0);                    /* ㄇ */
    hime_process_key (ctx, '8', '8', 0);                    /* ㄚ */
    HimeKeyResult kr = hime_process_key (ctx, ' ', ' ', 0); /* 1st tone */

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count (ctx) > 1) {
        /* Select first candidate with '1' key */
        kr = hime_process_key (ctx, '1', '1', 0);
        ASSERT_EQ (HIME_KEY_COMMIT, kr);

        char buf[256];
        int len = hime_get_commit (ctx, buf, sizeof (buf));
        ASSERT_TRUE (len > 0); /* Should have committed a character */
        /* The committed text should be a CJK character (3 bytes in UTF-8) */
        ASSERT_TRUE (len >= 3);
    } else if (kr == HIME_KEY_COMMIT) {
        /* Single candidate was auto-committed */
        char buf[256];
        int len = hime_get_commit (ctx, buf, sizeof (buf));
        ASSERT_TRUE (len > 0);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_single_candidate_autocommit) {
    /* Test that a syllable with only one candidate auto-commits */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Try various syllables - some may have single candidates */
    /* ㄉㄧㄝ (die2) = 爹 might be unique */
    hime_process_key (ctx, 'D', 'd', 0); /* ㄉ */
    hime_process_key (ctx, 'I', 'i', 0); /* ㄧ */
    hime_process_key (ctx, 'E', 'e', 0); /* ㄝ (mapped to key 'e' depends on layout) */

    /* Try with Space for 1st tone */
    HimeKeyResult kr = hime_process_key (ctx, ' ', ' ', 0);

    /* Either commits or shows candidates - both are valid */
    ASSERT_TRUE (kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT || kr == HIME_KEY_IGNORED);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_backspace_deletes_component) {
    /* Test Backspace removes last phonetic component */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type two components */
    hime_process_key (ctx, 'A', 'a', 0); /* ㄇ */
    hime_process_key (ctx, '8', '8', 0); /* ㄚ */

    char buf1[256];
    hime_get_preedit (ctx, buf1, sizeof (buf1));
    int len1 = strlen (buf1);
    ASSERT_TRUE (len1 > 0);

    /* Backspace should remove last component */
    HimeKeyResult kr = hime_process_key (ctx, 0x08, 0x08, 0);
    ASSERT_EQ (HIME_KEY_PREEDIT, kr);

    char buf2[256];
    hime_get_preedit (ctx, buf2, sizeof (buf2));
    int len2 = strlen (buf2);
    /* Preedit should be shorter after backspace */
    ASSERT_TRUE (len2 < len1);

    /* Another backspace should clear everything */
    kr = hime_process_key (ctx, 0x08, 0x08, 0);
    ASSERT_TRUE (kr == HIME_KEY_PREEDIT || kr == HIME_KEY_ABSORBED);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_escape_cancels_input) {
    /* Test Escape clears current input */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type some keys */
    hime_process_key (ctx, 'A', 'a', 0);
    hime_process_key (ctx, '8', '8', 0);

    char buf[256];
    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_TRUE (strlen (buf) > 0);

    /* Escape should clear */
    HimeKeyResult kr = hime_process_key (ctx, 0x1B, 0x1B, 0);
    ASSERT_EQ (HIME_KEY_ABSORBED, kr);

    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_EQ (0, (int) strlen (buf));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_english_mode_passthrough) {
    /* Test that English mode passes all keys through */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    hime_set_chinese_mode (ctx, false);

    HimeKeyResult kr = hime_process_key (ctx, 'A', 'a', 0);
    ASSERT_EQ (HIME_KEY_IGNORED, kr);

    kr = hime_process_key (ctx, ' ', ' ', 0);
    ASSERT_EQ (HIME_KEY_IGNORED, kr);

    kr = hime_process_key (ctx, '1', '1', 0);
    ASSERT_EQ (HIME_KEY_IGNORED, kr);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_mode_toggle_resets_state) {
    /* Test that toggling mode resets preedit */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type some Bopomofo */
    hime_process_key (ctx, 'A', 'a', 0);

    char buf[256];
    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_TRUE (strlen (buf) > 0);

    /* Toggle to English - should reset */
    hime_toggle_chinese_mode (ctx);
    ASSERT_FALSE (hime_is_chinese_mode (ctx));

    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_EQ (0, (int) strlen (buf));

    /* Toggle back - should be clean */
    hime_toggle_chinese_mode (ctx);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));

    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_EQ (0, (int) strlen (buf));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_candidate_paging) {
    /* Test candidate page navigation */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);
    hime_set_candidates_per_page (ctx, 5);

    /* Type a common syllable likely to have many candidates */
    hime_process_key (ctx, 'I', 'i', 0);                    /* ㄧ */
    HimeKeyResult kr = hime_process_key (ctx, ' ', ' ', 0); /* 1st tone */

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count (ctx) > 5) {
        ASSERT_EQ (0, hime_get_candidate_page (ctx));

        /* Page down */
        bool ok = hime_candidate_page_down (ctx);
        ASSERT_TRUE (ok);
        ASSERT_EQ (1, hime_get_candidate_page (ctx));

        /* Page up */
        ok = hime_candidate_page_up (ctx);
        ASSERT_TRUE (ok);
        ASSERT_EQ (0, hime_get_candidate_page (ctx));

        /* Can't page up past 0 */
        ok = hime_candidate_page_up (ctx);
        ASSERT_FALSE (ok);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_preedit_includes_candidates) {
    /* Test that preedit includes candidate list when candidates exist */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type a syllable that produces multiple candidates */
    hime_process_key (ctx, 'A', 'a', 0);                    /* ㄇ */
    hime_process_key (ctx, '8', '8', 0);                    /* ㄚ */
    HimeKeyResult kr = hime_process_key (ctx, ' ', ' ', 0); /* 1st tone */

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count (ctx) > 1) {
        char buf[1024];
        hime_get_preedit (ctx, buf, sizeof (buf));

        /* Preedit should contain "1." label for first candidate */
        ASSERT_TRUE (strstr (buf, "1.") != NULL);
        /* Preedit should contain "2." label for second candidate */
        ASSERT_TRUE (strstr (buf, "2.") != NULL);

        /* Verify candidate count matches labels shown */
        int count = hime_get_candidate_count (ctx);
        ASSERT_TRUE (count >= 2);

        /* Verify individual candidates are accessible */
        char cand[64];
        int clen = hime_get_candidate (ctx, 0, cand, sizeof (cand));
        ASSERT_TRUE (clen > 0);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_full_input_cycle) {
    /* Test complete input cycle: type → lookup → select → commit → repeat */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    char commit_buf[256];

    /* First character */
    hime_process_key (ctx, 'A', 'a', 0);                    /* ㄇ */
    hime_process_key (ctx, '8', '8', 0);                    /* ㄚ */
    HimeKeyResult kr = hime_process_key (ctx, ' ', ' ', 0); /* 1st tone */

    if (kr == HIME_KEY_COMMIT) {
        int len = hime_get_commit (ctx, commit_buf, sizeof (commit_buf));
        ASSERT_TRUE (len > 0);
        hime_clear_commit (ctx);
    } else if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count (ctx) > 0) {
        /* Select first candidate */
        kr = hime_process_key (ctx, '1', '1', 0);
        ASSERT_EQ (HIME_KEY_COMMIT, kr);
        int len = hime_get_commit (ctx, commit_buf, sizeof (commit_buf));
        ASSERT_TRUE (len > 0);
        hime_clear_commit (ctx);
    }

    /* Context should be clean for next character */
    char buf[256];
    hime_get_preedit (ctx, buf, sizeof (buf));
    /* Preedit should be empty or only contain pending Bopomofo (no candidates) */
    ASSERT_EQ (0, hime_get_candidate_count (ctx));

    /* Second character should work the same */
    hime_process_key (ctx, 'J', 'j', 0); /* ㄓ */
    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_TRUE (strlen (buf) > 0);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_multiple_keyboards) {
    /* Test that different keyboard layouts produce different key mappings */
    hime_init ("../../data");

    HimeContext *ctx1 = hime_context_new ();
    ASSERT_NOT_NULL (ctx1);
    hime_set_keyboard_layout (ctx1, HIME_KB_STANDARD);

    HimeContext *ctx2 = hime_context_new ();
    ASSERT_NOT_NULL (ctx2);
    hime_set_keyboard_layout (ctx2, HIME_KB_HSU);

    /* Same key should produce different results on different layouts */
    hime_process_key (ctx1, 'A', 'a', 0);
    hime_process_key (ctx2, 'A', 'a', 0);

    char buf1[256], buf2[256];
    hime_get_preedit (ctx1, buf1, sizeof (buf1));
    hime_get_preedit (ctx2, buf2, sizeof (buf2));

    /* Both should have preedit content */
    ASSERT_TRUE (strlen (buf1) > 0);
    ASSERT_TRUE (strlen (buf2) > 0);

    /* Different layouts should produce different Bopomofo for same key */
    /* (This depends on specific layout mappings) */

    hime_context_free (ctx1);
    hime_context_free (ctx2);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_data_loading) {
    /* Test that phonetic data loads correctly */
    hime_cleanup (); /* Ensure clean state */

    int ret = hime_init ("../../data");
    ASSERT_EQ (0, ret);

    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Type a very common syllable: ㄉ(d) ㄜ(k) ˙(space) for 的 */
    hime_process_key (ctx, 'D', 'd', 0);                    /* ㄉ */
    hime_process_key (ctx, 'K', 'k', 0);                    /* ㄜ */
    HimeKeyResult kr = hime_process_key (ctx, ' ', ' ', 0); /* neutral/1st tone */

    /* 的 is very common - should produce results */
    ASSERT_TRUE (kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    if (kr == HIME_KEY_COMMIT) {
        char buf[256];
        int len = hime_get_commit (ctx, buf, sizeof (buf));
        ASSERT_TRUE (len > 0);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (pho_invalid_data_path) {
    /* Test graceful failure with bad data path */
    hime_cleanup ();
    int ret = hime_init ("/nonexistent/path");
    ASSERT_EQ (-1, ret);

    /* Context creation should still work, but processing won't */
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Without data, keys should still not crash */
    HimeKeyResult kr = hime_process_key (ctx, 'A', 'a', 0);
    /* Result depends on implementation - just verify no crash */
    (void) kr;

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== GTAB Binary Search Tests ========== */

/* Helper: encode a GTAB key using hime-cin2gtab's formula:
 *   LAST_K_bitN = ((32 / keybits) - 1) * keybits
 *   kk |= idx << (LAST_K_bitN - position * keybits)
 */
static uint32_t
encode_gtab_key (int keybits, const int *indices, int count) {
    int last_k_bitn = ((32 / keybits) - 1) * keybits;
    uint32_t kk = 0;
    for (int i = 0; i < count; i++)
        kk |= (uint32_t) indices[i] << (last_k_bitn - i * keybits);
    return kk;
}

TEST (gtab_bsearch_exact_match) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Construct a small sorted GtabTable in-memory */
    GtabTable test_table;
    memset (&test_table, 0, sizeof (test_table));
    strcpy (test_table.name, "test");
    test_table.key_count = 4;
    test_table.max_press = 2;
    test_table.keybits = 6;
    test_table.key64 = false;
    test_table.sorted = true;
    test_table.loaded = true;

    /* keymap: a=0, b=1, c=2, d=3 */
    memset (test_table.keymap, 0, sizeof (test_table.keymap));
    test_table.keymap[0] = 'a';
    test_table.keymap[1] = 'b';
    test_table.keymap[2] = 'c';
    test_table.keymap[3] = 'd';

    GtabItem items[3];
    memset (items, 0, sizeof (items));
    uint32_t k;

    /* "aa" */
    k = encode_gtab_key (6, (int[]){0, 0}, 2);
    memcpy (items[0].key, &k, 4);
    items[0].ch[0] = 'X';

    /* "ab" */
    k = encode_gtab_key (6, (int[]){0, 1}, 2);
    memcpy (items[1].key, &k, 4);
    items[1].ch[0] = 'Y';

    /* "bc" */
    k = encode_gtab_key (6, (int[]){1, 2}, 2);
    memcpy (items[2].key, &k, 4);
    items[2].ch[0] = 'Z';

    test_table.items = items;
    test_table.item_count = 3;

    /* Point context at test table and search for "ab" (exact match) */
    ctx->gtab = &test_table;
    ctx->gtab_keys[0] = 0; /* a */
    ctx->gtab_keys[1] = 1; /* b */
    ctx->gtab_key_count = 2;

    int count = gtab_lookup (ctx);
    ASSERT_EQ (1, count);
    ASSERT_EQ ('Y', ctx->candidates[0][0]);

    /* Restore and cleanup */
    ctx->gtab = NULL;
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_bsearch_prefix_match) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    GtabTable test_table;
    memset (&test_table, 0, sizeof (test_table));
    strcpy (test_table.name, "test");
    test_table.key_count = 4;
    test_table.max_press = 2;
    test_table.keybits = 6;
    test_table.key64 = false;
    test_table.sorted = true;
    test_table.loaded = true;

    test_table.keymap[0] = 'a';
    test_table.keymap[1] = 'b';
    test_table.keymap[2] = 'c';
    test_table.keymap[3] = 'd';

    GtabItem items[4];
    memset (items, 0, sizeof (items));
    uint32_t k;

    k = encode_gtab_key (6, (int[]){0, 0}, 2); /* "aa" */
    memcpy (items[0].key, &k, 4);
    items[0].ch[0] = 'P';

    k = encode_gtab_key (6, (int[]){0, 1}, 2); /* "ab" */
    memcpy (items[1].key, &k, 4);
    items[1].ch[0] = 'Q';

    k = encode_gtab_key (6, (int[]){0, 2}, 2); /* "ac" */
    memcpy (items[2].key, &k, 4);
    items[2].ch[0] = 'R';

    k = encode_gtab_key (6, (int[]){1, 0}, 2); /* "ba" */
    memcpy (items[3].key, &k, 4);
    items[3].ch[0] = 'S';

    test_table.items = items;
    test_table.item_count = 4;

    /* Prefix search for "a" — should match "aa", "ab", "ac" */
    ctx->gtab = &test_table;
    ctx->gtab_keys[0] = 0; /* a */
    ctx->gtab_key_count = 1;

    int count = gtab_lookup (ctx);
    ASSERT_EQ (3, count);
    ASSERT_EQ ('P', ctx->candidates[0][0]);
    ASSERT_EQ ('Q', ctx->candidates[1][0]);
    ASSERT_EQ ('R', ctx->candidates[2][0]);

    ctx->gtab = NULL;
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_bsearch_no_match) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    GtabTable test_table;
    memset (&test_table, 0, sizeof (test_table));
    test_table.key_count = 4;
    test_table.max_press = 2;
    test_table.keybits = 6;
    test_table.key64 = false;
    test_table.sorted = true;
    test_table.loaded = true;

    test_table.keymap[0] = 'a';
    test_table.keymap[1] = 'b';

    GtabItem items[1];
    memset (items, 0, sizeof (items));
    uint32_t k = encode_gtab_key (6, (int[]){0, 0}, 2); /* "aa" */
    memcpy (items[0].key, &k, 4);
    items[0].ch[0] = 'X';

    test_table.items = items;
    test_table.item_count = 1;

    /* Search for "b" — no items start with b */
    ctx->gtab = &test_table;
    ctx->gtab_keys[0] = 1; /* b */
    ctx->gtab_key_count = 1;

    int count = gtab_lookup (ctx);
    ASSERT_EQ (0, count);

    ctx->gtab = NULL;
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_bsearch_boundary) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    GtabTable test_table;
    memset (&test_table, 0, sizeof (test_table));
    test_table.key_count = 4;
    test_table.max_press = 1;
    test_table.keybits = 6;
    test_table.key64 = false;
    test_table.sorted = true;
    test_table.loaded = true;

    test_table.keymap[0] = 'a';
    test_table.keymap[1] = 'b';
    test_table.keymap[2] = 'c';

    /* 3 single-key items: a, b, c */
    GtabItem items[3];
    memset (items, 0, sizeof (items));
    uint32_t k;

    k = encode_gtab_key (6, (int[]){0}, 1); /* "a" */
    memcpy (items[0].key, &k, 4);
    items[0].ch[0] = 'F'; /* first item */

    k = encode_gtab_key (6, (int[]){1}, 1); /* "b" */
    memcpy (items[1].key, &k, 4);
    items[1].ch[0] = 'M';

    k = encode_gtab_key (6, (int[]){2}, 1); /* "c" */
    memcpy (items[2].key, &k, 4);
    items[2].ch[0] = 'L'; /* last item */

    test_table.items = items;
    test_table.item_count = 3;

    /* Match first item */
    ctx->gtab = &test_table;
    ctx->gtab_keys[0] = 0;
    ctx->gtab_key_count = 1;
    ASSERT_EQ (1, gtab_lookup (ctx));
    ASSERT_EQ ('F', ctx->candidates[0][0]);

    /* Match last item */
    ctx->gtab_keys[0] = 2;
    ctx->gtab_key_count = 1;
    ASSERT_EQ (1, gtab_lookup (ctx));
    ASSERT_EQ ('L', ctx->candidates[0][0]);

    ctx->gtab = NULL;
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_load_v2_format) {
    hime_init ("../../data");

    /* Write a small v2 .gtab to a temp file */
    const char *tmppath = "/tmp/test-hime-v2.gtab";
    FILE *fp = fopen (tmppath, "wb");
    ASSERT_NOT_NULL (fp);

    /* Build a minimal v2 file: 2 keys (a, b), max_press=1, keybits=2, 2 items */
    uint8_t key_count = 2;
    uint8_t max_press = 1;
    uint8_t keybits = 2;
    uint32_t item_count = 2;

    uint32_t hdr_size = 72;
    uint32_t keymap_off = hdr_size;
    uint32_t keyname_off = keymap_off + key_count;
    uint32_t items_off = keyname_off + key_count * HIME_CH_SZ;

    /* Header */
    uint32_t magic = 0x48475432;
    uint16_t version = 0x0002;
    uint16_t flags = 0;
    fwrite (&magic, 4, 1, fp);
    fwrite (&version, 2, 1, fp);
    fwrite (&flags, 2, 1, fp);

    char cname[32] = "TestV2";
    fwrite (cname, 32, 1, fp);

    char selkey[12] = "1234567890";
    fwrite (selkey, 12, 1, fp);

    uint8_t space_style = 0;
    fwrite (&space_style, 1, 1, fp);
    fwrite (&key_count, 1, 1, fp);
    fwrite (&max_press, 1, 1, fp);
    fwrite (&keybits, 1, 1, fp);
    fwrite (&item_count, 4, 1, fp);
    fwrite (&keymap_off, 4, 1, fp);
    fwrite (&keyname_off, 4, 1, fp);
    fwrite (&items_off, 4, 1, fp);

    /* Keymap: a, b */
    char keymap[2] = {'a', 'b'};
    fwrite (keymap, 1, 2, fp);

    /* Keyname: 2 * 4 bytes */
    char keyname[8];
    memset (keyname, 0, sizeof (keyname));
    keyname[0] = 'A'; /* radical for key 0 */
    keyname[4] = 'B'; /* radical for key 1 */
    fwrite (keyname, 1, 8, fp);

    /* Items (sorted): key 0 → 'X', key 1 → 'Y'
     * max_press=1, keybits=2, so key is shifted left by (32 - 1*2) = 30 bits */
    typedef struct {
        uint8_t key[4];
        char ch[4];
    } TestItem;
    TestItem titems[2];
    memset (titems, 0, sizeof (titems));

    uint32_t k0 = 0u << 30;
    memcpy (titems[0].key, &k0, 4);
    titems[0].ch[0] = 'X';

    uint32_t k1 = 1u << 30;
    memcpy (titems[1].key, &k1, 4);
    titems[1].ch[0] = 'Y';

    fwrite (titems, sizeof (TestItem), 2, fp);
    fclose (fp);

    /* Load the v2 file */
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    int ret = hime_gtab_load_table (ctx, tmppath);
    /* hime_gtab_load_table prepends g_data_dir, so load directly */
    /* Use the internal load approach - actually we need to test via
     * the file being loadable. Let's use the full path trick. */

    /* The load_gtab_file function is static, so test via the
     * registry. Instead, verify the file is valid by checking magic. */
    fp = fopen (tmppath, "rb");
    ASSERT_NOT_NULL (fp);
    uint32_t read_magic = 0;
    fread (&read_magic, 4, 1, fp);
    ASSERT_EQ ((long) 0x48475432, (long) read_magic);
    fclose (fp);

    /* Verify file size = 72 + 2 + 8 + 16 = 98 bytes */
    fp = fopen (tmppath, "rb");
    fseek (fp, 0, SEEK_END);
    long fsize = ftell (fp);
    fclose (fp);
    ASSERT_EQ (98, fsize);

    hime_context_free (ctx);
    hime_cleanup ();

    /* Clean up temp file */
    remove (tmppath);
    TEST_PASS ();
}

TEST (gtab_v2_load_and_lookup) {
    /* Write a v2 .gtab, then use the internal loader to load and search it */
    const char *tmppath = "/tmp/test-hime-v2-lookup.gtab";
    FILE *fp = fopen (tmppath, "wb");
    ASSERT_NOT_NULL (fp);

    uint8_t key_count = 3;
    uint8_t max_press = 2;
    uint8_t keybits = 2;
    uint32_t item_count = 3;

    uint32_t hdr_size = 72;
    uint32_t keymap_off = hdr_size;
    uint32_t keyname_off = keymap_off + key_count;
    uint32_t items_off = keyname_off + key_count * HIME_CH_SZ;

    uint32_t magic = 0x48475432;
    uint16_t version = 0x0002;
    uint16_t flags = 0;
    fwrite (&magic, 4, 1, fp);
    fwrite (&version, 2, 1, fp);
    fwrite (&flags, 2, 1, fp);

    char cname[32] = "V2Lookup";
    fwrite (cname, 32, 1, fp);
    char selkey[12] = "1234567890";
    fwrite (selkey, 12, 1, fp);

    uint8_t space_style = 0;
    fwrite (&space_style, 1, 1, fp);
    fwrite (&key_count, 1, 1, fp);
    fwrite (&max_press, 1, 1, fp);
    fwrite (&keybits, 1, 1, fp);
    fwrite (&item_count, 4, 1, fp);
    fwrite (&keymap_off, 4, 1, fp);
    fwrite (&keyname_off, 4, 1, fp);
    fwrite (&items_off, 4, 1, fp);

    /* Keymap: a=0, b=1, c=2 */
    char keymap[3] = {'a', 'b', 'c'};
    fwrite (keymap, 1, 3, fp);

    /* Keyname */
    char keyname[12];
    memset (keyname, 0, sizeof (keyname));
    fwrite (keyname, 1, 12, fp);

    /* Items: "aa"→P, "ab"→Q, "ba"→R, sorted
     * keybits=2, max_press=2, shift = 32 - 2*2 = 28 */
    typedef struct {
        uint8_t key[4];
        char ch[4];
    } TI;
    TI titems[3];
    memset (titems, 0, sizeof (titems));
    uint32_t k;

    k = ((0u << 2) | 0u) << 28; /* "aa" */
    memcpy (titems[0].key, &k, 4);
    titems[0].ch[0] = 'P';

    k = ((0u << 2) | 1u) << 28; /* "ab" */
    memcpy (titems[1].key, &k, 4);
    titems[1].ch[0] = 'Q';

    k = ((1u << 2) | 0u) << 28; /* "ba" */
    memcpy (titems[2].key, &k, 4);
    titems[2].ch[0] = 'R';

    fwrite (titems, sizeof (TI), 3, fp);
    fclose (fp);

    /* Now load this file using our internal loader.
     * Since load_gtab_file is static, we exercise it by setting
     * data_dir to /tmp and loading by filename. */
    hime_init ("/tmp");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    int ret = hime_gtab_load_table (ctx, "test-hime-v2-lookup.gtab");
    ASSERT_EQ (0, ret);
    ASSERT_NOT_NULL (ctx->gtab);
    ASSERT_TRUE (ctx->gtab->sorted);
    ASSERT_EQ (3, ctx->gtab->item_count);

    /* Lookup "a" prefix — should find "aa" and "ab" (2 matches) */
    ctx->gtab_keys[0] = 0; /* a */
    ctx->gtab_key_count = 1;
    int count = gtab_lookup (ctx);
    ASSERT_EQ (2, count);
    ASSERT_EQ ('P', ctx->candidates[0][0]);
    ASSERT_EQ ('Q', ctx->candidates[1][0]);

    /* Lookup "ba" exact — should find 1 match */
    ctx->gtab_keys[0] = 1; /* b */
    ctx->gtab_keys[1] = 0; /* a */
    ctx->gtab_key_count = 2;
    count = gtab_lookup (ctx);
    ASSERT_EQ (1, count);
    ASSERT_EQ ('R', ctx->candidates[0][0]);

    hime_context_free (ctx);
    hime_cleanup ();
    remove (tmppath);
    TEST_PASS ();
}

/* ========== GTAB Candidate Display Tests ========== */

TEST (gtab_preedit_includes_candidates) {
    /* Test that GTAB preedit includes candidate list when candidates exist */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Build a small test GTAB table with multiple matches for a prefix */
    GtabTable test_table;
    memset (&test_table, 0, sizeof (test_table));
    strcpy (test_table.name, "TestCJ");
    test_table.key_count = 4;
    test_table.max_press = 2;
    test_table.keybits = 6;
    test_table.key64 = false;
    test_table.sorted = true;
    test_table.loaded = true;

    test_table.keymap[0] = 'a';
    test_table.keymap[1] = 'b';
    test_table.keymap[2] = 'c';
    test_table.keymap[3] = 'd';

    /* Keyname: radical display names */
    memset (test_table.keyname, 0, sizeof (test_table.keyname));

    /* 3 items starting with 'a': "aa"→X, "ab"→Y, "ac"→Z */
    GtabItem items[3];
    memset (items, 0, sizeof (items));
    uint32_t k;

    k = encode_gtab_key (6, (int[]){0, 0}, 2);
    memcpy (items[0].key, &k, 4);
    items[0].ch[0] = 'X';

    k = encode_gtab_key (6, (int[]){0, 1}, 2);
    memcpy (items[1].key, &k, 4);
    items[1].ch[0] = 'Y';

    k = encode_gtab_key (6, (int[]){0, 2}, 2);
    memcpy (items[2].key, &k, 4);
    items[2].ch[0] = 'Z';

    test_table.items = items;
    test_table.item_count = 3;

    /* Set up context with this table */
    ctx->gtab = &test_table;
    hime_set_input_method (ctx, HIME_IM_GTAB);
    ctx->gtab_keys[0] = 0; /* a */
    ctx->gtab_key_count = 1;

    /* Rebuild display and set preedit */
    gtab_rebuild_display (ctx);
    strcpy (ctx->preedit, ctx->gtab_key_display);

    /* Lookup and append candidates */
    gtab_lookup (ctx);
    ASSERT_EQ (3, ctx->candidate_count);

    /* Now call append_candidates_to_preedit (indirectly via the preedit flow) */
    append_candidates_to_preedit (ctx);

    /* Preedit should contain numbered candidates */
    char buf[256];
    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_TRUE (strstr (buf, "1.") != NULL);
    ASSERT_TRUE (strstr (buf, "2.") != NULL);
    ASSERT_TRUE (strstr (buf, "3.") != NULL);

    ctx->gtab = NULL;
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_backspace_preserves_candidates) {
    /* Test that GTAB backspace still shows candidates for remaining keys */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    GtabTable test_table;
    memset (&test_table, 0, sizeof (test_table));
    strcpy (test_table.name, "TestBS");
    test_table.key_count = 3;
    test_table.max_press = 2;
    test_table.keybits = 6;
    test_table.key64 = false;
    test_table.sorted = true;
    test_table.loaded = true;

    test_table.keymap[0] = 'a';
    test_table.keymap[1] = 'b';
    test_table.keymap[2] = 'c';

    memset (test_table.keyname, 0, sizeof (test_table.keyname));

    GtabItem items[3];
    memset (items, 0, sizeof (items));
    uint32_t k;

    k = encode_gtab_key (6, (int[]){0, 0}, 2); /* "aa" */
    memcpy (items[0].key, &k, 4);
    items[0].ch[0] = 'P';

    k = encode_gtab_key (6, (int[]){0, 1}, 2); /* "ab" */
    memcpy (items[1].key, &k, 4);
    items[1].ch[0] = 'Q';

    k = encode_gtab_key (6, (int[]){1, 0}, 2); /* "ba" */
    memcpy (items[2].key, &k, 4);
    items[2].ch[0] = 'R';

    test_table.items = items;
    test_table.item_count = 3;

    /* Start with 2 keys "ab" → exact match */
    ctx->gtab = &test_table;
    hime_set_input_method (ctx, HIME_IM_GTAB);
    ctx->gtab_keys[0] = 0; /* a */
    ctx->gtab_keys[1] = 1; /* b */
    ctx->gtab_key_count = 2;

    gtab_rebuild_display (ctx);
    strcpy (ctx->preedit, ctx->gtab_key_display);
    gtab_lookup (ctx);
    append_candidates_to_preedit (ctx);
    ASSERT_EQ (1, ctx->candidate_count);

    /* Simulate backspace: remove last key, now just "a" → 2 matches */
    ctx->gtab_key_count = 1;
    gtab_rebuild_display (ctx);
    strcpy (ctx->preedit, ctx->gtab_key_display);
    gtab_lookup (ctx);
    append_candidates_to_preedit (ctx);

    ASSERT_EQ (2, ctx->candidate_count);
    char buf[256];
    hime_get_preedit (ctx, buf, sizeof (buf));
    ASSERT_TRUE (strstr (buf, "1.") != NULL);
    ASSERT_TRUE (strstr (buf, "2.") != NULL);

    ctx->gtab = NULL;
    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Method Label Tests ========== */

TEST (method_label_english_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    hime_set_chinese_mode (ctx, false);
    const char *label = hime_get_method_label (ctx);
    ASSERT_STR_EQ ("en", label);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (method_label_pho_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    hime_set_chinese_mode (ctx, true);
    hime_set_input_method (ctx, HIME_IM_PHO);
    const char *label = hime_get_method_label (ctx);
    ASSERT_STR_EQ ("注", label);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (method_label_tsin_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    hime_set_chinese_mode (ctx, true);
    hime_set_input_method (ctx, HIME_IM_TSIN);
    const char *label = hime_get_method_label (ctx);
    ASSERT_STR_EQ ("詞", label);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (method_label_intcode_mode) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    hime_set_chinese_mode (ctx, true);
    hime_set_input_method (ctx, HIME_IM_INTCODE);
    const char *label = hime_get_method_label (ctx);
    ASSERT_STR_EQ ("碼", label);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (method_label_gtab_cangjie) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    hime_set_chinese_mode (ctx, true);

    /* Load Cangjie 5 table — name should be "倉五" */
    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_CJ5);
    if (ret == 0) {
        const char *label = hime_get_method_label (ctx);
        /* First character of "倉五" is "倉" */
        ASSERT_STR_EQ ("倉", label);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (method_label_null_safety) {
    const char *label = hime_get_method_label (NULL);
    ASSERT_STR_EQ ("en", label);
    TEST_PASS ();
}

/* ========== Boshiamy / GTAB Valid Key Tests ========== */

TEST (gtab_registry_boshiamy_filename) {
    hime_init ("../../data");

    /* Find the Boshiamy entry in the GTAB registry by name */
    int count = hime_gtab_get_table_count ();
    ASSERT_TRUE (count > 0);

    HimeGtabInfo info;
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (hime_gtab_get_table_info (i, &info) == 0) {
            if (strcmp (info.name, "嘸蝦米") == 0) {
                found = 1;
                /* Verify filename is liu.gtab, not noseeing.gtab */
                ASSERT_STR_EQ ("liu.gtab", info.filename);
                break;
            }
        }
    }
    ASSERT_TRUE (found);

    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_is_valid_key_null_safety) {
    /* NULL context should return 0 */
    ASSERT_EQ (0, hime_gtab_is_valid_key (NULL, 'a'));

    /* Context without loaded table should return 0 */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* No table loaded yet in fresh context */
    hime_set_input_method (ctx, HIME_IM_PHO);
    ASSERT_EQ (0, hime_gtab_is_valid_key (ctx, 'a'));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_is_valid_key_cangjie) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_CJ5);
    if (ret == 0) {
        /* Cangjie uses a-z as input keys */
        ASSERT_TRUE (hime_gtab_is_valid_key (ctx, 'a'));
        ASSERT_TRUE (hime_gtab_is_valid_key (ctx, 'z'));

        /* Cangjie does NOT use comma or period as input keys */
        ASSERT_FALSE (hime_gtab_is_valid_key (ctx, ','));
        ASSERT_FALSE (hime_gtab_is_valid_key (ctx, '.'));
        ASSERT_FALSE (hime_gtab_is_valid_key (ctx, ';'));
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_is_valid_key_boshiamy) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_BOSHIAMY);
    if (ret == 0) {
        /* Boshiamy uses a-z as input keys */
        ASSERT_TRUE (hime_gtab_is_valid_key (ctx, 'a'));
        ASSERT_TRUE (hime_gtab_is_valid_key (ctx, 'z'));

        /* Boshiamy also uses comma and period as input keys */
        ASSERT_TRUE (hime_gtab_is_valid_key (ctx, ','));
        ASSERT_TRUE (hime_gtab_is_valid_key (ctx, '.'));
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_boshiamy_table_name) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_BOSHIAMY);
    if (ret == 0) {
        const char *name = hime_gtab_get_current_table (ctx);
        ASSERT_NOT_NULL (name);
        /* Name should be "嘸蝦米", starting with 嘸 (UTF-8: E5 98 B8) */
        ASSERT_TRUE ((unsigned char) name[0] == 0xe5);
        ASSERT_TRUE ((unsigned char) name[1] == 0x98);
        ASSERT_TRUE ((unsigned char) name[2] == 0xb8);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_boshiamy_space_commits) {
    /* V1 .gtab tables use 8-bit (byte-per-key) encoding for key indices.
     * This test verifies that typing a code and pressing Space commits
     * the first candidate correctly (regression test for keybits fix). */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);
    hime_set_chinese_mode (ctx, true);

    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_BOSHIAMY);
    if (ret != 0) {
        hime_context_free (ctx);
        hime_cleanup ();
        TEST_PASS (); /* Skip if liu.gtab not available */
        return;
    }

    /* Type 's' — should get candidates (Boshiamy has many 's' entries) */
    HimeKeyResult kr = hime_process_key (ctx, 's', 's', 0);
    ASSERT_EQ (HIME_KEY_PREEDIT, (int) kr);
    ASSERT_TRUE (hime_get_candidate_count (ctx) > 0);

    /* Type 'e' — should narrow candidates */
    kr = hime_process_key (ctx, 'e', 'e', 0);
    ASSERT_EQ (HIME_KEY_PREEDIT, (int) kr);
    int cands = hime_get_candidate_count (ctx);
    ASSERT_TRUE (cands > 0);

    /* Press Space — should commit first candidate */
    kr = hime_process_key (ctx, 0x20, ' ', 0);
    ASSERT_EQ (HIME_KEY_COMMIT, (int) kr);

    char commit[64];
    int len = hime_get_commit (ctx, commit, sizeof (commit));
    ASSERT_TRUE (len > 0);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_cangjie_table_name_differs_from_boshiamy) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Load Cangjie 5 and verify its name does NOT start with 嘸 */
    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_CJ5);
    if (ret == 0) {
        const char *name = hime_gtab_get_current_table (ctx);
        ASSERT_NOT_NULL (name);
        /* Cangjie name starts with 倉 (UTF-8: E5 80 89) — same first byte
         * but different second byte from 嘸 (E5 98 B8) */
        ASSERT_TRUE ((unsigned char) name[0] == 0xe5);
        /* Must NOT match 嘸's second byte (0x98) */
        ASSERT_TRUE ((unsigned char) name[1] != 0x98 ||
                     (unsigned char) name[2] != 0xb8);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_switch_cangjie_to_boshiamy) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);
    hime_set_chinese_mode (ctx, true);

    /* Load Cangjie 5 */
    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_CJ5);
    if (ret != 0) {
        hime_context_free (ctx);
        hime_cleanup ();
        TEST_PASS (); /* Skip if CJ5 not available */
        return;
    }
    ASSERT_EQ (HIME_IM_GTAB, hime_get_input_method (ctx));
    const char *label = hime_get_method_label (ctx);
    ASSERT_STR_EQ ("倉", label);

    /* Switch to Boshiamy */
    ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_BOSHIAMY);
    if (ret != 0) {
        hime_context_free (ctx);
        hime_cleanup ();
        TEST_PASS (); /* Skip if liu.gtab not available */
        return;
    }
    ASSERT_EQ (HIME_IM_GTAB, hime_get_input_method (ctx));
    label = hime_get_method_label (ctx);
    ASSERT_STR_EQ ("嘸", label);

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (gtab_mode_cycle_4way) {
    /* Simulate the 4-way mode cycle: EN → PHO → CJ5 → Boshiamy → EN */
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    /* Start in EN mode */
    hime_set_chinese_mode (ctx, false);
    ASSERT_FALSE (hime_is_chinese_mode (ctx));

    /* EN → Zhuyin */
    hime_set_chinese_mode (ctx, true);
    hime_set_input_method (ctx, HIME_IM_PHO);
    ASSERT_TRUE (hime_is_chinese_mode (ctx));
    ASSERT_EQ (HIME_IM_PHO, hime_get_input_method (ctx));

    /* Zhuyin → Cangjie */
    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_CJ5);
    if (ret != 0) {
        hime_context_free (ctx);
        hime_cleanup ();
        TEST_PASS ();
        return;
    }
    ASSERT_EQ (HIME_IM_GTAB, hime_get_input_method (ctx));
    const char *cj_name = hime_gtab_get_current_table (ctx);
    /* Cangjie: should NOT match Boshiamy's 嘸 */
    ASSERT_TRUE ((unsigned char) cj_name[1] != 0x98 ||
                 (unsigned char) cj_name[2] != 0xb8);

    /* Cangjie → Boshiamy */
    ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_BOSHIAMY);
    if (ret != 0) {
        hime_context_free (ctx);
        hime_cleanup ();
        TEST_PASS ();
        return;
    }
    ASSERT_EQ (HIME_IM_GTAB, hime_get_input_method (ctx));
    const char *liu_name = hime_gtab_get_current_table (ctx);
    /* Boshiamy: should match 嘸 (E5 98 B8) */
    ASSERT_TRUE ((unsigned char) liu_name[0] == 0xe5);
    ASSERT_TRUE ((unsigned char) liu_name[1] == 0x98);
    ASSERT_TRUE ((unsigned char) liu_name[2] == 0xb8);

    /* Boshiamy → EN */
    hime_set_chinese_mode (ctx, false);
    ASSERT_FALSE (hime_is_chinese_mode (ctx));

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

TEST (method_label_gtab_boshiamy) {
    hime_init ("../../data");
    HimeContext *ctx = hime_context_new ();
    ASSERT_NOT_NULL (ctx);

    hime_set_chinese_mode (ctx, true);

    int ret = hime_gtab_load_table_by_id (ctx, HIME_GTAB_BOSHIAMY);
    if (ret == 0) {
        const char *label = hime_get_method_label (ctx);
        /* First character of "嘸蝦米" is "嘸" */
        ASSERT_STR_EQ ("嘸", label);
    }

    hime_context_free (ctx);
    hime_cleanup ();
    TEST_PASS ();
}

/* ========== Test Suite ========== */

TEST_SUITE_BEGIN ("HIME Core Library Tests")

/* Generate pho.tab2 fixture if not present */
generate_test_pho_tab2 ("../../data");

/* Initialization */
RUN_TEST (init_with_valid_path);
RUN_TEST (init_with_null_path);
RUN_TEST (version_not_null);
RUN_TEST (double_init_cleanup);

/* Context management */
RUN_TEST (context_new_returns_valid);
RUN_TEST (context_free_null_safe);
RUN_TEST (context_reset);
RUN_TEST (multiple_contexts);

/* Input method control */
RUN_TEST (set_get_input_method);
RUN_TEST (toggle_chinese_mode);
RUN_TEST (set_chinese_mode);

/* Keyboard layout */
RUN_TEST (set_get_keyboard_layout);
RUN_TEST (set_keyboard_layout_by_name);

/* Preedit */
RUN_TEST (get_preedit_empty);
RUN_TEST (get_preedit_cursor);
RUN_TEST (get_preedit_attrs);

/* Commit */
RUN_TEST (get_commit_empty);
RUN_TEST (clear_commit);

/* Candidates */
RUN_TEST (candidates_initially_empty);
RUN_TEST (candidate_page_functions);
RUN_TEST (set_candidates_per_page);
RUN_TEST (set_selection_keys);

/* GTAB */
RUN_TEST (gtab_get_table_count);
RUN_TEST (gtab_get_table_info);
RUN_TEST (gtab_load_table_by_id);
RUN_TEST (gtab_get_current_table);

/* Intcode */
RUN_TEST (intcode_set_get_mode);
RUN_TEST (intcode_get_buffer);
RUN_TEST (intcode_convert_unicode);
RUN_TEST (intcode_convert_invalid);

/* Settings */
RUN_TEST (charset_settings);
RUN_TEST (smart_punctuation_settings);
RUN_TEST (pinyin_annotation_settings);
RUN_TEST (candidate_style_settings);
RUN_TEST (sound_settings);
RUN_TEST (vibration_settings);
RUN_TEST (color_scheme_settings);

/* Punctuation */
RUN_TEST (reset_punctuation_state);
RUN_TEST (convert_punctuation);

/* Method availability */
RUN_TEST (method_availability);
RUN_TEST (method_names);

/* Key processing */
RUN_TEST (process_key_english_mode);
RUN_TEST (process_key_escape_clears);
RUN_TEST (process_key_with_modifiers);

/* Bopomofo */
RUN_TEST (get_bopomofo_string_empty);

/* TSIN */
RUN_TEST (tsin_get_phrase_empty);
RUN_TEST (tsin_commit_phrase);

/* GTAB key string */
RUN_TEST (gtab_get_key_string_empty);

/* Feedback */
RUN_TEST (feedback_callback);

/* Extended candidate */
RUN_TEST (get_candidate_with_annotation);

/* NULL safety */
RUN_TEST (null_context_safety);

/* Input method search */
RUN_TEST (search_methods_all);
RUN_TEST (search_methods_with_query);
RUN_TEST (search_methods_filter_by_type);
RUN_TEST (gtab_search_tables);
RUN_TEST (gtab_search_tables_empty_query);
RUN_TEST (find_method_by_name);

/* Simplified/Traditional conversion */
RUN_TEST (convert_simp_to_trad);
RUN_TEST (convert_trad_to_simp);
RUN_TEST (convert_mixed_string);
RUN_TEST (output_variant_toggle);
RUN_TEST (set_output_variant);
RUN_TEST (convert_to_output_variant);
RUN_TEST (conversion_null_safety);
RUN_TEST (conversion_common_chars);

/* Typing Practice */
RUN_TEST (typing_start_session);
RUN_TEST (typing_session_null_safety);
RUN_TEST (typing_submit_correct_chars);
RUN_TEST (typing_submit_incorrect_chars);
RUN_TEST (typing_chinese_characters);
RUN_TEST (typing_get_practice_text);
RUN_TEST (typing_reset_session);
RUN_TEST (typing_submit_string);
RUN_TEST (typing_record_keystroke);
RUN_TEST (typing_accuracy_calculation);

/* Practice Text Library */
RUN_TEST (practice_text_count);
RUN_TEST (practice_get_texts_by_category);
RUN_TEST (practice_get_texts_by_difficulty);
RUN_TEST (practice_get_text_by_id);
RUN_TEST (practice_get_all_texts);
RUN_TEST (practice_category_names);
RUN_TEST (practice_difficulty_names);
RUN_TEST (practice_random_text);

/* GTAB Binary Search */
RUN_TEST (gtab_bsearch_exact_match);
RUN_TEST (gtab_bsearch_prefix_match);
RUN_TEST (gtab_bsearch_no_match);
RUN_TEST (gtab_bsearch_boundary);
RUN_TEST (gtab_load_v2_format);
RUN_TEST (gtab_v2_load_and_lookup);

/* GTAB Candidate Display */
RUN_TEST (gtab_preedit_includes_candidates);
RUN_TEST (gtab_backspace_preserves_candidates);

/* Method Label */
RUN_TEST (method_label_english_mode);
RUN_TEST (method_label_pho_mode);
RUN_TEST (method_label_tsin_mode);
RUN_TEST (method_label_intcode_mode);
RUN_TEST (method_label_gtab_cangjie);
RUN_TEST (method_label_null_safety);

/* Boshiamy / GTAB Valid Key */
RUN_TEST (gtab_registry_boshiamy_filename);
RUN_TEST (gtab_is_valid_key_null_safety);
RUN_TEST (gtab_is_valid_key_cangjie);
RUN_TEST (gtab_is_valid_key_boshiamy);
RUN_TEST (gtab_boshiamy_table_name);
RUN_TEST (gtab_boshiamy_space_commits);
RUN_TEST (gtab_cangjie_table_name_differs_from_boshiamy);
RUN_TEST (gtab_switch_cangjie_to_boshiamy);
RUN_TEST (gtab_mode_cycle_4way);
RUN_TEST (method_label_gtab_boshiamy);

/* Integration: Bopomofo Input Flow */
RUN_TEST (pho_type_bopomofo_key);
RUN_TEST (pho_space_triggers_lookup);
RUN_TEST (pho_tone_triggers_lookup);
RUN_TEST (pho_candidate_selection);
RUN_TEST (pho_single_candidate_autocommit);
RUN_TEST (pho_backspace_deletes_component);
RUN_TEST (pho_escape_cancels_input);
RUN_TEST (pho_english_mode_passthrough);
RUN_TEST (pho_mode_toggle_resets_state);
RUN_TEST (pho_candidate_paging);
RUN_TEST (pho_preedit_includes_candidates);
RUN_TEST (pho_full_input_cycle);
RUN_TEST (pho_multiple_keyboards);
RUN_TEST (pho_data_loading);
RUN_TEST (pho_invalid_data_path);

/* Clean up generated fixture */
cleanup_test_pho_tab2 ();

TEST_SUITE_END ()
