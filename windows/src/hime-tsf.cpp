/*
 * HIME Text Service Framework (TSF) Implementation
 *
 * This file implements a minimal Windows TSF text service that
 * wraps the HIME core library for Windows IME support.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

#include <ctfutb.h>
#include <msctf.h>
#include <olectl.h>
#include <windows.h>

/* GDI+ for PNG icon loading */
#include <gdiplus.h>

/* MinGW compatibility */
#ifndef TF_CLIENTID_NULL
#define TF_CLIENTID_NULL 0
#endif

#ifndef swprintf
#define swprintf _snwprintf
#endif

extern "C" {
#include "hime-core.h"
}

/* MinGW ctfutb.h lacks ITfLangBarItemButton and related types.
 * Define them here following the Windows SDK definitions. */
#ifndef TF_LBI_STYLE_BTN_BUTTON
#define TF_LBI_STYLE_BTN_BUTTON 0x00010000
#endif
#ifndef TF_LBI_STYLE_SHOWNINTRAY
#define TF_LBI_STYLE_SHOWNINTRAY 0x00000002
#endif
#ifndef TF_LBI_ICON
#define TF_LBI_ICON 0x00000001
#endif
#ifndef TF_LBI_TEXT
#define TF_LBI_TEXT 0x00000002
#endif
#ifndef TF_LBI_TOOLTIP
#define TF_LBI_TOOLTIP 0x00000004
#endif
#ifndef TF_LBI_STATUS
#define TF_LBI_STATUS 0x00000010
#endif

/* Menu item flags for ITfMenu::AddMenuItem */
#ifndef TF_LBMENUF_CHECKED
#define TF_LBMENUF_CHECKED 0x00000001
#endif
#ifndef TF_LBMENUF_GRAYED
#define TF_LBMENUF_GRAYED 0x00000002
#endif
#ifndef TF_LBMENUF_SEPARATOR
#define TF_LBMENUF_SEPARATOR 0x00000004
#endif

/* Right-click menu item IDs */
#define HIME_MENU_ID_ZHUYIN     10
#define HIME_MENU_ID_GTAB_BASE  11   /* ID for GTAB index i = 11 + i */
#define HIME_MENU_ID_TOOLBAR    50
#define HIME_MENU_ID_SETTINGS   51
#define HIME_MENU_ID_ABOUT      52

/* Maximum number of input method slots */
#define HIME_MAX_METHODS 32

/* Style flags for language bar buttons with menus */
#ifndef TF_LBI_STYLE_BTN_MENU
#define TF_LBI_STYLE_BTN_MENU 0x00020000
#endif

typedef enum { TF_LBI_CLK_RIGHT = 1,
               TF_LBI_CLK_LEFT = 2 } TfLBIClick;

/* Minimal ITfMenu forward declaration */
MIDL_INTERFACE ("6f8a98e4-aaa0-4f15-8c5b-07e0df0a3dd8")
ITfMenu : public IUnknown {
  public:
    virtual HRESULT STDMETHODCALLTYPE AddMenuItem (UINT uId, DWORD dwFlags,
                                                   HBITMAP hbmp, HBITMAP hbmpMask,
                                                   const WCHAR *pch, ULONG cch,
                                                   ITfMenu *pSubMenu) = 0;
};

/* ITfLangBarItemButton interface */
static const GUID IID_ITfLangBarItemButton = {
    0x28c7f1d0, 0xde25, 0x11d2, {0xaf, 0xdd, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}};

MIDL_INTERFACE ("28c7f1d0-de25-11d2-afdd-00105a2799b5")
ITfLangBarItemButton : public ITfLangBarItem {
  public:
    virtual HRESULT STDMETHODCALLTYPE OnClick (TfLBIClick click, POINT pt,
                                               const RECT *prcArea) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitMenu (ITfMenu * pMenu) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnMenuSelect (UINT wID) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIcon (HICON * phIcon) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetText (BSTR * pbstrText) = 0;
};

/* GUIDs */
// {B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}
static const GUID CLSID_HimeTextService = {
    0xb8a45c32, 0x5f6d, 0x4e2a, {0x9c, 0x1b, 0x0d, 0x3e, 0x4f, 0x5a, 0x6b, 0x7c}};

// {C9B56D43-6E7F-5F3B-AD2C-1E4F5061C8D9}
static const GUID GUID_HimeProfile = {
    0xc9b56d43, 0x6e7f, 0x5f3b, {0xad, 0x2c, 0x1e, 0x4f, 0x50, 0x61, 0xc8, 0xd9}};

// {DA8E7A60-3F71-4B9C-BE2D-5E6F70819A3B}
static const GUID GUID_HimeLangBarButton = {
    0xda8e7a60, 0x3f71, 0x4b9c, {0xbe, 0x2d, 0x5e, 0x6f, 0x70, 0x81, 0x9a, 0x3b}};

// {EB9F8B71-4C82-5DAD-CF3E-6F7081A2B4C3}
static const GUID GUID_HimeModeButton = {
    0xeb9f8b71, 0x4c82, 0x5dad, {0xcf, 0x3e, 0x6f, 0x70, 0x81, 0xa2, 0xb4, 0xc3}};

/* MinGW may lack these category GUIDs */
#ifndef GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT
// {25504FB4-7BAB-4BC1-9C69-CF81890F0EF5}
static const GUID GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT = {
    0x25504fb4, 0x7bab, 0x4bc1, {0x9c, 0x69, 0xcf, 0x81, 0x89, 0x0f, 0x0e, 0xf5}};
#endif
#ifndef GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT
// {13A016DF-560B-46CD-947A-4C3AF1E0E35D}
static const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT = {
    0x13a016df, 0x560b, 0x46cd, {0x94, 0x7a, 0x4c, 0x3a, 0xf1, 0xe0, 0xe3, 0x5d}};
#endif

/* Constants */
#ifdef NDEBUG
static const WCHAR TEXTSERVICE_DESC[] = L"姬 HIME";
#else
static const WCHAR TEXTSERVICE_DESC[] = L"姬 HIME [DEBUG]";
#endif
static const WCHAR TEXTSERVICE_MODEL[] = L"Apartment";
static HINSTANCE g_hInst = NULL;
static LONG g_cRefDll = 0;

/* GDI+ state for PNG icon loading (lazy-initialized, NOT in DllMain
 * because GdiplusStartup/Shutdown create/join threads and will deadlock
 * under the loader lock) */
static ULONG_PTR g_gdiplusToken = 0;
static BOOL g_gdiplusInitDone = FALSE;
static HICON g_appIcon = NULL; /* Cached hime.png app icon */
static HICON g_enIcon = NULL;  /* Cached English mode icon */

/* Floating toolbar state */
static HWND g_hwndToolbar = NULL;
static BOOL g_toolbarVisible = FALSE;
static const WCHAR TOOLBAR_CLASS[] = L"HimeToolbarClass";
static BOOL g_toolbarClassRegistered = FALSE;
static BOOL g_fullWidth = TRUE; /* Full-width character mode */

/* Active service pointer for toolbar and dialogs */
static class HimeTextService *g_pActiveService = NULL;

/* ========== Data-driven input method system ========== */

/* Method slot: one entry per discoverable input method */
typedef struct {
    int gtab_index;          /* Index in core GTAB registry, -1 for Zhuyin */
    HimeGtabTable gtab_id;   /* GTAB table ID */
    BOOL available;          /* .gtab file exists on disk */
    BOOL enabled;            /* User preference: include in toggle cycle */
    WCHAR display_name[64];  /* e.g. L"倉五 (Cangjie 5)" */
    char icon_filename[64];  /* e.g. "cj5.png" */
    HICON cached_icon;       /* Per-method icon cache */
    int group;               /* 0=Chinese, 1=International, 2=Symbols */
} HimeMethodSlot;

static HimeMethodSlot g_methods[HIME_MAX_METHODS];
static int g_methodCount = 0;
static BOOL g_methodsDiscovered = FALSE;

/* Static metadata: English names and groups for GTAB tables.
 * HimeGtabInfo only has Chinese names; we need English for Windows UI. */
static const struct { const char *filename; const WCHAR *ename; int group; } GTAB_META[] = {
    {"cj.gtab",            L"Cangjie",       0},
    {"cj5.gtab",           L"Cangjie 5",     0},
    {"cj543.gtab",         L"Cangjie 543",   0},
    {"cj-punc.gtab",       L"CJ Punc",       0},
    {"simplex.gtab",       L"Simplex",        0},
    {"simplex-punc.gtab",  L"Simplex Punc",   0},
    {"dayi3.gtab",         L"DaYi",           0},
    {"ar30.gtab",          L"Array 30",       0},
    {"array40.gtab",       L"Array 40",       0},
    {"ar30-big.gtab",      L"Array 30 Big",   0},
    {"liu.gtab",           L"Boshiamy",        0},
    {"pinyin.gtab",        L"Pinyin",          0},
    {"jyutping.gtab",      L"Jyutping",        0},
    {"hangul.gtab",        L"Hangul",          1},
    {"hangul-roman.gtab",  L"Hangul Roman",    1},
    {"vims.gtab",          L"VIMS",            1},
    {"symbols.gtab",       L"Symbols",         2},
    {"greek.gtab",         L"Greek",           2},
    {"russian.gtab",       L"Russian",         2},
    {"esperanto.gtab",     L"Esperanto",       2},
    {"latin-letters.gtab", L"Latin",           2},
    {NULL, NULL, 0}
};

static void
_EnsureGdiplusInit (void) {
    if (!g_gdiplusInitDone) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup (&g_gdiplusToken, &gdiplusStartupInput, NULL);
        g_gdiplusInitDone = TRUE;
    }
}

/* Debug logging - only active in debug builds */
static char g_logPath[MAX_PATH] = {0};
static int g_logFailed = 0;

static void hime_log (const char *fmt, ...) {
#ifdef NDEBUG
    (void) fmt;
    return;
#else
    if (g_logFailed)
        return;

    if (!g_logPath[0]) {
        DWORD pid = GetCurrentProcessId ();

        /* Try c:\mu\tmp\hime\ first */
        CreateDirectoryW (L"c:\\mu", NULL);
        CreateDirectoryW (L"c:\\mu\\tmp", NULL);
        CreateDirectoryW (L"c:\\mu\\tmp\\hime", NULL);

        char tryPath[MAX_PATH];
        snprintf (tryPath, MAX_PATH, "c:\\mu\\tmp\\hime\\test-%lu.log", (unsigned long) pid);
        FILE *fp = fopen (tryPath, "a");
        if (fp) {
            fclose (fp);
            strncpy (g_logPath, tryPath, MAX_PATH - 1);
        } else {
            /* Fallback: write next to the DLL */
            WCHAR dllPath[MAX_PATH];
            GetModuleFileNameW (g_hInst, dllPath, MAX_PATH);
            WCHAR *slash = wcsrchr (dllPath, L'\\');
            if (slash) {
                WCHAR logPathW[MAX_PATH];
                swprintf (logPathW, MAX_PATH, L"%.*s\\hime-test-%lu.log",
                          (int) (slash - dllPath + 1), dllPath, (unsigned long) pid);
                WideCharToMultiByte (CP_ACP, 0, logPathW, -1, g_logPath, MAX_PATH, NULL, NULL);
            }

            fp = fopen (g_logPath, "a");
            if (fp) {
                fclose (fp);
            } else {
                /* Last resort: TEMP dir */
                WCHAR tempDir[MAX_PATH];
                GetTempPathW (MAX_PATH, tempDir);
                WCHAR logPathW[MAX_PATH];
                swprintf (logPathW, MAX_PATH, L"%shime-test-%lu.log",
                          tempDir, (unsigned long) pid);
                WideCharToMultiByte (CP_ACP, 0, logPathW, -1, g_logPath, MAX_PATH, NULL, NULL);
            }
        }

        /* Log where we're writing */
        fp = fopen (g_logPath, "a");
        if (fp) {
            fprintf (fp, "=== HIME TSF PID=%lu ===\n", (unsigned long) pid);
            fprintf (fp, "Log path: %s\n", g_logPath);
            fclose (fp);
        } else {
            g_logFailed = 1;
            OutputDebugStringW (L"HIME: ALL log paths failed!\n");
            return;
        }
    }

    FILE *fp = fopen (g_logPath, "a");
    if (!fp)
        return;

    va_list ap;
    va_start (ap, fmt);
    vfprintf (fp, fmt, ap);
    va_end (ap);
    fprintf (fp, "\n");
    fclose (fp);
#endif /* !NDEBUG */
}

/* Check whether a .gtab file is present in the data directory.
 * Searches next to the DLL, in data/, or ../data/. */
static BOOL
_IsGtabFilePresent (const char *filename) {
    WCHAR dllDir[MAX_PATH];
    GetModuleFileNameW (g_hInst, dllDir, MAX_PATH);
    WCHAR *sep = wcsrchr (dllDir, L'\\');
    if (sep)
        *sep = L'\0';

    WCHAR filenameW[128];
    MultiByteToWideChar (CP_UTF8, 0, filename, -1, filenameW, 128);

    const WCHAR *subFmts[] = {L"%ls\\data\\%ls", L"%ls\\%ls", L"%ls\\..\\data\\%ls"};
    for (int i = 0; i < 3; i++) {
        WCHAR tryPath[MAX_PATH];
        _snwprintf (tryPath, MAX_PATH, subFmts[i], dllDir, filenameW);
        if (GetFileAttributesW (tryPath) != INVALID_FILE_ATTRIBUTES) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Look up GTAB_META entry by filename. Returns NULL if not found. */
static const WCHAR *
_LookupGtabEname (const char *filename, int *outGroup) {
    for (int i = 0; GTAB_META[i].filename; i++) {
        if (strcmp (GTAB_META[i].filename, filename) == 0) {
            if (outGroup)
                *outGroup = GTAB_META[i].group;
            return GTAB_META[i].ename;
        }
    }
    if (outGroup)
        *outGroup = 0;
    return NULL;
}

/* Discover all available input methods and populate g_methods[].
 * Called once during Activate() after hime_init(). */
static void
_DiscoverMethods (void) {
    if (g_methodsDiscovered)
        return;
    g_methodsDiscovered = TRUE;
    g_methodCount = 0;

    /* Slot 0: Zhuyin (always available) */
    HimeMethodSlot *slot = &g_methods[g_methodCount];
    memset (slot, 0, sizeof (*slot));
    slot->gtab_index = -1;
    slot->gtab_id = (HimeGtabTable) -1;
    slot->available = TRUE;
    slot->enabled = TRUE;
    wcscpy (slot->display_name, L"\x6CE8\x97F3 (Zhuyin)"); /* 注音 */
    strcpy (slot->icon_filename, "juyin.png");
    slot->cached_icon = NULL;
    slot->group = 0;
    g_methodCount++;

    /* GTAB methods from core registry */
    int tableCount = hime_gtab_get_table_count ();
    for (int i = 0; i < tableCount && g_methodCount < HIME_MAX_METHODS; i++) {
        HimeGtabInfo info;
        if (hime_gtab_get_table_info (i, &info) != 0)
            continue;

        slot = &g_methods[g_methodCount];
        memset (slot, 0, sizeof (*slot));
        slot->gtab_index = i;

        /* Find the GTAB table ID by looking up the registry */
        int group = 0;
        const WCHAR *ename = _LookupGtabEname (info.filename, &group);

        /* Build display name: "倉五 (Cangjie 5)" */
        WCHAR cnameW[64];
        MultiByteToWideChar (CP_UTF8, 0, info.name, -1, cnameW, 64);
        if (ename) {
            _snwprintf (slot->display_name, 64, L"%ls (%ls)", cnameW, ename);
        } else {
            wcsncpy (slot->display_name, cnameW, 63);
        }
        slot->display_name[63] = L'\0';

        strncpy (slot->icon_filename, info.icon, 63);
        slot->icon_filename[63] = '\0';
        slot->group = group;

        /* Check file availability */
        slot->available = _IsGtabFilePresent (info.filename);

        /* Default enabled: all available methods.
         * Users can disable unwanted ones via the Settings dialog,
         * which persists to the registry for future sessions. */
        slot->enabled = slot->available;

        slot->cached_icon = NULL;

        /* Determine gtab_id from GTAB_META — walk the enum entries.
         * We use the registry index to load by filename anyway, so
         * store the index for menu ID mapping. */
        slot->gtab_id = (HimeGtabTable) -1;
        /* We don't strictly need the enum ID since we load by filename */

        g_methodCount++;
    }

    hime_log ("DiscoverMethods: found %d methods (%d GTAB)", g_methodCount, tableCount);
    for (int i = 0; i < g_methodCount; i++) {
        char nameA[128];
        WideCharToMultiByte (CP_UTF8, 0, g_methods[i].display_name, -1, nameA, 128, NULL, NULL);
        hime_log ("  [%d] %s avail=%d enabled=%d group=%d icon=%s",
                  i, nameA, g_methods[i].available, g_methods[i].enabled,
                  g_methods[i].group, g_methods[i].icon_filename);
    }
}

/* Find the g_methods[] index for the currently active input method.
 * Returns 0 (Zhuyin) if not found or not in Chinese mode. */
static int
_GetCurrentMethodIndex (HimeContext *ctx) {
    if (!ctx || !hime_is_chinese_mode (ctx))
        return -1; /* EN mode */

    HimeInputMethod method = hime_get_input_method (ctx);
    if (method == HIME_IM_PHO)
        return 0; /* Zhuyin is always slot 0 */

    if (method == HIME_IM_GTAB) {
        const char *tblName = hime_gtab_get_current_table (ctx);
        if (!tblName || !tblName[0])
            return 0;

        /* Match by table display name (Chinese name from core) */
        for (int i = 1; i < g_methodCount; i++) {
            if (g_methods[i].gtab_index < 0)
                continue;
            HimeGtabInfo info;
            if (hime_gtab_get_table_info (g_methods[i].gtab_index, &info) == 0) {
                if (strcmp (info.name, tblName) == 0)
                    return i;
            }
        }
    }

    return 0;
}

/* Switch to a specific method by g_methods[] index.
 * Does NOT change Chinese mode — caller should set it. */
static void
_SwitchToMethod (HimeContext *ctx, int methodIdx) {
    if (!ctx || methodIdx < 0 || methodIdx >= g_methodCount)
        return;

    HimeMethodSlot *slot = &g_methods[methodIdx];

    if (slot->gtab_index < 0) {
        /* Zhuyin */
        hime_set_chinese_mode (ctx, true);
        hime_set_input_method (ctx, HIME_IM_PHO);
    } else {
        /* GTAB: load by filename */
        HimeGtabInfo info;
        if (hime_gtab_get_table_info (slot->gtab_index, &info) == 0) {
            hime_set_chinese_mode (ctx, true);
            hime_gtab_load_table (ctx, info.filename);
        }
    }
}

/* Load settings from registry. Non-fatal if key doesn't exist. */
static void
_LoadSettings (void) {
    HKEY hKey;
    LONG rc = RegOpenKeyExW (HKEY_CURRENT_USER, L"Software\\HIME", 0, KEY_READ, &hKey);
    if (rc != ERROR_SUCCESS)
        return;

    /* Check saved method count — reject stale settings from older versions
     * that had fewer methods (e.g. only 3 hardcoded methods). */
    DWORD savedCount = 0;
    DWORD dwordSize = sizeof (savedCount);
    DWORD type = 0;
    rc = RegQueryValueExW (hKey, L"MethodCount", NULL, &type,
                            (BYTE *) &savedCount, &dwordSize);
    if (rc != ERROR_SUCCESS || type != REG_DWORD || (int) savedCount != g_methodCount) {
        hime_log ("LoadSettings: skipping stale settings (saved=%lu current=%d)",
                  (unsigned long) savedCount, g_methodCount);
        RegCloseKey (hKey);
        /* Delete stale key so it doesn't confuse future loads */
        RegDeleteKeyW (HKEY_CURRENT_USER, L"Software\\HIME");
        return;
    }

    WCHAR data[1024] = {0};
    DWORD dataSize = sizeof (data);
    type = 0;
    rc = RegQueryValueExW (hKey, L"EnabledMethods", NULL, &type, (BYTE *) data, &dataSize);
    RegCloseKey (hKey);

    if (rc != ERROR_SUCCESS || type != REG_SZ || data[0] == L'\0')
        return;

    /* Parse comma-separated list of enabled method identifiers.
     * Format: "zhuyin,cj5.gtab,liu.gtab,..." */
    hime_log ("LoadSettings: '%ls'", data);

    /* First, disable all methods */
    for (int i = 0; i < g_methodCount; i++)
        g_methods[i].enabled = FALSE;

    WCHAR *token = wcstok (data, L",");
    while (token) {
        /* Trim whitespace */
        while (*token == L' ') token++;

        if (wcscmp (token, L"zhuyin") == 0) {
            g_methods[0].enabled = TRUE;
        } else {
            /* Convert to UTF-8 for filename comparison */
            char tokenA[128];
            WideCharToMultiByte (CP_UTF8, 0, token, -1, tokenA, 128, NULL, NULL);
            for (int i = 1; i < g_methodCount; i++) {
                HimeGtabInfo info;
                if (g_methods[i].gtab_index >= 0 &&
                    hime_gtab_get_table_info (g_methods[i].gtab_index, &info) == 0 &&
                    strcmp (info.filename, tokenA) == 0) {
                    g_methods[i].enabled = TRUE;
                    break;
                }
            }
        }
        token = wcstok (NULL, L",");
    }

    /* Ensure at least one method is enabled */
    BOOL anyEnabled = FALSE;
    for (int i = 0; i < g_methodCount; i++) {
        if (g_methods[i].available && g_methods[i].enabled) {
            anyEnabled = TRUE;
            break;
        }
    }
    if (!anyEnabled) {
        g_methods[0].enabled = TRUE; /* Fallback to Zhuyin */
    }
}

/* Save settings to registry */
static void
_SaveSettings (void) {
    HKEY hKey;
    LONG rc = RegCreateKeyExW (HKEY_CURRENT_USER, L"Software\\HIME", 0, NULL,
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS)
        return;

    WCHAR data[1024] = {0};
    int pos = 0;

    for (int i = 0; i < g_methodCount; i++) {
        if (!g_methods[i].enabled)
            continue;

        if (pos > 0) {
            data[pos++] = L',';
        }

        if (g_methods[i].gtab_index < 0) {
            wcscpy (&data[pos], L"zhuyin");
            pos += 6;
        } else {
            HimeGtabInfo info;
            if (hime_gtab_get_table_info (g_methods[i].gtab_index, &info) == 0) {
                WCHAR fnW[128];
                MultiByteToWideChar (CP_UTF8, 0, info.filename, -1, fnW, 128);
                int fnLen = wcslen (fnW);
                wcscpy (&data[pos], fnW);
                pos += fnLen;
            }
        }
    }
    data[pos] = L'\0';

    RegSetValueExW (hKey, L"EnabledMethods", 0, REG_SZ,
                     (const BYTE *) data, (pos + 1) * sizeof (WCHAR));

    /* Save method count so _LoadSettings can detect stale settings */
    DWORD count = (DWORD) g_methodCount;
    RegSetValueExW (hKey, L"MethodCount", 0, REG_DWORD,
                     (const BYTE *) &count, sizeof (count));

    RegCloseKey (hKey);
    hime_log ("SaveSettings: '%ls' (count=%d)", data, g_methodCount);
}

/* Forward declarations */
class HimeTextService;
class HimeClassFactory;
class HimeLangBarButton;
class HimeModeButton;

/* Forward declarations for toolbar and dialogs */
static void _ShowToolbar (BOOL show);
static void _UpdateToolbar (void);
static void _ShowAboutDialog (void);
static void _ShowSettingsDialog (void);
static HICON _CreateModeIconForContext (HimeContext *ctx);
static HICON _CreateAppIcon (void);
static void _CycleInputMethod (HimeContext *ctx);

/* ========== HimeLangBarButton Class (declaration only — implementation after HimeTextService) ========== */

class HimeLangBarButton : public ITfLangBarItemButton,
                          public ITfSource {
  public:
    HimeLangBarButton (HimeTextService *pService);
    ~HimeLangBarButton ();

    /* IUnknown */
    STDMETHODIMP QueryInterface (REFIID riid, void **ppvObj);
    STDMETHODIMP_ (ULONG)
    AddRef ();
    STDMETHODIMP_ (ULONG)
    Release ();

    /* ITfLangBarItem */
    STDMETHODIMP GetInfo (TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP GetStatus (DWORD *pdwStatus);
    STDMETHODIMP Show (BOOL fShow);
    STDMETHODIMP GetTooltipString (BSTR *pbstrToolTip);

    /* ITfLangBarItemButton */
    STDMETHODIMP OnClick (TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP InitMenu (ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect (UINT wID);
    STDMETHODIMP GetIcon (HICON *phIcon);
    STDMETHODIMP GetText (BSTR *pbstrText);

    /* ITfSource */
    STDMETHODIMP AdviseSink (REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink (DWORD dwCookie);

    /* Called by HimeTextService when mode changes */
    void Update ();

  private:
    LONG m_cRef;
    HimeTextService *m_pService;
    ITfLangBarItemSink *m_pLangBarItemSink;
    DWORD m_dwSinkCookie;
};

/* ========== HimeModeButton Class (mode indicator — shows current IME icon) ========== */

class HimeModeButton : public ITfLangBarItemButton,
                       public ITfSource {
  public:
    HimeModeButton (HimeTextService *pService);
    ~HimeModeButton ();

    /* IUnknown */
    STDMETHODIMP QueryInterface (REFIID riid, void **ppvObj);
    STDMETHODIMP_ (ULONG)
    AddRef ();
    STDMETHODIMP_ (ULONG)
    Release ();

    /* ITfLangBarItem */
    STDMETHODIMP GetInfo (TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP GetStatus (DWORD *pdwStatus);
    STDMETHODIMP Show (BOOL fShow);
    STDMETHODIMP GetTooltipString (BSTR *pbstrToolTip);

    /* ITfLangBarItemButton */
    STDMETHODIMP OnClick (TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP InitMenu (ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect (UINT wID);
    STDMETHODIMP GetIcon (HICON *phIcon);
    STDMETHODIMP GetText (BSTR *pbstrText);

    /* ITfSource */
    STDMETHODIMP AdviseSink (REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink (DWORD dwCookie);

    /* Called by HimeTextService when mode changes */
    void Update ();

  private:
    LONG m_cRef;
    HimeTextService *m_pService;
    ITfLangBarItemSink *m_pLangBarItemSink;
    DWORD m_dwSinkCookie;
};

/* ========== HimeTextService Class ========== */

/* Edit session for composition updates */
class HimeEditSession : public ITfEditSession {
  public:
    HimeEditSession (class HimeTextService *pService, ITfContext *pContext, int action)
        : m_cRef (1), m_pService (pService), m_pContext (pContext), m_action (action) {
        m_pContext->AddRef ();
    }

    ~HimeEditSession () {
        m_pContext->Release ();
    }

    /* IUnknown */
    STDMETHODIMP QueryInterface (REFIID riid, void **ppvObj) {
        if (ppvObj == NULL)
            return E_INVALIDARG;
        *ppvObj = NULL;
        if (IsEqualIID (riid, IID_IUnknown) || IsEqualIID (riid, IID_ITfEditSession)) {
            *ppvObj = (ITfEditSession *) this;
            AddRef ();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_ (ULONG)
    AddRef () { return InterlockedIncrement (&m_cRef); }
    STDMETHODIMP_ (ULONG)
    Release () {
        LONG cr = InterlockedDecrement (&m_cRef);
        if (cr == 0)
            delete this;
        return cr;
    }

    /* ITfEditSession */
    STDMETHODIMP DoEditSession (TfEditCookie ec);

    enum { ACTION_START_COMPOSITION = 1,
           ACTION_UPDATE_COMPOSITION,
           ACTION_COMMIT,
           ACTION_END_COMPOSITION };

  private:
    LONG m_cRef;
    class HimeTextService *m_pService;
    ITfContext *m_pContext;
    int m_action;
};

class HimeTextService : public ITfTextInputProcessor,
                        public ITfKeyEventSink,
                        public ITfCompositionSink {
  public:
    HimeTextService ();
    ~HimeTextService ();

    /* IUnknown */
    STDMETHODIMP QueryInterface (REFIID riid, void **ppvObj);
    STDMETHODIMP_ (ULONG)
    AddRef ();
    STDMETHODIMP_ (ULONG)
    Release ();

    /* ITfTextInputProcessor */
    STDMETHODIMP Activate (ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
    STDMETHODIMP Deactivate ();

    /* ITfKeyEventSink */
    STDMETHODIMP OnSetFocus (BOOL fForeground);
    STDMETHODIMP OnTestKeyDown (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnTestKeyUp (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyDown (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyUp (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnPreservedKey (ITfContext *pContext, REFGUID rguid, BOOL *pfEaten);

    /* ITfCompositionSink */
    STDMETHODIMP OnCompositionTerminated (TfEditCookie ecWrite, ITfComposition *pComposition);

    /* Called by edit session */
    HRESULT DoStartComposition (TfEditCookie ec, ITfContext *pContext);
    HRESULT DoUpdateComposition (TfEditCookie ec, ITfContext *pContext);
    HRESULT DoCommit (TfEditCookie ec, ITfContext *pContext);
    HRESULT DoEndComposition (TfEditCookie ec, ITfContext *pContext);

    HimeContext *GetHimeContext () { return m_himeCtx; }
    ITfComposition *GetComposition () { return m_pComposition; }
    void SetComposition (ITfComposition *pComp) { m_pComposition = pComp; }
    TfClientId GetClientId () { return m_tfClientId; }

    void UpdateLanguageBar ();

  private:
    HRESULT _InitKeystrokeSink ();
    void _UninitKeystrokeSink ();
    HRESULT _InitLanguageBar ();
    void _UninitLanguageBar ();
    HRESULT _RequestEditSession (ITfContext *pContext, int action);
    void _EndComposition ();

    LONG m_cRef;
    ITfThreadMgr *m_pThreadMgr;
    TfClientId m_tfClientId;
    ITfKeystrokeMgr *m_pKeystrokeMgr;
    ITfComposition *m_pComposition;
    HimeContext *m_himeCtx;
    HimeLangBarButton *m_pLangBarButton;
    HimeModeButton *m_pModeButton;
    DWORD m_dwCookie;
    WCHAR m_commitBuf[256];
    int m_commitLen;
    WCHAR m_accumBuf[1024]; /* Accumulated committed chars in current composition */
    int m_accumLen;
    BOOL m_justCommitted; /* Swallow Space/Enter immediately after a commit */
};

/* ========== HimeClassFactory Class ========== */

class HimeClassFactory : public IClassFactory {
  public:
    /* IUnknown */
    STDMETHODIMP QueryInterface (REFIID riid, void **ppvObj);
    STDMETHODIMP_ (ULONG)
    AddRef ();
    STDMETHODIMP_ (ULONG)
    Release ();

    /* IClassFactory */
    STDMETHODIMP CreateInstance (IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
    STDMETHODIMP LockServer (BOOL fLock);
};

/* ========== HimeTextService Implementation ========== */

HimeTextService::HimeTextService ()
    : m_cRef (1),
      m_pThreadMgr (NULL),
      m_tfClientId (TF_CLIENTID_NULL),
      m_pKeystrokeMgr (NULL),
      m_pComposition (NULL),
      m_himeCtx (NULL),
      m_pLangBarButton (NULL),
      m_pModeButton (NULL),
      m_dwCookie (0),
      m_commitLen (0),
      m_accumLen (0),
      m_justCommitted (FALSE) {
    memset (m_commitBuf, 0, sizeof (m_commitBuf));
    memset (m_accumBuf, 0, sizeof (m_accumBuf));
    InterlockedIncrement (&g_cRefDll);
}

HimeTextService::~HimeTextService () {
    if (m_himeCtx) {
        hime_context_free (m_himeCtx);
    }
    InterlockedDecrement (&g_cRefDll);
}

STDMETHODIMP HimeTextService::QueryInterface (REFIID riid, void **ppvObj) {
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID (riid, IID_IUnknown) ||
        IsEqualIID (riid, IID_ITfTextInputProcessor)) {
        *ppvObj = (ITfTextInputProcessor *) this;
    } else if (IsEqualIID (riid, IID_ITfKeyEventSink)) {
        *ppvObj = (ITfKeyEventSink *) this;
    } else if (IsEqualIID (riid, IID_ITfCompositionSink)) {
        *ppvObj = (ITfCompositionSink *) this;
    }

    if (*ppvObj) {
        AddRef ();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_ (ULONG)
HimeTextService::AddRef () {
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_ (ULONG)
HimeTextService::Release () {
    LONG cr = InterlockedDecrement (&m_cRef);
    if (cr == 0) {
        delete this;
    }
    return cr;
}

STDMETHODIMP HimeTextService::Activate (ITfThreadMgr *pThreadMgr, TfClientId tfClientId) {
    m_pThreadMgr = pThreadMgr;
    m_pThreadMgr->AddRef ();
    m_tfClientId = tfClientId;
    g_pActiveService = this;

    /* Initialize HIME core - find data directory */
    WCHAR dllPath[MAX_PATH];
    GetModuleFileNameW (g_hInst, dllPath, MAX_PATH);

    char dllPathUtf8[MAX_PATH];
    WideCharToMultiByte (CP_UTF8, 0, dllPath, -1, dllPathUtf8, MAX_PATH, NULL, NULL);
    hime_log ("Activate: DLL path: %s", dllPathUtf8);

    /* Try multiple data directory locations:
     * 1. Same directory as DLL: <dll_dir>/pho.tab2
     * 2. data/ subdirectory:    <dll_dir>/data/pho.tab2
     * 3. Parent's data/ dir:    <dll_dir>/../data/pho.tab2
     */
    WCHAR basePath[MAX_PATH];
    wcscpy (basePath, dllPath);
    WCHAR *lastSlash = wcsrchr (basePath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0'; /* Keep trailing backslash */
    }

    /* Try each path in order */
    const WCHAR *dataPaths[] = {
        L"",        /* Same directory as DLL */
        L"data",    /* data/ subdirectory */
        L"..\\data" /* Parent's data/ directory */
    };

    char dataDir[MAX_PATH];
    BOOL initialized = FALSE;

    for (int i = 0; i < 3; i++) {
        WCHAR tryPath[MAX_PATH];
        swprintf (tryPath, MAX_PATH, L"%s%s", basePath, dataPaths[i]);

        WideCharToMultiByte (CP_UTF8, 0, tryPath, -1, dataDir, MAX_PATH, NULL, NULL);
        hime_log ("Activate: Trying data path [%d]: %s", i, dataDir);

        /* Check if pho.tab2 exists at this path */
        char testFile[MAX_PATH * 2];
        snprintf (testFile, sizeof (testFile), "%s/pho.tab2", dataDir);
        FILE *fp = fopen (testFile, "rb");
        if (fp) {
            hime_log ("Activate: Found pho.tab2 at: %s", testFile);
            fclose (fp);
        } else {
            snprintf (testFile, sizeof (testFile), "%s/data/pho.tab2", dataDir);
            fp = fopen (testFile, "rb");
            if (fp) {
                hime_log ("Activate: Found pho.tab2 at: %s", testFile);
                fclose (fp);
            } else {
                hime_log ("Activate: pho.tab2 NOT found for path [%d]", i);
            }
        }

        int rc = hime_init (dataDir);
        hime_log ("Activate: hime_init returned %d", rc);

        if (rc == 0) {
            initialized = TRUE;
            break;
        }
    }

    if (initialized) {
        /* Discover all available input methods */
        _DiscoverMethods ();
        _LoadSettings ();

        m_himeCtx = hime_context_new ();
        if (m_himeCtx) {
            hime_log ("Activate: Context created OK, chinese_mode=%d",
                      hime_is_chinese_mode (m_himeCtx));

            /* Pre-load all available+enabled GTAB tables */
            for (int i = 0; i < g_methodCount; i++) {
                if (g_methods[i].gtab_index >= 0 && g_methods[i].available && g_methods[i].enabled) {
                    HimeGtabInfo info;
                    if (hime_gtab_get_table_info (g_methods[i].gtab_index, &info) == 0) {
                        int rc = hime_gtab_load_table (m_himeCtx, info.filename);
                        hime_log ("Activate: Pre-load %s returned %d", info.filename, rc);
                    }
                }
            }

            /* Switch back to Zhuyin (PHO) as default input method */
            hime_set_input_method (m_himeCtx, HIME_IM_PHO);
            hime_log ("Activate: Default method set to PHO");
        } else {
            hime_log ("Activate: ERROR - Failed to create context");
        }
    } else {
        hime_log ("Activate: ERROR - hime_init failed for ALL paths");
    }

    /* Initialize keystroke sink */
    HRESULT hr = _InitKeystrokeSink ();
    hime_log ("Activate: _InitKeystrokeSink returned 0x%08lx", (unsigned long) hr);

    /* Initialize language bar indicator */
    hr = _InitLanguageBar ();
    hime_log ("Activate: _InitLanguageBar returned 0x%08lx", (unsigned long) hr);

    return S_OK;
}

STDMETHODIMP HimeTextService::Deactivate () {
    /* Hide and destroy floating toolbar */
    _ShowToolbar (FALSE);

    _UninitLanguageBar ();
    _UninitKeystrokeSink ();

    if (g_pActiveService == this)
        g_pActiveService = NULL;

    if (m_himeCtx) {
        hime_context_free (m_himeCtx);
        m_himeCtx = NULL;
    }

    if (m_pThreadMgr) {
        m_pThreadMgr->Release ();
        m_pThreadMgr = NULL;
    }

    m_tfClientId = TF_CLIENTID_NULL;

    return S_OK;
}

HRESULT HimeTextService::_InitKeystrokeSink () {
    HRESULT hr;
    ITfKeystrokeMgr *pKeystrokeMgr;

    hr = m_pThreadMgr->QueryInterface (IID_ITfKeystrokeMgr, (void **) &pKeystrokeMgr);
    if (FAILED (hr))
        return hr;

    hr = pKeystrokeMgr->AdviseKeyEventSink (m_tfClientId, (ITfKeyEventSink *) this, TRUE);
    if (FAILED (hr)) {
        pKeystrokeMgr->Release ();
        return hr;
    }

    m_pKeystrokeMgr = pKeystrokeMgr;
    return S_OK;
}

void HimeTextService::_UninitKeystrokeSink () {
    if (m_pKeystrokeMgr) {
        m_pKeystrokeMgr->UnadviseKeyEventSink (m_tfClientId);
        m_pKeystrokeMgr->Release ();
        m_pKeystrokeMgr = NULL;
    }
}

HRESULT HimeTextService::_InitLanguageBar () {
    ITfLangBarItemMgr *pLangBarItemMgr = NULL;
    HRESULT hr = m_pThreadMgr->QueryInterface (IID_ITfLangBarItemMgr, (void **) &pLangBarItemMgr);
    hime_log ("_InitLanguageBar: QueryInterface(LangBarItemMgr) hr=0x%08lx", (unsigned long) hr);
    if (FAILED (hr))
        return hr;

    /* Create main HIME button (app icon, right-click menu) */
    m_pLangBarButton = new HimeLangBarButton (this);
    if (!m_pLangBarButton) {
        hime_log ("_InitLanguageBar: FAILED to allocate LangBarButton");
        pLangBarItemMgr->Release ();
        return E_OUTOFMEMORY;
    }

    hr = pLangBarItemMgr->AddItem (m_pLangBarButton);
    hime_log ("_InitLanguageBar: AddItem(LangBarButton) hr=0x%08lx", (unsigned long) hr);
    if (FAILED (hr)) {
        m_pLangBarButton->Release ();
        m_pLangBarButton = NULL;
        pLangBarItemMgr->Release ();
        return hr;
    }

    /* Create mode indicator button (shows current IME icon in system tray).
     * This is the button that shows the current input method's icon.
     * Issue #4: If this fails, the mode icon won't appear in the tray. */
    m_pModeButton = new HimeModeButton (this);
    if (m_pModeButton) {
        hr = pLangBarItemMgr->AddItem (m_pModeButton);
        hime_log ("_InitLanguageBar: AddItem(ModeButton) hr=0x%08lx", (unsigned long) hr);
        if (FAILED (hr)) {
            hime_log ("_InitLanguageBar: ModeButton AddItem FAILED - tray icon won't show");
            m_pModeButton->Release ();
            m_pModeButton = NULL;
            /* Non-fatal: main button still works */
        }
    } else {
        hime_log ("_InitLanguageBar: FAILED to allocate ModeButton");
    }

    pLangBarItemMgr->Release ();
    return S_OK;
}

void HimeTextService::_UninitLanguageBar () {
    if (m_pThreadMgr) {
        ITfLangBarItemMgr *pLangBarItemMgr = NULL;
        HRESULT hr = m_pThreadMgr->QueryInterface (IID_ITfLangBarItemMgr, (void **) &pLangBarItemMgr);
        if (SUCCEEDED (hr)) {
            if (m_pModeButton)
                pLangBarItemMgr->RemoveItem (m_pModeButton);
            if (m_pLangBarButton)
                pLangBarItemMgr->RemoveItem (m_pLangBarButton);
            pLangBarItemMgr->Release ();
        }
    }
    if (m_pModeButton) {
        m_pModeButton->Release ();
        m_pModeButton = NULL;
    }
    if (m_pLangBarButton) {
        m_pLangBarButton->Release ();
        m_pLangBarButton = NULL;
    }
}

void HimeTextService::UpdateLanguageBar () {
    if (m_pLangBarButton) {
        m_pLangBarButton->Update ();
    }
    if (m_pModeButton) {
        m_pModeButton->Update ();
    }
    _UpdateToolbar ();
}

HRESULT HimeTextService::_RequestEditSession (ITfContext *pContext, int action) {
    HimeEditSession *pEditSession = new HimeEditSession (this, pContext, action);
    if (!pEditSession)
        return E_OUTOFMEMORY;

    /* Use synchronous edit sessions to ensure correct ordering.
     * Async sessions can cause commits and compositions to execute
     * out of order, resulting in reversed text. */
    HRESULT hrSession;
    HRESULT hr = pContext->RequestEditSession (m_tfClientId, pEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
    pEditSession->Release ();
    return SUCCEEDED (hr) ? hrSession : hr;
}

/* Edit session implementation */
STDMETHODIMP HimeEditSession::DoEditSession (TfEditCookie ec) {
    switch (m_action) {
    case ACTION_START_COMPOSITION:
        return m_pService->DoStartComposition (ec, m_pContext);
    case ACTION_UPDATE_COMPOSITION:
        return m_pService->DoUpdateComposition (ec, m_pContext);
    case ACTION_COMMIT:
        return m_pService->DoCommit (ec, m_pContext);
    case ACTION_END_COMPOSITION:
        return m_pService->DoEndComposition (ec, m_pContext);
    }
    return E_UNEXPECTED;
}

HRESULT HimeTextService::DoStartComposition (TfEditCookie ec, ITfContext *pContext) {
    HRESULT hr;
    ITfInsertAtSelection *pInsertAtSelection = NULL;
    ITfRange *pRangeInsert = NULL;
    ITfContextComposition *pContextComposition = NULL;

    /* Get insertion point */
    hr = pContext->QueryInterface (IID_ITfInsertAtSelection, (void **) &pInsertAtSelection);
    if (FAILED (hr))
        goto Exit;

    hr = pInsertAtSelection->InsertTextAtSelection (ec, TF_IAS_QUERYONLY, NULL, 0, &pRangeInsert);
    if (FAILED (hr))
        goto Exit;

    /* Start composition */
    hr = pContext->QueryInterface (IID_ITfContextComposition, (void **) &pContextComposition);
    if (FAILED (hr))
        goto Exit;

    hr = pContextComposition->StartComposition (ec, pRangeInsert, (ITfCompositionSink *) this, &m_pComposition);

Exit:
    if (pContextComposition)
        pContextComposition->Release ();
    if (pRangeInsert)
        pRangeInsert->Release ();
    if (pInsertAtSelection)
        pInsertAtSelection->Release ();
    return hr;
}

HRESULT HimeTextService::DoUpdateComposition (TfEditCookie ec, ITfContext *pContext) {
    if (!m_pComposition) {
        /* Start composition if not started */
        HRESULT hr = DoStartComposition (ec, pContext);
        if (FAILED (hr))
            return hr;
    }

    if (!m_pComposition)
        return E_FAIL;

    /* Get preedit string */
    char preeditUtf8[HIME_MAX_PREEDIT * 2];
    char rawPreedit[HIME_MAX_PREEDIT];
    int len = hime_get_preedit (m_himeCtx, rawPreedit, sizeof (rawPreedit));

    if (len <= 0)
        return S_OK;

    /* Prepend method label to preedit so users always know which mode is active.
     * e.g., "[倉]竹手一 1.嗨" or "[注]ㄇㄚ 1.媽" */
    const char *label = hime_get_method_label (m_himeCtx);
    snprintf (preeditUtf8, sizeof (preeditUtf8), "[%s]%s", label, rawPreedit);
    len = strlen (preeditUtf8);

#ifndef NDEBUG
    /* Debug: log mode and candidate count (not in composition text) */
    hime_log ("Preedit: chinese=%d candidates=%d text='%s'",
              m_himeCtx ? hime_is_chinese_mode (m_himeCtx) : 0,
              m_himeCtx ? hime_get_candidate_count (m_himeCtx) : 0,
              preeditUtf8);
#endif

    /* Convert preedit to wide string */
    WCHAR preeditW[HIME_MAX_PREEDIT * 2];
    int wlen = MultiByteToWideChar (CP_UTF8, 0, preeditUtf8, len, preeditW, HIME_MAX_PREEDIT * 2);
    if (wlen <= 0)
        return E_FAIL;

    /* Get composition range and set text */
    ITfRange *pRange = NULL;
    HRESULT hr = m_pComposition->GetRange (&pRange);
    if (SUCCEEDED (hr) && pRange) {
        hr = pRange->SetText (ec, 0, preeditW, wlen);
        pRange->Release ();
    }

    return hr;
}

HRESULT HimeTextService::DoCommit (TfEditCookie ec, ITfContext *pContext) {
    if (m_commitLen <= 0)
        return S_OK;

    if (m_pComposition) {
        /* Replace composition text with committed character, then end */
        ITfRange *pRange = NULL;
        HRESULT hr = m_pComposition->GetRange (&pRange);
        if (SUCCEEDED (hr) && pRange) {
            pRange->SetText (ec, 0, m_commitBuf, m_commitLen);
            pRange->Release ();
        }
        m_pComposition->EndComposition (ec);
        m_pComposition->Release ();
        m_pComposition = NULL;
    } else {
        /* No active composition — insert at selection */
        ITfInsertAtSelection *pInsertAtSelection = NULL;
        HRESULT hr = pContext->QueryInterface (IID_ITfInsertAtSelection, (void **) &pInsertAtSelection);
        if (SUCCEEDED (hr)) {
            ITfRange *pRange = NULL;
            hr = pInsertAtSelection->InsertTextAtSelection (ec, 0, m_commitBuf, m_commitLen, &pRange);
            if (pRange)
                pRange->Release ();
            pInsertAtSelection->Release ();
        }
    }

    m_commitLen = 0;
    return S_OK;
}

HRESULT HimeTextService::DoEndComposition (TfEditCookie ec, ITfContext *pContext) {
    if (m_pComposition) {
        /* Clear composition text so preedit/candidate text doesn't leak into document */
        ITfRange *pRange = NULL;
        HRESULT hr = m_pComposition->GetRange (&pRange);
        if (SUCCEEDED (hr) && pRange) {
            pRange->SetText (ec, 0, L"", 0);
            pRange->Release ();
        }
        m_pComposition->EndComposition (ec);
        m_pComposition->Release ();
        m_pComposition = NULL;
    }
    /* Clear accumulated text */
    m_accumLen = 0;
    if (m_himeCtx) {
        hime_context_reset (m_himeCtx);
    }
    return S_OK;
}

STDMETHODIMP HimeTextService::OnSetFocus (BOOL fForeground) {
    return S_OK;
}

/* Helper: resolve the bare character for a key, ignoring Ctrl/Alt modifiers.
 * This is the same technique Rime/Weasel uses to reliably detect Ctrl+` etc.
 * Returns the Unicode character the key would produce without Ctrl/Alt held. */
static WCHAR resolveBaseChar (WPARAM wParam, LPARAM lParam) {
    BYTE table[256];
    GetKeyboardState (table);
    /* Clear Ctrl and Alt so ToUnicodeEx sees the bare key */
    table[VK_CONTROL] = 0;
    table[VK_LCONTROL] = 0;
    table[VK_RCONTROL] = 0;
    table[VK_MENU] = 0;
    table[VK_LMENU] = 0;
    table[VK_RMENU] = 0;

    WCHAR buf[8] = {0};
    UINT scanCode = (lParam >> 16) & 0xFF;
    int ret = ToUnicodeEx ((UINT) wParam, scanCode, table, buf, 8, 0, NULL);
    return (ret == 1) ? buf[0] : 0;
}

STDMETHODIMP HimeTextService::OnTestKeyDown (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;

    if (!m_himeCtx) {
        return S_OK;
    }

    /* Ctrl+` or F4: eat for Zhuyin ↔ Cangjie switching (before chinese_mode check).
     * Check VK_OEM_3 directly AND resolveBaseChar for maximum compatibility. */
    {
        BYTE ks[256];
        GetKeyboardState (ks);
        bool ctrlHeld = (ks[VK_CONTROL] & 0x80) || (GetKeyState (VK_CONTROL) & 0x8000);

        if (ctrlHeld && wParam == VK_OEM_3) {
            hime_log ("OnTestKeyDown: Ctrl+` eaten (VK_OEM_3)");
            *pfEaten = TRUE;
            return S_OK;
        }
        if (ctrlHeld) {
            WCHAR baseChar = resolveBaseChar (wParam, lParam);
            if (baseChar == L'`') {
                hime_log ("OnTestKeyDown: Ctrl+` eaten (resolveBaseChar)");
                *pfEaten = TRUE;
                return S_OK;
            }
        }
    }
    if (wParam == VK_F4) {
        hime_log ("OnTestKeyDown: F4 eaten");
        *pfEaten = TRUE;
        return S_OK;
    }

    if (!hime_is_chinese_mode (m_himeCtx)) {
        return S_OK;
    }

    HimeInputMethod method = hime_get_input_method (m_himeCtx);

    /* Common keys eaten by all methods */
    if (wParam == VK_SPACE)
        *pfEaten = TRUE;
    else if (wParam == VK_BACK)
        *pfEaten = TRUE;
    else if (wParam == VK_ESCAPE)
        *pfEaten = TRUE;
    else if (wParam == VK_RETURN)
        *pfEaten = TRUE;
    else if (wParam >= 'A' && wParam <= 'Z')
        *pfEaten = TRUE;
    else if (wParam >= '0' && wParam <= '9')
        *pfEaten = TRUE;

    /* Punctuation keys used by some input methods (Zhuyin, Boshiamy, etc.)
     * Check against the current table's keymap for GTAB methods. */
    if (!*pfEaten && (method == HIME_IM_PHO || method == HIME_IM_GTAB)) {
        if (method == HIME_IM_PHO) {
            if (wParam == VK_OEM_1)
                *pfEaten = TRUE; /* ; → ㄤ */
            else if (wParam == VK_OEM_COMMA)
                *pfEaten = TRUE; /* , → ㄝ */
            else if (wParam == VK_OEM_PERIOD)
                *pfEaten = TRUE; /* . → ㄡ */
            else if (wParam == VK_OEM_2)
                *pfEaten = TRUE; /* / → ㄥ */
            else if (wParam == VK_OEM_MINUS)
                *pfEaten = TRUE; /* - → ㄦ */
        } else {
            /* GTAB: eat any key that's in the current table's keymap */
            WCHAR wch[3] = {0};
            BYTE ks[256];
            GetKeyboardState (ks);
            int r = ToUnicode ((UINT) wParam, (lParam >> 16) & 0xFF, ks, wch, 2, 0);
            if (r == 1 && wch[0] > 0 && wch[0] < 128) {
                if (hime_gtab_is_valid_key (m_himeCtx, (char) wch[0]))
                    *pfEaten = TRUE;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP HimeTextService::OnTestKeyUp (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP HimeTextService::OnKeyDown (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;

    if (!m_himeCtx) {
        return S_OK;
    }

    /* Convert virtual key to character */
    BYTE keyState[256];
    GetKeyboardState (keyState);

    WCHAR wch[3] = {0};
    int result = ToUnicode ((UINT) wParam, (lParam >> 16) & 0xFF, keyState, wch, 2, 0);

    UINT charCode = 0;
    if (result == 1) {
        charCode = wch[0];
    }

    /* Ensure known keys always have correct charCode even if ToUnicode fails.
     * ToUnicode can return 0 when a TSF IME is active, losing the character.
     * We must map all keys used in Zhuyin keyboard layouts. */
    if (charCode == 0) {
        if (wParam == VK_SPACE)
            charCode = ' ';
        else if (wParam == VK_RETURN)
            charCode = '\r';
        else if (wParam == VK_BACK)
            charCode = 0x08;
        else if (wParam == VK_ESCAPE)
            charCode = 0x1B;
        else if (wParam >= '0' && wParam <= '9')
            charCode = (UINT) wParam;
        else if (wParam >= 'A' && wParam <= 'Z')
            charCode = (UINT) wParam + 32; /* lowercase */
        else if (wParam == VK_OEM_COMMA)
            charCode = ','; /* ㄝ */
        else if (wParam == VK_OEM_PERIOD)
            charCode = '.'; /* ㄡ */
        else if (wParam == VK_OEM_1)
            charCode = ';'; /* ㄤ */
        else if (wParam == VK_OEM_2)
            charCode = '/'; /* ㄥ */
        else if (wParam == VK_OEM_MINUS)
            charCode = '-'; /* ㄦ */
    }

    UINT modifiers = 0;
    if (keyState[VK_SHIFT] & 0x80)
        modifiers |= HIME_MOD_SHIFT;
    if (keyState[VK_CONTROL] & 0x80)
        modifiers |= HIME_MOD_CONTROL;
    if (keyState[VK_MENU] & 0x80)
        modifiers |= HIME_MOD_ALT;

    /* Ctrl+` or F4: toggle between Zhuyin and Cangjie.
     * Check both VK_OEM_3 directly and resolveBaseChar for maximum compatibility. */
    bool isToggle = false;
    if (wParam == VK_F4) {
        isToggle = true;
    } else if (modifiers & HIME_MOD_CONTROL) {
        if (wParam == VK_OEM_3) {
            isToggle = true;
        } else {
            WCHAR baseChar = resolveBaseChar (wParam, lParam);
            if (baseChar == L'`')
                isToggle = true;
        }
    }

    if (isToggle)
        hime_log ("Toggle detected: vk=0x%02x mod=0x%x", (unsigned) wParam, modifiers);

    if (isToggle) {
        _RequestEditSession (pContext, HimeEditSession::ACTION_END_COMPOSITION);
        _CycleInputMethod (m_himeCtx);
        hime_log ("Toggle: now '%s'", hime_get_method_label (m_himeCtx));
        UpdateLanguageBar ();
        *pfEaten = TRUE;
        return S_OK;
    }

    /* After a commit, swallow the next Space or Enter key.
     * When Cangjie auto-commits on the last key of a code (e.g., ytumb→端),
     * the user often presses Space/Enter as a "confirm" habit. Without this,
     * that Space/Enter passes through to the app and can delete or replace
     * the just-committed character. */
    if (m_justCommitted) {
        m_justCommitted = FALSE;
        if (wParam == VK_SPACE || wParam == VK_RETURN) {
            hime_log ("OnKeyDown: swallowed post-commit vk=0x%02x", (unsigned) wParam);
            *pfEaten = TRUE;
            return S_OK;
        }
    }

    /* Process key through HIME */
    HimeKeyResult kr = hime_process_key (m_himeCtx, (uint32_t) wParam, charCode, modifiers);
    hime_log ("OnKeyDown: vk=0x%02x char=0x%04x mod=0x%x result=%d",
              (unsigned) wParam, charCode, modifiers, (int) kr);

    switch (kr) {
    case HIME_KEY_COMMIT: {
        char commitUtf8[256];
        int len = hime_get_commit (m_himeCtx, commitUtf8, sizeof (commitUtf8));
        if (len > 0) {
            /* Convert to wide string and request edit session */
            m_commitLen = MultiByteToWideChar (CP_UTF8, 0, commitUtf8, len,
                                               m_commitBuf, sizeof (m_commitBuf) / sizeof (WCHAR));
            if (m_commitLen > 0) {
                _RequestEditSession (pContext, HimeEditSession::ACTION_COMMIT);
            }
        }
        hime_clear_commit (m_himeCtx);
        m_justCommitted = TRUE;
        *pfEaten = TRUE;
        break;
    }
    case HIME_KEY_PREEDIT: {
        _RequestEditSession (pContext, HimeEditSession::ACTION_UPDATE_COMPOSITION);
        *pfEaten = TRUE;
        break;
    }
    case HIME_KEY_ABSORBED: {
        _RequestEditSession (pContext, HimeEditSession::ACTION_END_COMPOSITION);
        UpdateLanguageBar ();
        *pfEaten = TRUE;
        break;
    }
    default:
        break;
    }

    return S_OK;
}

STDMETHODIMP HimeTextService::OnKeyUp (ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP HimeTextService::OnPreservedKey (ITfContext *pContext, REFGUID rguid, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP HimeTextService::OnCompositionTerminated (TfEditCookie ecWrite, ITfComposition *pComposition) {
    /* Clear composition text so debug prefix doesn't leak into document */
    if (m_pComposition) {
        ITfRange *pRange = NULL;
        m_pComposition->GetRange (&pRange);
        if (pRange) {
            pRange->SetText (ecWrite, 0, L"", 0);
            pRange->Release ();
        }
        m_pComposition->Release ();
        m_pComposition = NULL;
    }
    if (m_himeCtx) {
        hime_context_reset (m_himeCtx);
    }
    return S_OK;
}

void HimeTextService::_EndComposition () {
    if (m_pComposition) {
        /* Note: Can't call EndComposition here without edit cookie.
         * The composition will be ended via the next edit session. */
        m_pComposition->Release ();
        m_pComposition = NULL;
    }
    /* Clear accumulated text */
    m_accumLen = 0;
    if (m_himeCtx) {
        hime_context_reset (m_himeCtx);
    }
}

/* ========== HimeLangBarButton Implementation ========== */

HimeLangBarButton::HimeLangBarButton (HimeTextService *pService)
    : m_cRef (1),
      m_pService (pService),
      m_pLangBarItemSink (NULL),
      m_dwSinkCookie (0) {
}

HimeLangBarButton::~HimeLangBarButton () {
    if (m_pLangBarItemSink) {
        m_pLangBarItemSink->Release ();
    }
}

STDMETHODIMP HimeLangBarButton::QueryInterface (REFIID riid, void **ppvObj) {
    if (ppvObj == NULL)
        return E_INVALIDARG;
    *ppvObj = NULL;

    if (IsEqualIID (riid, IID_IUnknown) ||
        IsEqualIID (riid, IID_ITfLangBarItem) ||
        IsEqualIID (riid, IID_ITfLangBarItemButton)) {
        *ppvObj = (ITfLangBarItemButton *) this;
    } else if (IsEqualIID (riid, IID_ITfSource)) {
        *ppvObj = (ITfSource *) this;
    }

    if (*ppvObj) {
        AddRef ();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_ (ULONG)
HimeLangBarButton::AddRef () {
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_ (ULONG)
HimeLangBarButton::Release () {
    LONG cr = InterlockedDecrement (&m_cRef);
    if (cr == 0)
        delete this;
    return cr;
}

STDMETHODIMP HimeLangBarButton::GetInfo (TF_LANGBARITEMINFO *pInfo) {
    if (!pInfo)
        return E_INVALIDARG;

    pInfo->clsidService = CLSID_HimeTextService;
    pInfo->guidItem = GUID_HimeLangBarButton;
    pInfo->dwStyle = TF_LBI_STYLE_BTN_MENU | TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
    pInfo->ulSort = 0;
    wcsncpy (pInfo->szDescription, L"HIME", ARRAYSIZE (pInfo->szDescription));
    return S_OK;
}

STDMETHODIMP HimeLangBarButton::GetStatus (DWORD *pdwStatus) {
    if (!pdwStatus)
        return E_INVALIDARG;
    *pdwStatus = 0;
    return S_OK;
}

STDMETHODIMP HimeLangBarButton::Show (BOOL fShow) {
    return E_NOTIMPL;
}

STDMETHODIMP HimeLangBarButton::GetTooltipString (BSTR *pbstrToolTip) {
    if (!pbstrToolTip)
        return E_INVALIDARG;

    *pbstrToolTip = SysAllocString (L"\u59EC HIME Input Method Editor");
    return (*pbstrToolTip) ? S_OK : E_OUTOFMEMORY;
}

/* Load a PNG file as an HICON using GDI+, scaled to the given size.
 * Returns NULL on failure. */
static HICON
_LoadPngAsIcon (const WCHAR *pngPath, int size) {
    _EnsureGdiplusInit ();
    if (!g_gdiplusToken)
        return NULL;

    Gdiplus::Bitmap *bmp = Gdiplus::Bitmap::FromFile (pngPath);
    if (!bmp || bmp->GetLastStatus () != Gdiplus::Ok) {
        delete bmp;
        return NULL;
    }

    /* Scale to target size */
    Gdiplus::Bitmap *scaled = new Gdiplus::Bitmap (size, size, PixelFormat32bppARGB);
    if (!scaled) {
        delete bmp;
        return NULL;
    }

    Gdiplus::Graphics g (scaled);
    g.SetInterpolationMode (Gdiplus::InterpolationModeHighQualityBicubic);
    g.DrawImage (bmp, 0, 0, size, size);

    HICON hIcon = NULL;
    scaled->GetHICON (&hIcon);

    delete scaled;
    delete bmp;
    return hIcon;
}

/* Create a GDI-rendered fallback icon with label text and colored background */
static HICON
_CreateFallbackIcon (const char *label, COLORREF bgColor, int size) {
    WCHAR labelW[8];
    MultiByteToWideChar (CP_UTF8, 0, label, -1, labelW, 8);

    HDC hdcScreen = GetDC (NULL);
    HDC hdcMem = CreateCompatibleDC (hdcScreen);

    BITMAPINFO bmi;
    ZeroMemory (&bmi, sizeof (bmi));
    bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; /* Top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *pvBits = NULL;
    HBITMAP hbmColor = CreateDIBSection (hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    HBITMAP hbmOld = (HBITMAP) SelectObject (hdcMem, hbmColor);

    RECT rc = {0, 0, size, size};
    HBRUSH hBrush = CreateSolidBrush (bgColor);
    FillRect (hdcMem, &rc, hBrush);
    DeleteObject (hBrush);

    SetBkMode (hdcMem, TRANSPARENT);
    SetTextColor (hdcMem, RGB (255, 255, 255));

    int fontHeight = (wcslen (labelW) <= 2) ? 13 : 10;
    HFONT hFont = CreateFontW (fontHeight, 0, 0, 0, FW_BOLD,
                               FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY, DEFAULT_PITCH,
                               L"Microsoft JhengHei");
    HFONT hOldFont = (HFONT) SelectObject (hdcMem, hFont);
    DrawTextW (hdcMem, labelW, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject (hdcMem, hOldFont);
    DeleteObject (hFont);
    SelectObject (hdcMem, hbmOld);

    if (pvBits) {
        BYTE *pixels = (BYTE *) pvBits;
        for (int i = 0; i < size * size; i++)
            pixels[i * 4 + 3] = 255;
    }

    HBITMAP hbmMask = CreateBitmap (size, size, 1, 1, NULL);
    HDC hdcMask = CreateCompatibleDC (hdcScreen);
    HBITMAP hbmMaskOld = (HBITMAP) SelectObject (hdcMask, hbmMask);
    RECT rcMask = {0, 0, size, size};
    FillRect (hdcMask, &rcMask, (HBRUSH) GetStockObject (BLACK_BRUSH));
    SelectObject (hdcMask, hbmMaskOld);
    DeleteDC (hdcMask);

    ICONINFO ii;
    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbmColor;
    HICON hIcon = CreateIconIndirect (&ii);

    DeleteObject (hbmMask);
    DeleteObject (hbmColor);
    DeleteDC (hdcMem);
    ReleaseDC (NULL, hdcScreen);
    return hIcon;
}

/* Standalone mode icon creator used by HimeModeButton and toolbar */
static HICON
_CreateModeIconForContext (HimeContext *ctx) {
    const int SIZE = 16;
    int methodIdx = _GetCurrentMethodIndex (ctx);

    /* EN mode: use cached English icon */
    if (methodIdx < 0) {
        if (g_enIcon) {
            HICON copy = CopyIcon (g_enIcon);
            if (copy)
                return copy;
        }
        /* Try loading hime-tray.png */
        _EnsureGdiplusInit ();
        HICON hIcon = NULL;
        if (g_gdiplusToken) {
            WCHAR dllDir[MAX_PATH];
            GetModuleFileNameW (g_hInst, dllDir, MAX_PATH);
            WCHAR *sep = wcsrchr (dllDir, L'\\');
            if (sep) *sep = L'\0';
            WCHAR pngPath[MAX_PATH];
            _snwprintf (pngPath, MAX_PATH, L"%ls\\icons\\hime-tray.png", dllDir);
            hIcon = _LoadPngAsIcon (pngPath, SIZE);
        }
        if (!hIcon)
            hIcon = _CreateFallbackIcon ("EN", RGB (80, 80, 80), SIZE);
        if (hIcon) {
            if (g_enIcon) DestroyIcon (g_enIcon);
            g_enIcon = hIcon;
            return CopyIcon (hIcon);
        }
        return NULL;
    }

    /* Chinese mode: use per-method cached icon */
    HimeMethodSlot *slot = &g_methods[methodIdx];
    if (slot->cached_icon) {
        HICON copy = CopyIcon (slot->cached_icon);
        if (copy)
            return copy;
    }

    /* Try loading PNG icon */
    _EnsureGdiplusInit ();
    HICON hIcon = NULL;
    if (g_gdiplusToken && slot->icon_filename[0]) {
        WCHAR dllDir[MAX_PATH];
        GetModuleFileNameW (g_hInst, dllDir, MAX_PATH);
        WCHAR *sep = wcsrchr (dllDir, L'\\');
        if (sep) *sep = L'\0';

        WCHAR iconW[64];
        MultiByteToWideChar (CP_UTF8, 0, slot->icon_filename, -1, iconW, 64);
        WCHAR pngPath[MAX_PATH];
        _snwprintf (pngPath, MAX_PATH, L"%ls\\icons\\%ls", dllDir, iconW);
        hIcon = _LoadPngAsIcon (pngPath, SIZE);
    }

    /* Fallback: GDI text icon using method label */
    if (!hIcon) {
        const char *label = ctx ? hime_get_method_label (ctx) : "?";
        COLORREF bgColor;
        if (methodIdx == 0)
            bgColor = RGB (0, 90, 180);   /* Blue for Zhuyin */
        else if (slot->group == 1)
            bgColor = RGB (180, 90, 0);   /* Orange for International */
        else if (slot->group == 2)
            bgColor = RGB (128, 0, 128);  /* Purple for Symbols */
        else
            bgColor = RGB (0, 130, 60);   /* Green for Chinese GTAB */
        hIcon = _CreateFallbackIcon (label, bgColor, SIZE);
    }

    if (hIcon) {
        if (slot->cached_icon) DestroyIcon (slot->cached_icon);
        slot->cached_icon = hIcon;
        return CopyIcon (hIcon);
    }
    return NULL;
}

/* Create the HIME app icon (hime.png) for the main language bar button */
static HICON
_CreateAppIcon (void) {
    const int SIZE = 16;

    /* Return cached icon */
    if (g_appIcon) {
        HICON copy = CopyIcon (g_appIcon);
        if (copy)
            return copy;
    }

    /* Try loading hime.png via GDI+ */
    _EnsureGdiplusInit ();
    HICON hIcon = NULL;
    if (g_gdiplusToken) {
        WCHAR dllDir[MAX_PATH];
        GetModuleFileNameW (g_hInst, dllDir, MAX_PATH);
        WCHAR *sep = wcsrchr (dllDir, L'\\');
        if (sep)
            *sep = L'\0';

        WCHAR pngPath[MAX_PATH];
        _snwprintf (pngPath, MAX_PATH, L"%ls\\icons\\hime.png", dllDir);

        hIcon = _LoadPngAsIcon (pngPath, SIZE);
    }

    /* Fallback: draw a "姬" icon with purple background */
    if (!hIcon) {
        HDC hdcScreen = GetDC (NULL);
        HDC hdcMem = CreateCompatibleDC (hdcScreen);

        BITMAPINFO bmi;
        ZeroMemory (&bmi, sizeof (bmi));
        bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = SIZE;
        bmi.bmiHeader.biHeight = -SIZE;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *pvBits = NULL;
        HBITMAP hbmColor = CreateDIBSection (hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        HBITMAP hbmOld = (HBITMAP) SelectObject (hdcMem, hbmColor);

        RECT rc = {0, 0, SIZE, SIZE};
        HBRUSH hBrush = CreateSolidBrush (RGB (100, 50, 150));
        FillRect (hdcMem, &rc, hBrush);
        DeleteObject (hBrush);

        SetBkMode (hdcMem, TRANSPARENT);
        SetTextColor (hdcMem, RGB (255, 255, 255));
        HFONT hFont = CreateFontW (13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                   DEFAULT_PITCH, L"Microsoft JhengHei");
        HFONT hOldFont = (HFONT) SelectObject (hdcMem, hFont);
        DrawTextW (hdcMem, L"\u59EC", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject (hdcMem, hOldFont);
        DeleteObject (hFont);
        SelectObject (hdcMem, hbmOld);

        if (pvBits) {
            BYTE *pixels = (BYTE *) pvBits;
            for (int i = 0; i < SIZE * SIZE; i++)
                pixels[i * 4 + 3] = 255;
        }

        HBITMAP hbmMask = CreateBitmap (SIZE, SIZE, 1, 1, NULL);
        HDC hdcMask = CreateCompatibleDC (hdcScreen);
        HBITMAP hbmMaskOld = (HBITMAP) SelectObject (hdcMask, hbmMask);
        RECT rcMask = {0, 0, SIZE, SIZE};
        FillRect (hdcMask, &rcMask, (HBRUSH) GetStockObject (BLACK_BRUSH));
        SelectObject (hdcMask, hbmMaskOld);
        DeleteDC (hdcMask);

        ICONINFO ii;
        ii.fIcon = TRUE;
        ii.xHotspot = 0;
        ii.yHotspot = 0;
        ii.hbmMask = hbmMask;
        ii.hbmColor = hbmColor;
        hIcon = CreateIconIndirect (&ii);

        DeleteObject (hbmMask);
        DeleteObject (hbmColor);
        DeleteDC (hdcMem);
        ReleaseDC (NULL, hdcScreen);
    }

    if (hIcon) {
        g_appIcon = hIcon;
        return CopyIcon (hIcon);
    }
    return hIcon;
}

STDMETHODIMP HimeLangBarButton::GetIcon (HICON *phIcon) {
    if (!phIcon)
        return E_INVALIDARG;
    /* Main button shows the HIME app icon, not the mode icon */
    *phIcon = _CreateAppIcon ();
    return (*phIcon) ? S_OK : E_FAIL;
}

STDMETHODIMP HimeLangBarButton::GetText (BSTR *pbstrText) {
    if (!pbstrText)
        return E_INVALIDARG;

    *pbstrText = SysAllocString (L"\u59EC HIME"); /* 姬 HIME */
    return (*pbstrText) ? S_OK : E_OUTOFMEMORY;
}

/* Helper: cycle to next enabled method. Used by both main button and mode button.
 * Cycle: EN → method[0] → method[1] → ... → EN (only enabled+available methods) */
static void
_CycleInputMethod (HimeContext *ctx) {
    if (!ctx)
        return;

    /* Build list of enabled+available method indices */
    int enabled[HIME_MAX_METHODS];
    int enabledCount = 0;
    for (int i = 0; i < g_methodCount; i++) {
        if (g_methods[i].available && g_methods[i].enabled) {
            enabled[enabledCount++] = i;
        }
    }

    if (enabledCount == 0) {
        /* No methods enabled — stay in EN */
        hime_set_chinese_mode (ctx, false);
        return;
    }

    int current = _GetCurrentMethodIndex (ctx);

    if (current < 0) {
        /* EN mode → first enabled method */
        _SwitchToMethod (ctx, enabled[0]);
        return;
    }

    /* Find current in enabled list and advance */
    int pos = -1;
    for (int i = 0; i < enabledCount; i++) {
        if (enabled[i] == current) {
            pos = i;
            break;
        }
    }

    if (pos < 0 || pos + 1 >= enabledCount) {
        /* Current not in enabled list, or last enabled method → EN */
        hime_set_chinese_mode (ctx, false);
    } else {
        /* Advance to next enabled */
        _SwitchToMethod (ctx, enabled[pos + 1]);
    }
}

STDMETHODIMP HimeLangBarButton::OnClick (TfLBIClick click, POINT pt, const RECT *prcArea) {
    /* Left-click cycles through enabled methods */
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (!ctx)
        return S_OK;

    _CycleInputMethod (ctx);

    m_pService->UpdateLanguageBar ();
    return S_OK;
}

STDMETHODIMP HimeLangBarButton::InitMenu (ITfMenu *pMenu) {
    if (!pMenu)
        return E_INVALIDARG;

    int currentIdx = -1;
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (ctx)
        currentIdx = _GetCurrentMethodIndex (ctx);

    /* Show all available methods, grouped by category.
     * Group headers: Chinese, International, Symbols */
    static const WCHAR *groupNames[] = {L"--- Chinese ---", L"--- International ---", L"--- Symbols ---"};
    int lastGroup = -1;

    for (int i = 0; i < g_methodCount; i++) {
        if (!g_methods[i].available)
            continue;

        /* Insert group separator when group changes */
        if (g_methods[i].group != lastGroup) {
            if (lastGroup >= 0) {
                pMenu->AddMenuItem (0, TF_LBMENUF_SEPARATOR, NULL, NULL, NULL, 0, NULL);
            }
            /* Only show group header if there are international or symbol methods */
            if (g_methods[i].group > 0) {
                pMenu->AddMenuItem (0, TF_LBMENUF_GRAYED, NULL, NULL,
                                    groupNames[g_methods[i].group],
                                    (ULONG) wcslen (groupNames[g_methods[i].group]), NULL);
            }
            lastGroup = g_methods[i].group;
        }

        DWORD flags = (i == currentIdx) ? TF_LBMENUF_CHECKED : 0;
        UINT menuId = (i == 0) ? HIME_MENU_ID_ZHUYIN
                                : (UINT) (HIME_MENU_ID_GTAB_BASE + g_methods[i].gtab_index);

        pMenu->AddMenuItem (menuId, flags, NULL, NULL,
                            g_methods[i].display_name,
                            (ULONG) wcslen (g_methods[i].display_name), NULL);
    }

    /* Separator */
    pMenu->AddMenuItem (0, TF_LBMENUF_SEPARATOR, NULL, NULL, NULL, 0, NULL);

    /* IME Toolbar toggle */
    DWORD flagToolbar = g_toolbarVisible ? TF_LBMENUF_CHECKED : 0;
    pMenu->AddMenuItem (HIME_MENU_ID_TOOLBAR, flagToolbar, NULL, NULL,
                        L"IME Toolbar", 11, NULL);

    /* Separator */
    pMenu->AddMenuItem (0, TF_LBMENUF_SEPARATOR, NULL, NULL, NULL, 0, NULL);

    /* Settings */
    pMenu->AddMenuItem (HIME_MENU_ID_SETTINGS, 0, NULL, NULL,
                        L"Settings...", 11, NULL);

    /* About */
    pMenu->AddMenuItem (HIME_MENU_ID_ABOUT, 0, NULL, NULL,
                        L"About HIME", 10, NULL);

    return S_OK;
}

STDMETHODIMP HimeLangBarButton::OnMenuSelect (UINT wID) {
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;

    if (wID == HIME_MENU_ID_ZHUYIN) {
        /* Zhuyin (slot 0) */
        if (ctx) {
            _SwitchToMethod (ctx, 0);
            m_pService->UpdateLanguageBar ();
        }
    } else if (wID >= HIME_MENU_ID_GTAB_BASE && wID < HIME_MENU_ID_TOOLBAR) {
        /* GTAB method: find slot by gtab_index */
        int gtabIdx = (int) wID - HIME_MENU_ID_GTAB_BASE;
        if (ctx) {
            for (int i = 1; i < g_methodCount; i++) {
                if (g_methods[i].gtab_index == gtabIdx) {
                    _SwitchToMethod (ctx, i);
                    m_pService->UpdateLanguageBar ();
                    break;
                }
            }
        }
    } else if (wID == HIME_MENU_ID_TOOLBAR) {
        g_toolbarVisible = !g_toolbarVisible;
        _ShowToolbar (g_toolbarVisible);
    } else if (wID == HIME_MENU_ID_SETTINGS) {
        _ShowSettingsDialog ();
    } else if (wID == HIME_MENU_ID_ABOUT) {
        _ShowAboutDialog ();
    }

    return S_OK;
}

STDMETHODIMP HimeLangBarButton::AdviseSink (REFIID riid, IUnknown *punk, DWORD *pdwCookie) {
    if (!IsEqualIID (riid, IID_ITfLangBarItemSink))
        return CONNECT_E_CANNOTCONNECT;

    if (m_pLangBarItemSink) {
        return CONNECT_E_ADVISELIMIT;
    }

    HRESULT hr = punk->QueryInterface (IID_ITfLangBarItemSink, (void **) &m_pLangBarItemSink);
    if (FAILED (hr)) {
        m_pLangBarItemSink = NULL;
        return hr;
    }

    *pdwCookie = 1;
    m_dwSinkCookie = 1;
    return S_OK;
}

STDMETHODIMP HimeLangBarButton::UnadviseSink (DWORD dwCookie) {
    if (dwCookie != m_dwSinkCookie || !m_pLangBarItemSink)
        return CONNECT_E_NOCONNECTION;

    m_pLangBarItemSink->Release ();
    m_pLangBarItemSink = NULL;
    m_dwSinkCookie = 0;
    return S_OK;
}

void HimeLangBarButton::Update () {
    if (m_pLangBarItemSink) {
        m_pLangBarItemSink->OnUpdate (TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
    }
}

/* ========== HimeModeButton Implementation ========== */

HimeModeButton::HimeModeButton (HimeTextService *pService)
    : m_cRef (1),
      m_pService (pService),
      m_pLangBarItemSink (NULL),
      m_dwSinkCookie (0) {
}

HimeModeButton::~HimeModeButton () {
    if (m_pLangBarItemSink) {
        m_pLangBarItemSink->Release ();
    }
}

STDMETHODIMP HimeModeButton::QueryInterface (REFIID riid, void **ppvObj) {
    if (ppvObj == NULL)
        return E_INVALIDARG;
    *ppvObj = NULL;

    if (IsEqualIID (riid, IID_IUnknown) ||
        IsEqualIID (riid, IID_ITfLangBarItem) ||
        IsEqualIID (riid, IID_ITfLangBarItemButton)) {
        *ppvObj = (ITfLangBarItemButton *) this;
    } else if (IsEqualIID (riid, IID_ITfSource)) {
        *ppvObj = (ITfSource *) this;
    }

    if (*ppvObj) {
        AddRef ();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_ (ULONG)
HimeModeButton::AddRef () {
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_ (ULONG)
HimeModeButton::Release () {
    LONG cr = InterlockedDecrement (&m_cRef);
    if (cr == 0)
        delete this;
    return cr;
}

STDMETHODIMP HimeModeButton::GetInfo (TF_LANGBARITEMINFO *pInfo) {
    if (!pInfo)
        return E_INVALIDARG;

    pInfo->clsidService = CLSID_HimeTextService;
    pInfo->guidItem = GUID_HimeModeButton;
    pInfo->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
    pInfo->ulSort = 1;
    wcsncpy (pInfo->szDescription, L"HIME Mode", ARRAYSIZE (pInfo->szDescription));
    return S_OK;
}

STDMETHODIMP HimeModeButton::GetStatus (DWORD *pdwStatus) {
    if (!pdwStatus)
        return E_INVALIDARG;
    *pdwStatus = 0;
    return S_OK;
}

STDMETHODIMP HimeModeButton::Show (BOOL fShow) {
    return E_NOTIMPL;
}

STDMETHODIMP HimeModeButton::GetTooltipString (BSTR *pbstrToolTip) {
    if (!pbstrToolTip)
        return E_INVALIDARG;

    const char *label = "";
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (ctx) {
        label = hime_get_method_label (ctx);
    }

    WCHAR tooltip[128];
    WCHAR labelW[32];
    MultiByteToWideChar (CP_UTF8, 0, label, -1, labelW, 32);
    swprintf (tooltip, 128, L"HIME - %s", labelW);

    *pbstrToolTip = SysAllocString (tooltip);
    return (*pbstrToolTip) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP HimeModeButton::OnClick (TfLBIClick click, POINT pt, const RECT *prcArea) {
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (!ctx)
        return S_OK;

    _CycleInputMethod (ctx);
    m_pService->UpdateLanguageBar ();
    return S_OK;
}

STDMETHODIMP HimeModeButton::InitMenu (ITfMenu *pMenu) {
    return E_NOTIMPL;
}

STDMETHODIMP HimeModeButton::OnMenuSelect (UINT wID) {
    return E_NOTIMPL;
}

STDMETHODIMP HimeModeButton::GetIcon (HICON *phIcon) {
    if (!phIcon)
        return E_INVALIDARG;
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    *phIcon = _CreateModeIconForContext (ctx);
    hime_log ("ModeButton::GetIcon: ctx=%p icon=%p methodIdx=%d",
              (void *) ctx, (void *) *phIcon,
              ctx ? _GetCurrentMethodIndex (ctx) : -99);
    return (*phIcon) ? S_OK : E_FAIL;
}

STDMETHODIMP HimeModeButton::GetText (BSTR *pbstrText) {
    if (!pbstrText)
        return E_INVALIDARG;

    const char *label = "en";
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (ctx)
        label = hime_get_method_label (ctx);

    WCHAR labelW[16];
    MultiByteToWideChar (CP_UTF8, 0, label, -1, labelW, 16);
    if (wcscmp (labelW, L"en") == 0)
        wcscpy (labelW, L"\u82F1"); /* 英 */

    WCHAR text[32];
    swprintf (text, 32, L"%s \u59EC", labelW); /* X 姬 */
    *pbstrText = SysAllocString (text);
    return (*pbstrText) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP HimeModeButton::AdviseSink (REFIID riid, IUnknown *punk, DWORD *pdwCookie) {
    if (!IsEqualIID (riid, IID_ITfLangBarItemSink))
        return CONNECT_E_CANNOTCONNECT;

    if (m_pLangBarItemSink)
        return CONNECT_E_ADVISELIMIT;

    HRESULT hr = punk->QueryInterface (IID_ITfLangBarItemSink, (void **) &m_pLangBarItemSink);
    if (FAILED (hr)) {
        m_pLangBarItemSink = NULL;
        return hr;
    }

    *pdwCookie = 2;
    m_dwSinkCookie = 2;
    return S_OK;
}

STDMETHODIMP HimeModeButton::UnadviseSink (DWORD dwCookie) {
    if (dwCookie != m_dwSinkCookie || !m_pLangBarItemSink)
        return CONNECT_E_NOCONNECTION;

    m_pLangBarItemSink->Release ();
    m_pLangBarItemSink = NULL;
    m_dwSinkCookie = 0;
    return S_OK;
}

void HimeModeButton::Update () {
    hime_log ("ModeButton::Update: sink=%p", (void *) m_pLangBarItemSink);
    if (m_pLangBarItemSink) {
        m_pLangBarItemSink->OnUpdate (TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
    }
}

/* ========== About & Settings Dialogs ========== */

static void
_ShowAboutDialog (void) {
    MessageBoxW (NULL,
                 L"\u59EC HIME Input Method Editor\n"
                 L"\n"
                 L"Version: 0.10.1\n"
                 L"Updated: 2026-02-15\n"
                 L"\n"
                 L"https://github.com/nicehime/hime",
                 L"About HIME",
                 MB_OK | MB_ICONINFORMATION);
}

/* Settings dialog: shows checkboxes for enabling/disabling IME methods.
 * At least one Chinese method must remain enabled. */

#define IDC_CHECK_METHOD_BASE 200 /* Checkbox IDs: 200 + method index */

static INT_PTR CALLBACK
_SettingsDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        /* Set initial checkbox states from g_methods[] */
        for (int i = 0; i < g_methodCount; i++) {
            int cbId = IDC_CHECK_METHOD_BASE + i;
            if (!g_methods[i].available) {
                EnableWindow (GetDlgItem (hDlg, cbId), FALSE);
                CheckDlgButton (hDlg, cbId, BST_UNCHECKED);
            } else {
                CheckDlgButton (hDlg, cbId, g_methods[i].enabled ? BST_CHECKED : BST_UNCHECKED);
            }
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDOK: {
            /* Read checkbox states */
            BOOL anyEnabled = FALSE;
            for (int i = 0; i < g_methodCount; i++) {
                if (!g_methods[i].available)
                    continue;
                int cbId = IDC_CHECK_METHOD_BASE + i;
                if (IsDlgButtonChecked (hDlg, cbId) == BST_CHECKED) {
                    anyEnabled = TRUE;
                    break;
                }
            }

            if (!anyEnabled) {
                MessageBoxW (hDlg,
                             L"\u81F3\u5C11\u8981\u555F\u7528\u4E00\u500B\u8F38\u5165\u6CD5\u3002\n"
                             L"At least one input method must be enabled.",
                             L"HIME Settings",
                             MB_OK | MB_ICONWARNING);
                return TRUE;
            }

            /* Apply new states */
            for (int i = 0; i < g_methodCount; i++) {
                if (!g_methods[i].available)
                    continue;
                int cbId = IDC_CHECK_METHOD_BASE + i;
                g_methods[i].enabled = (IsDlgButtonChecked (hDlg, cbId) == BST_CHECKED);
            }

            /* If current method was just disabled, switch to first enabled */
            HimeContext *ctx = g_pActiveService ? g_pActiveService->GetHimeContext () : NULL;
            if (ctx && hime_is_chinese_mode (ctx)) {
                int curIdx = _GetCurrentMethodIndex (ctx);
                if (curIdx >= 0 && !g_methods[curIdx].enabled) {
                    /* Find first enabled+available method */
                    for (int i = 0; i < g_methodCount; i++) {
                        if (g_methods[i].available && g_methods[i].enabled) {
                            _SwitchToMethod (ctx, i);
                            break;
                        }
                    }
                    if (g_pActiveService)
                        g_pActiveService->UpdateLanguageBar ();
                }
            }

            /* Pre-load newly enabled GTAB tables */
            if (ctx) {
                for (int i = 0; i < g_methodCount; i++) {
                    if (g_methods[i].gtab_index >= 0 && g_methods[i].available && g_methods[i].enabled) {
                        HimeGtabInfo info;
                        if (hime_gtab_get_table_info (g_methods[i].gtab_index, &info) == 0 && !info.loaded) {
                            hime_gtab_load_table (ctx, info.filename);
                        }
                    }
                }
            }

            _SaveSettings ();
            EndDialog (hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog (hDlg, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog (hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

/* Create the settings dialog programmatically using a DLGTEMPLATE in memory.
 * Generates checkboxes dynamically from g_methods[], grouped by category. */
static void
_ShowSettingsDialog (void) {
    /* Align helper: DLGTEMPLATE items must be DWORD-aligned */
#define ALIGN_DWORD(p) (((BYTE *) (p)) + ((4 - ((ULONG_PTR) (p) &3)) & 3))

    BYTE buf[8192];
    memset (buf, 0, sizeof (buf));
    BYTE *p = buf;

    /* Count available methods to size the dialog */
    int availCount = 0;
    for (int i = 0; i < g_methodCount; i++) {
        if (g_methods[i].available)
            availCount++;
    }

    /* Count group headers needed (International, Symbols — skip Chinese header) */
    int groupHeaders = 0;
    int lastGroup = -1;
    for (int i = 0; i < g_methodCount; i++) {
        if (!g_methods[i].available)
            continue;
        if (g_methods[i].group != lastGroup && g_methods[i].group > 0) {
            groupHeaders++;
        }
        lastGroup = g_methods[i].group;
    }

    int totalItems = availCount + groupHeaders + 2; /* +2 for OK/Cancel */
    int yBase = 10;
    int dialogHeight = yBase + (availCount + groupHeaders) * 16 + 30;
    if (dialogHeight < 100) dialogHeight = 100;

    /* DLGTEMPLATE */
    DLGTEMPLATE *dlg = (DLGTEMPLATE *) p;
    dlg->style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT;
    dlg->cdit = (WORD) totalItems;
    dlg->cx = 200;
    dlg->cy = (short) dialogHeight;
    p += sizeof (DLGTEMPLATE);

    /* Menu (none) */
    *(WORD *) p = 0;
    p += sizeof (WORD);
    /* Class (default) */
    *(WORD *) p = 0;
    p += sizeof (WORD);
    /* Title */
    const WCHAR *title = L"HIME Settings — Toggle Cycle Methods";
    int titleLen = (int) (wcslen (title) + 1) * (int) sizeof (WCHAR);
    memcpy (p, title, titleLen);
    p += titleLen;
    /* Font size */
    *(WORD *) p = 9;
    p += sizeof (WORD);
    /* Font name */
    const WCHAR *fontName = L"Segoe UI";
    int fontLen = (int) (wcslen (fontName) + 1) * (int) sizeof (WCHAR);
    memcpy (p, fontName, fontLen);
    p += fontLen;

    /* Helper macro for adding DLGITEMTEMPLATE */
#define ADD_ITEM(id_, style_, x_, y_, cx_, cy_, cls_, text_) \
    do {                                                     \
        p = ALIGN_DWORD (p);                                 \
        DLGITEMTEMPLATE *item = (DLGITEMTEMPLATE *) p;       \
        item->style = (style_) | WS_CHILD | WS_VISIBLE;      \
        item->x = (x_);                                      \
        item->y = (y_);                                      \
        item->cx = (cx_);                                    \
        item->cy = (cy_);                                    \
        item->id = (id_);                                    \
        p += sizeof (DLGITEMTEMPLATE);                       \
        *(WORD *) p = 0xFFFF;                                \
        p += sizeof (WORD);                                  \
        *(WORD *) p = (cls_);                                \
        p += sizeof (WORD);                                  \
        const WCHAR *_t = (text_);                           \
        int _tl = (int) (wcslen (_t) + 1) * (int) sizeof (WCHAR); \
        memcpy (p, _t, _tl);                                 \
        p += _tl;                                            \
        *(WORD *) p = 0;                                     \
        p += sizeof (WORD);                                  \
    } while (0)

    int y = yBase;
    static const WCHAR *groupLabels[] = {NULL, L"International", L"Symbols"};
    lastGroup = -1;

    for (int i = 0; i < g_methodCount; i++) {
        if (!g_methods[i].available)
            continue;

        /* Group header for International/Symbols */
        if (g_methods[i].group != lastGroup && g_methods[i].group > 0) {
            ADD_ITEM (0xFFFF, SS_LEFT, 10, (short) y, 180, 12, 0x0082,
                      groupLabels[g_methods[i].group]);
            y += 14;
            lastGroup = g_methods[i].group;
        } else if (g_methods[i].group != lastGroup) {
            lastGroup = g_methods[i].group;
        }

        int cbId = IDC_CHECK_METHOD_BASE + i;
        ADD_ITEM ((WORD) cbId, BS_AUTOCHECKBOX | WS_TABSTOP,
                  14, (short) y, 180, 12, 0x0080, g_methods[i].display_name);
        y += 14;
    }

    y += 8;

    /* OK button */
    ADD_ITEM (IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP,
              50, (short) y, 50, 14, 0x0080, L"OK");

    /* Cancel button */
    ADD_ITEM (IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP,
              105, (short) y, 50, 14, 0x0080, L"Cancel");

#undef ADD_ITEM
#undef ALIGN_DWORD

    /* Adjust dialog height to fit content */
    dlg->cy = (short) (y + 24);

    hime_log ("_ShowSettingsDialog: buffer used %d of 8192 bytes, %d items, dlg height=%d",
              (int) (p - buf), (int) totalItems, (int) dlg->cy);
    INT_PTR dlgResult = DialogBoxIndirectW (g_hInst, (DLGTEMPLATE *) buf, NULL, _SettingsDlgProc);
    hime_log ("_ShowSettingsDialog: DialogBoxIndirectW returned %lld (err=%lu)",
              (long long) dlgResult, (dlgResult == -1) ? GetLastError () : 0);
}

/* ========== Floating Toolbar ========== */

static LRESULT CALLBACK
_ToolbarWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint (hwnd, &ps);

        /* Fill background */
        RECT rc;
        GetClientRect (hwnd, &rc);
        HBRUSH hBrush = CreateSolidBrush (RGB (240, 240, 240));
        FillRect (hdc, &rc, hBrush);
        DeleteObject (hBrush);

        /* Draw thin border */
        HPEN hPen = CreatePen (PS_SOLID, 1, RGB (180, 180, 180));
        HPEN hOldPen = (HPEN) SelectObject (hdc, hPen);
        Rectangle (hdc, 0, 0, rc.right, rc.bottom);
        SelectObject (hdc, hOldPen);
        DeleteObject (hPen);

        SetBkMode (hdc, TRANSPARENT);

        /* Draw mode icon area (16x16 at position 4,6) */
        HimeContext *ctx = g_pActiveService ? g_pActiveService->GetHimeContext () : NULL;
        HICON hModeIcon = _CreateModeIconForContext (ctx);
        if (hModeIcon) {
            DrawIconEx (hdc, 4, 6, hModeIcon, 16, 16, 0, NULL, DI_NORMAL);
            DestroyIcon (hModeIcon);
        }

        /* Draw full/half-width indicator */
        HFONT hFont = CreateFontW (16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                   DEFAULT_PITCH, L"Microsoft JhengHei");
        HFONT hOldFont = (HFONT) SelectObject (hdc, hFont);
        SetTextColor (hdc, RGB (50, 50, 50));

        RECT rcFH = {24, 2, 48, 26};
        DrawTextW (hdc, g_fullWidth ? L"\uFF21" : L"A", 1, &rcFH,
                   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        /* Draw settings gear text */
        RECT rcGear = {50, 2, 74, 26};
        DrawTextW (hdc, L"\u2699", 1, &rcGear,
                   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject (hdc, hOldFont);
        DeleteObject (hFont);

        EndPaint (hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD (lParam);
        HimeContext *ctx = g_pActiveService ? g_pActiveService->GetHimeContext () : NULL;

        if (x < 24) {
            /* Clicked mode icon — cycle input method */
            if (ctx) {
                _CycleInputMethod (ctx);
                if (g_pActiveService)
                    g_pActiveService->UpdateLanguageBar ();
            }
        } else if (x >= 50) {
            /* Clicked settings gear */
            _ShowSettingsDialog ();
        } else {
            /* Clicked full/half-width area — toggle */
            g_fullWidth = !g_fullWidth;
            InvalidateRect (hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_DESTROY:
        g_hwndToolbar = NULL;
        return 0;
    }

    return DefWindowProc (hwnd, msg, wParam, lParam);
}

static void
_EnsureToolbarClassRegistered (void) {
    if (g_toolbarClassRegistered)
        return;

    WNDCLASSW wc;
    ZeroMemory (&wc, sizeof (wc));
    wc.lpfnWndProc = _ToolbarWndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.lpszClassName = TOOLBAR_CLASS;

    if (RegisterClassW (&wc))
        g_toolbarClassRegistered = TRUE;
}

static void
_ShowToolbar (BOOL show) {
    hime_log ("_ShowToolbar: show=%d current_hwnd=%p classReg=%d",
              (int) show, (void *) g_hwndToolbar, (int) g_toolbarClassRegistered);
    if (show) {
        _EnsureToolbarClassRegistered ();
        if (!g_toolbarClassRegistered) {
            hime_log ("_ShowToolbar: FAILED to register toolbar window class (err=%lu)",
                      GetLastError ());
            return;
        }

        if (!g_hwndToolbar) {
            /* Position: bottom-right of screen, above taskbar */
            RECT rcWork;
            SystemParametersInfo (SPI_GETWORKAREA, 0, &rcWork, 0);
            int w = 78, h = 28;
            int x = rcWork.right - w - 10;
            int y = rcWork.bottom - h - 10;

            hime_log ("_ShowToolbar: creating window at (%d,%d) %dx%d hInst=%p",
                      x, y, w, h, (void *) g_hInst);
            g_hwndToolbar = CreateWindowExW (
                WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                TOOLBAR_CLASS, L"HIME Toolbar",
                WS_POPUP,
                x, y, w, h,
                NULL, NULL, g_hInst, NULL);
            if (!g_hwndToolbar) {
                hime_log ("_ShowToolbar: CreateWindowExW FAILED err=%lu", GetLastError ());
            }
        }
        if (g_hwndToolbar) {
            ShowWindow (g_hwndToolbar, SW_SHOWNOACTIVATE);
            InvalidateRect (g_hwndToolbar, NULL, TRUE);
            hime_log ("_ShowToolbar: shown hwnd=%p", (void *) g_hwndToolbar);
        }
        g_toolbarVisible = TRUE;
    } else {
        if (g_hwndToolbar) {
            DestroyWindow (g_hwndToolbar);
            g_hwndToolbar = NULL;
        }
        g_toolbarVisible = FALSE;
        hime_log ("_ShowToolbar: hidden");
    }
}

static void
_UpdateToolbar (void) {
    if (g_toolbarVisible && g_hwndToolbar) {
        InvalidateRect (g_hwndToolbar, NULL, TRUE);
    }
}

/* ========== HimeClassFactory Implementation ========== */

STDMETHODIMP HimeClassFactory::QueryInterface (REFIID riid, void **ppvObj) {
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID (riid, IID_IUnknown) || IsEqualIID (riid, IID_IClassFactory)) {
        *ppvObj = (IClassFactory *) this;
        AddRef ();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_ (ULONG)
HimeClassFactory::AddRef () {
    InterlockedIncrement (&g_cRefDll);
    return 2;
}

STDMETHODIMP_ (ULONG)
HimeClassFactory::Release () {
    InterlockedDecrement (&g_cRefDll);
    return 1;
}

STDMETHODIMP HimeClassFactory::CreateInstance (IUnknown *pUnkOuter, REFIID riid, void **ppvObj) {
    if (ppvObj == NULL)
        return E_INVALIDARG;
    *ppvObj = NULL;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    HimeTextService *pService = new HimeTextService ();
    if (pService == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = pService->QueryInterface (riid, ppvObj);
    pService->Release ();

    return hr;
}

STDMETHODIMP HimeClassFactory::LockServer (BOOL fLock) {
    if (fLock) {
        InterlockedIncrement (&g_cRefDll);
    } else {
        InterlockedDecrement (&g_cRefDll);
    }
    return S_OK;
}

/* ========== DLL Entry Points ========== */

static HimeClassFactory g_ClassFactory;

STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, void **ppvObj) {
    if (ppvObj == NULL)
        return E_INVALIDARG;
    *ppvObj = NULL;

    if (!IsEqualCLSID (rclsid, CLSID_HimeTextService)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return g_ClassFactory.QueryInterface (riid, ppvObj);
}

STDAPI DllCanUnloadNow () {
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}

/* Helper to convert GUID to string */
static BOOL GuidToString (REFGUID guid, WCHAR *buf, int bufSize) {
    return StringFromGUID2 (guid, buf, bufSize) > 0;
}

/* Helper to create registry key */
static LONG CreateRegKeyAndValue (HKEY hKeyRoot, const WCHAR *subKey, const WCHAR *valueName, const WCHAR *value) {
    HKEY hKey;
    LONG result = RegCreateKeyExW (hKeyRoot, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE, NULL, &hKey, NULL);
    if (result == ERROR_SUCCESS) {
        if (value) {
            result = RegSetValueExW (hKey, valueName, 0, REG_SZ,
                                     (const BYTE *) value, (wcslen (value) + 1) * sizeof (WCHAR));
        }
        RegCloseKey (hKey);
    }
    return result;
}

/* Helper to delete registry key tree */
static LONG DeleteRegKeyTree (HKEY hKeyRoot, const WCHAR *subKey) {
    return RegDeleteTreeW (hKeyRoot, subKey);
}

STDAPI DllRegisterServer () {
    HRESULT hr = S_OK;
    WCHAR modulePath[MAX_PATH];
    WCHAR clsidStr[64];
    WCHAR subKey[256];

    /* Get module path */
    GetModuleFileNameW (g_hInst, modulePath, MAX_PATH);
    hime_log ("DllRegisterServer: module path: %ls", modulePath);

    /* Convert CLSID to string */
    if (!GuidToString (CLSID_HimeTextService, clsidStr, 64)) {
        hime_log ("DllRegisterServer: FAILED to convert CLSID to string");
        return E_FAIL;
    }
    hime_log ("DllRegisterServer: CLSID=%ls", clsidStr);

    /* Register CLSID under HKCR\CLSID */
    swprintf (subKey, 256, L"CLSID\\%s", clsidStr);
    LONG regResult = CreateRegKeyAndValue (HKEY_CLASSES_ROOT, subKey, NULL, TEXTSERVICE_DESC);
    hime_log ("DllRegisterServer: HKCR\\CLSID result=%ld", regResult);
    if (regResult != ERROR_SUCCESS) {
        return E_FAIL;
    }

    /* Register InprocServer32 */
    swprintf (subKey, 256, L"CLSID\\%s\\InprocServer32", clsidStr);
    regResult = CreateRegKeyAndValue (HKEY_CLASSES_ROOT, subKey, NULL, modulePath);
    hime_log ("DllRegisterServer: InprocServer32 path result=%ld", regResult);
    if (regResult != ERROR_SUCCESS) {
        return E_FAIL;
    }
    regResult = CreateRegKeyAndValue (HKEY_CLASSES_ROOT, subKey, L"ThreadingModel", TEXTSERVICE_MODEL);
    hime_log ("DllRegisterServer: ThreadingModel result=%ld", regResult);
    if (regResult != ERROR_SUCCESS) {
        return E_FAIL;
    }

    /* Register with TSF using ITfInputProcessorProfiles */
    ITfInputProcessorProfiles *pProfiles = NULL;
    hr = CoCreateInstance (CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfInputProcessorProfiles, (void **) &pProfiles);
    hime_log ("DllRegisterServer: CoCreateInstance(InputProcessorProfiles) hr=0x%08lx", (unsigned long) hr);
    if (SUCCEEDED (hr) && pProfiles) {
        /* Register language profile */
        hr = pProfiles->Register (CLSID_HimeTextService);
        hime_log ("DllRegisterServer: Register(CLSID) hr=0x%08lx", (unsigned long) hr);
        if (SUCCEEDED (hr)) {
            /* Add language profile for Traditional Chinese (zh-TW) */
            hr = pProfiles->AddLanguageProfile (
                CLSID_HimeTextService,
                MAKELANGID (LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), /* 0x0404 */
                GUID_HimeProfile,
                TEXTSERVICE_DESC,
                (ULONG) wcslen (TEXTSERVICE_DESC),
                modulePath,
                (ULONG) wcslen (modulePath),
                0 /* Icon index */
            );
            hime_log ("DllRegisterServer: AddLanguageProfile(0x0404) hr=0x%08lx", (unsigned long) hr);
        }
        pProfiles->Release ();
    }

    /* Register with ITfCategoryMgr for keyboard input */
    ITfCategoryMgr *pCategoryMgr = NULL;
    hr = CoCreateInstance (CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfCategoryMgr, (void **) &pCategoryMgr);
    hime_log ("DllRegisterServer: CoCreateInstance(CategoryMgr) hr=0x%08lx", (unsigned long) hr);
    if (SUCCEEDED (hr) && pCategoryMgr) {
        HRESULT hr2;
        /* Register as keyboard */
        hr2 = pCategoryMgr->RegisterCategory (CLSID_HimeTextService,
                                        GUID_TFCAT_TIP_KEYBOARD,
                                        CLSID_HimeTextService);
        hime_log ("DllRegisterServer: RegisterCategory(KEYBOARD) hr=0x%08lx", (unsigned long) hr2);
        /* Register systray support (shows language bar in system tray) */
        hr2 = pCategoryMgr->RegisterCategory (CLSID_HimeTextService,
                                        GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
                                        CLSID_HimeTextService);
        hime_log ("DllRegisterServer: RegisterCategory(SYSTRAYSUPPORT) hr=0x%08lx", (unsigned long) hr2);
        /* Register immersive/UWP support (Windows 8+) */
        hr2 = pCategoryMgr->RegisterCategory (CLSID_HimeTextService,
                                        GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
                                        CLSID_HimeTextService);
        hime_log ("DllRegisterServer: RegisterCategory(IMMERSIVESUPPORT) hr=0x%08lx", (unsigned long) hr2);
        pCategoryMgr->Release ();
    }

    hime_log ("DllRegisterServer: complete, final hr=0x%08lx", (unsigned long) hr);
    return hr;
}

STDAPI DllUnregisterServer () {
    HRESULT hr = S_OK;
    WCHAR clsidStr[64];
    WCHAR subKey[256];

    hime_log ("DllUnregisterServer: starting");

    /* Convert CLSID to string */
    if (!GuidToString (CLSID_HimeTextService, clsidStr, 64)) {
        hime_log ("DllUnregisterServer: FAILED to convert CLSID");
        return E_FAIL;
    }
    hime_log ("DllUnregisterServer: CLSID=%ls", clsidStr);

    /* Unregister from TSF.
     * Remove the language profile first, then unregister the text service.
     * This ensures Windows removes the keyboard entry from Settings. */
    ITfInputProcessorProfiles *pProfiles = NULL;
    hr = CoCreateInstance (CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfInputProcessorProfiles, (void **) &pProfiles);
    hime_log ("DllUnregisterServer: CoCreateInstance(Profiles) hr=0x%08lx", (unsigned long) hr);
    if (SUCCEEDED (hr) && pProfiles) {
        HRESULT hr2;
        hr2 = pProfiles->RemoveLanguageProfile (
            CLSID_HimeTextService,
            MAKELANGID (LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL),
            GUID_HimeProfile);
        hime_log ("DllUnregisterServer: RemoveLanguageProfile hr=0x%08lx", (unsigned long) hr2);
        hr2 = pProfiles->Unregister (CLSID_HimeTextService);
        hime_log ("DllUnregisterServer: Unregister hr=0x%08lx", (unsigned long) hr2);
        pProfiles->Release ();
    }

    /* Unregister from category */
    ITfCategoryMgr *pCategoryMgr = NULL;
    hr = CoCreateInstance (CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfCategoryMgr, (void **) &pCategoryMgr);
    hime_log ("DllUnregisterServer: CoCreateInstance(CategoryMgr) hr=0x%08lx", (unsigned long) hr);
    if (SUCCEEDED (hr) && pCategoryMgr) {
        pCategoryMgr->UnregisterCategory (CLSID_HimeTextService,
                                          GUID_TFCAT_TIP_KEYBOARD,
                                          CLSID_HimeTextService);
        pCategoryMgr->UnregisterCategory (CLSID_HimeTextService,
                                          GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
                                          CLSID_HimeTextService);
        pCategoryMgr->UnregisterCategory (CLSID_HimeTextService,
                                          GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
                                          CLSID_HimeTextService);
        hime_log ("DllUnregisterServer: UnregisterCategory calls done");
        pCategoryMgr->Release ();
    }

    /* Delete CLSID registry keys */
    swprintf (subKey, 256, L"CLSID\\%s", clsidStr);
    LONG regResult = DeleteRegKeyTree (HKEY_CLASSES_ROOT, subKey);
    hime_log ("DllUnregisterServer: DeleteRegKeyTree(HKCR\\CLSID) result=%ld", regResult);

    hime_log ("DllUnregisterServer: complete");
    return S_OK;
}

BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        DisableThreadLibraryCalls (hInstance);
        break;
    case DLL_PROCESS_DETACH:
        /* Destroy floating toolbar */
        if (g_hwndToolbar) {
            DestroyWindow (g_hwndToolbar);
            g_hwndToolbar = NULL;
        }
        if (g_toolbarClassRegistered) {
            UnregisterClassW (TOOLBAR_CLASS, hInstance);
            g_toolbarClassRegistered = FALSE;
        }

        /* Destroy cached icons */
        for (int i = 0; i < g_methodCount; i++) {
            if (g_methods[i].cached_icon) {
                DestroyIcon (g_methods[i].cached_icon);
                g_methods[i].cached_icon = NULL;
            }
        }
        if (g_enIcon) {
            DestroyIcon (g_enIcon);
            g_enIcon = NULL;
        }
        if (g_appIcon) {
            DestroyIcon (g_appIcon);
            g_appIcon = NULL;
        }

        g_pActiveService = NULL;

        /* Note: Do NOT call GdiplusShutdown() here.  DllMain runs under
         * the loader lock and GdiplusShutdown tries to join its background
         * thread, which causes a deadlock (the thread needs the loader lock
         * to exit).  The OS reclaims all resources when the process exits
         * or the DLL is unloaded. */

        hime_cleanup ();
        break;
    }
    return TRUE;
}
