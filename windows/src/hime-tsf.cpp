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
static HICON g_iconCache[3] = {NULL, NULL, NULL}; /* EN, PHO, GTAB */
static int g_iconCacheMode[3] = {-1, -1, -1};

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

/* Forward declarations */
class HimeTextService;
class HimeClassFactory;
class HimeLangBarButton;

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
    HICON _CreateModeIcon ();

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
        m_himeCtx = hime_context_new ();
        if (m_himeCtx) {
            hime_log ("Activate: Context created OK, chinese_mode=%d",
                      hime_is_chinese_mode (m_himeCtx));

            /* Pre-load Cangjie 5 table so it's ready for switching.
             * This switches the context to GTAB mode temporarily. */
            int cj5_rc = hime_gtab_load_table (m_himeCtx, "cj5.gtab");
            hime_log ("Activate: Pre-load cj5.gtab returned %d", cj5_rc);

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
    _UninitLanguageBar ();
    _UninitKeystrokeSink ();

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
    if (FAILED (hr))
        return hr;

    m_pLangBarButton = new HimeLangBarButton (this);
    if (!m_pLangBarButton) {
        pLangBarItemMgr->Release ();
        return E_OUTOFMEMORY;
    }

    hr = pLangBarItemMgr->AddItem (m_pLangBarButton);
    pLangBarItemMgr->Release ();

    if (FAILED (hr)) {
        m_pLangBarButton->Release ();
        m_pLangBarButton = NULL;
    }
    return hr;
}

void HimeTextService::_UninitLanguageBar () {
    if (m_pLangBarButton && m_pThreadMgr) {
        ITfLangBarItemMgr *pLangBarItemMgr = NULL;
        HRESULT hr = m_pThreadMgr->QueryInterface (IID_ITfLangBarItemMgr, (void **) &pLangBarItemMgr);
        if (SUCCEEDED (hr)) {
            pLangBarItemMgr->RemoveItem (m_pLangBarButton);
            pLangBarItemMgr->Release ();
        }
        m_pLangBarButton->Release ();
        m_pLangBarButton = NULL;
    }
}

void HimeTextService::UpdateLanguageBar () {
    if (m_pLangBarButton) {
        m_pLangBarButton->Update ();
    }
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

    /* Zhuyin-specific punctuation keys (not needed for Cangjie) */
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

        /* 3-way cycle: EN → Zhuyin → Cangjie → EN */
        if (!hime_is_chinese_mode (m_himeCtx)) {
            /* EN → Zhuyin */
            hime_set_chinese_mode (m_himeCtx, true);
            hime_set_input_method (m_himeCtx, HIME_IM_PHO);
            hime_log ("Toggle: EN -> Zhuyin");
        } else if (hime_get_input_method (m_himeCtx) == HIME_IM_PHO) {
            /* Zhuyin → Cangjie */
            hime_gtab_load_table_by_id (m_himeCtx, HIME_GTAB_CJ5);
            hime_log ("Toggle: Zhuyin -> Cangjie 5");
        } else {
            /* Cangjie → EN */
            hime_set_chinese_mode (m_himeCtx, false);
            hime_log ("Toggle: Cangjie -> EN");
        }

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
    pInfo->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
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

    const char *label = "";
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (ctx) {
        label = hime_get_method_label (ctx);
    }

    /* Build tooltip like "HIME - 注音" */
    WCHAR tooltip[128];
    WCHAR labelW[32];
    MultiByteToWideChar (CP_UTF8, 0, label, -1, labelW, 32);
    swprintf (tooltip, 128, L"HIME - %s", labelW);

    *pbstrToolTip = SysAllocString (tooltip);
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

/* Determine which PNG icon file to use for the current mode.
 * Returns a mode index (0=EN, 1=PHO, 2=GTAB) and sets pngName. */
static int
_GetModeIndex (HimeContext *ctx, const WCHAR **pngName) {
    bool isChinese = ctx && hime_is_chinese_mode (ctx);
    if (!isChinese) {
        *pngName = L"hime-tray.png";
        return 0;
    }
    HimeInputMethod method = hime_get_input_method (ctx);
    if (method == HIME_IM_PHO) {
        *pngName = L"juyin.png";
        return 1;
    }
    *pngName = L"cj5.png";
    return 2;
}

HICON HimeLangBarButton::_CreateModeIcon () {
    const int SIZE = 16;

    /* Get current mode */
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    const WCHAR *pngName = L"hime-tray.png";
    int modeIdx = _GetModeIndex (ctx, &pngName);

    /* Return cached icon if mode hasn't changed */
    if (g_iconCache[modeIdx]) {
        HICON copy = CopyIcon (g_iconCache[modeIdx]);
        if (copy)
            return copy;
    }

    /* Try loading PNG icon via GDI+ */
    HICON hIcon = NULL;
    if (g_gdiplusToken) {
        WCHAR dllDir[MAX_PATH];
        GetModuleFileNameW (g_hInst, dllDir, MAX_PATH);
        WCHAR *sep = wcsrchr (dllDir, L'\\');
        if (sep)
            *sep = L'\0';

        WCHAR pngPath[MAX_PATH];
        _snwprintf (pngPath, MAX_PATH, L"%ls\\icons\\%ls", dllDir, pngName);

        hIcon = _LoadPngAsIcon (pngPath, SIZE);
    }

    /* Fall back to GDI-drawn icon if PNG not available */
    if (!hIcon) {
        const char *label = "EN";
        if (ctx) {
            label = hime_get_method_label (ctx);
        }

        WCHAR labelW[8];
        MultiByteToWideChar (CP_UTF8, 0, label, -1, labelW, 8);

        /* Choose background color based on mode */
        COLORREF bgColor;
        if (modeIdx == 0) {
            bgColor = RGB (80, 80, 80); /* Dark gray for English */
        } else if (modeIdx == 1) {
            bgColor = RGB (0, 90, 180); /* Blue for Zhuyin */
        } else {
            bgColor = RGB (0, 130, 60); /* Green for Cangjie/GTAB */
        }

        /* Create a 16x16 icon using GDI */
        HDC hdcScreen = GetDC (NULL);
        HDC hdcMem = CreateCompatibleDC (hdcScreen);

        BITMAPINFO bmi;
        ZeroMemory (&bmi, sizeof (bmi));
        bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = SIZE;
        bmi.bmiHeader.biHeight = -SIZE; /* Top-down */
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *pvBits = NULL;
        HBITMAP hbmColor = CreateDIBSection (hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        HBITMAP hbmOld = (HBITMAP) SelectObject (hdcMem, hbmColor);

        /* Fill background */
        RECT rc = {0, 0, SIZE, SIZE};
        HBRUSH hBrush = CreateSolidBrush (bgColor);
        FillRect (hdcMem, &rc, hBrush);
        DeleteObject (hBrush);

        /* Draw label text */
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

        /* Set alpha channel to opaque (GDI doesn't set it) */
        if (pvBits) {
            BYTE *pixels = (BYTE *) pvBits;
            for (int i = 0; i < SIZE * SIZE; i++) {
                pixels[i * 4 + 3] = 255; /* Alpha */
            }
        }

        /* Create mask bitmap (all black = fully opaque icon) */
        HBITMAP hbmMask = CreateBitmap (SIZE, SIZE, 1, 1, NULL);
        HDC hdcMask = CreateCompatibleDC (hdcScreen);
        HBITMAP hbmMaskOld = (HBITMAP) SelectObject (hdcMask, hbmMask);
        RECT rcMask = {0, 0, SIZE, SIZE};
        HBRUSH hBlack = (HBRUSH) GetStockObject (BLACK_BRUSH);
        FillRect (hdcMask, &rcMask, hBlack);
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

    /* Cache the icon */
    if (hIcon) {
        if (g_iconCache[modeIdx])
            DestroyIcon (g_iconCache[modeIdx]);
        g_iconCache[modeIdx] = hIcon;
        return CopyIcon (hIcon);
    }

    return hIcon;
}

STDMETHODIMP HimeLangBarButton::GetIcon (HICON *phIcon) {
    if (!phIcon)
        return E_INVALIDARG;
    *phIcon = _CreateModeIcon ();
    return (*phIcon) ? S_OK : E_FAIL;
}

STDMETHODIMP HimeLangBarButton::GetText (BSTR *pbstrText) {
    if (!pbstrText)
        return E_INVALIDARG;

    /* Build mode-specific text: "英姬", "注姬", "倉姬", etc. */
    const char *label = "en";
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (ctx) {
        label = hime_get_method_label (ctx);
    }

    WCHAR labelW[16];
    MultiByteToWideChar (CP_UTF8, 0, label, -1, labelW, 16);

    /* Map English-mode "en" label to "英" for display */
    if (wcscmp (labelW, L"en") == 0) {
        wcscpy (labelW, L"英");
    }

    WCHAR text[32];
    swprintf (text, 32, L"%s 姬", labelW);

    *pbstrText = SysAllocString (text);
    return (*pbstrText) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP HimeLangBarButton::OnClick (TfLBIClick click, POINT pt, const RECT *prcArea) {
    /* Left-click cycles: EN → Zhuyin → Cangjie → EN */
    HimeContext *ctx = m_pService ? m_pService->GetHimeContext () : NULL;
    if (!ctx)
        return S_OK;

    if (!hime_is_chinese_mode (ctx)) {
        hime_set_chinese_mode (ctx, true);
        hime_set_input_method (ctx, HIME_IM_PHO);
    } else if (hime_get_input_method (ctx) == HIME_IM_PHO) {
        hime_gtab_load_table_by_id (ctx, HIME_GTAB_CJ5);
    } else {
        hime_set_chinese_mode (ctx, false);
    }

    Update ();
    return S_OK;
}

STDMETHODIMP HimeLangBarButton::InitMenu (ITfMenu *pMenu) {
    return E_NOTIMPL;
}

STDMETHODIMP HimeLangBarButton::OnMenuSelect (UINT wID) {
    return E_NOTIMPL;
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

    /* Convert CLSID to string */
    if (!GuidToString (CLSID_HimeTextService, clsidStr, 64)) {
        return E_FAIL;
    }

    /* Register CLSID under HKCR\CLSID */
    swprintf (subKey, 256, L"CLSID\\%s", clsidStr);
    if (CreateRegKeyAndValue (HKEY_CLASSES_ROOT, subKey, NULL, TEXTSERVICE_DESC) != ERROR_SUCCESS) {
        return E_FAIL;
    }

    /* Register InprocServer32 */
    swprintf (subKey, 256, L"CLSID\\%s\\InprocServer32", clsidStr);
    if (CreateRegKeyAndValue (HKEY_CLASSES_ROOT, subKey, NULL, modulePath) != ERROR_SUCCESS) {
        return E_FAIL;
    }
    if (CreateRegKeyAndValue (HKEY_CLASSES_ROOT, subKey, L"ThreadingModel", TEXTSERVICE_MODEL) != ERROR_SUCCESS) {
        return E_FAIL;
    }

    /* Register with TSF using ITfInputProcessorProfiles */
    ITfInputProcessorProfiles *pProfiles = NULL;
    hr = CoCreateInstance (CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfInputProcessorProfiles, (void **) &pProfiles);
    if (SUCCEEDED (hr) && pProfiles) {
        /* Register language profile */
        hr = pProfiles->Register (CLSID_HimeTextService);
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
        }
        pProfiles->Release ();
    }

    /* Register with ITfCategoryMgr for keyboard input */
    ITfCategoryMgr *pCategoryMgr = NULL;
    hr = CoCreateInstance (CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfCategoryMgr, (void **) &pCategoryMgr);
    if (SUCCEEDED (hr) && pCategoryMgr) {
        /* Register as keyboard */
        pCategoryMgr->RegisterCategory (CLSID_HimeTextService,
                                        GUID_TFCAT_TIP_KEYBOARD,
                                        CLSID_HimeTextService);
        /* Register systray support (shows language bar in system tray) */
        pCategoryMgr->RegisterCategory (CLSID_HimeTextService,
                                        GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
                                        CLSID_HimeTextService);
        /* Register immersive/UWP support (Windows 8+) */
        pCategoryMgr->RegisterCategory (CLSID_HimeTextService,
                                        GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
                                        CLSID_HimeTextService);
        pCategoryMgr->Release ();
    }

    return hr;
}

STDAPI DllUnregisterServer () {
    HRESULT hr = S_OK;
    WCHAR clsidStr[64];
    WCHAR subKey[256];

    /* Convert CLSID to string */
    if (!GuidToString (CLSID_HimeTextService, clsidStr, 64)) {
        return E_FAIL;
    }

    /* Unregister from TSF */
    ITfInputProcessorProfiles *pProfiles = NULL;
    hr = CoCreateInstance (CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfInputProcessorProfiles, (void **) &pProfiles);
    if (SUCCEEDED (hr) && pProfiles) {
        pProfiles->Unregister (CLSID_HimeTextService);
        pProfiles->Release ();
    }

    /* Unregister from category */
    ITfCategoryMgr *pCategoryMgr = NULL;
    hr = CoCreateInstance (CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                           IID_ITfCategoryMgr, (void **) &pCategoryMgr);
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
        pCategoryMgr->Release ();
    }

    /* Delete CLSID registry keys */
    swprintf (subKey, 256, L"CLSID\\%s", clsidStr);
    DeleteRegKeyTree (HKEY_CLASSES_ROOT, subKey);

    return S_OK;
}

BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        DisableThreadLibraryCalls (hInstance);
        break;
    case DLL_PROCESS_DETACH:
        /* Destroy cached icons */
        for (int i = 0; i < 3; i++) {
            if (g_iconCache[i]) {
                DestroyIcon (g_iconCache[i]);
                g_iconCache[i] = NULL;
            }
        }

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
