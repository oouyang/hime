/*
 * HIME Android Typing Practice Activity
 *
 * Provides a typing practice interface for practicing input methods.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextWatcher;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.view.Gravity;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import org.hime.android.core.HimeEngine;

/**
 * TypingPracticeActivity provides a typing practice interface.
 */
public class TypingPracticeActivity extends Activity {
    /* Practice categories */
    public static final int CATEGORY_ENGLISH = 0;
    public static final int CATEGORY_ZHUYIN = 1;
    public static final int CATEGORY_PINYIN = 2;
    public static final int CATEGORY_CANGJIE = 3;
    public static final int CATEGORY_MIXED = 4;

    /* Practice difficulties */
    public static final int DIFFICULTY_EASY = 1;
    public static final int DIFFICULTY_MEDIUM = 2;
    public static final int DIFFICULTY_HARD = 3;

    /* Built-in practice texts */
    private static final String[][] PRACTICE_TEXTS = {
            /* English - Easy */
            {"The quick brown fox jumps over the lazy dog.", ""}, {"Pack my box with five dozen liquor jugs.", ""},
            {"How vexingly quick daft zebras jump!", ""},

            /* English - Medium */
            {"Programming is the art of telling a computer what to do.", ""},
            {"To be or not to be, that is the question.", ""},

            /* Traditional Chinese */
            {"你好嗎？", "ni3 hao3 ma"}, {"謝謝你。", "xie4 xie4 ni3"},
            {"今天天氣很好。", "jin1 tian1 tian1 qi4 hen3 hao3"},
            {"我喜歡學習中文。", "wo3 xi3 huan1 xue2 xi2 zhong1 wen2"},

            /* Simplified Chinese */
            {"你好吗？", "ni hao ma"}, {"谢谢你。", "xie xie ni"}, {"今天天气很好。", "jin tian tian qi hen hao"},

            /* Mixed */
            {"Hello 你好 World 世界", ""}, {"Programming 程式設計 is fun 很有趣", ""}};

    /* UI Components */
    private Spinner categorySpinner;
    private Spinner difficultySpinner;
    private TextView practiceTextView;
    private TextView hintTextView;
    private ProgressBar progressBar;
    private EditText inputField;
    private TextView statsTextView;
    private Button newTextButton;
    private Button resetButton;

    /* State */
    private String currentPracticeText = "";
    private String currentHint = "";
    private int currentPosition = 0;
    private int totalCharacters = 0;
    private int correctCount = 0;
    private int incorrectCount = 0;
    private long startTimeMs = 0;
    private boolean sessionActive = false;

    /* Engine */
    private HimeEngine engine;

    /* Handler for UI updates */
    private Handler handler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        /* Initialize engine */
        engine = new HimeEngine();
        engine.init(this);

        setupUI();
        loadRandomPracticeText();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (engine != null) {
            engine.destroy();
        }
    }

    private void setupUI() {
        ScrollView scrollView = new ScrollView(this);
        scrollView.setBackgroundColor(0xFF1E1E1E);

        LinearLayout mainLayout = new LinearLayout(this);
        mainLayout.setOrientation(LinearLayout.VERTICAL);
        mainLayout.setPadding(32, 32, 32, 32);

        /* Title */
        TextView title = new TextView(this);
        title.setText("Typing Practice / 打字練習");
        title.setTextSize(24);
        title.setTextColor(Color.WHITE);
        title.setTypeface(null, Typeface.BOLD);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, 0, 0, 32);
        mainLayout.addView(title);

        /* Category selector */
        TextView categoryLabel = new TextView(this);
        categoryLabel.setText("Category / 類別:");
        categoryLabel.setTextColor(0xFFAAAAAA);
        mainLayout.addView(categoryLabel);

        categorySpinner = new Spinner(this);
        ArrayAdapter<String> categoryAdapter = new ArrayAdapter<>(
                this, android.R.layout.simple_spinner_item, new String[] {"English", "注音", "拼音", "倉頡", "Mixed"});
        categoryAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        categorySpinner.setAdapter(categoryAdapter);
        categorySpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                loadRandomPracticeText();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
        mainLayout.addView(categorySpinner);

        /* Difficulty selector */
        TextView difficultyLabel = new TextView(this);
        difficultyLabel.setText("Difficulty / 難度:");
        difficultyLabel.setTextColor(0xFFAAAAAA);
        difficultyLabel.setPadding(0, 24, 0, 0);
        mainLayout.addView(difficultyLabel);

        difficultySpinner = new Spinner(this);
        ArrayAdapter<String> difficultyAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item,
                new String[] {"Easy / 簡單", "Medium / 中等", "Hard / 困難"});
        difficultyAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        difficultySpinner.setAdapter(difficultyAdapter);
        difficultySpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                loadRandomPracticeText();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
        mainLayout.addView(difficultySpinner);

        /* Practice text label */
        TextView practiceLabel = new TextView(this);
        practiceLabel.setText("Type this / 請輸入:");
        practiceLabel.setTextColor(0xFFAAAAAA);
        practiceLabel.setPadding(0, 32, 0, 8);
        mainLayout.addView(practiceLabel);

        /* Practice text display */
        practiceTextView = new TextView(this);
        practiceTextView.setTextSize(24);
        practiceTextView.setTextColor(Color.WHITE);
        practiceTextView.setGravity(Gravity.CENTER);
        practiceTextView.setBackgroundColor(0xFF2D2D2D);
        practiceTextView.setPadding(24, 24, 24, 24);
        practiceTextView.setMinHeight(120);
        mainLayout.addView(practiceTextView);

        /* Hint */
        hintTextView = new TextView(this);
        hintTextView.setTextSize(14);
        hintTextView.setTextColor(0xFF888888);
        hintTextView.setGravity(Gravity.CENTER);
        hintTextView.setPadding(0, 8, 0, 16);
        mainLayout.addView(hintTextView);

        /* Progress bar */
        progressBar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        progressBar.setMax(100);
        progressBar.setProgress(0);
        LinearLayout.LayoutParams progressParams =
                new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 16);
        progressParams.bottomMargin = 16;
        mainLayout.addView(progressBar, progressParams);

        /* Input field */
        inputField = new EditText(this);
        inputField.setHint("Start typing here... / 在這裡輸入...");
        inputField.setTextSize(20);
        inputField.setTextColor(Color.WHITE);
        inputField.setHintTextColor(0xFF666666);
        inputField.setBackgroundColor(0xFF3D3D3D);
        inputField.setPadding(24, 24, 24, 24);
        inputField.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                if (count > 0 && sessionActive) {
                    processInput(s.toString());
                }
            }
            @Override
            public void afterTextChanged(Editable s) {}
        });
        mainLayout.addView(inputField);

        /* Stats display */
        statsTextView = new TextView(this);
        statsTextView.setTextSize(14);
        statsTextView.setTextColor(0xFFAAAAAA);
        statsTextView.setGravity(Gravity.CENTER);
        statsTextView.setPadding(0, 16, 0, 24);
        statsTextView.setTypeface(Typeface.MONOSPACE);
        mainLayout.addView(statsTextView);

        /* Button row */
        LinearLayout buttonRow = new LinearLayout(this);
        buttonRow.setOrientation(LinearLayout.HORIZONTAL);
        buttonRow.setGravity(Gravity.CENTER);

        newTextButton = new Button(this);
        newTextButton.setText("New Text / 換題");
        newTextButton.setOnClickListener(v -> loadRandomPracticeText());
        LinearLayout.LayoutParams btnParams =
                new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1);
        btnParams.rightMargin = 8;
        buttonRow.addView(newTextButton, btnParams);

        resetButton = new Button(this);
        resetButton.setText("Reset / 重置");
        resetButton.setOnClickListener(v -> resetSession());
        LinearLayout.LayoutParams resetParams =
                new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1);
        resetParams.leftMargin = 8;
        buttonRow.addView(resetButton, resetParams);

        mainLayout.addView(buttonRow);

        scrollView.addView(mainLayout);
        setContentView(scrollView);
    }

    private void loadRandomPracticeText() {
        int category = categorySpinner.getSelectedItemPosition();

        /* Select a random text based on category */
        int startIdx = 0;
        int endIdx = PRACTICE_TEXTS.length;

        switch (category) {
            case CATEGORY_ENGLISH:
                startIdx = 0;
                endIdx = 5;
                break;
            case CATEGORY_ZHUYIN:
            case CATEGORY_CANGJIE:
                startIdx = 5;
                endIdx = 9;
                break;
            case CATEGORY_PINYIN:
                startIdx = 9;
                endIdx = 12;
                break;
            case CATEGORY_MIXED:
                startIdx = 12;
                endIdx = 14;
                break;
        }

        int idx = startIdx + (int) (Math.random() * (endIdx - startIdx));
        if (idx >= PRACTICE_TEXTS.length) {
            idx = 0;
        }

        currentPracticeText = PRACTICE_TEXTS[idx][0];
        currentHint = PRACTICE_TEXTS[idx][1];
        totalCharacters = currentPracticeText.length();

        startNewSession();
    }

    private void startNewSession() {
        currentPosition = 0;
        correctCount = 0;
        incorrectCount = 0;
        startTimeMs = System.currentTimeMillis();
        sessionActive = true;

        inputField.setText("");
        progressBar.setProgress(0);

        updatePracticeTextDisplay();
        updateStats();
    }

    private void resetSession() {
        currentPosition = 0;
        correctCount = 0;
        incorrectCount = 0;
        startTimeMs = System.currentTimeMillis();

        inputField.setText("");
        progressBar.setProgress(0);

        updatePracticeTextDisplay();
        updateStats();
    }

    private void processInput(String text) {
        if (text.length() == 0 || currentPosition >= totalCharacters) {
            return;
        }

        /* Get the last character typed */
        String lastChar = text.substring(text.length() - 1);

        /* Get expected character */
        String expectedChar = currentPracticeText.substring(currentPosition, currentPosition + 1);

        if (lastChar.equals(expectedChar)) {
            correctCount++;
        } else {
            incorrectCount++;
        }

        currentPosition++;
        updatePracticeTextDisplay();
        updateStats();

        /* Check completion */
        if (currentPosition >= totalCharacters) {
            sessionActive = false;
            showCompletionDialog();
        }
    }

    private void updatePracticeTextDisplay() {
        SpannableString spannable = new SpannableString(currentPracticeText);

        /* Highlight typed portion in green */
        if (currentPosition > 0) {
            spannable.setSpan(new ForegroundColorSpan(0xFF4CAF50), 0, Math.min(currentPosition, totalCharacters),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }

        /* Highlight current character with yellow background */
        if (currentPosition < totalCharacters) {
            spannable.setSpan(new BackgroundColorSpan(0xFFFFEB3B), currentPosition, currentPosition + 1,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            spannable.setSpan(new ForegroundColorSpan(Color.BLACK), currentPosition, currentPosition + 1,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }

        practiceTextView.setText(spannable);

        if (currentHint.length() > 0) {
            hintTextView.setText("Hint: " + currentHint);
            hintTextView.setVisibility(View.VISIBLE);
        } else {
            hintTextView.setVisibility(View.GONE);
        }

        /* Update progress */
        if (totalCharacters > 0) {
            progressBar.setProgress(currentPosition * 100 / totalCharacters);
        }
    }

    private void updateStats() {
        long elapsedMs = System.currentTimeMillis() - startTimeMs;
        double elapsedMin = elapsedMs / 60000.0;

        double accuracy = 100.0;
        if (correctCount + incorrectCount > 0) {
            accuracy = (double) correctCount / (correctCount + incorrectCount) * 100.0;
        }

        double cpm = 0;
        if (elapsedMin > 0) {
            cpm = correctCount / elapsedMin;
        }

        statsTextView.setText(String.format("Progress: %d/%d | Correct: %d | Incorrect: %d\n"
                        + "Accuracy: %.1f%% | Speed: %.1f chars/min",
                currentPosition, totalCharacters, correctCount, incorrectCount, accuracy, cpm));
    }

    private void showCompletionDialog() {
        long elapsedMs = System.currentTimeMillis() - startTimeMs;
        double accuracy = 100.0;
        if (correctCount + incorrectCount > 0) {
            accuracy = (double) correctCount / (correctCount + incorrectCount) * 100.0;
        }
        double cpm = correctCount / (elapsedMs / 60000.0);

        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
        builder.setTitle("Completed! / 完成!");
        builder.setMessage(String.format(
                "Accuracy: %.1f%%\nSpeed: %.1f chars/min\nTime: %.1f seconds", accuracy, cpm, elapsedMs / 1000.0));
        builder.setPositiveButton("New Text / 換題", (dialog, which) -> loadRandomPracticeText());
        builder.setNegativeButton("OK", null);
        builder.show();
    }
}
