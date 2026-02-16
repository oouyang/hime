/*
 * HIME Android Unit Tests - Config Path Functions
 *
 * Java port of the config path tests from tests/test-hime-conf.c.
 * Tests the pure string-manipulation functions for building config
 * file paths and parsing XMODIFIERS.
 *
 * These tests run on the JVM without Android framework.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android;

import static org.junit.Assert.*;

import org.junit.Test;

/**
 * Tests for HIME config path logic (Java reimplementation).
 * Verifies path construction and XMODIFIERS parsing that matches
 * the C implementation in hime-conf.c.
 */
public class HimeConfTest {
    /**
     * Java reimplementation of get_hime_dir from hime-conf.c.
     */
    private static String getHimeDir(String home) {
        if (home == null) {
            home = "";
        }
        return home + "/.config/hime";
    }

    /**
     * Java reimplementation of get_hime_conf_fname from hime-conf.c.
     */
    private static String getHimeConfFname(String home, String name) {
        return getHimeDir(home) + "/config/" + name;
    }

    /**
     * Java reimplementation of get_hime_xim_name from hime-conf.c.
     * Parses XMODIFIERS environment variable value.
     */
    private static String getHimeXimName(String xmodifiers) {
        if (xmodifiers == null) {
            return "hime";
        }

        String find = "@im=";
        int pos = xmodifiers.indexOf(find);
        if (pos < 0) {
            return "hime";
        }

        String name = xmodifiers.substring(pos + find.length());

        /* Truncate to 31 chars (matching sstr[32] in C) */
        if (name.length() > 31) {
            name = name.substring(0, 31);
        }

        /* Truncate at dot */
        int dotPos = name.indexOf('.');
        if (dotPos >= 0) {
            name = name.substring(0, dotPos);
        }

        return name;
    }

    /* ========== get_hime_dir tests ========== */

    @Test
    public void getHimeDir_normal() {
        assertEquals("/home/user/.config/hime", getHimeDir("/home/user"));
    }

    @Test
    public void getHimeDir_emptyHome() {
        assertEquals("/.config/hime", getHimeDir(""));
    }

    @Test
    public void getHimeDir_nullHome() {
        assertEquals("/.config/hime", getHimeDir(null));
    }

    @Test
    public void getHimeDir_rootHome() {
        assertEquals("//.config/hime", getHimeDir("/"));
    }

    @Test
    public void getHimeDir_windowsStylePath() {
        assertEquals("C:\\Users\\test/.config/hime", getHimeDir("C:\\Users\\test"));
    }

    /* ========== get_hime_conf_fname tests ========== */

    @Test
    public void getHimeConfFname_simple() {
        assertEquals("/home/user/.config/hime/config/foo", getHimeConfFname("/home/user", "foo"));
    }

    @Test
    public void getHimeConfFname_nestedName() {
        assertEquals("/home/user/.config/hime/config/bar.conf", getHimeConfFname("/home/user", "bar.conf"));
    }

    @Test
    public void getHimeConfFname_emptyName() {
        assertEquals("/home/user/.config/hime/config/", getHimeConfFname("/home/user", ""));
    }

    /* ========== get_hime_xim_name tests ========== */

    @Test
    public void ximName_noEnv() {
        assertEquals("hime", getHimeXimName(null));
    }

    @Test
    public void ximName_withIm() {
        assertEquals("fcitx", getHimeXimName("@im=fcitx"));
    }

    @Test
    public void ximName_withDot() {
        assertEquals("hime", getHimeXimName("@im=hime.en"));
    }

    @Test
    public void ximName_noImPrefix() {
        assertEquals("hime", getHimeXimName("something"));
    }

    @Test
    public void ximName_empty() {
        assertEquals("hime", getHimeXimName(""));
    }

    @Test
    public void ximName_atImOnly() {
        assertEquals("", getHimeXimName("@im="));
    }

    @Test
    public void ximName_longName() {
        String name = getHimeXimName("@im=abcdefghijklmnopqrstuvwxyz12345678");
        assertEquals(31, name.length());
        assertEquals("abcdefghijklmnopqrstuvwxyz12345", name);
    }

    @Test
    public void ximName_withExtra() {
        assertEquals("test", getHimeXimName("@im=test.extra"));
    }

    @Test
    public void ximName_dotAtStart() {
        assertEquals("", getHimeXimName("@im=.hidden"));
    }

    @Test
    public void ximName_multipleDots() {
        assertEquals("a", getHimeXimName("@im=a.b.c"));
    }

    @Test
    public void ximName_imInMiddle() {
        assertEquals("test", getHimeXimName("prefix@im=test"));
    }
}
