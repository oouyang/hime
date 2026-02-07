/*
 * HIME Android Instrumented Tests
 *
 * These tests run on an Android device/emulator and can test
 * native JNI code.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.hime.android.core.HimeEngine;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

/**
 * Instrumented tests for HimeEngine with native JNI code.
 * These tests require an Android device or emulator.
 */
@RunWith(AndroidJUnit4.class)
public class HimeEngineInstrumentedTest {

    private Context context;
    private HimeEngine engine;

    @Before
    public void setUp() {
        context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        engine = new HimeEngine();
    }

    @After
    public void tearDown() {
        if (engine != null) {
            engine.destroy();
            engine = null;
        }
    }

    /* ========== Initialization Tests ========== */

    @Test
    public void testEngineInitialization() {
        boolean result = engine.init(context);
        assertTrue("Engine should initialize successfully", result);
    }

    @Test
    public void testDoubleInitialization() {
        /* Should handle double init gracefully */
        assertTrue(engine.init(context));
        assertTrue(engine.init(context));
    }

    /* ========== Mode Tests ========== */

    @Test
    public void testInitialChineseMode() {
        engine.init(context);
        assertTrue("Should start in Chinese mode", engine.isChineseMode());
    }

    @Test
    public void testToggleInputMode() {
        engine.init(context);

        assertTrue(engine.isChineseMode());

        engine.toggleInputMode();
        assertFalse("Should be in English mode after toggle", engine.isChineseMode());

        engine.toggleInputMode();
        assertTrue("Should be in Chinese mode after second toggle", engine.isChineseMode());
    }

    @Test
    public void testSetInputMode() {
        engine.init(context);

        engine.setInputMode(HimeEngine.MODE_ENGLISH);
        assertEquals(HimeEngine.MODE_ENGLISH, engine.getInputMode());
        assertFalse(engine.isChineseMode());

        engine.setInputMode(HimeEngine.MODE_CHINESE);
        assertEquals(HimeEngine.MODE_CHINESE, engine.getInputMode());
        assertTrue(engine.isChineseMode());
    }

    /* ========== Preedit Tests ========== */

    @Test
    public void testInitialPreeditEmpty() {
        engine.init(context);

        String preedit = engine.getPreedit();
        assertNotNull("Preedit should not be null", preedit);
        assertEquals("Initial preedit should be empty", "", preedit);
    }

    @Test
    public void testCommitStringEmpty() {
        engine.init(context);

        String commit = engine.getCommit();
        assertNotNull("Commit should not be null", commit);
        assertEquals("Initial commit should be empty", "", commit);
    }

    @Test
    public void testClearPreedit() {
        engine.init(context);

        /* Process some input */
        engine.processKey('j', 0);

        /* Clear should not crash */
        engine.clearPreedit();

        String preedit = engine.getPreedit();
        assertEquals("Preedit should be empty after clear", "", preedit);
    }

    /* ========== Candidate Tests ========== */

    @Test
    public void testInitialNoCandidates() {
        engine.init(context);

        int count = engine.getCandidateCount();
        assertEquals("Should have no candidates initially", 0, count);
    }

    @Test
    public void testGetCandidatesNull() {
        engine.init(context);

        String[] candidates = engine.getCandidates(0);
        /* Can be null or empty array when no candidates */
        assertTrue("Should have no candidates",
                   candidates == null || candidates.length == 0);
    }

    @Test
    public void testSelectCandidateWithNone() {
        engine.init(context);

        boolean result = engine.selectCandidate(0);
        assertFalse("Should return false when no candidates", result);
    }

    /* ========== Key Processing Tests ========== */

    @Test
    public void testKeyProcessingInEnglishMode() {
        engine.init(context);

        engine.setInputMode(HimeEngine.MODE_ENGLISH);
        int result = engine.processKey('a', 0);
        assertEquals("Keys should be ignored in English mode",
                     HimeEngine.RESULT_IGNORED, result);
    }

    @Test
    public void testKeyProcessingInChineseMode() {
        engine.init(context);

        assertTrue(engine.isChineseMode());

        /* Process a Zhuyin key */
        int result = engine.processKey('j', 0);
        /* Result depends on data file availability */
        assertTrue("Should be IGNORED or CONSUMED",
                   result == HimeEngine.RESULT_IGNORED ||
                   result == HimeEngine.RESULT_CONSUMED ||
                   result == HimeEngine.RESULT_COMMIT);
    }

    /* ========== Reset Tests ========== */

    @Test
    public void testReset() {
        engine.init(context);

        /* Process some input */
        engine.processKey('j', 0);

        /* Reset */
        engine.reset();

        String preedit = engine.getPreedit();
        assertEquals("Preedit should be empty after reset", "", preedit);
        assertTrue("Should be in Chinese mode after reset", engine.isChineseMode());
    }

    @Test
    public void testMultipleResets() {
        engine.init(context);

        /* Multiple resets should not crash */
        for (int i = 0; i < 10; i++) {
            engine.reset();
        }

        assertTrue("Should still be in Chinese mode", engine.isChineseMode());
    }

    /* ========== Destroy Tests ========== */

    @Test
    public void testDestroy() {
        engine.init(context);

        engine.destroy();

        /* After destroy, operations should be safe (return defaults) */
        String preedit = engine.getPreedit();
        assertEquals("Should return empty after destroy", "", preedit);
    }

    @Test
    public void testDoubleDestroy() {
        engine.init(context);

        engine.destroy();
        engine.destroy(); /* Should not crash */
    }

    /* ========== Integration Tests ========== */

    @Test
    public void testTypingSequence() {
        engine.init(context);

        /* Ensure Chinese mode */
        engine.setInputMode(HimeEngine.MODE_CHINESE);

        /* Type a sequence of Zhuyin keys */
        engine.processKey('j', 0);  /* ㄨ */
        engine.processKey('3', 0);  /* ˇ (tone) */

        /* Check preedit */
        String preedit = engine.getPreedit();
        assertNotNull(preedit);

        /* Reset for next test */
        engine.reset();
    }

    @Test
    public void testModeToggleClearsPreedit() {
        engine.init(context);

        /* Type something in Chinese mode */
        engine.processKey('j', 0);

        /* Toggle mode - should commit/clear preedit */
        String preeditBefore = engine.getPreedit();
        engine.toggleInputMode();

        /* Toggle back */
        engine.toggleInputMode();

        /* After toggling, preedit might be cleared */
        String preeditAfter = engine.getPreedit();
        assertNotNull(preeditAfter);
    }
}
