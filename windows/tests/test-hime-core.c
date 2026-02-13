/*
 * HIME Core Library System Tests for Windows
 *
 * Tests the full input pipeline with data files loaded,
 * verifying everything works end-to-end on the system.
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

/* ========== Helper: find data directory ========== */

static const char *find_data_dir(void) {
    /* Try common data directory locations */
    static const char *paths[] = {
        "data",
        "../data",
        "../../data",
        "../../../data",
        "bin/data",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/pho.tab2", paths[i]);
        FILE *fp = fopen(test_path, "rb");
        if (fp) {
            fclose(fp);
            printf("  [Data found at: %s]\n", paths[i]);
            return paths[i];
        }
    }
    return NULL;
}

/* ========== Unit Tests (no data needed) ========== */

TEST(version) {
    const char *ver = hime_version();
    ASSERT_TRUE(ver != NULL);
    ASSERT_TRUE(strlen(ver) > 0);
    printf("(v%s) ", ver);
}

TEST(context_create) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);
    hime_context_free(ctx);
}

TEST(context_initial_state) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    ASSERT_TRUE(hime_is_chinese_mode(ctx));
    ASSERT_EQ(HIME_IM_PHO, hime_get_input_method(ctx));
    ASSERT_TRUE(!hime_has_candidates(ctx));
    ASSERT_EQ(0, hime_get_candidate_count(ctx));

    hime_context_free(ctx);
}

TEST(context_reset) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_context_reset(ctx);
    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    hime_context_free(ctx);
}

TEST(toggle_mode) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    ASSERT_TRUE(hime_is_chinese_mode(ctx));

    bool result = hime_toggle_chinese_mode(ctx);
    ASSERT_TRUE(!result);
    ASSERT_TRUE(!hime_is_chinese_mode(ctx));

    result = hime_toggle_chinese_mode(ctx);
    ASSERT_TRUE(result);
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

    ASSERT_EQ(10, hime_get_candidates_per_page(ctx));

    hime_set_candidates_per_page(ctx, 5);
    ASSERT_EQ(5, hime_get_candidates_per_page(ctx));

    hime_set_candidates_per_page(ctx, 0);
    ASSERT_EQ(1, hime_get_candidates_per_page(ctx));

    hime_set_candidates_per_page(ctx, 100);
    ASSERT_EQ(10, hime_get_candidates_per_page(ctx));

    hime_context_free(ctx);
}

TEST(key_in_english_mode) {
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_set_chinese_mode(ctx, false);

    HimeKeyResult result = hime_process_key(ctx, 'A', 'a', 0);
    ASSERT_EQ(HIME_KEY_IGNORED, result);

    result = hime_process_key(ctx, ' ', ' ', 0);
    ASSERT_EQ(HIME_KEY_IGNORED, result);

    hime_context_free(ctx);
}

TEST(null_context_safety) {
    ASSERT_TRUE(!hime_is_chinese_mode(NULL));
    ASSERT_EQ(HIME_IM_PHO, hime_get_input_method(NULL));
    ASSERT_EQ(-1, hime_set_input_method(NULL, HIME_IM_PHO));
    ASSERT_TRUE(!hime_toggle_chinese_mode(NULL));

    char buf[64];
    ASSERT_EQ(-1, hime_get_preedit(NULL, buf, sizeof(buf)));
    ASSERT_EQ(-1, hime_get_commit(NULL, buf, sizeof(buf)));
    ASSERT_EQ(0, hime_get_preedit_cursor(NULL));
    ASSERT_TRUE(!hime_has_candidates(NULL));
    ASSERT_EQ(0, hime_get_candidate_count(NULL));
    ASSERT_EQ(HIME_KEY_IGNORED, hime_process_key(NULL, 'A', 'a', 0));
    ASSERT_EQ(HIME_KEY_IGNORED, hime_select_candidate(NULL, 0));

    hime_context_reset(NULL);
    hime_set_chinese_mode(NULL, true);
    hime_clear_commit(NULL);
    hime_context_free(NULL);
}

/* ========== System Tests (require pho.tab2 data) ========== */

TEST(sys_init_data) {
    /* Verify hime_init loads phonetic table successfully */
    const char *data = find_data_dir();
    ASSERT_TRUE(data != NULL);

    hime_cleanup();
    int ret = hime_init(data);
    ASSERT_EQ(0, ret);
    /* Double init should be safe */
    ret = hime_init(data);
    ASSERT_EQ(0, ret);
}

TEST(sys_init_bad_path) {
    /* Init with nonexistent path should fail gracefully */
    hime_cleanup();
    int ret = hime_init("/nonexistent/path/to/data");
    ASSERT_EQ(-1, ret);

    /* Context still works but has no data */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);
    hime_context_free(ctx);

    /* Re-init with good path for subsequent tests */
    const char *data = find_data_dir();
    if (data) hime_init(data);
}

TEST(sys_type_single_bopomofo) {
    /* Type a single Bopomofo key and verify preedit */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* 'j' -> ㄓ on standard keyboard */
    HimeKeyResult kr = hime_process_key(ctx, 'J', 'j', 0);
    ASSERT_EQ(HIME_KEY_PREEDIT, kr);

    char buf[256];
    int len = hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_TRUE(len > 0);
    printf("(preedit='%s') ", buf);

    hime_context_free(ctx);
}

TEST(sys_type_multi_bopomofo) {
    /* Type multiple Bopomofo components */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* Standard: 'a' -> ㄇ, '8' -> ㄚ */
    hime_process_key(ctx, 'A', 'a', 0);
    char buf1[256];
    hime_get_preedit(ctx, buf1, sizeof(buf1));
    int len1 = strlen(buf1);
    ASSERT_TRUE(len1 > 0);

    hime_process_key(ctx, '8', '8', 0);
    char buf2[256];
    hime_get_preedit(ctx, buf2, sizeof(buf2));
    int len2 = strlen(buf2);
    /* Should be longer with two components */
    ASSERT_TRUE(len2 > len1);
    printf("(preedit='%s') ", buf2);

    hime_context_free(ctx);
}

TEST(sys_space_triggers_lookup) {
    /* Space (1st tone) should trigger candidate lookup */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* ㄇ(a) + ㄚ(8) + Space(1st tone) */
    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);
    HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

    ASSERT_TRUE(kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    if (kr == HIME_KEY_PREEDIT) {
        int count = hime_get_candidate_count(ctx);
        printf("(%d candidates) ", count);
        ASSERT_TRUE(count > 0);
    } else {
        char buf[256];
        hime_get_commit(ctx, buf, sizeof(buf));
        printf("(auto-commit='%s') ", buf);
    }

    hime_context_free(ctx);
}

TEST(sys_tone_triggers_lookup) {
    /* Pressing a tone key (not space) should trigger lookup */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* ㄇ(a) + ㄚ(8) + ˇ(3rd tone = '3') */
    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);
    HimeKeyResult kr = hime_process_key(ctx, '3', '3', 0);

    ASSERT_TRUE(kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    if (kr == HIME_KEY_PREEDIT) {
        ASSERT_TRUE(hime_get_candidate_count(ctx) > 0);
    }

    hime_context_free(ctx);
}

TEST(sys_candidate_in_preedit) {
    /* Preedit should include candidate list after lookup */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);
    HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 1) {
        char buf[1024];
        hime_get_preedit(ctx, buf, sizeof(buf));
        /* Preedit should contain numbered candidates */
        ASSERT_TRUE(strstr(buf, "1.") != NULL);
        ASSERT_TRUE(strstr(buf, "2.") != NULL);
        printf("(preedit='%.60s...') ", buf);
    }

    hime_context_free(ctx);
}

TEST(sys_select_candidate) {
    /* Select candidate with number key -> commit Chinese character */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);
    HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 1) {
        /* Press '1' to select first candidate */
        kr = hime_process_key(ctx, '1', '1', 0);
        ASSERT_EQ(HIME_KEY_COMMIT, kr);

        char buf[256];
        int len = hime_get_commit(ctx, buf, sizeof(buf));
        ASSERT_TRUE(len > 0);
        /* UTF-8 CJK character is 3 bytes */
        ASSERT_TRUE(len >= 3);
        printf("(committed='%s') ", buf);
    } else if (kr == HIME_KEY_COMMIT) {
        char buf[256];
        hime_get_commit(ctx, buf, sizeof(buf));
        printf("(auto='%s') ", buf);
    }

    hime_context_free(ctx);
}

TEST(sys_select_candidate_2) {
    /* Select second candidate */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);
    HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 2) {
        /* Press '2' to select second candidate */
        kr = hime_process_key(ctx, '2', '2', 0);
        ASSERT_EQ(HIME_KEY_COMMIT, kr);

        char buf[256];
        int len = hime_get_commit(ctx, buf, sizeof(buf));
        ASSERT_TRUE(len >= 3);
        printf("(committed='%s') ", buf);
    }

    hime_context_free(ctx);
}

TEST(sys_commit_then_continue) {
    /* After committing, should be able to type next character */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    /* First character */
    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);
    HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 0) {
        kr = hime_process_key(ctx, '1', '1', 0);
        ASSERT_EQ(HIME_KEY_COMMIT, kr);
        hime_clear_commit(ctx);
    } else if (kr == HIME_KEY_COMMIT) {
        hime_clear_commit(ctx);
    }

    /* State should be clean */
    ASSERT_EQ(0, hime_get_candidate_count(ctx));
    char buf[256];
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_EQ(0, (int)strlen(buf));

    /* Second character should work */
    kr = hime_process_key(ctx, 'J', 'j', 0);
    ASSERT_EQ(HIME_KEY_PREEDIT, kr);
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_TRUE(strlen(buf) > 0);
    printf("(next preedit='%s') ", buf);

    hime_context_free(ctx);
}

TEST(sys_backspace) {
    /* Backspace removes last phonetic component */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);

    char buf1[256];
    hime_get_preedit(ctx, buf1, sizeof(buf1));

    /* Backspace */
    HimeKeyResult kr = hime_process_key(ctx, 0x08, 0x08, 0);
    ASSERT_EQ(HIME_KEY_PREEDIT, kr);

    char buf2[256];
    hime_get_preedit(ctx, buf2, sizeof(buf2));
    ASSERT_TRUE(strlen(buf2) < strlen(buf1));

    /* Another backspace clears all */
    kr = hime_process_key(ctx, 0x08, 0x08, 0);
    ASSERT_TRUE(kr == HIME_KEY_PREEDIT || kr == HIME_KEY_ABSORBED);

    hime_context_free(ctx);
}

TEST(sys_escape) {
    /* Escape cancels all input */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'A', 'a', 0);
    hime_process_key(ctx, '8', '8', 0);

    HimeKeyResult kr = hime_process_key(ctx, 0x1B, 0x1B, 0);
    ASSERT_EQ(HIME_KEY_ABSORBED, kr);

    char buf[256];
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_EQ(0, (int)strlen(buf));
    ASSERT_EQ(0, hime_get_candidate_count(ctx));

    hime_context_free(ctx);
}

TEST(sys_candidate_paging) {
    /* Page through candidates */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);
    hime_set_candidates_per_page(ctx, 5);

    /* ㄧ(i) + Space - common syllable with many candidates */
    hime_process_key(ctx, 'I', 'i', 0);
    HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

    if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 5) {
        int total = hime_get_candidate_count(ctx);
        printf("(%d total) ", total);

        ASSERT_EQ(0, hime_get_candidate_page(ctx));

        bool ok = hime_candidate_page_down(ctx);
        ASSERT_TRUE(ok);
        ASSERT_EQ(1, hime_get_candidate_page(ctx));

        ok = hime_candidate_page_up(ctx);
        ASSERT_TRUE(ok);
        ASSERT_EQ(0, hime_get_candidate_page(ctx));
    }

    hime_context_free(ctx);
}

TEST(sys_common_char_de) {
    /* Test typing 的: ㄉ(d) + ㄜ(k) + Space(1st tone) */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'D', 'd', 0);
    hime_process_key(ctx, 'K', 'k', 0);
    HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

    ASSERT_TRUE(kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    char buf[256];
    if (kr == HIME_KEY_COMMIT) {
        hime_get_commit(ctx, buf, sizeof(buf));
        printf("(的='%s') ", buf);
    } else if (hime_get_candidate_count(ctx) > 0) {
        /* Select first */
        kr = hime_process_key(ctx, '1', '1', 0);
        if (kr == HIME_KEY_COMMIT) {
            hime_get_commit(ctx, buf, sizeof(buf));
            printf("(的='%s') ", buf);
        }
    }

    hime_context_free(ctx);
}

TEST(sys_common_char_ni) {
    /* Test typing 你: ㄋ(s) + ㄧ(i) + ˇ(3rd tone = '3') */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'S', 's', 0);
    hime_process_key(ctx, 'I', 'i', 0);
    HimeKeyResult kr = hime_process_key(ctx, '3', '3', 0);

    ASSERT_TRUE(kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    char buf[256];
    if (kr == HIME_KEY_COMMIT) {
        hime_get_commit(ctx, buf, sizeof(buf));
        printf("(你='%s') ", buf);
    } else if (hime_get_candidate_count(ctx) > 0) {
        kr = hime_process_key(ctx, '1', '1', 0);
        if (kr == HIME_KEY_COMMIT) {
            hime_get_commit(ctx, buf, sizeof(buf));
            printf("(你='%s') ", buf);
        }
    }

    hime_context_free(ctx);
}

TEST(sys_common_char_hao) {
    /* Test typing 好: ㄏ(c) + ㄠ(l) + ˇ(3) */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'C', 'c', 0);
    hime_process_key(ctx, 'L', 'l', 0);
    HimeKeyResult kr = hime_process_key(ctx, '3', '3', 0);

    ASSERT_TRUE(kr == HIME_KEY_COMMIT || kr == HIME_KEY_PREEDIT);

    char buf[256];
    if (kr == HIME_KEY_COMMIT) {
        hime_get_commit(ctx, buf, sizeof(buf));
        printf("(好='%s') ", buf);
    } else if (hime_get_candidate_count(ctx) > 0) {
        kr = hime_process_key(ctx, '1', '1', 0);
        if (kr == HIME_KEY_COMMIT) {
            hime_get_commit(ctx, buf, sizeof(buf));
            printf("(好='%s') ", buf);
        }
    }

    hime_context_free(ctx);
}

TEST(sys_full_sentence) {
    /* Type multiple characters in sequence: 你好 */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    char result[256] = {0};
    int rpos = 0;
    char buf[256];
    HimeKeyResult kr;

    /* 你: ㄋ(s) + ㄧ(i) + ˇ(3) */
    hime_process_key(ctx, 'S', 's', 0);
    hime_process_key(ctx, 'I', 'i', 0);
    kr = hime_process_key(ctx, '3', '3', 0);

    if (kr == HIME_KEY_COMMIT) {
        int len = hime_get_commit(ctx, buf, sizeof(buf));
        if (len > 0) { memcpy(result + rpos, buf, len); rpos += len; }
        hime_clear_commit(ctx);
    } else if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 0) {
        kr = hime_process_key(ctx, '1', '1', 0);
        if (kr == HIME_KEY_COMMIT) {
            int len = hime_get_commit(ctx, buf, sizeof(buf));
            if (len > 0) { memcpy(result + rpos, buf, len); rpos += len; }
            hime_clear_commit(ctx);
        }
    }

    /* 好: ㄏ(c) + ㄠ(l) + ˇ(3) */
    hime_process_key(ctx, 'C', 'c', 0);
    hime_process_key(ctx, 'L', 'l', 0);
    kr = hime_process_key(ctx, '3', '3', 0);

    if (kr == HIME_KEY_COMMIT) {
        int len = hime_get_commit(ctx, buf, sizeof(buf));
        if (len > 0) { memcpy(result + rpos, buf, len); rpos += len; }
        hime_clear_commit(ctx);
    } else if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 0) {
        kr = hime_process_key(ctx, '1', '1', 0);
        if (kr == HIME_KEY_COMMIT) {
            int len = hime_get_commit(ctx, buf, sizeof(buf));
            if (len > 0) { memcpy(result + rpos, buf, len); rpos += len; }
            hime_clear_commit(ctx);
        }
    }

    result[rpos] = '\0';
    ASSERT_TRUE(rpos > 0);
    printf("(result='%s') ", result);

    hime_context_free(ctx);
}

TEST(sys_mode_toggle_resets) {
    /* Toggling mode should clear preedit */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_process_key(ctx, 'A', 'a', 0);

    char buf[256];
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_TRUE(strlen(buf) > 0);

    hime_toggle_chinese_mode(ctx);
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_EQ(0, (int)strlen(buf));

    hime_toggle_chinese_mode(ctx);
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_EQ(0, (int)strlen(buf));

    hime_context_free(ctx);
}

TEST(sys_keyboard_layout_standard) {
    /* Verify standard keyboard layout works */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_set_keyboard_layout(ctx, HIME_KB_STANDARD);
    ASSERT_EQ(HIME_KB_STANDARD, hime_get_keyboard_layout(ctx));

    hime_process_key(ctx, 'A', 'a', 0);
    char buf[256];
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_TRUE(strlen(buf) > 0);
    printf("(std 'a'='%s') ", buf);

    hime_context_free(ctx);
}

TEST(sys_keyboard_layout_hsu) {
    /* Verify HSU keyboard layout works */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    hime_set_keyboard_layout(ctx, HIME_KB_HSU);
    ASSERT_EQ(HIME_KB_HSU, hime_get_keyboard_layout(ctx));

    hime_process_key(ctx, 'A', 'a', 0);
    char buf[256];
    hime_get_preedit(ctx, buf, sizeof(buf));
    ASSERT_TRUE(strlen(buf) > 0);
    printf("(hsu 'a'='%s') ", buf);

    hime_context_free(ctx);
}

TEST(sys_multiple_contexts) {
    /* Two contexts should be independent */
    HimeContext *ctx1 = hime_context_new();
    HimeContext *ctx2 = hime_context_new();
    ASSERT_TRUE(ctx1 != NULL);
    ASSERT_TRUE(ctx2 != NULL);

    hime_process_key(ctx1, 'A', 'a', 0);
    /* ctx2 should still be empty */
    char buf[256];
    hime_get_preedit(ctx2, buf, sizeof(buf));
    ASSERT_EQ(0, (int)strlen(buf));

    /* ctx1 should have content */
    hime_get_preedit(ctx1, buf, sizeof(buf));
    ASSERT_TRUE(strlen(buf) > 0);

    hime_context_free(ctx1);
    hime_context_free(ctx2);
}

TEST(sys_rapid_input) {
    /* Rapidly type and commit 5 characters */
    HimeContext *ctx = hime_context_new();
    ASSERT_TRUE(ctx != NULL);

    int committed = 0;
    char buf[256];

    /* 5 iterations of: type syllable -> select */
    for (int i = 0; i < 5; i++) {
        hime_process_key(ctx, 'A', 'a', 0);
        hime_process_key(ctx, '8', '8', 0);
        HimeKeyResult kr = hime_process_key(ctx, ' ', ' ', 0);

        if (kr == HIME_KEY_COMMIT) {
            committed++;
            hime_clear_commit(ctx);
        } else if (kr == HIME_KEY_PREEDIT && hime_get_candidate_count(ctx) > 0) {
            kr = hime_process_key(ctx, '1', '1', 0);
            if (kr == HIME_KEY_COMMIT) {
                committed++;
                hime_clear_commit(ctx);
            }
        }
    }

    printf("(%d/5 committed) ", committed);
    ASSERT_EQ(5, committed);

    hime_context_free(ctx);
}

TEST(sys_cleanup) {
    /* Cleanup should be safe */
    hime_cleanup();
    hime_cleanup(); /* Double cleanup safe */
}

/* ========== Main ========== */

int main(int argc, char *argv[]) {
    printf("=== HIME Core System Tests ===\n");

    printf("\n--- Unit Tests (no data needed) ---\n");
    RUN_TEST(version);
    RUN_TEST(context_create);
    RUN_TEST(context_initial_state);
    RUN_TEST(context_reset);
    RUN_TEST(toggle_mode);
    RUN_TEST(set_chinese_mode);
    RUN_TEST(set_input_method);
    RUN_TEST(preedit_empty);
    RUN_TEST(commit_empty);
    RUN_TEST(candidates_per_page);
    RUN_TEST(key_in_english_mode);
    RUN_TEST(null_context_safety);

    printf("\n--- System Tests (with phonetic data) ---\n");
    RUN_TEST(sys_init_data);
    RUN_TEST(sys_init_bad_path);
    RUN_TEST(sys_type_single_bopomofo);
    RUN_TEST(sys_type_multi_bopomofo);
    RUN_TEST(sys_space_triggers_lookup);
    RUN_TEST(sys_tone_triggers_lookup);
    RUN_TEST(sys_candidate_in_preedit);
    RUN_TEST(sys_select_candidate);
    RUN_TEST(sys_select_candidate_2);
    RUN_TEST(sys_commit_then_continue);
    RUN_TEST(sys_backspace);
    RUN_TEST(sys_escape);
    RUN_TEST(sys_candidate_paging);

    printf("\n--- Common Characters ---\n");
    RUN_TEST(sys_common_char_de);
    RUN_TEST(sys_common_char_ni);
    RUN_TEST(sys_common_char_hao);
    RUN_TEST(sys_full_sentence);

    printf("\n--- Integration ---\n");
    RUN_TEST(sys_mode_toggle_resets);
    RUN_TEST(sys_keyboard_layout_standard);
    RUN_TEST(sys_keyboard_layout_hsu);
    RUN_TEST(sys_multiple_contexts);
    RUN_TEST(sys_rapid_input);
    RUN_TEST(sys_cleanup);

    printf("\n=== Results ===\n");
    printf("Total:  %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
