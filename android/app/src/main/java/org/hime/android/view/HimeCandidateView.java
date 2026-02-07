/*
 * HIME Android Candidate View
 *
 * Displays candidate characters for selection.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

package org.hime.android.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

/**
 * HimeCandidateView displays character candidates for selection.
 * Shows a horizontal scrollable list of candidate characters.
 */
public class HimeCandidateView extends View {
    private static final String TAG = "HimeCandidateView";

    /* View dimensions */
    private static final float HEIGHT_DP = 44f;
    private static final float CANDIDATE_PADDING_DP = 12f;
    private static final float CANDIDATE_MIN_WIDTH_DP = 40f;
    private static final float NAV_BUTTON_WIDTH_DP = 36f;

    /* Colors */
    private static final int COLOR_BACKGROUND = 0xFF404040;
    private static final int COLOR_CANDIDATE = 0xFF505050;
    private static final int COLOR_CANDIDATE_PRESSED = 0xFF606060;
    private static final int COLOR_TEXT = 0xFFFFFFFF;
    private static final int COLOR_PAGE_INFO = 0xFFAAAAAA;
    private static final int COLOR_NAV_BUTTON = 0xFF606060;

    /* Paints */
    private Paint backgroundPaint;
    private Paint candidatePaint;
    private Paint textPaint;
    private Paint pageInfoPaint;
    private Paint navButtonPaint;

    /* Dimensions */
    private float density;
    private float height;
    private float candidatePadding;
    private float candidateMinWidth;
    private float navButtonWidth;

    /* Data */
    private String[] candidates;
    private int currentPage;
    private int totalCount;
    private int pressedIndex = -1;

    /* Calculated layout */
    private float[] candidatePositions;
    private float[] candidateWidths;

    /* Listener */
    private OnCandidateSelectedListener listener;

    public interface OnCandidateSelectedListener {
        void onCandidateSelected(int index);
    }

    public HimeCandidateView(Context context) {
        super(context);
        init(context);
    }

    public HimeCandidateView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public HimeCandidateView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    private void init(Context context) {
        density = context.getResources().getDisplayMetrics().density;
        height = HEIGHT_DP * density;
        candidatePadding = CANDIDATE_PADDING_DP * density;
        candidateMinWidth = CANDIDATE_MIN_WIDTH_DP * density;
        navButtonWidth = NAV_BUTTON_WIDTH_DP * density;

        /* Background paint */
        backgroundPaint = new Paint();
        backgroundPaint.setColor(COLOR_BACKGROUND);
        backgroundPaint.setStyle(Paint.Style.FILL);

        /* Candidate background paint */
        candidatePaint = new Paint();
        candidatePaint.setAntiAlias(true);
        candidatePaint.setStyle(Paint.Style.FILL);

        /* Text paint */
        textPaint = new Paint();
        textPaint.setAntiAlias(true);
        textPaint.setColor(COLOR_TEXT);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setTextSize(height * 0.45f);

        /* Page info paint */
        pageInfoPaint = new Paint();
        pageInfoPaint.setAntiAlias(true);
        pageInfoPaint.setColor(COLOR_PAGE_INFO);
        pageInfoPaint.setTextAlign(Paint.Align.CENTER);
        pageInfoPaint.setTextSize(height * 0.3f);

        /* Navigation button paint */
        navButtonPaint = new Paint();
        navButtonPaint.setAntiAlias(true);
        navButtonPaint.setColor(COLOR_NAV_BUTTON);
        navButtonPaint.setStyle(Paint.Style.FILL);
    }

    public void setOnCandidateSelectedListener(OnCandidateSelectedListener listener) {
        this.listener = listener;
    }

    public void setCandidates(String[] candidates, int page, int totalCount) {
        this.candidates = candidates;
        this.currentPage = page;
        this.totalCount = totalCount;
        this.pressedIndex = -1;

        calculateLayout();
        invalidate();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        setMeasuredDimension(width, (int) height);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        calculateLayout();
    }

    private void calculateLayout() {
        if (candidates == null || candidates.length == 0) {
            candidatePositions = null;
            candidateWidths = null;
            return;
        }

        int count = candidates.length;
        candidatePositions = new float[count];
        candidateWidths = new float[count];

        /* Calculate width for each candidate */
        float totalWidth = getWidth() - navButtonWidth * 2; /* Leave space for nav buttons */
        float x = navButtonWidth;

        for (int i = 0; i < count; i++) {
            /* Measure text width */
            float textWidth = textPaint.measureText(candidates[i]);
            float width = Math.max(textWidth + candidatePadding * 2, candidateMinWidth);

            candidatePositions[i] = x;
            candidateWidths[i] = width;
            x += width;

            /* If we exceed the available width, scale down */
            if (x > getWidth() - navButtonWidth && i < count - 1) {
                /* Recalculate with equal widths */
                float equalWidth = totalWidth / count;
                x = navButtonWidth;
                for (int j = 0; j < count; j++) {
                    candidatePositions[j] = x;
                    candidateWidths[j] = equalWidth;
                    x += equalWidth;
                }
                break;
            }
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        /* Draw background */
        canvas.drawRect(0, 0, getWidth(), getHeight(), backgroundPaint);

        if (candidates == null || candidates.length == 0) {
            return;
        }

        /* Draw navigation buttons */
        drawNavButton(canvas, true);  /* Previous */
        drawNavButton(canvas, false); /* Next */

        /* Draw candidates */
        for (int i = 0; i < candidates.length; i++) {
            drawCandidate(canvas, i);
        }

        /* Draw page info */
        drawPageInfo(canvas);
    }

    private void drawNavButton(Canvas canvas, boolean isPrev) {
        float x = isPrev ? 0 : getWidth() - navButtonWidth;
        float padding = height * 0.15f;

        RectF rect = new RectF(x + padding, padding, x + navButtonWidth - padding, height - padding);

        navButtonPaint.setColor(COLOR_NAV_BUTTON);
        canvas.drawRoundRect(rect, height * 0.1f, height * 0.1f, navButtonPaint);

        /* Draw arrow */
        textPaint.setColor(COLOR_TEXT);
        String arrow = isPrev ? "◀" : "▶";
        float textY = height / 2 + textPaint.getTextSize() * 0.35f;
        canvas.drawText(arrow, x + navButtonWidth / 2, textY, textPaint);
    }

    private void drawCandidate(Canvas canvas, int index) {
        if (candidatePositions == null || index >= candidatePositions.length)
            return;

        float x = candidatePositions[index];
        float width = candidateWidths[index];
        float padding = height * 0.1f;

        RectF rect = new RectF(x + 2, padding, x + width - 2, height - padding);

        /* Draw background */
        int color = (index == pressedIndex) ? COLOR_CANDIDATE_PRESSED : COLOR_CANDIDATE;
        candidatePaint.setColor(color);
        canvas.drawRoundRect(rect, height * 0.1f, height * 0.1f, candidatePaint);

        /* Draw text */
        textPaint.setColor(COLOR_TEXT);
        float textY = height / 2 + textPaint.getTextSize() * 0.35f;
        canvas.drawText(candidates[index], x + width / 2, textY, textPaint);

        /* Draw selection number (1-9, 0) */
        String numStr = String.valueOf((index + 1) % 10);
        float numX = x + width - candidatePadding;
        float numY = padding + pageInfoPaint.getTextSize();
        pageInfoPaint.setTextAlign(Paint.Align.RIGHT);
        canvas.drawText(numStr, numX, numY, pageInfoPaint);
        pageInfoPaint.setTextAlign(Paint.Align.CENTER);
    }

    private void drawPageInfo(Canvas canvas) {
        if (totalCount <= 0)
            return;

        int totalPages = (totalCount + 9) / 10; /* 10 candidates per page */
        String info = String.format("%d/%d", currentPage + 1, totalPages);

        /* Draw in the center-top area */
        float x = getWidth() / 2;
        float y = height * 0.25f;
        canvas.drawText(info, x, y, pageInfoPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                pressedIndex = findCandidateAt(x);
                invalidate();
                return true;

            case MotionEvent.ACTION_MOVE:
                int newIndex = findCandidateAt(x);
                if (newIndex != pressedIndex) {
                    pressedIndex = newIndex;
                    invalidate();
                }
                return true;

            case MotionEvent.ACTION_UP:
                /* Check for nav button press */
                if (x < navButtonWidth) {
                    /* Previous page - handled by IME service */
                    if (listener != null) {
                        /* Signal previous with negative index */
                        listener.onCandidateSelected(-1);
                    }
                } else if (x > getWidth() - navButtonWidth) {
                    /* Next page */
                    if (listener != null) {
                        listener.onCandidateSelected(-2);
                    }
                } else if (pressedIndex >= 0 && listener != null) {
                    listener.onCandidateSelected(pressedIndex);
                }
                pressedIndex = -1;
                invalidate();
                return true;

            case MotionEvent.ACTION_CANCEL:
                pressedIndex = -1;
                invalidate();
                return true;
        }

        return super.onTouchEvent(event);
    }

    private int findCandidateAt(float x) {
        if (candidatePositions == null)
            return -1;

        /* Skip if in nav button areas */
        if (x < navButtonWidth || x > getWidth() - navButtonWidth) {
            return -1;
        }

        for (int i = 0; i < candidatePositions.length; i++) {
            float pos = candidatePositions[i];
            float width = candidateWidths[i];
            if (x >= pos && x < pos + width) {
                return i;
            }
        }

        return -1;
    }
}
