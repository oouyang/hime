# HIME Windows Language Bar Indicator

## Overview

HIME displays a system tray language bar button that shows the current input mode, similar to the built-in Windows Zhuyin IME. The icon updates dynamically when the user switches methods.

## Implementation

The language bar is implemented as a TSF `ITfLangBarItemButton` COM object (`HimeLangBarButton` class) in `windows/src/hime-tsf.cpp`.

### Architecture

```
HimeTextService (ITfTextInputProcessor)
  ├── _InitLanguageBar()   — called from Activate()
  ├── _UninitLanguageBar() — called from Deactivate()
  ├── UpdateLanguageBar()  — called after mode changes
  └── m_pLangBarButton ──→ HimeLangBarButton
                              ├── ITfLangBarItemButton  (icon, text, click)
                              ├── ITfSource              (sink management)
                              └── m_pLangBarItemSink ──→ TSF notification sink
```

### Icon Generation

Icons are created programmatically via GDI (`_CreateModeIcon()`), no external `.ico` files needed:

| Mode | Label | Background Color | RGB |
|------|-------|-----------------|-----|
| English | EN | Dark gray | (80, 80, 80) |
| Zhuyin (注音) | 注 | Blue | (0, 90, 180) |
| Cangjie/GTAB | 倉 | Green | (0, 130, 60) |

The icon is a 16×16 32-bit ARGB bitmap with the mode label drawn in white bold text (Microsoft JhengHei font).

### Mode Update Triggers

`UpdateLanguageBar()` is called from:

1. **Ctrl+\` / F4 toggle** — switches between Zhuyin and Cangjie
2. **`HIME_KEY_ABSORBED` handler** — covers Shift key EN/CN toggle
3. **`OnClick()` on the tray icon** — left-click cycles methods

### Category Registration

`DllRegisterServer()` registers two additional TSF categories:

- `GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT` — enables system tray icon display
- `GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT` — enables Windows 8+ UWP app support

### MinGW Compatibility

MinGW's `ctfutb.h` lacks `ITfLangBarItemButton` and related types. These are defined manually in `hime-tsf.cpp`:

- `ITfLangBarItemButton` interface (MIDL_INTERFACE)
- `ITfMenu` interface (forward declaration)
- `TfLBIClick` enum
- `TF_LBI_STYLE_BTN_BUTTON`, `TF_LBI_STYLE_SHOWNINTRAY` constants
- `TF_LBI_ICON`, `TF_LBI_TEXT`, `TF_LBI_TOOLTIP` update flags

### Build Requirements

The `gdi32` library is linked for GDI icon creation functions (`CreateDIBSection`, `CreateFontW`, `DrawTextW`, `CreateIconIndirect`, etc.).

## Testing

1. Cross-compile: `cd platform/windows/build && cmake .. -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake && make`
2. Deploy to Windows Sandbox (see `platform/windows/sandbox/README.md`)
3. Verify: system tray shows HIME icon with mode label
4. Press Ctrl+\` → icon updates between 注 and 倉
5. Press Shift → icon updates to EN
6. Left-click the tray icon → cycles input method
