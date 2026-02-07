/*
 * Unit tests for UTF-8 functions (locale.c)
 */

#include "test-framework.h"

#include <glib.h>
#include <string.h>

/* Function declarations from locale.c */
int utf8_sz(char *s);
int utf8cpy(char *t, char *s);
int u8cpy(char *t, char *s);
int utf8_tlen(char *s, int N);
gboolean utf8_eq(char *a, char *b);
gboolean utf8_str_eq(char *a, char *b, int len);
int utf8_str_N(char *str);
void utf8cpyn(char *t, char *s, int n);
void utf8cpy_bytes(char *t, char *s, int n);
void utf8cpyN(char *t, char *s, int N);

/* ============ utf8_sz tests ============ */

TEST(utf8_sz_ascii) {
    /* ASCII characters are 1 byte */
    ASSERT_EQ(1, utf8_sz("a"));
    ASSERT_EQ(1, utf8_sz("Z"));
    ASSERT_EQ(1, utf8_sz("0"));
    ASSERT_EQ(1, utf8_sz(" "));
    ASSERT_EQ(1, utf8_sz("~"));
    TEST_PASS();
}

TEST(utf8_sz_2byte) {
    /* 2-byte UTF-8: U+0080 to U+07FF (Latin extended, etc.) */
    /* ñ = U+00F1 = 0xC3 0xB1 */
    ASSERT_EQ(2, utf8_sz("\xc3\xb1"));
    /* é = U+00E9 = 0xC3 0xA9 */
    ASSERT_EQ(2, utf8_sz("\xc3\xa9"));
    TEST_PASS();
}

TEST(utf8_sz_3byte) {
    /* 3-byte UTF-8: U+0800 to U+FFFF (CJK characters) */
    /* 中 = U+4E2D = 0xE4 0xB8 0xAD */
    ASSERT_EQ(3, utf8_sz("中"));
    /* 日 = U+65E5 */
    ASSERT_EQ(3, utf8_sz("日"));
    /* ㄅ (Bopomofo) = U+3105 */
    ASSERT_EQ(3, utf8_sz("ㄅ"));
    TEST_PASS();
}

TEST(utf8_sz_4byte) {
    /* 4-byte UTF-8: U+10000 to U+10FFFF (emoji, rare CJK) */
    /* 𠀀 = U+20000 = 0xF0 0xA0 0x80 0x80 */
    ASSERT_EQ(4, utf8_sz("\xf0\xa0\x80\x80"));
    TEST_PASS();
}

/* ============ utf8cpy tests ============ */

TEST(utf8cpy_ascii) {
    char buf[10] = {0};
    int len = utf8cpy(buf, "A");
    ASSERT_EQ(1, len);
    ASSERT_STR_EQ("A", buf);
    TEST_PASS();
}

TEST(utf8cpy_chinese) {
    char buf[10] = {0};
    int len = utf8cpy(buf, "中文");
    ASSERT_EQ(3, len);  /* Only copies first character */
    ASSERT_STR_EQ("中", buf);
    TEST_PASS();
}

TEST(utf8cpy_mixed) {
    char buf[10] = {0};
    /* Should only copy first UTF-8 character */
    int len = utf8cpy(buf, "日本語");
    ASSERT_EQ(3, len);
    ASSERT_STR_EQ("日", buf);
    TEST_PASS();
}

/* ============ u8cpy tests ============ */

TEST(u8cpy_no_null_terminator) {
    char buf[10] = "XXXXXXXXX";
    int len = u8cpy(buf, "中");
    ASSERT_EQ(3, len);
    /* u8cpy does NOT null-terminate */
    ASSERT_TRUE(buf[0] == '\xe4');
    ASSERT_TRUE(buf[1] == '\xb8');
    ASSERT_TRUE(buf[2] == '\xad');
    ASSERT_TRUE(buf[3] == 'X');  /* Original content preserved */
    TEST_PASS();
}

/* ============ utf8_tlen tests ============ */

TEST(utf8_tlen_ascii) {
    /* Total bytes for N ASCII characters */
    ASSERT_EQ(5, utf8_tlen("hello", 5));
    ASSERT_EQ(3, utf8_tlen("hello", 3));
    TEST_PASS();
}

TEST(utf8_tlen_chinese) {
    /* 中文 = 2 characters, 6 bytes */
    ASSERT_EQ(6, utf8_tlen("中文", 2));
    ASSERT_EQ(3, utf8_tlen("中文", 1));
    TEST_PASS();
}

TEST(utf8_tlen_mixed) {
    /* a中b = 3 characters: 1 + 3 + 1 = 5 bytes */
    ASSERT_EQ(5, utf8_tlen("a中b", 3));
    ASSERT_EQ(4, utf8_tlen("a中b", 2));
    ASSERT_EQ(1, utf8_tlen("a中b", 1));
    TEST_PASS();
}

/* ============ utf8_eq tests ============ */

TEST(utf8_eq_same_ascii) {
    ASSERT_TRUE(utf8_eq("a", "a"));
    ASSERT_TRUE(utf8_eq("Z", "Z"));
    TEST_PASS();
}

TEST(utf8_eq_same_chinese) {
    ASSERT_TRUE(utf8_eq("中", "中"));
    ASSERT_TRUE(utf8_eq("日", "日"));
    TEST_PASS();
}

TEST(utf8_eq_different) {
    ASSERT_FALSE(utf8_eq("a", "b"));
    ASSERT_FALSE(utf8_eq("中", "文"));
    ASSERT_FALSE(utf8_eq("a", "中"));
    TEST_PASS();
}

TEST(utf8_eq_only_first_char) {
    /* utf8_eq compares only the first UTF-8 character */
    ASSERT_TRUE(utf8_eq("abc", "aXX"));
    ASSERT_TRUE(utf8_eq("中文", "中國"));
    TEST_PASS();
}

/* ============ utf8_str_eq tests ============ */

TEST(utf8_str_eq_same) {
    ASSERT_TRUE(utf8_str_eq("hello", "hello", 5));
    ASSERT_TRUE(utf8_str_eq("中文", "中文", 2));
    TEST_PASS();
}

TEST(utf8_str_eq_partial) {
    /* Compare only first N characters */
    ASSERT_TRUE(utf8_str_eq("hello", "help", 3));
    ASSERT_TRUE(utf8_str_eq("中文字", "中文", 2));
    TEST_PASS();
}

TEST(utf8_str_eq_different) {
    ASSERT_FALSE(utf8_str_eq("hello", "world", 5));
    ASSERT_FALSE(utf8_str_eq("中文", "日本", 2));
    TEST_PASS();
}

/* ============ utf8_str_N tests ============ */

TEST(utf8_str_N_ascii) {
    ASSERT_EQ(5, utf8_str_N("hello"));
    ASSERT_EQ(0, utf8_str_N(""));
    TEST_PASS();
}

TEST(utf8_str_N_chinese) {
    ASSERT_EQ(2, utf8_str_N("中文"));
    ASSERT_EQ(3, utf8_str_N("日本語"));
    TEST_PASS();
}

TEST(utf8_str_N_mixed) {
    /* a中b = 3 characters */
    ASSERT_EQ(3, utf8_str_N("a中b"));
    /* Hello中文 = 7 characters */
    ASSERT_EQ(7, utf8_str_N("Hello中文"));
    TEST_PASS();
}

/* ============ utf8cpyn tests ============ */

TEST(utf8cpyn_full) {
    char buf[20] = {0};
    utf8cpyn(buf, "hello", 5);
    ASSERT_STR_EQ("hello", buf);
    TEST_PASS();
}

TEST(utf8cpyn_partial) {
    char buf[20] = {0};
    utf8cpyn(buf, "hello", 3);
    ASSERT_STR_EQ("hel", buf);
    TEST_PASS();
}

TEST(utf8cpyn_chinese) {
    char buf[20] = {0};
    utf8cpyn(buf, "中文字", 2);
    ASSERT_STR_EQ("中文", buf);
    TEST_PASS();
}

TEST(utf8cpyn_mixed) {
    char buf[20] = {0};
    utf8cpyn(buf, "a中b", 2);
    ASSERT_STR_EQ("a中", buf);
    TEST_PASS();
}

/* ============ utf8cpy_bytes tests ============ */

TEST(utf8cpy_bytes_ascii) {
    char buf[20] = {0};
    utf8cpy_bytes(buf, "hello", 5);
    ASSERT_STR_EQ("hello", buf);
    TEST_PASS();
}

TEST(utf8cpy_bytes_limit) {
    char buf[20] = {0};
    /* Copy at most 3 bytes, but must be complete UTF-8 chars */
    utf8cpy_bytes(buf, "hello", 3);
    ASSERT_STR_EQ("hel", buf);
    TEST_PASS();
}

TEST(utf8cpy_bytes_chinese) {
    char buf[20] = {0};
    /* 中文 = 6 bytes, limit to 6 should get both */
    utf8cpy_bytes(buf, "中文", 6);
    ASSERT_STR_EQ("中文", buf);
    TEST_PASS();
}

TEST(utf8cpy_bytes_partial_char) {
    char buf[20] = {0};
    /* 中文 = 6 bytes, limit to 5 */
    /* Note: utf8cpy_bytes copies complete chars while total bytes < limit */
    /* After first char (3 bytes), tn=3 < 5, so it copies second char too */
    /* After second char, tn=6, loop exits, result is 6 bytes */
    utf8cpy_bytes(buf, "中文", 5);
    ASSERT_EQ(6, (int)strlen(buf));
    TEST_PASS();
}

/* ============ utf8cpyN tests ============ */

TEST(utf8cpyN_full) {
    char buf[20] = {0};
    utf8cpyN(buf, "hello", 5);
    ASSERT_STR_EQ("hello", buf);
    TEST_PASS();
}

TEST(utf8cpyN_chinese) {
    char buf[20] = {0};
    utf8cpyN(buf, "日本語", 2);
    ASSERT_STR_EQ("日本", buf);
    TEST_PASS();
}

/* ============ Test Suite ============ */

TEST_SUITE_BEGIN("UTF-8 Functions")
    /* utf8_sz */
    RUN_TEST(utf8_sz_ascii);
    RUN_TEST(utf8_sz_2byte);
    RUN_TEST(utf8_sz_3byte);
    RUN_TEST(utf8_sz_4byte);

    /* utf8cpy */
    RUN_TEST(utf8cpy_ascii);
    RUN_TEST(utf8cpy_chinese);
    RUN_TEST(utf8cpy_mixed);

    /* u8cpy */
    RUN_TEST(u8cpy_no_null_terminator);

    /* utf8_tlen */
    RUN_TEST(utf8_tlen_ascii);
    RUN_TEST(utf8_tlen_chinese);
    RUN_TEST(utf8_tlen_mixed);

    /* utf8_eq */
    RUN_TEST(utf8_eq_same_ascii);
    RUN_TEST(utf8_eq_same_chinese);
    RUN_TEST(utf8_eq_different);
    RUN_TEST(utf8_eq_only_first_char);

    /* utf8_str_eq */
    RUN_TEST(utf8_str_eq_same);
    RUN_TEST(utf8_str_eq_partial);
    RUN_TEST(utf8_str_eq_different);

    /* utf8_str_N */
    RUN_TEST(utf8_str_N_ascii);
    RUN_TEST(utf8_str_N_chinese);
    RUN_TEST(utf8_str_N_mixed);

    /* utf8cpyn */
    RUN_TEST(utf8cpyn_full);
    RUN_TEST(utf8cpyn_partial);
    RUN_TEST(utf8cpyn_chinese);
    RUN_TEST(utf8cpyn_mixed);

    /* utf8cpy_bytes */
    RUN_TEST(utf8cpy_bytes_ascii);
    RUN_TEST(utf8cpy_bytes_limit);
    RUN_TEST(utf8cpy_bytes_chinese);
    RUN_TEST(utf8cpy_bytes_partial_char);

    /* utf8cpyN */
    RUN_TEST(utf8cpyN_full);
    RUN_TEST(utf8cpyN_chinese);
TEST_SUITE_END()
