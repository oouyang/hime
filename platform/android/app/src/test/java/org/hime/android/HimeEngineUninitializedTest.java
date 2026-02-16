/*
 * HIME Android Unit Tests - Engine API Contract
 *
 * Verifies the HimeEngine API contract: constant values, enum ranges,
 * and interface definitions. These tests run on the JVM without
 * native code — they only access static fields and interfaces.
 *
 * Tests that require HimeEngine instantiation (which loads native code)
 * should be in the instrumented test suite (androidTest/).
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android;

import static org.junit.Assert.*;

import org.hime.android.core.HimeEngine;
import org.junit.Test;

/**
 * API contract tests for HimeEngine.
 * Only accesses compile-time constants (static final int) which are
 * inlined by javac and don't trigger the static initializer.
 */
public class HimeEngineUninitializedTest {

    /* ========== Constant Range Validation ========== */

    @Test
    public void modeConstants_areNonNegative() {
        assertTrue(HimeEngine.MODE_CHINESE >= 0);
        assertTrue(HimeEngine.MODE_ENGLISH >= 0);
    }

    @Test
    public void resultConstants_areNonNegative() {
        assertTrue(HimeEngine.RESULT_IGNORED >= 0);
        assertTrue(HimeEngine.RESULT_CONSUMED >= 0);
        assertTrue(HimeEngine.RESULT_COMMIT >= 0);
    }

    @Test
    public void candidatesPerPage_isReasonable() {
        assertTrue("Should be at least 1", HimeEngine.CANDIDATES_PER_PAGE >= 1);
        assertTrue("Should be at most 20", HimeEngine.CANDIDATES_PER_PAGE <= 20);
    }

    @Test
    public void charsetConstants_areBinary() {
        assertEquals(0, HimeEngine.CHARSET_TRADITIONAL);
        assertEquals(1, HimeEngine.CHARSET_SIMPLIFIED);
    }

    @Test
    public void candidateStyleConstants_areSequential() {
        assertEquals(HimeEngine.CANDIDATE_STYLE_HORIZONTAL + 1,
                     HimeEngine.CANDIDATE_STYLE_VERTICAL);
        assertEquals(HimeEngine.CANDIDATE_STYLE_VERTICAL + 1,
                     HimeEngine.CANDIDATE_STYLE_POPUP);
    }

    @Test
    public void colorSchemeConstants_cover3Options() {
        /* Light, Dark, System = 3 distinct values */
        assertNotEquals(HimeEngine.COLOR_SCHEME_LIGHT, HimeEngine.COLOR_SCHEME_DARK);
        assertNotEquals(HimeEngine.COLOR_SCHEME_DARK, HimeEngine.COLOR_SCHEME_SYSTEM);
        assertNotEquals(HimeEngine.COLOR_SCHEME_LIGHT, HimeEngine.COLOR_SCHEME_SYSTEM);
    }

    @Test
    public void keyboardLayouts_cover7Options() {
        /* 7 distinct layouts from STANDARD to DVORAK */
        int[] layouts = {
            HimeEngine.KB_STANDARD,
            HimeEngine.KB_HSU,
            HimeEngine.KB_ETEN,
            HimeEngine.KB_ETEN26,
            HimeEngine.KB_IBM,
            HimeEngine.KB_PINYIN,
            HimeEngine.KB_DVORAK
        };
        for (int i = 0; i < layouts.length; i++) {
            for (int j = i + 1; j < layouts.length; j++) {
                assertNotEquals("Layouts " + i + " and " + j + " should differ",
                                layouts[i], layouts[j]);
            }
        }
    }

    @Test
    public void feedbackTypes_cover7Events() {
        int[] types = {
            HimeEngine.FEEDBACK_KEY_PRESS,
            HimeEngine.FEEDBACK_KEY_DELETE,
            HimeEngine.FEEDBACK_KEY_ENTER,
            HimeEngine.FEEDBACK_KEY_SPACE,
            HimeEngine.FEEDBACK_CANDIDATE,
            HimeEngine.FEEDBACK_MODE_CHANGE,
            HimeEngine.FEEDBACK_ERROR
        };
        /* All 7 types should be distinct */
        for (int i = 0; i < types.length; i++) {
            for (int j = i + 1; j < types.length; j++) {
                assertNotEquals("Types " + i + " and " + j + " should differ",
                                types[i], types[j]);
            }
        }
    }

    @Test
    public void feedbackTypes_areSequential() {
        assertEquals(0, HimeEngine.FEEDBACK_KEY_PRESS);
        assertEquals(1, HimeEngine.FEEDBACK_KEY_DELETE);
        assertEquals(2, HimeEngine.FEEDBACK_KEY_ENTER);
        assertEquals(3, HimeEngine.FEEDBACK_KEY_SPACE);
        assertEquals(4, HimeEngine.FEEDBACK_CANDIDATE);
        assertEquals(5, HimeEngine.FEEDBACK_MODE_CHANGE);
        assertEquals(6, HimeEngine.FEEDBACK_ERROR);
    }

    /* ========== Constant Consistency ========== */

    @Test
    public void modeChineseIsDefault() {
        /* Chinese mode should be 0 (default/initial state) */
        assertEquals(0, HimeEngine.MODE_CHINESE);
    }

    @Test
    public void standardLayoutIsDefault() {
        /* Standard keyboard should be 0 (default) */
        assertEquals(0, HimeEngine.KB_STANDARD);
    }

    @Test
    public void traditionalCharsetIsDefault() {
        /* Traditional Chinese should be 0 (default) */
        assertEquals(0, HimeEngine.CHARSET_TRADITIONAL);
    }

    @Test
    public void horizontalCandidateStyleIsDefault() {
        /* Horizontal style should be 0 (default) */
        assertEquals(0, HimeEngine.CANDIDATE_STYLE_HORIZONTAL);
    }

    @Test
    public void lightColorSchemeIsDefault() {
        /* Light scheme should be 0 (default) */
        assertEquals(0, HimeEngine.COLOR_SCHEME_LIGHT);
    }

    @Test
    public void ignoredResultIsDefault() {
        /* IGNORED should be 0 (default, key not handled) */
        assertEquals(0, HimeEngine.RESULT_IGNORED);
    }
}
