/*
 * HIME Android Keyboard View
 *
 * Custom keyboard view with Bopomofo/Zhuyin layout.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

/**
 * HimeKeyboardView renders a custom Bopomofo/Zhuyin keyboard.
 * Provides standard Taiwan phonetic keyboard layout.
 */
public class HimeKeyboardView extends View {
    private static final String TAG = "HimeKeyboardView";

    /* Keyboard dimensions */
    private static final int NUM_ROWS = 5;
    private static final float KEY_HEIGHT_DP = 48f;
    private static final float KEY_PADDING_DP = 2f;
    private static final float KEY_RADIUS_DP = 6f;

    /* Colors */
    private static final int COLOR_BACKGROUND = 0xFF303030;
    private static final int COLOR_KEY_NORMAL = 0xFF505050;
    private static final int COLOR_KEY_PRESSED = 0xFF707070;
    private static final int COLOR_KEY_SPECIAL = 0xFF404080;
    private static final int COLOR_TEXT_PRIMARY = 0xFFFFFFFF;
    private static final int COLOR_TEXT_SECONDARY = 0xFFAAAAAA;
    private static final int COLOR_MODE_INDICATOR = 0xFF00AA00;

    /* Standard Zhuyin keyboard layout (5 rows) */
    /* Row format: main_char|sub_char|key_code */
    private static final String[][] ZHUYIN_LAYOUT = {
            /* Row 1: Numbers/tone marks */
            {"1|ㄅ|1", "2|ㄉ|2", "3|ˇ|3", "4|ˋ|4", "5|ㄓ|5", "6|ˊ|6", "7|˙|7", "8|ㄚ|8", "9|ㄞ|9", "0|ㄢ|0"},

            /* Row 2: QWERTY with Zhuyin */
            {"q|ㄆ|q", "w|ㄊ|w", "e|ㄍ|e", "r|ㄐ|r", "t|ㄔ|t", "y|ㄗ|y", "u|ㄧ|u", "i|ㄛ|i", "o|ㄟ|o", "p|ㄣ|p"},

            /* Row 3: ASDF with Zhuyin */
            {"a|ㄇ|a", "s|ㄋ|s", "d|ㄎ|d", "f|ㄑ|f", "g|ㄕ|g", "h|ㄘ|h", "j|ㄨ|j", "k|ㄜ|k", "l|ㄠ|l", ";|ㄤ|;"},

            /* Row 4: ZXCV with Zhuyin + special keys */
            {"SHIFT||SHIFT", "z|ㄈ|z", "x|ㄌ|x", "c|ㄏ|c", "v|ㄒ|v", "b|ㄖ|b", "n|ㄙ|n", "m|ㄩ|m", ",|ㄝ|,",
                    "DEL||DEL"},

            /* Row 5: Bottom row - mode, space, special */
            {"MODE||MODE", ",||,", "SPACE||SPACE", ".||.", "ENTER||ENTER"}};

    /* English QWERTY layout */
    private static final String[][] ENGLISH_LAYOUT = {
            {"1||1", "2||2", "3||3", "4||4", "5||5", "6||6", "7||7", "8||8", "9||9", "0||0"},
            {"q||q", "w||w", "e||e", "r||r", "t||t", "y||y", "u||u", "i||i", "o||o", "p||p"},
            {"a||a", "s||s", "d||d", "f||f", "g||g", "h||h", "j||j", "k||k", "l||l", "'||'"},
            {"SHIFT||SHIFT", "z||z", "x||x", "c||c", "v||v", "b||b", "n||n", "m||m", "?||?", "DEL||DEL"},
            {"MODE||MODE", ",||,", "SPACE||SPACE", ".||.", "ENTER||ENTER"}};

    /* Paints */
    private Paint backgroundPaint;
    private Paint keyPaint;
    private Paint textPaint;
    private Paint subTextPaint;

    /* Layout state */
    private float keyWidth;
    private float keyHeight;
    private float keyPadding;
    private float keyRadius;
    private float density;

    /* Input state */
    private boolean chineseMode = true;
    private boolean shiftMode = false;
    private int pressedRow = -1;
    private int pressedCol = -1;

    /* Listener */
    private KeyboardListener listener;

    public interface KeyboardListener {
        void onKeyPressed(String key);
        void onLongKeyPressed(String key);
    }

    public HimeKeyboardView(Context context) {
        super(context);
        init(context);
    }

    public HimeKeyboardView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public HimeKeyboardView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    private void init(Context context) {
        density = context.getResources().getDisplayMetrics().density;
        keyHeight = KEY_HEIGHT_DP * density;
        keyPadding = KEY_PADDING_DP * density;
        keyRadius = KEY_RADIUS_DP * density;

        /* Background paint */
        backgroundPaint = new Paint();
        backgroundPaint.setColor(COLOR_BACKGROUND);
        backgroundPaint.setStyle(Paint.Style.FILL);

        /* Key paint */
        keyPaint = new Paint();
        keyPaint.setAntiAlias(true);
        keyPaint.setStyle(Paint.Style.FILL);

        /* Primary text paint */
        textPaint = new Paint();
        textPaint.setAntiAlias(true);
        textPaint.setColor(COLOR_TEXT_PRIMARY);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setTypeface(Typeface.DEFAULT_BOLD);

        /* Secondary text paint (Zhuyin symbols) */
        subTextPaint = new Paint();
        subTextPaint.setAntiAlias(true);
        subTextPaint.setColor(COLOR_TEXT_SECONDARY);
        subTextPaint.setTextAlign(Paint.Align.CENTER);
    }

    public void setKeyboardListener(KeyboardListener listener) {
        this.listener = listener;
    }

    public void setChineseMode(boolean chinese) {
        this.chineseMode = chinese;
        invalidate();
    }

    public boolean isChineseMode() {
        return chineseMode;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = (int) (keyHeight * NUM_ROWS + keyPadding * 2);
        setMeasuredDimension(width, height);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        /* Recalculate key width based on view width */
        keyWidth = (w - keyPadding * 2) / 10f;

        /* Adjust text sizes based on key size */
        textPaint.setTextSize(keyHeight * 0.35f);
        subTextPaint.setTextSize(keyHeight * 0.25f);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        /* Draw background */
        canvas.drawRect(0, 0, getWidth(), getHeight(), backgroundPaint);

        /* Select layout based on mode */
        String[][] layout = chineseMode ? ZHUYIN_LAYOUT : ENGLISH_LAYOUT;

        /* Draw keys */
        for (int row = 0; row < layout.length; row++) {
            drawKeyRow(canvas, layout[row], row);
        }
    }

    private void drawKeyRow(Canvas canvas, String[] keys, int row) {
        float y = keyPadding + row * keyHeight;
        float totalWidth = getWidth() - keyPadding * 2;

        /* Calculate key widths for this row */
        float[] widths = calculateKeyWidths(keys, totalWidth);

        float x = keyPadding;
        for (int col = 0; col < keys.length; col++) {
            String[] parts = keys[col].split("\\|");
            String main = parts[0];
            String sub = parts.length > 1 ? parts[1] : "";
            String code = parts.length > 2 ? parts[2] : main;

            /* Determine key color */
            int keyColor = COLOR_KEY_NORMAL;
            if (row == pressedRow && col == pressedCol) {
                keyColor = COLOR_KEY_PRESSED;
            } else if (isSpecialKey(main)) {
                keyColor = COLOR_KEY_SPECIAL;
            }

            /* Draw key background */
            RectF keyRect =
                    new RectF(x + keyPadding, y + keyPadding, x + widths[col] - keyPadding, y + keyHeight - keyPadding);
            keyPaint.setColor(keyColor);
            canvas.drawRoundRect(keyRect, keyRadius, keyRadius, keyPaint);

            /* Draw key text */
            drawKeyText(canvas, main, sub, keyRect);

            /* Draw mode indicator for MODE key */
            if (main.equals("MODE")) {
                drawModeIndicator(canvas, keyRect);
            }

            x += widths[col];
        }
    }

    private float[] calculateKeyWidths(String[] keys, float totalWidth) {
        float[] widths = new float[keys.length];
        float standardWidth = totalWidth / 10f;

        /* Special key widths */
        float remaining = totalWidth;
        int standardKeys = 0;

        for (int i = 0; i < keys.length; i++) {
            String main = keys[i].split("\\|")[0];

            if (main.equals("SPACE")) {
                widths[i] = standardWidth * 4; /* Space is 4x */
                remaining -= widths[i];
            } else if (main.equals("SHIFT") || main.equals("DEL") || main.equals("ENTER")) {
                widths[i] = standardWidth * 1.5f; /* 1.5x */
                remaining -= widths[i];
            } else if (main.equals("MODE")) {
                widths[i] = standardWidth * 1.5f;
                remaining -= widths[i];
            } else {
                standardKeys++;
            }
        }

        /* Distribute remaining width to standard keys */
        float stdWidth = standardKeys > 0 ? remaining / standardKeys : standardWidth;
        for (int i = 0; i < keys.length; i++) {
            if (widths[i] == 0) {
                widths[i] = stdWidth;
            }
        }

        return widths;
    }

    private void drawKeyText(Canvas canvas, String main, String sub, RectF rect) {
        float centerX = rect.centerX();
        float centerY = rect.centerY();

        /* Handle special keys */
        String displayMain = getDisplayText(main);

        if (sub != null && !sub.isEmpty() && chineseMode) {
            /* Two-line layout: main on top, Zhuyin below */
            float mainY = centerY - keyHeight * 0.08f;
            float subY = centerY + keyHeight * 0.22f;

            canvas.drawText(displayMain, centerX, mainY, textPaint);
            canvas.drawText(sub, centerX, subY, subTextPaint);
        } else {
            /* Single centered text */
            float textY = centerY + textPaint.getTextSize() * 0.35f;
            canvas.drawText(displayMain, centerX, textY, textPaint);
        }
    }

    private String getDisplayText(String key) {
        switch (key) {
            case "DEL":
                return "⌫";
            case "ENTER":
                return "↵";
            case "SPACE":
                return "␣";
            case "SHIFT":
                return shiftMode ? "⇧" : "⇪";
            case "MODE":
                return chineseMode ? "中" : "EN";
            default:
                if (shiftMode && key.length() == 1 && Character.isLetter(key.charAt(0))) {
                    return key.toUpperCase();
                }
                return key;
        }
    }

    private void drawModeIndicator(Canvas canvas, RectF rect) {
        /* Draw small indicator dot */
        Paint indicatorPaint = new Paint();
        indicatorPaint.setColor(chineseMode ? COLOR_MODE_INDICATOR : COLOR_TEXT_SECONDARY);
        indicatorPaint.setAntiAlias(true);

        float radius = keyHeight * 0.06f;
        float x = rect.right - radius * 3;
        float y = rect.top + radius * 3;
        canvas.drawCircle(x, y, radius, indicatorPaint);
    }

    private boolean isSpecialKey(String key) {
        return key.equals("DEL") || key.equals("ENTER") || key.equals("SPACE") || key.equals("SHIFT")
                || key.equals("MODE") || key.equals("PREV") || key.equals("NEXT");
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                int[] pos = findKeyAt(x, y);
                if (pos != null) {
                    pressedRow = pos[0];
                    pressedCol = pos[1];
                    invalidate();
                }
                return true;

            case MotionEvent.ACTION_MOVE:
                /* Track finger movement */
                int[] newPos = findKeyAt(x, y);
                if (newPos == null || newPos[0] != pressedRow || newPos[1] != pressedCol) {
                    if (newPos != null) {
                        pressedRow = newPos[0];
                        pressedCol = newPos[1];
                    } else {
                        pressedRow = -1;
                        pressedCol = -1;
                    }
                    invalidate();
                }
                return true;

            case MotionEvent.ACTION_UP:
                if (pressedRow >= 0 && pressedCol >= 0) {
                    handleKeyPress(pressedRow, pressedCol);
                }
                pressedRow = -1;
                pressedCol = -1;
                invalidate();
                return true;

            case MotionEvent.ACTION_CANCEL:
                pressedRow = -1;
                pressedCol = -1;
                invalidate();
                return true;
        }

        return super.onTouchEvent(event);
    }

    private int[] findKeyAt(float x, float y) {
        String[][] layout = chineseMode ? ZHUYIN_LAYOUT : ENGLISH_LAYOUT;
        float totalWidth = getWidth() - keyPadding * 2;

        int row = (int) ((y - keyPadding) / keyHeight);
        if (row < 0 || row >= layout.length)
            return null;

        float[] widths = calculateKeyWidths(layout[row], totalWidth);
        float keyX = keyPadding;

        for (int col = 0; col < layout[row].length; col++) {
            if (x >= keyX && x < keyX + widths[col]) {
                return new int[] {row, col};
            }
            keyX += widths[col];
        }

        return null;
    }

    private void handleKeyPress(int row, int col) {
        String[][] layout = chineseMode ? ZHUYIN_LAYOUT : ENGLISH_LAYOUT;
        if (row >= layout.length || col >= layout[row].length)
            return;

        String[] parts = layout[row][col].split("\\|");
        String code = parts.length > 2 ? parts[2] : parts[0];

        /* Handle SHIFT locally */
        if (code.equals("SHIFT")) {
            shiftMode = !shiftMode;
            invalidate();
            return;
        }

        /* Apply shift to letters */
        if (shiftMode && code.length() == 1 && Character.isLetter(code.charAt(0))) {
            code = code.toUpperCase();
            shiftMode = false;
            invalidate();
        }

        /* Notify listener */
        if (listener != null) {
            listener.onKeyPressed(code);
        }
    }
}
