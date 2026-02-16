/*
 * HIME Android Input Method Service
 *
 * Main IME service handling keyboard input events.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android.ime;

import android.inputmethodservice.InputMethodService;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import org.hime.android.core.HimeEngine;
import org.hime.android.view.HimeCandidateView;
import org.hime.android.view.HimeKeyboardView;

/**
 * HimeInputMethodService is the main entry point for the HIME Android IME.
 * Extends InputMethodService to provide Bopomofo/Zhuyin input functionality.
 */
public class HimeInputMethodService extends InputMethodService implements HimeKeyboardView.KeyboardListener {
    private static final String TAG = "HimeIME";

    private HimeEngine engine;
    private HimeKeyboardView keyboardView;
    private HimeCandidateView candidateView;
    private int currentPage = 0;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");

        engine = new HimeEngine();
        if (!engine.init(this)) {
            Log.e(TAG, "Failed to initialize HIME engine");
        }
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        if (engine != null) {
            engine.destroy();
            engine = null;
        }
        super.onDestroy();
    }

    @Override
    public View onCreateInputView() {
        Log.d(TAG, "onCreateInputView");

        keyboardView = new HimeKeyboardView(this);
        keyboardView.setKeyboardListener(this);
        keyboardView.setChineseMode(engine != null && engine.isChineseMode());

        return keyboardView;
    }

    @Override
    public View onCreateCandidatesView() {
        Log.d(TAG, "onCreateCandidatesView");

        candidateView = new HimeCandidateView(this);
        candidateView.setOnCandidateSelectedListener(this::onCandidateSelected);

        return candidateView;
    }

    @Override
    public void onStartInputView(EditorInfo info, boolean restarting) {
        super.onStartInputView(info, restarting);
        Log.d(TAG, "onStartInputView, restarting=" + restarting);

        if (!restarting && engine != null) {
            engine.reset();
            currentPage = 0;
            updateCandidates();
        }

        if (keyboardView != null) {
            keyboardView.setChineseMode(engine != null && engine.isChineseMode());
        }
    }

    @Override
    public void onFinishInputView(boolean finishingInput) {
        super.onFinishInputView(finishingInput);
        Log.d(TAG, "onFinishInputView");

        if (engine != null) {
            /* Commit any pending preedit */
            String preedit = engine.getPreedit();
            if (preedit != null && !preedit.isEmpty()) {
                commitText(preedit);
                engine.clearPreedit();
            }
        }

        setCandidatesViewShown(false);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        /* Handle hardware keyboard events */
        if (engine != null && engine.isChineseMode()) {
            char c = (char) event.getUnicodeChar();
            if (c != 0 && processKeyInput(c)) {
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    /* HimeKeyboardView.KeyboardListener implementation */

    @Override
    public void onKeyPressed(String key) {
        if (key == null || key.isEmpty())
            return;

        Log.d(TAG, "onKeyPressed: " + key);

        if (engine == null) {
            commitText(key);
            return;
        }

        /* Special keys */
        switch (key) {
            case "DEL":
                handleBackspace();
                return;
            case "ENTER":
                handleEnter();
                return;
            case "SPACE":
                handleSpace();
                return;
            case "MODE":
                toggleMode();
                return;
            case "PREV":
                navigateCandidates(-1);
                return;
            case "NEXT":
                navigateCandidates(1);
                return;
        }

        /* Regular key input */
        if (engine.isChineseMode()) {
            processKeyInput(key.charAt(0));
        } else {
            commitText(key);
        }
    }

    @Override
    public void onLongKeyPressed(String key) {
        /* Long press can be used for alternate characters */
        Log.d(TAG, "onLongKeyPressed: " + key);
    }

    /* Key processing */

    /* Key processing result from hime-core (must match HimeKeyResult enum) */
    private static final int RESULT_PREEDIT = 3;

    private boolean processKeyInput(char c) {
        int result = engine.processKey(c, 0);
        Log.d(TAG, "processKeyInput: char=" + c + " result=" + result);

        switch (result) {
            case HimeEngine.RESULT_COMMIT:
                String commit = engine.getCommit();
                Log.d(TAG, "processKeyInput: commit='" + commit + "'");
                if (commit != null && !commit.isEmpty()) {
                    commitText(commit);
                }
                updatePreedit();
                updateCandidates();
                return true;

            case RESULT_PREEDIT:
                /* Preedit updated (e.g. Bopomofo symbols entered, candidates shown) */
                updatePreedit();
                updateCandidates();
                return true;

            case HimeEngine.RESULT_CONSUMED:
                updatePreedit();
                updateCandidates();
                return true;

            default:
                return false;
        }
    }

    private void handleBackspace() {
        String preedit = engine != null ? engine.getPreedit() : "";

        if (preedit != null && !preedit.isEmpty()) {
            /* Delete from preedit */
            engine.processKey('\b', 0);
            updatePreedit();
            updateCandidates();
        } else {
            /* Delete from text field */
            InputConnection ic = getCurrentInputConnection();
            if (ic != null) {
                ic.deleteSurroundingText(1, 0);
            }
        }
    }

    private void handleEnter() {
        String preedit = engine != null ? engine.getPreedit() : "";

        if (preedit != null && !preedit.isEmpty()) {
            /* Commit preedit as-is */
            commitText(preedit);
            engine.clearPreedit();
            updatePreedit();
            updateCandidates();
        } else {
            /* Send enter key */
            sendDownUpKeyEvents(KeyEvent.KEYCODE_ENTER);
        }
    }

    private void handleSpace() {
        if (engine == null || !engine.isChineseMode()) {
            commitText(" ");
            return;
        }

        /* In Zhuyin, Space is tone-1 key. Process it through the engine first
         * so the tone triggers candidate lookup. */
        int result = engine.processKey(' ', 0);
        Log.d(TAG, "handleSpace: processKey result=" + result);

        if (result == HimeEngine.RESULT_COMMIT) {
            String commit = engine.getCommit();
            Log.d(TAG, "handleSpace: commit='" + commit + "'");
            if (commit != null && !commit.isEmpty()) {
                commitText(commit);
            }
            updatePreedit();
            updateCandidates();
        } else if (result == RESULT_PREEDIT || result == HimeEngine.RESULT_CONSUMED) {
            updatePreedit();
            updateCandidates();
        } else {
            /* Engine didn't handle it — treat as literal space */
            String preedit = engine.getPreedit();
            if (preedit != null && !preedit.isEmpty()) {
                commitText(preedit);
                engine.clearPreedit();
                updatePreedit();
                updateCandidates();
            } else {
                commitText(" ");
            }
        }
    }

    private void toggleMode() {
        if (engine != null) {
            engine.toggleInputMode();

            /* Clear any pending composition when switching modes */
            String preedit = engine.getPreedit();
            if (preedit != null && !preedit.isEmpty()) {
                commitText(preedit);
                engine.clearPreedit();
            }

            updatePreedit();
            updateCandidates();

            if (keyboardView != null) {
                keyboardView.setChineseMode(engine.isChineseMode());
            }
        }
    }

    private void navigateCandidates(int direction) {
        int count = engine != null ? engine.getCandidateCount() : 0;
        int maxPage = (count + HimeEngine.CANDIDATES_PER_PAGE - 1) / HimeEngine.CANDIDATES_PER_PAGE - 1;

        currentPage += direction;
        if (currentPage < 0)
            currentPage = 0;
        if (currentPage > maxPage)
            currentPage = maxPage;

        updateCandidates();
    }

    private void onCandidateSelected(int index) {
        if (engine == null)
            return;

        /* Handle navigation from CandidateView (-1=prev, -2=next) */
        if (index == -1) {
            navigateCandidates(-1);
            return;
        }
        if (index == -2) {
            navigateCandidates(1);
            return;
        }

        int actualIndex = currentPage * HimeEngine.CANDIDATES_PER_PAGE + index;
        Log.d(TAG, "onCandidateSelected: index=" + index + " actual=" + actualIndex);
        if (engine.selectCandidate(actualIndex)) {
            String commit = engine.getCommit();
            Log.d(TAG, "onCandidateSelected: commit='" + commit + "'");
            if (commit != null && !commit.isEmpty()) {
                commitText(commit);
            }
            currentPage = 0;
            updatePreedit();
            updateCandidates();
        }
    }

    /* UI updates */

    private void updatePreedit() {
        if (engine == null)
            return;

        String preedit = engine.getPreedit();
        InputConnection ic = getCurrentInputConnection();

        if (ic != null) {
            /* Show preedit as composing text */
            if (preedit != null && !preedit.isEmpty()) {
                ic.setComposingText(preedit, 1);
            } else {
                ic.finishComposingText();
            }
        }
    }

    private void updateCandidates() {
        if (engine == null || candidateView == null)
            return;

        String[] candidates = engine.getCandidates(currentPage);
        int totalCount = engine.getCandidateCount();

        if (candidates != null && candidates.length > 0) {
            candidateView.setCandidates(candidates, currentPage, totalCount);
            setCandidatesViewShown(true);
        } else {
            candidateView.setCandidates(null, 0, 0);
            setCandidatesViewShown(false);
        }
    }

    private void commitText(String text) {
        InputConnection ic = getCurrentInputConnection();
        if (ic != null && text != null) {
            ic.commitText(text, 1);
        }
    }
}
