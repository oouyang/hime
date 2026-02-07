/*
 * HIME Android Engine
 *
 * Java wrapper for hime-core native library.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android.core;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * HimeEngine provides Java interface to the native hime-core library.
 * Handles Bopomofo/Zhuyin phonetic input processing.
 */
public class HimeEngine {
    private static final String TAG = "HimeEngine";

    /* Input modes */
    public static final int MODE_CHINESE = 0;
    public static final int MODE_ENGLISH = 1;

    /* Key processing results */
    public static final int RESULT_IGNORED = 0;
    public static final int RESULT_CONSUMED = 1;
    public static final int RESULT_COMMIT = 2;

    /* Candidates per page */
    public static final int CANDIDATES_PER_PAGE = 10;

    private boolean initialized = false;

    static {
        System.loadLibrary("hime-jni");
    }

    /**
     * Initialize the engine with application context.
     * Copies data files to internal storage if needed.
     */
    public boolean init(Context context) {
        if (initialized) {
            return true;
        }

        try {
            /* Copy data files from assets to internal storage */
            String dataPath = copyDataFiles(context);
            if (dataPath == null) {
                Log.e(TAG, "Failed to copy data files");
                return false;
            }

            /* Initialize native engine */
            initialized = nativeInit(dataPath);
            if (!initialized) {
                Log.e(TAG, "Failed to initialize native engine");
            }
            return initialized;

        } catch (Exception e) {
            Log.e(TAG, "Init error: " + e.getMessage());
            return false;
        }
    }

    /**
     * Destroy the engine and release resources.
     */
    public void destroy() {
        if (initialized) {
            nativeDestroy();
            initialized = false;
        }
    }

    /**
     * Process a key input.
     *
     * @param keyChar   The character of the key
     * @param modifiers Key modifiers (shift, ctrl, etc.)
     * @return Result code: RESULT_IGNORED, RESULT_CONSUMED, or RESULT_COMMIT
     */
    public int processKey(char keyChar, int modifiers) {
        if (!initialized) return RESULT_IGNORED;
        return nativeProcessKey(keyChar, modifiers);
    }

    /**
     * Get the current preedit (composition) string.
     * Shows Bopomofo symbols being composed.
     */
    public String getPreedit() {
        if (!initialized) return "";
        String preedit = nativeGetPreedit();
        return preedit != null ? preedit : "";
    }

    /**
     * Get the committed text.
     * Returns characters that should be inserted into the text field.
     */
    public String getCommit() {
        if (!initialized) return "";
        String commit = nativeGetCommit();
        return commit != null ? commit : "";
    }

    /**
     * Get candidates for the specified page.
     *
     * @param page Page number (0-based)
     * @return Array of candidate strings, or null if none
     */
    public String[] getCandidates(int page) {
        if (!initialized) return null;
        return nativeGetCandidates(page);
    }

    /**
     * Get total number of candidates.
     */
    public int getCandidateCount() {
        if (!initialized) return 0;
        return nativeGetCandidateCount();
    }

    /**
     * Select a candidate by index.
     *
     * @param index Candidate index (0-based)
     * @return true if selection was successful
     */
    public boolean selectCandidate(int index) {
        if (!initialized) return false;
        return nativeSelectCandidate(index);
    }

    /**
     * Clear the current preedit/composition.
     */
    public void clearPreedit() {
        if (initialized) {
            nativeClearPreedit();
        }
    }

    /**
     * Reset the engine state.
     */
    public void reset() {
        if (initialized) {
            nativeReset();
        }
    }

    /**
     * Set input mode (Chinese or English).
     */
    public void setInputMode(int mode) {
        if (initialized) {
            nativeSetInputMode(mode);
        }
    }

    /**
     * Get current input mode.
     */
    public int getInputMode() {
        if (!initialized) return MODE_CHINESE;
        return nativeGetInputMode();
    }

    /**
     * Toggle between Chinese and English modes.
     */
    public void toggleInputMode() {
        int current = getInputMode();
        setInputMode(current == MODE_CHINESE ? MODE_ENGLISH : MODE_CHINESE);
    }

    /**
     * Check if in Chinese input mode.
     */
    public boolean isChineseMode() {
        return getInputMode() == MODE_CHINESE;
    }

    /* Copy data files from assets to internal storage */
    private String copyDataFiles(Context context) {
        File dataDir = new File(context.getFilesDir(), "hime_data");
        if (!dataDir.exists()) {
            dataDir.mkdirs();
        }

        try {
            /* Copy pho.tab2 phonetic table */
            copyAssetFile(context, "pho.tab2", new File(dataDir, "pho.tab2"));

            /* Copy other data files as needed */
            copyAssetFile(context, "tsin32.idx", new File(dataDir, "tsin32.idx"));
            copyAssetFile(context, "tsin32", new File(dataDir, "tsin32"));

            return dataDir.getAbsolutePath();

        } catch (IOException e) {
            Log.e(TAG, "Failed to copy data files: " + e.getMessage());
            return null;
        }
    }

    /* Copy a single asset file to destination */
    private void copyAssetFile(Context context, String assetName, File destFile) throws IOException {
        /* Skip if already exists and has content */
        if (destFile.exists() && destFile.length() > 0) {
            return;
        }

        try {
            InputStream in = context.getAssets().open(assetName);
            FileOutputStream out = new FileOutputStream(destFile);

            byte[] buffer = new byte[4096];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }

            in.close();
            out.close();

        } catch (IOException e) {
            /* Asset may not exist, which is OK for optional files */
            Log.w(TAG, "Asset not found: " + assetName);
        }
    }

    /* Native methods */
    private native boolean nativeInit(String dataPath);
    private native void nativeDestroy();
    private native int nativeProcessKey(char keyChar, int modifiers);
    private native String nativeGetPreedit();
    private native String nativeGetCommit();
    private native String[] nativeGetCandidates(int page);
    private native int nativeGetCandidateCount();
    private native boolean nativeSelectCandidate(int index);
    private native void nativeClearPreedit();
    private native void nativeReset();
    private native void nativeSetInputMode(int mode);
    private native int nativeGetInputMode();
}
