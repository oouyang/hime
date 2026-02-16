/*
 * HIME Android Unit Tests
 *
 * JUnit tests for HimeEngine (Java layer).
 * These tests run on the JVM without Android framework.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android;

import static org.junit.Assert.*;

import org.hime.android.core.HimeEngine;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

/**
 * Unit tests for HimeEngine Java wrapper.
 *
 * Note: These tests verify the Java API layer constants only.
 * Tests that require the native library (including instantiation)
 * should be run as instrumented tests on a device/emulator.
 */
public class HimeEngineTest {
    /**
     * Test that input mode constants are defined correctly.
     */
    @Test
    public void testModeConstants() {
        assertEquals("MODE_CHINESE should be 0", 0, HimeEngine.MODE_CHINESE);
        assertEquals("MODE_ENGLISH should be 1", 1, HimeEngine.MODE_ENGLISH);
    }

    /**
     * Test that key result constants are defined correctly.
     */
    @Test
    public void testResultConstants() {
        assertEquals("RESULT_IGNORED should be 0", 0, HimeEngine.RESULT_IGNORED);
        assertEquals("RESULT_CONSUMED should be 1", 1, HimeEngine.RESULT_CONSUMED);
        assertEquals("RESULT_COMMIT should be 2", 2, HimeEngine.RESULT_COMMIT);
    }

    /**
     * Test candidates per page constant.
     */
    @Test
    public void testCandidatesPerPage() {
        assertEquals("CANDIDATES_PER_PAGE should be 10", 10, HimeEngine.CANDIDATES_PER_PAGE);
    }

    /**
     * Test character set constants.
     */
    @Test
    public void testCharsetConstants() {
        assertEquals(0, HimeEngine.CHARSET_TRADITIONAL);
        assertEquals(1, HimeEngine.CHARSET_SIMPLIFIED);
    }

    /**
     * Test candidate style constants.
     */
    @Test
    public void testCandidateStyleConstants() {
        assertEquals(0, HimeEngine.CANDIDATE_STYLE_HORIZONTAL);
        assertEquals(1, HimeEngine.CANDIDATE_STYLE_VERTICAL);
        assertEquals(2, HimeEngine.CANDIDATE_STYLE_POPUP);
    }

    /**
     * Test color scheme constants.
     */
    @Test
    public void testColorSchemeConstants() {
        assertEquals(0, HimeEngine.COLOR_SCHEME_LIGHT);
        assertEquals(1, HimeEngine.COLOR_SCHEME_DARK);
        assertEquals(2, HimeEngine.COLOR_SCHEME_SYSTEM);
    }

    /**
     * Test keyboard layout constants.
     */
    @Test
    public void testKeyboardLayoutConstants() {
        assertEquals(0, HimeEngine.KB_STANDARD);
        assertEquals(1, HimeEngine.KB_HSU);
        assertEquals(2, HimeEngine.KB_ETEN);
        assertEquals(3, HimeEngine.KB_ETEN26);
        assertEquals(4, HimeEngine.KB_IBM);
        assertEquals(5, HimeEngine.KB_PINYIN);
        assertEquals(6, HimeEngine.KB_DVORAK);
    }

    /**
     * Test feedback type constants.
     */
    @Test
    public void testFeedbackConstants() {
        assertEquals(0, HimeEngine.FEEDBACK_KEY_PRESS);
        assertEquals(1, HimeEngine.FEEDBACK_KEY_DELETE);
        assertEquals(2, HimeEngine.FEEDBACK_KEY_ENTER);
        assertEquals(3, HimeEngine.FEEDBACK_KEY_SPACE);
        assertEquals(4, HimeEngine.FEEDBACK_CANDIDATE);
        assertEquals(5, HimeEngine.FEEDBACK_MODE_CHANGE);
        assertEquals(6, HimeEngine.FEEDBACK_ERROR);
    }

    /**
     * Test that mode constants are distinct.
     */
    @Test
    public void testModeConstantsDistinct() {
        assertNotEquals(HimeEngine.MODE_CHINESE, HimeEngine.MODE_ENGLISH);
    }

    /**
     * Test that result constants are distinct.
     */
    @Test
    public void testResultConstantsDistinct() {
        assertNotEquals(HimeEngine.RESULT_IGNORED, HimeEngine.RESULT_CONSUMED);
        assertNotEquals(HimeEngine.RESULT_CONSUMED, HimeEngine.RESULT_COMMIT);
        assertNotEquals(HimeEngine.RESULT_IGNORED, HimeEngine.RESULT_COMMIT);
    }

    /**
     * Test keyboard layout constants are sequential.
     */
    @Test
    public void testKeyboardLayoutSequential() {
        assertEquals(HimeEngine.KB_STANDARD + 1, HimeEngine.KB_HSU);
        assertEquals(HimeEngine.KB_HSU + 1, HimeEngine.KB_ETEN);
        assertEquals(HimeEngine.KB_ETEN + 1, HimeEngine.KB_ETEN26);
        assertEquals(HimeEngine.KB_ETEN26 + 1, HimeEngine.KB_IBM);
        assertEquals(HimeEngine.KB_IBM + 1, HimeEngine.KB_PINYIN);
        assertEquals(HimeEngine.KB_PINYIN + 1, HimeEngine.KB_DVORAK);
    }
}
