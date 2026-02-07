/*
 * HIME Windows Typing Practice UI Implementation
 *
 * Win32 dialog-based typing practice interface.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <commctrl.h>

#include "../../shared/include/hime-core.h"
#include "hime-typing-ui.h"

/* Control IDs */
#define IDC_CATEGORY_COMBO 1001
#define IDC_DIFFICULTY_COMBO 1002
#define IDC_PRACTICE_TEXT 1003
#define IDC_HINT_LABEL 1004
#define IDC_PROGRESS_BAR 1005
#define IDC_INPUT_EDIT 1006
#define IDC_STATS_LABEL 1007
#define IDC_NEW_TEXT_BTN 1008
#define IDC_RESET_BTN 1009

/* Window class name */
static const wchar_t *TYPING_WND_CLASS = L"HIMETypingPractice";

/* Instance handle */
static HINSTANCE g_hInstance = NULL;

/* Dialog state */
typedef struct {
    HimeContext *ctx;
    wchar_t practice_text[512];
    wchar_t hint[256];
    int current_position;
    int total_characters;
    int correct_count;
    int incorrect_count;
    DWORD start_time;
    BOOL session_active;

    /* Control handles */
    HWND hwndCategoryCombo;
    HWND hwndDifficultyCombo;
    HWND hwndPracticeText;
    HWND hwndHintLabel;
    HWND hwndProgressBar;
    HWND hwndInputEdit;
    HWND hwndStatsLabel;
    HWND hwndNewTextBtn;
    HWND hwndResetBtn;
} TypingDialogState;

/* Built-in practice texts */
static const wchar_t *PRACTICE_TEXTS[] = {
    /* English - Easy */
    L"The quick brown fox jumps over the lazy dog.",
    L"Pack my box with five dozen liquor jugs.",
    L"How vexingly quick daft zebras jump!",

    /* Traditional Chinese */
    L"\x4F60\x597D\x55CE\xFF1F",                   /* 你好嗎？ */
    L"\x8B1D\x8B1D\x4F60\x3002",                   /* 謝謝你。 */
    L"\x4ECA\x5929\x5929\x6C23\x5F88\x597D\x3002", /* 今天天氣很好。 */

    /* Simplified Chinese */
    L"\x4F60\x597D\x5417\xFF1F", /* 你好吗？ */
    L"\x8C22\x8C22\x4F60\x3002", /* 谢谢你。 */

    /* Mixed */
    L"Hello \x4F60\x597D World \x4E16\x754C", /* Hello 你好 World 世界 */
    NULL};

static const wchar_t *PRACTICE_HINTS[] = {
    L"",
    L"",
    L"",
    L"ni3 hao3 ma",
    L"xie4 xie4 ni3",
    L"jin1 tian1 tian1 qi4 hen3 hao3",
    L"ni hao ma",
    L"xie xie ni",
    L"",
    NULL};

/* Count UTF-16 characters */
static int utf16_char_count (const wchar_t *str) {
    if (!str)
        return 0;
    return (int) wcslen (str);
}

/* Load random practice text */
static void load_random_text (TypingDialogState *state, int category) {
    int start_idx = 0;
    int end_idx = 9;

    switch (category) {
    case 0: /* English */
        start_idx = 0;
        end_idx = 3;
        break;
    case 1: /* Zhuyin */
    case 3: /* Cangjie */
        start_idx = 3;
        end_idx = 6;
        break;
    case 2: /* Pinyin */
        start_idx = 6;
        end_idx = 8;
        break;
    case 4: /* Mixed */
        start_idx = 8;
        end_idx = 9;
        break;
    }

    int idx = start_idx + (rand () % (end_idx - start_idx));
    if (idx >= 9)
        idx = 0;

    wcscpy (state->practice_text, PRACTICE_TEXTS[idx]);
    wcscpy (state->hint, PRACTICE_HINTS[idx]);
    state->total_characters = utf16_char_count (state->practice_text);
}

/* Start new session */
static void start_new_session (HWND hwnd, TypingDialogState *state) {
    state->current_position = 0;
    state->correct_count = 0;
    state->incorrect_count = 0;
    state->start_time = GetTickCount ();
    state->session_active = TRUE;

    /* Clear input */
    SetWindowTextW (state->hwndInputEdit, L"");

    /* Reset progress bar */
    SendMessage (state->hwndProgressBar, PBM_SETPOS, 0, 0);

    /* Update display */
    SetWindowTextW (state->hwndPracticeText, state->practice_text);

    if (wcslen (state->hint) > 0) {
        wchar_t hint_text[300];
        swprintf (hint_text, 300, L"Hint: %s", state->hint);
        SetWindowTextW (state->hwndHintLabel, hint_text);
    } else {
        SetWindowTextW (state->hwndHintLabel, L"");
    }

    SetWindowTextW (state->hwndStatsLabel, L"Progress: 0/0 | Accuracy: 100%");
}

/* Reset session */
static void reset_session (HWND hwnd, TypingDialogState *state) {
    state->current_position = 0;
    state->correct_count = 0;
    state->incorrect_count = 0;
    state->start_time = GetTickCount ();

    SetWindowTextW (state->hwndInputEdit, L"");
    SendMessage (state->hwndProgressBar, PBM_SETPOS, 0, 0);
    SetWindowTextW (state->hwndStatsLabel, L"Progress: 0/0 | Accuracy: 100%");
}

/* Update statistics display */
static void update_stats (TypingDialogState *state) {
    DWORD elapsed_ms = GetTickCount () - state->start_time;
    double elapsed_min = elapsed_ms / 60000.0;

    double accuracy = 100.0;
    if (state->correct_count + state->incorrect_count > 0) {
        accuracy = (double) state->correct_count /
                   (state->correct_count + state->incorrect_count) * 100.0;
    }

    double cpm = 0;
    if (elapsed_min > 0) {
        cpm = state->correct_count / elapsed_min;
    }

    wchar_t stats[256];
    swprintf (stats, 256,
              L"Progress: %d/%d | Correct: %d | Incorrect: %d\n"
              L"Accuracy: %.1f%% | Speed: %.1f chars/min",
              state->current_position, state->total_characters,
              state->correct_count, state->incorrect_count, accuracy, cpm);
    SetWindowTextW (state->hwndStatsLabel, stats);

    /* Update progress bar */
    if (state->total_characters > 0) {
        int progress = state->current_position * 100 / state->total_characters;
        SendMessage (state->hwndProgressBar, PBM_SETPOS, progress, 0);
    }
}

/* Process typed character */
static void process_input (HWND hwnd, TypingDialogState *state) {
    if (!state->session_active)
        return;
    if (state->current_position >= state->total_characters)
        return;

    wchar_t input[512];
    GetWindowTextW (state->hwndInputEdit, input, 512);
    int input_len = (int) wcslen (input);

    if (input_len == 0)
        return;

    /* Get last character */
    wchar_t typed = input[input_len - 1];
    wchar_t expected = state->practice_text[state->current_position];

    if (typed == expected) {
        state->correct_count++;
    } else {
        state->incorrect_count++;
    }

    state->current_position++;
    update_stats (state);

    /* Check completion */
    if (state->current_position >= state->total_characters) {
        state->session_active = FALSE;

        DWORD elapsed_ms = GetTickCount () - state->start_time;
        double accuracy = 100.0;
        if (state->correct_count + state->incorrect_count > 0) {
            accuracy = (double) state->correct_count /
                       (state->correct_count + state->incorrect_count) * 100.0;
        }
        double cpm = state->correct_count / (elapsed_ms / 60000.0);

        wchar_t msg[256];
        swprintf (msg, 256,
                  L"Completed!\n\nAccuracy: %.1f%%\nSpeed: %.1f chars/min\nTime: %.1f seconds",
                  accuracy, cpm, elapsed_ms / 1000.0);
        MessageBoxW (hwnd, msg, L"Typing Practice", MB_OK | MB_ICONINFORMATION);
    }
}

/* Dialog procedure */
static INT_PTR CALLBACK TypingDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TypingDialogState *state =
        (TypingDialogState *) GetWindowLongPtr (hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_INITDIALOG: {
        state = (TypingDialogState *) lParam;
        SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) state);

        /* Initialize HIME */
        hime_init (NULL);
        state->ctx = hime_context_new ();

        /* Create controls */
        int y = 20;
        HFONT hFont =
            CreateFontW (16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        /* Category label and combo */
        CreateWindowW (L"STATIC", L"Category / 類別:", WS_CHILD | WS_VISIBLE,
                       20, y, 150, 20, hwnd, NULL, g_hInstance, NULL);
        y += 25;

        state->hwndCategoryCombo = CreateWindowW (
            L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, y, 350,
            200, hwnd, (HMENU) IDC_CATEGORY_COMBO, g_hInstance, NULL);
        SendMessageW (state->hwndCategoryCombo, CB_ADDSTRING, 0, (LPARAM) L"English");
        SendMessageW (state->hwndCategoryCombo, CB_ADDSTRING, 0, (LPARAM) L"注音 (Zhuyin)");
        SendMessageW (state->hwndCategoryCombo, CB_ADDSTRING, 0, (LPARAM) L"拼音 (Pinyin)");
        SendMessageW (state->hwndCategoryCombo, CB_ADDSTRING, 0, (LPARAM) L"倉頡 (Cangjie)");
        SendMessageW (state->hwndCategoryCombo, CB_ADDSTRING, 0, (LPARAM) L"Mixed / 混合");
        SendMessage (state->hwndCategoryCombo, CB_SETCURSEL, 0, 0);
        y += 35;

        /* Difficulty label and combo */
        CreateWindowW (L"STATIC", L"Difficulty / 難度:", WS_CHILD | WS_VISIBLE,
                       20, y, 150, 20, hwnd, NULL, g_hInstance, NULL);
        y += 25;

        state->hwndDifficultyCombo = CreateWindowW (
            L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, y, 350,
            200, hwnd, (HMENU) IDC_DIFFICULTY_COMBO, g_hInstance, NULL);
        SendMessageW (state->hwndDifficultyCombo, CB_ADDSTRING, 0, (LPARAM) L"Easy / 簡單");
        SendMessageW (state->hwndDifficultyCombo, CB_ADDSTRING, 0, (LPARAM) L"Medium / 中等");
        SendMessageW (state->hwndDifficultyCombo, CB_ADDSTRING, 0, (LPARAM) L"Hard / 困難");
        SendMessage (state->hwndDifficultyCombo, CB_SETCURSEL, 0, 0);
        y += 45;

        /* Practice text label */
        CreateWindowW (L"STATIC", L"Type this / 請輸入:", WS_CHILD | WS_VISIBLE,
                       20, y, 150, 20, hwnd, NULL, g_hInstance, NULL);
        y += 25;

        /* Practice text display */
        state->hwndPracticeText = CreateWindowW (
            L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER, 20,
            y, 350, 60, hwnd, (HMENU) IDC_PRACTICE_TEXT, g_hInstance, NULL);
        SendMessageW (state->hwndPracticeText, WM_SETFONT, (WPARAM) hFont, TRUE);
        y += 70;

        /* Hint label */
        state->hwndHintLabel =
            CreateWindowW (L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 20,
                           y, 350, 20, hwnd, (HMENU) IDC_HINT_LABEL, g_hInstance, NULL);
        y += 30;

        /* Progress bar */
        state->hwndProgressBar = CreateWindowW (
            PROGRESS_CLASSW, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 20, y,
            350, 20, hwnd, (HMENU) IDC_PROGRESS_BAR, g_hInstance, NULL);
        SendMessage (state->hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM (0, 100));
        y += 30;

        /* Input edit */
        state->hwndInputEdit = CreateWindowW (
            L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            20, y, 350, 30, hwnd, (HMENU) IDC_INPUT_EDIT, g_hInstance, NULL);
        SendMessageW (state->hwndInputEdit, WM_SETFONT, (WPARAM) hFont, TRUE);
        y += 40;

        /* Stats label */
        state->hwndStatsLabel = CreateWindowW (
            L"STATIC", L"Progress: 0/0 | Accuracy: 100%",
            WS_CHILD | WS_VISIBLE | SS_CENTER, 20, y, 350, 40, hwnd,
            (HMENU) IDC_STATS_LABEL, g_hInstance, NULL);
        y += 50;

        /* Buttons */
        state->hwndNewTextBtn = CreateWindowW (
            L"BUTTON", L"New Text / 換題", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, y, 160, 35, hwnd, (HMENU) IDC_NEW_TEXT_BTN, g_hInstance, NULL);

        state->hwndResetBtn = CreateWindowW (
            L"BUTTON", L"Reset / 重置", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            210, y, 160, 35, hwnd, (HMENU) IDC_RESET_BTN, g_hInstance, NULL);

        /* Load initial text */
        load_random_text (state, 0);
        start_new_session (hwnd, state);

        return TRUE;
    }

    case WM_COMMAND: {
        int wmId = LOWORD (wParam);
        int wmEvent = HIWORD (wParam);

        switch (wmId) {
        case IDC_INPUT_EDIT:
            if (wmEvent == EN_CHANGE) {
                process_input (hwnd, state);
            }
            break;

        case IDC_CATEGORY_COMBO:
            if (wmEvent == CBN_SELCHANGE) {
                int category = (int) SendMessage (state->hwndCategoryCombo,
                                                  CB_GETCURSEL, 0, 0);
                load_random_text (state, category);
                start_new_session (hwnd, state);
            }
            break;

        case IDC_NEW_TEXT_BTN: {
            int category =
                (int) SendMessage (state->hwndCategoryCombo, CB_GETCURSEL, 0, 0);
            load_random_text (state, category);
            start_new_session (hwnd, state);
            break;
        }

        case IDC_RESET_BTN:
            reset_session (hwnd, state);
            break;

        case IDCANCEL:
            EndDialog (hwnd, 0);
            break;
        }
        return TRUE;
    }

    case WM_CLOSE:
        if (state && state->ctx) {
            hime_typing_end_session (state->ctx);
            hime_context_free (state->ctx);
        }
        EndDialog (hwnd, 0);
        return TRUE;
    }

    return FALSE;
}

/* Initialize typing practice UI module */
int hime_typing_ui_init (HINSTANCE hInstance) {
    g_hInstance = hInstance;

    /* Initialize common controls */
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof (icc);
    icc.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx (&icc);

    return 0;
}

/* Show typing practice dialog */
INT_PTR hime_typing_ui_show (HWND hwndParent) {
    /* Create dialog template in memory */
    DLGTEMPLATE dlgTemplate = {0};
    dlgTemplate.style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION |
                        WS_SYSMENU | DS_SETFONT;
    dlgTemplate.dwExtendedStyle = 0;
    dlgTemplate.cdit = 0;
    dlgTemplate.x = 0;
    dlgTemplate.y = 0;
    dlgTemplate.cx = 260; /* Dialog units */
    dlgTemplate.cy = 280;

    /* Allocate state */
    TypingDialogState *state =
        (TypingDialogState *) calloc (1, sizeof (TypingDialogState));
    if (!state)
        return -1;

    /* Create dialog */
    INT_PTR result = DialogBoxIndirectParamW (
        g_hInstance, &dlgTemplate, hwndParent, TypingDlgProc, (LPARAM) state);

    free (state);
    return result;
}

/* Cleanup typing practice UI module */
void hime_typing_ui_cleanup (void) {
    hime_cleanup ();
}
