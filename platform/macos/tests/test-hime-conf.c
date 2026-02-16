/*
 * HIME macOS Unit Tests - Config Path Functions
 *
 * Tests pure string-manipulation functions for building config file
 * paths and parsing XMODIFIERS. Self-contained: includes local
 * reimplementations to avoid pulling in hime.h, X11, and GTK.
 *
 * Compiles as a standalone C executable (no macOS framework dependency).
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include "../../../tests/test-framework.h"

/* Local reimplementation of get_hime_dir from hime-conf.c */
static void
get_hime_dir (char *tt) {
    char *home = getenv ("HOME");
    if (!home)
        home = "";
    strcpy (tt, home);
    strcat (tt, "/.config/hime");
}

/* Local reimplementation of get_hime_conf_fname from hime-conf.c */
static void
get_hime_conf_fname (char *name, char fname[]) {
    get_hime_dir (fname);
    strcat (strcat (fname, "/config/"), name);
}

/* Local reimplementation of get_hime_xim_name from hime-conf.c */
static char *
get_hime_xim_name (void) {
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
    setenv ("HOME", "/Users/testuser", 1);
    get_hime_dir (buf);
    ASSERT_STR_EQ ("/Users/testuser/.config/hime", buf);
    TEST_PASS ();
}

TEST (get_hime_dir_empty_home) {
    char buf[256];
    setenv ("HOME", "", 1);
    get_hime_dir (buf);
    ASSERT_STR_EQ ("/.config/hime", buf);
    TEST_PASS ();
}

TEST (get_hime_dir_no_home) {
    char buf[256];
    unsetenv ("HOME");
    get_hime_dir (buf);
    ASSERT_STR_EQ ("/.config/hime", buf);
    TEST_PASS ();
}

TEST (get_hime_dir_root_home) {
    char buf[256];
    setenv ("HOME", "/", 1);
    get_hime_dir (buf);
    ASSERT_STR_EQ ("//.config/hime", buf);
    TEST_PASS ();
}

/* --- get_hime_conf_fname tests --- */

TEST (get_hime_conf_fname_simple) {
    char buf[256];
    setenv ("HOME", "/Users/testuser", 1);
    get_hime_conf_fname ("foo", buf);
    ASSERT_STR_EQ ("/Users/testuser/.config/hime/config/foo", buf);
    TEST_PASS ();
}

TEST (get_hime_conf_fname_with_ext) {
    char buf[256];
    setenv ("HOME", "/Users/testuser", 1);
    get_hime_conf_fname ("bar.conf", buf);
    ASSERT_STR_EQ ("/Users/testuser/.config/hime/config/bar.conf", buf);
    TEST_PASS ();
}

TEST (get_hime_conf_fname_empty) {
    char buf[256];
    setenv ("HOME", "/Users/testuser", 1);
    get_hime_conf_fname ("", buf);
    ASSERT_STR_EQ ("/Users/testuser/.config/hime/config/", buf);
    TEST_PASS ();
}

/* --- get_hime_xim_name tests --- */

TEST (xim_name_no_env) {
    unsetenv ("XMODIFIERS");
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
    TEST_PASS ();
}

TEST (xim_name_with_im) {
    setenv ("XMODIFIERS", "@im=fcitx", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("fcitx", name);
    TEST_PASS ();
}

TEST (xim_name_with_dot) {
    setenv ("XMODIFIERS", "@im=hime.en", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
    TEST_PASS ();
}

TEST (xim_name_no_im_prefix) {
    setenv ("XMODIFIERS", "something", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
    TEST_PASS ();
}

TEST (xim_name_empty) {
    setenv ("XMODIFIERS", "", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("hime", name);
    TEST_PASS ();
}

TEST (xim_name_at_im_only) {
    setenv ("XMODIFIERS", "@im=", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("", name);
    TEST_PASS ();
}

TEST (xim_name_long_name) {
    setenv ("XMODIFIERS", "@im=abcdefghijklmnopqrstuvwxyz12345678", 1);
    char *name = get_hime_xim_name ();
    ASSERT_EQ (31, (int) strlen (name));
    ASSERT_TRUE (strncmp (name, "abcdefghijklmnopqrstuvwxyz12345", 31) == 0);
    TEST_PASS ();
}

TEST (xim_name_with_extra) {
    setenv ("XMODIFIERS", "@im=test.extra", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("test", name);
    TEST_PASS ();
}

TEST (xim_name_multiple_dots) {
    setenv ("XMODIFIERS", "@im=a.b.c", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("a", name);
    TEST_PASS ();
}

TEST (xim_name_dot_at_start) {
    setenv ("XMODIFIERS", "@im=.hidden", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("", name);
    TEST_PASS ();
}

TEST (xim_name_im_in_middle) {
    setenv ("XMODIFIERS", "prefix@im=test", 1);
    char *name = get_hime_xim_name ();
    ASSERT_STR_EQ ("test", name);
    TEST_PASS ();
}

TEST_SUITE_BEGIN ("hime-conf tests (macOS)")
RUN_TEST (get_hime_dir_normal);
RUN_TEST (get_hime_dir_empty_home);
RUN_TEST (get_hime_dir_no_home);
RUN_TEST (get_hime_dir_root_home);
RUN_TEST (get_hime_conf_fname_simple);
RUN_TEST (get_hime_conf_fname_with_ext);
RUN_TEST (get_hime_conf_fname_empty);
RUN_TEST (xim_name_no_env);
RUN_TEST (xim_name_with_im);
RUN_TEST (xim_name_with_dot);
RUN_TEST (xim_name_no_im_prefix);
RUN_TEST (xim_name_empty);
RUN_TEST (xim_name_at_im_only);
RUN_TEST (xim_name_long_name);
RUN_TEST (xim_name_with_extra);
RUN_TEST (xim_name_multiple_dots);
RUN_TEST (xim_name_dot_at_start);
RUN_TEST (xim_name_im_in_middle);
TEST_SUITE_END ()
