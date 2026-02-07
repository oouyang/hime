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

import org.hime.android.core.HimeEngine;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.*;

/**
 * Unit tests for HimeEngine Java wrapper.
 *
 * Note: These tests verify the Java API layer. Tests that require
 * the native library should be run as instrumented tests.
 */
public class HimeEngineTest {

    /**
     * Test that engine constants are defined correctly.
     */
    @Test
    public void testConstants() {
        assertEquals("MODE_CHINESE should be 0", 0, HimeEngine.MODE_CHINESE);
        assertEquals("MODE_ENGLISH should be 1", 1, HimeEngine.MODE_ENGLISH);

        assertEquals("RESULT_IGNORED should be 0", 0, HimeEngine.RESULT_IGNORED);
        assertEquals("RESULT_CONSUMED should be 1", 1, HimeEngine.RESULT_CONSUMED);
        assertEquals("RESULT_COMMIT should be 2", 2, HimeEngine.RESULT_COMMIT);

        assertEquals("CANDIDATES_PER_PAGE should be 10", 10, HimeEngine.CANDIDATES_PER_PAGE);
    }

    /**
     * Test engine instantiation.
     */
    @Test
    public void testEngineInstantiation() {
        HimeEngine engine = new HimeEngine();
        assertNotNull("Engine should be created", engine);
    }
}
