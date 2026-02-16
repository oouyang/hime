/*
 * HIME iOS Unit Tests - Config Path Functions
 *
 * XCTest-based tests for the pure string-manipulation functions that
 * build config file paths and parse XMODIFIERS. Self-contained:
 * includes local reimplementations to avoid pulling in hime.h,
 * X11, and GTK dependencies.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#import <XCTest/XCTest.h>
#include <stdlib.h>
#include <string.h>

/* Local reimplementation of get_hime_dir from hime-conf.c */
static void test_get_hime_dir(char *tt) {
    char *home = getenv("HOME");
    if (!home)
        home = "";
    strcpy(tt, home);
    strcat(tt, "/.config/hime");
}

/* Local reimplementation of get_hime_conf_fname from hime-conf.c */
static void test_get_hime_conf_fname(char *name, char fname[]) {
    test_get_hime_dir(fname);
    strcat(strcat(fname, "/config/"), name);
}

/* Local reimplementation of get_hime_xim_name from hime-conf.c */
static char *test_get_hime_xim_name(void) {
    char *xim_name;

    if ((xim_name = getenv("XMODIFIERS"))) {
        static char find[] = "@im=";
        static char sstr[32];
        char *p = strstr(xim_name, find);

        if (p == NULL)
            return "hime";

        p += strlen(find);
        strncpy(sstr, p, sizeof(sstr));
        sstr[sizeof(sstr) - 1] = 0;

        if ((p = strchr(sstr, '.')))
            *p = 0;

        return sstr;
    }

    return "hime";
}

#pragma mark - get_hime_dir Tests

@interface HimeConfDirTests : XCTestCase
@end

@implementation HimeConfDirTests

- (void)testGetHimeDirNormal {
    char buf[256];
    setenv("HOME", "/home/user", 1);
    test_get_hime_dir(buf);
    XCTAssertEqual(strcmp(buf, "/home/user/.config/hime"), 0);
}

- (void)testGetHimeDirEmptyHome {
    char buf[256];
    setenv("HOME", "", 1);
    test_get_hime_dir(buf);
    XCTAssertEqual(strcmp(buf, "/.config/hime"), 0);
}

- (void)testGetHimeDirNoHome {
    char buf[256];
    unsetenv("HOME");
    test_get_hime_dir(buf);
    XCTAssertEqual(strcmp(buf, "/.config/hime"), 0);
}

- (void)testGetHimeDirMacOSPath {
    char buf[256];
    setenv("HOME", "/Users/testuser", 1);
    test_get_hime_dir(buf);
    XCTAssertEqual(strcmp(buf, "/Users/testuser/.config/hime"), 0);
}

@end

#pragma mark - get_hime_conf_fname Tests

@interface HimeConfFnameTests : XCTestCase
@end

@implementation HimeConfFnameTests

- (void)testGetHimeConfFnameSimple {
    char buf[256];
    setenv("HOME", "/home/user", 1);
    test_get_hime_conf_fname("foo", buf);
    XCTAssertEqual(strcmp(buf, "/home/user/.config/hime/config/foo"), 0);
}

- (void)testGetHimeConfFnameWithExtension {
    char buf[256];
    setenv("HOME", "/home/user", 1);
    test_get_hime_conf_fname("bar.conf", buf);
    XCTAssertEqual(strcmp(buf, "/home/user/.config/hime/config/bar.conf"), 0);
}

- (void)testGetHimeConfFnameEmptyName {
    char buf[256];
    setenv("HOME", "/home/user", 1);
    test_get_hime_conf_fname("", buf);
    XCTAssertEqual(strcmp(buf, "/home/user/.config/hime/config/"), 0);
}

@end

#pragma mark - get_hime_xim_name Tests

@interface HimeConfXimNameTests : XCTestCase
@end

@implementation HimeConfXimNameTests

- (void)testXimNameNoEnv {
    unsetenv("XMODIFIERS");
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "hime"), 0);
}

- (void)testXimNameWithIm {
    setenv("XMODIFIERS", "@im=fcitx", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "fcitx"), 0);
}

- (void)testXimNameWithDot {
    setenv("XMODIFIERS", "@im=hime.en", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "hime"), 0);
}

- (void)testXimNameNoImPrefix {
    setenv("XMODIFIERS", "something", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "hime"), 0);
}

- (void)testXimNameEmpty {
    setenv("XMODIFIERS", "", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "hime"), 0);
}

- (void)testXimNameAtImOnly {
    setenv("XMODIFIERS", "@im=", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, ""), 0);
}

- (void)testXimNameLongName {
    setenv("XMODIFIERS", "@im=abcdefghijklmnopqrstuvwxyz12345678", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual((int)strlen(name), 31, @"Should be truncated to 31 chars");
    XCTAssertEqual(strncmp(name, "abcdefghijklmnopqrstuvwxyz12345", 31), 0);
}

- (void)testXimNameWithExtra {
    setenv("XMODIFIERS", "@im=test.extra", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "test"), 0);
}

- (void)testXimNameMultipleDots {
    setenv("XMODIFIERS", "@im=a.b.c", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "a"), 0);
}

- (void)testXimNameDotAtStart {
    setenv("XMODIFIERS", "@im=.hidden", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, ""), 0);
}

- (void)testXimNameImInMiddle {
    setenv("XMODIFIERS", "prefix@im=test", 1);
    char *name = test_get_hime_xim_name();
    XCTAssertEqual(strcmp(name, "test"), 0);
}

@end
