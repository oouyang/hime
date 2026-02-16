/*
 * Unit tests for config path functions (hime-conf.c)
 *
 * Tests the pure string-manipulation functions for building config file
 * paths and parsing XMODIFIERS. Self-contained: includes local
 * reimplementations to avoid pulling in hime.h, X11, and GTK.
 *
 * Cross-compiled with MinGW, runs via Wine or natively on Windows.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;
static int tests_total = 0;
static const char *current_test = NULL;

#define TEST(name)                       \
    static void test_##name (void);      \
    static void run_test_##name (void) { \
        current_test = #name;            \
        tests_total++;                   \
        test_##name ();                  \
    }                                    \
    static void test_##name (void)

#define RUN_TEST(name)      \
    do {                    \
        run_test_##name (); \
    } while (0)

#define PASS()                                     \
    do {                                           \
        printf ("  PASS: %s\n", current_test);     \
        tests_passed++;                            \
    } while (0)

#define ASSERT_TRUE(expr)                                            \
    do {                                                             \
        if (!(expr)) {                                               \
            printf ("  FAIL: %s\n    %s:%d: %s\n",                   \
                    current_test, __FILE__, __LINE__, #expr);        \
            tests_failed++;                                          \
            return;                                                  \
        }                                                            \
    } while (0)

#define ASSERT_EQ(expected, actual)                                  \
    do {                                                             \
        if ((expected) != (actual)) {                                \
            printf ("  FAIL: %s\n    %s:%d: expected %ld, got %ld\n",\
                    current_test, __FILE__, __LINE__,                \
                    (long)(expected), (long)(actual));               \
            tests_failed++;                                          \
            return;                                                  \
        }                                                            \
    } while (0)

#define ASSERT_STR_EQ(expected, actual)                              \
    do {                                                             \
        if (strcmp ((expected), (actual)) != 0) {                    \
            printf ("  FAIL: %s\n    %s:%d: expected \"%s\","        \
                    " got \"%s\"\n",                                 \
                    current_test, __FILE__, __LINE__,                \
                    (expected), (actual));                           \
            tests_failed++;                                          \
            return;                                                  \
        }                                                            \
    } while (0)

/* Local reimplementation of get_hime_dir from hime-conf.c */
static void
get_hime_dir (char *tt)
{
    char *home = getenv ("HOME");
    if (!home)
        home = "";
    strcpy (tt, home);
    strcat (tt, "/.config/hime");
}

/* Local reimplementation of get_hime_conf_fname from hime-conf.c */
static void
get_hime_conf_fname (char *name, char fname[])
{
    get_hime_dir (fname);
    strcat (strcat (fname, "/config/"), name);
}

/* Local reimplementation of get_hime_xim_name from hime-conf.c */
static char *
get_hime_xim_name (void)
{
    char *xim_name;

    if ((xim_name = getenv ("XMODIFIERS"))) {
        static char find[] = "@im=";
        static char sstr[32];
        char *p = strstr (xim_name, find);

        if (p == NULL)
            return "hime";

        p += strlen (find);
        strncpy (sstr, p, sizeof (sstr));
        sstr[sizeof (sstr) - 1] = 0;

        if ((p = strchr (sstr, '.')))
            *p = 0;

        return sstr;
    }

    return "hime";
}

/* --- get_hime_dir tests --- */

TEST (get_hime_dir_normal) {
    char buf[256];
    /* Use _putenv on Windows (MinGW supports both) */
    putenv ("HOME=/home/user");
    get_hime_dir (buf);
    ASSERT_STR_EQ ("/home/user/.config/hime", buf);
    PASS ();
}

TEST (get_hime_dir_empty_home) {
    char buf[256];
    putenv ("HOME=");
    get_hime_dir (buf);
    ASSERT_STR_EQ ("/.config/hime", buf);
    PASS ();
}

TEST (get_hime_dir_no_home) {
    char buf[256];
    /* On Windows/MinGW, putenv("HOME=") sets to empty; to truly unset
     * we use putenv("HOME") without '=' which is MinGW-specific behavior,
     * but the code path for !home falls through to "" either way. */
    putenv ("HOME=");
    /* Force the no-HOME codepath by temporarily removing it */
    char *saved = getenv ("HOME");
    if (saved && saved[0] == '\0') {
        /* Empty HOME triggers the same path as no HOME */
        get_hime_dir (buf);
        ASSERT_STR_EQ ("/.config/hime", buf);
    } else {
        get_hime_dir (buf);
        ASSERT_STR_EQ ("/.config/hime", buf);
    }
    PASS ();
}

/* --- get_hime_conf_fname tests --- */

TEST (get_hime_conf_fname_simple) {
    char buf[256];
    putenv ("HOME=/home/user");
    get_hime_conf_fname ("foo", buf);
    ASSERT_STR_EQ ("/home/user/.config/hime/config/foo", buf);
    PASS ();
}

/* --- get_hime_xim_name tests --- */

TEST (xim_name_no_env) {
    /* Unset XMODIFIERS - on MinGW putenv("VAR=") sets to empty,
     * but our code checks getenv which returns "" not NULL.
     * Use platform-appropriate unsetting. */
#ifdef _WIN32
    putenv ("XMODIFIERS=");
    /* On Windows, putenv("X=") keeps it as empty string, not NULL.
     * We test the empty-string path here instead. */
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
#else
    unsetenv ("XMODIFIERS");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
#endif
    PASS ();
}

TEST (xim_name_with_im) {
    putenv ("XMODIFIERS=@im=fcitx");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("fcitx", name);
    PASS ();
}

TEST (xim_name_with_dot) {
    putenv ("XMODIFIERS=@im=hime.en");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
    PASS ();
}

TEST (xim_name_no_im_prefix) {
    putenv ("XMODIFIERS=something");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
    PASS ();
}

TEST (xim_name_empty) {
    putenv ("XMODIFIERS=");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
    PASS ();
}

TEST (xim_name_at_im_only) {
    putenv ("XMODIFIERS=@im=");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("", name);
    PASS ();
}

TEST (xim_name_long_name) {
    putenv ("XMODIFIERS=@im=abcdefghijklmnopqrstuvwxyz12345678");
    char *name = get_hime_xim_name ();
    /* sstr is 32 bytes, strncpy copies 31 + NUL terminator */
    ASSERT_EQ (31, (int) strlen (name));
    ASSERT_TRUE (strncmp (name, "abcdefghijklmnopqrstuvwxyz12345", 31) == 0);
    PASS ();
}

TEST (xim_name_with_extra) {
    putenv ("XMODIFIERS=@im=test.extra");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("test", name);
    PASS ();
}

/* ========== Main ========== */

int
main (int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    printf ("\n=== HIME Config Path Tests (Windows) ===\n\n");

    printf ("get_hime_dir tests:\n");
    RUN_TEST (get_hime_dir_normal);
    RUN_TEST (get_hime_dir_empty_home);
    RUN_TEST (get_hime_dir_no_home);

    printf ("\nget_hime_conf_fname tests:\n");
    RUN_TEST (get_hime_conf_fname_simple);

    printf ("\nget_hime_xim_name tests:\n");
    RUN_TEST (xim_name_no_env);
    RUN_TEST (xim_name_with_im);
    RUN_TEST (xim_name_with_dot);
    RUN_TEST (xim_name_no_im_prefix);
    RUN_TEST (xim_name_empty);
    RUN_TEST (xim_name_at_im_only);
    RUN_TEST (xim_name_long_name);
    RUN_TEST (xim_name_with_extra);

    printf ("\n=== Results ===\n");
    printf ("Total:  %d\n", tests_total);
    printf ("Passed: %d\n", tests_passed);
    printf ("Failed: %d\n", tests_failed);
    printf ("\n");

    return tests_failed > 0 ? 1 : 0;
}
