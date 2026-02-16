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

    /* Character set constants */
    public static final int CHARSET_TRADITIONAL = 0;
    public static final int CHARSET_SIMPLIFIED = 1;

    /* Candidate style constants */
    public static final int CANDIDATE_STYLE_HORIZONTAL = 0;
    public static final int CANDIDATE_STYLE_VERTICAL = 1;
    public static final int CANDIDATE_STYLE_POPUP = 2;

    /* Color scheme constants */
    public static final int COLOR_SCHEME_LIGHT = 0;
    public static final int COLOR_SCHEME_DARK = 1;
    public static final int COLOR_SCHEME_SYSTEM = 2;

    /* Keyboard layout constants */
    public static final int KB_STANDARD = 0;
    public static final int KB_HSU = 1;
    public static final int KB_ETEN = 2;
    public static final int KB_ETEN26 = 3;
    public static final int KB_IBM = 4;
    public static final int KB_PINYIN = 5;
    public static final int KB_DVORAK = 6;

    /* Feedback type constants */
    public static final int FEEDBACK_KEY_PRESS = 0;
    public static final int FEEDBACK_KEY_DELETE = 1;
    public static final int FEEDBACK_KEY_ENTER = 2;
    public static final int FEEDBACK_KEY_SPACE = 3;
    public static final int FEEDBACK_CANDIDATE = 4;
    public static final int FEEDBACK_MODE_CHANGE = 5;
    public static final int FEEDBACK_ERROR = 6;

    /* Feedback listener interface */
    public interface FeedbackListener {
        void onFeedback(int feedbackType);
    }

    private boolean initialized = false;
    private FeedbackListener feedbackListener = null;

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
        if (!initialized)
            return RESULT_IGNORED;
        return nativeProcessKey(keyChar, modifiers);
    }

    /**
     * Get the current preedit (composition) string.
     * Shows Bopomofo symbols being composed.
     */
    public String getPreedit() {
        if (!initialized)
            return "";
        String preedit = nativeGetPreedit();
        return preedit != null ? preedit : "";
    }

    /**
     * Get the committed text.
     * Returns characters that should be inserted into the text field.
     */
    public String getCommit() {
        if (!initialized)
            return "";
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
        if (!initialized)
            return null;
        return nativeGetCandidates(page);
    }

    /**
     * Get total number of candidates.
     */
    public int getCandidateCount() {
        if (!initialized)
            return 0;
        return nativeGetCandidateCount();
    }

    /**
     * Select a candidate by index.
     *
     * @param index Candidate index (0-based)
     * @return true if selection was successful
     */
    public boolean selectCandidate(int index) {
        if (!initialized)
            return false;
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
        if (!initialized)
            return MODE_CHINESE;
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

    /* ========== Character Set ========== */

    /**
     * Get current character set (Traditional/Simplified).
     */
    public int getCharset() {
        if (!initialized)
            return CHARSET_TRADITIONAL;
        return nativeGetCharset();
    }

    /**
     * Set character set.
     */
    public void setCharset(int charset) {
        if (initialized) {
            nativeSetCharset(charset);
        }
    }

    /**
     * Toggle between Traditional and Simplified.
     */
    public int toggleCharset() {
        if (!initialized)
            return CHARSET_TRADITIONAL;
        return nativeToggleCharset();
    }

    /* ========== Smart Punctuation ========== */

    /**
     * Get smart punctuation enabled state.
     */
    public boolean isSmartPunctuationEnabled() {
        if (!initialized)
            return false;
        return nativeGetSmartPunctuation();
    }

    /**
     * Enable/disable smart punctuation.
     */
    public void setSmartPunctuation(boolean enabled) {
        if (initialized) {
            nativeSetSmartPunctuation(enabled);
        }
    }

    /**
     * Convert ASCII punctuation to Chinese punctuation.
     * @return Converted string, or null if no conversion
     */
    public String convertPunctuation(char ascii) {
        if (!initialized)
            return null;
        return nativeConvertPunctuation(ascii);
    }

    /**
     * Reset punctuation pairing state.
     */
    public void resetPunctuationState() {
        if (initialized) {
            nativeResetPunctuationState();
        }
    }

    /* ========== Pinyin Annotation ========== */

    /**
     * Get Pinyin annotation enabled state.
     */
    public boolean isPinyinAnnotationEnabled() {
        if (!initialized)
            return false;
        return nativeGetPinyinAnnotation();
    }

    /**
     * Enable/disable Pinyin annotation.
     */
    public void setPinyinAnnotation(boolean enabled) {
        if (initialized) {
            nativeSetPinyinAnnotation(enabled);
        }
    }

    /* ========== Candidate Style ========== */

    /**
     * Get candidate display style.
     */
    public int getCandidateStyle() {
        if (!initialized)
            return CANDIDATE_STYLE_HORIZONTAL;
        return nativeGetCandidateStyle();
    }

    /**
     * Set candidate display style.
     */
    public void setCandidateStyle(int style) {
        if (initialized) {
            nativeSetCandidateStyle(style);
        }
    }

    /* ========== Color Scheme ========== */

    /**
     * Get color scheme.
     */
    public int getColorScheme() {
        if (!initialized)
            return COLOR_SCHEME_LIGHT;
        return nativeGetColorScheme();
    }

    /**
     * Set color scheme.
     */
    public void setColorScheme(int scheme) {
        if (initialized) {
            nativeSetColorScheme(scheme);
        }
    }

    /**
     * Notify of system dark mode state.
     */
    public void setSystemDarkMode(boolean isDark) {
        if (initialized) {
            nativeSetSystemDarkMode(isDark);
        }
    }

    /* ========== Keyboard Layout ========== */

    /**
     * Get current keyboard layout.
     */
    public int getKeyboardLayout() {
        if (!initialized)
            return KB_STANDARD;
        return nativeGetKeyboardLayout();
    }

    /**
     * Set keyboard layout.
     * @param layout One of KB_STANDARD, KB_HSU, KB_ETEN, KB_ETEN26, KB_IBM, KB_PINYIN, KB_DVORAK
     * @return 0 on success, -1 on error
     */
    public int setKeyboardLayout(int layout) {
        if (!initialized)
            return -1;
        return nativeSetKeyboardLayout(layout);
    }

    /**
     * Set keyboard layout by name.
     * @param layoutName Layout name ("standard", "hsu", "eten", "eten26", "ibm", "pinyin", "dvorak")
     * @return 0 on success, -1 if layout not found
     */
    public int setKeyboardLayoutByName(String layoutName) {
        if (!initialized || layoutName == null)
            return -1;
        return nativeSetKeyboardLayoutByName(layoutName);
    }

    /* ========== Feedback ========== */

    /**
     * Get sound feedback enabled state.
     */
    public boolean isSoundEnabled() {
        if (!initialized)
            return false;
        return nativeGetSoundEnabled();
    }

    /**
     * Enable/disable sound feedback.
     */
    public void setSoundEnabled(boolean enabled) {
        if (initialized) {
            nativeSetSoundEnabled(enabled);
        }
    }

    /**
     * Get vibration feedback enabled state.
     */
    public boolean isVibrationEnabled() {
        if (!initialized)
            return false;
        return nativeGetVibrationEnabled();
    }

    /**
     * Enable/disable vibration feedback.
     */
    public void setVibrationEnabled(boolean enabled) {
        if (initialized) {
            nativeSetVibrationEnabled(enabled);
        }
    }

    /**
     * Get vibration duration in milliseconds.
     */
    public int getVibrationDuration() {
        if (!initialized)
            return 20;
        return nativeGetVibrationDuration();
    }

    /**
     * Set vibration duration in milliseconds.
     */
    public void setVibrationDuration(int durationMs) {
        if (initialized) {
            nativeSetVibrationDuration(durationMs);
        }
    }

    /**
     * Set feedback listener.
     */
    public void setFeedbackListener(FeedbackListener listener) {
        this.feedbackListener = listener;
    }

    /**
     * Called from native code when feedback event occurs.
     */
    @SuppressWarnings("unused")
    private void onNativeFeedback(int feedbackType) {
        if (feedbackListener != null) {
            feedbackListener.onFeedback(feedbackType);
        }
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

    /* Character set native methods */
    private native int nativeGetCharset();
    private native void nativeSetCharset(int charset);
    private native int nativeToggleCharset();

    /* Smart punctuation native methods */
    private native boolean nativeGetSmartPunctuation();
    private native void nativeSetSmartPunctuation(boolean enabled);
    private native String nativeConvertPunctuation(char ascii);
    private native void nativeResetPunctuationState();

    /* Pinyin annotation native methods */
    private native boolean nativeGetPinyinAnnotation();
    private native void nativeSetPinyinAnnotation(boolean enabled);

    /* Candidate style native methods */
    private native int nativeGetCandidateStyle();
    private native void nativeSetCandidateStyle(int style);

    /* Color scheme native methods */
    private native int nativeGetColorScheme();
    private native void nativeSetColorScheme(int scheme);
    private native void nativeSetSystemDarkMode(boolean isDark);

    /* Feedback native methods */
    private native boolean nativeGetSoundEnabled();
    private native void nativeSetSoundEnabled(boolean enabled);
    private native boolean nativeGetVibrationEnabled();
    private native void nativeSetVibrationEnabled(boolean enabled);
    private native int nativeGetVibrationDuration();
    private native void nativeSetVibrationDuration(int durationMs);

    /* Keyboard layout native methods */
    private native int nativeGetKeyboardLayout();
    private native int nativeSetKeyboardLayout(int layout);
    private native int nativeSetKeyboardLayoutByName(String layoutName);
}
