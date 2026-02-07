/*
 * HIME Windows Typing Practice UI
 *
 * Win32 dialog-based typing practice interface.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#ifndef HIME_TYPING_UI_H
#define HIME_TYPING_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

/* Initialize typing practice UI module */
int hime_typing_ui_init (HINSTANCE hInstance);

/* Show typing practice dialog */
INT_PTR hime_typing_ui_show (HWND hwndParent);

/* Cleanup typing practice UI module */
void hime_typing_ui_cleanup (void);

#ifdef __cplusplus
}
#endif

#endif /* HIME_TYPING_UI_H */
