/*
 * Unit tests for utility functions (util.c)
 */

#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "test-framework.h"

/* Function declarations from util.c / hime.h */
void *zmalloc (int n);
void *memdup (void *p, int n);
char *myfgets (char *buf, int bufN, FILE *fp);

/* ============ zmalloc tests ============ */

TEST (zmalloc_basic) {
    void *p = zmalloc (100);
    ASSERT_NOT_NULL (p);

    /* Should be zero-initialized */
    unsigned char *bytes = (unsigned char *) p;
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ (0, bytes[i]);
    }

    free (p);
    TEST_PASS ();
}

TEST (zmalloc_small) {
    void *p = zmalloc (1);
    ASSERT_NOT_NULL (p);

    unsigned char *bytes = (unsigned char *) p;
    ASSERT_EQ (0, bytes[0]);

    free (p);
    TEST_PASS ();
}

TEST (zmalloc_larger) {
    void *p = zmalloc (4096);
    ASSERT_NOT_NULL (p);

    /* Check first and last bytes */
    unsigned char *bytes = (unsigned char *) p;
    ASSERT_EQ (0, bytes[0]);
    ASSERT_EQ (0, bytes[4095]);

    free (p);
    TEST_PASS ();
}

/* ============ memdup tests ============ */

TEST (memdup_basic) {
    char original[] = "Hello, World!";
    char *copy = (char *) memdup (original, sizeof (original));

    ASSERT_NOT_NULL (copy);
    ASSERT_STR_EQ (original, copy);
    ASSERT_TRUE (copy != original); /* Different memory location */

    free (copy);
    TEST_PASS ();
}

TEST (memdup_binary) {
    unsigned char data[] = {0x00, 0x01, 0xFF, 0x80, 0x7F};
    unsigned char *copy = (unsigned char *) memdup (data, sizeof (data));

    ASSERT_NOT_NULL (copy);
    ASSERT_MEM_EQ (data, copy, sizeof (data));

    free (copy);
    TEST_PASS ();
}

TEST (memdup_null_input) {
    void *copy = memdup (NULL, 10);
    ASSERT_NULL (copy);
    TEST_PASS ();
}

TEST (memdup_zero_size) {
    char data[] = "test";
    void *copy = memdup (data, 0);
    ASSERT_NULL (copy);
    TEST_PASS ();
}

/* ============ myfgets tests ============ */

/* Helper to create a temp file with content */
static FILE *create_temp_file (const char *content) {
    FILE *fp = tmpfile ();
    if (fp && content) {
        fwrite (content, 1, strlen (content), fp);
        rewind (fp);
    }
    return fp;
}

TEST (myfgets_simple_line) {
    FILE *fp = create_temp_file ("Hello\n");
    ASSERT_NOT_NULL (fp);

    char buf[100];
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("Hello", buf);

    fclose (fp);
    TEST_PASS ();
}

TEST (myfgets_no_newline) {
    FILE *fp = create_temp_file ("NoNewline");
    ASSERT_NOT_NULL (fp);

    char buf[100];
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("NoNewline", buf);

    fclose (fp);
    TEST_PASS ();
}

TEST (myfgets_crlf) {
    /* Test Windows-style line ending \r\n */
    FILE *fp = create_temp_file ("Windows\r\nLine");
    ASSERT_NOT_NULL (fp);

    char buf[100];
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("Windows", buf);

    /* Second line */
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("Line", buf);

    fclose (fp);
    TEST_PASS ();
}

TEST (myfgets_cr_only) {
    /* Test old Mac-style line ending \r */
    FILE *fp = create_temp_file ("OldMac\rStyle");
    ASSERT_NOT_NULL (fp);

    char buf[100];
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("OldMac", buf);

    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("Style", buf);

    fclose (fp);
    TEST_PASS ();
}

TEST (myfgets_lf_cr) {
    /* Test reverse order \n\r */
    FILE *fp = create_temp_file ("Reverse\n\rOrder");
    ASSERT_NOT_NULL (fp);

    char buf[100];
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("Reverse", buf);

    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("Order", buf);

    fclose (fp);
    TEST_PASS ();
}

TEST (myfgets_empty_lines) {
    FILE *fp = create_temp_file ("\n\nLine3\n");
    ASSERT_NOT_NULL (fp);

    char buf[100];

    /* First empty line */
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("", buf);

    /* Second empty line */
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("", buf);

    /* Third line with content */
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("Line3", buf);

    fclose (fp);
    TEST_PASS ();
}

TEST (myfgets_buffer_limit) {
    FILE *fp = create_temp_file ("VeryLongLineHere\n");
    ASSERT_NOT_NULL (fp);

    /* Note: myfgets reads up to bufN chars (not bufN-1), then writes null
     * terminator. With buf[11], it reads max 10 chars + null at position 10 */
    char buf[11];
    myfgets (buf, 10, fp);
    /* Function reads while out - buf < bufN, so reads 10 chars max */
    ASSERT_EQ (10, (int) strlen (buf));
    ASSERT_STR_EQ ("VeryLongLi", buf);

    fclose (fp);
    TEST_PASS ();
}

TEST (myfgets_utf8_content) {
    FILE *fp = create_temp_file ("中文測試\nEnglish\n");
    ASSERT_NOT_NULL (fp);

    char buf[100];
    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("中文測試", buf);

    myfgets (buf, sizeof (buf), fp);
    ASSERT_STR_EQ ("English", buf);

    fclose (fp);
    TEST_PASS ();
}

/* ============ Test Suite ============ */

TEST_SUITE_BEGIN ("Utility Functions")
/* zmalloc */
RUN_TEST (zmalloc_basic);
RUN_TEST (zmalloc_small);
RUN_TEST (zmalloc_larger);

/* memdup */
RUN_TEST (memdup_basic);
RUN_TEST (memdup_binary);
RUN_TEST (memdup_null_input);
RUN_TEST (memdup_zero_size);

/* myfgets */
RUN_TEST (myfgets_simple_line);
RUN_TEST (myfgets_no_newline);
RUN_TEST (myfgets_crlf);
RUN_TEST (myfgets_cr_only);
RUN_TEST (myfgets_lf_cr);
RUN_TEST (myfgets_empty_lines);
RUN_TEST (myfgets_buffer_limit);
RUN_TEST (myfgets_utf8_content);
TEST_SUITE_END ()
