/*
 * HIME Text Service Framework (TSF) Implementation
 *
 * This file implements a minimal Windows TSF text service that
 * wraps the HIME core library for Windows IME support.
 *
 * Copyright (C) 2020 The HIME team, Taiwan
 * License: GNU LGPL v2.1
 */

#include <windows.h>
#include <msctf.h>
#include <olectl.h>
#include <assert.h>
#include <stdio.h>
#include <wchar.h>

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

/* GUIDs */
// {B8A45C32-5F6D-4E2A-9C1B-0D3E4F5A6B7C}
static const GUID CLSID_HimeTextService = {
    0xb8a45c32, 0x5f6d, 0x4e2a,
    {0x9c, 0x1b, 0x0d, 0x3e, 0x4f, 0x5a, 0x6b, 0x7c}
};

// {C9B56D43-6E7F-5F3B-AD2C-1E4F5061C8D9}
static const GUID GUID_HimeProfile = {
    0xc9b56d43, 0x6e7f, 0x5f3b,
    {0xad, 0x2c, 0x1e, 0x4f, 0x50, 0x61, 0xc8, 0xd9}
};

/* Constants */
static const WCHAR TEXTSERVICE_DESC[] = L"HIME 輸入法";
static const WCHAR TEXTSERVICE_MODEL[] = L"Apartment";
static HINSTANCE g_hInst = NULL;
static LONG g_cRefDll = 0;

/* Forward declarations */
class HimeTextService;
class HimeClassFactory;

/* ========== HimeTextService Class ========== */

/* Edit session for composition updates */
class HimeEditSession : public ITfEditSession {
public:
    HimeEditSession(class HimeTextService *pService, ITfContext *pContext, int action)
        : m_cRef(1), m_pService(pService), m_pContext(pContext), m_action(action) {
        m_pContext->AddRef();
    }

    ~HimeEditSession() {
        m_pContext->Release();
    }

    /* IUnknown */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) {
        if (ppvObj == NULL) return E_INVALIDARG;
        *ppvObj = NULL;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
            *ppvObj = (ITfEditSession *)this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() {
        LONG cr = InterlockedDecrement(&m_cRef);
        if (cr == 0) delete this;
        return cr;
    }

    /* ITfEditSession */
    STDMETHODIMP DoEditSession(TfEditCookie ec);

    enum { ACTION_START_COMPOSITION = 1, ACTION_UPDATE_COMPOSITION, ACTION_COMMIT };

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
    HimeTextService();
    ~HimeTextService();

    /* IUnknown */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /* ITfTextInputProcessor */
    STDMETHODIMP Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
    STDMETHODIMP Deactivate();

    /* ITfKeyEventSink */
    STDMETHODIMP OnSetFocus(BOOL fForeground);
    STDMETHODIMP OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten);

    /* ITfCompositionSink */
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition);

    /* Called by edit session */
    HRESULT DoStartComposition(TfEditCookie ec, ITfContext *pContext);
    HRESULT DoUpdateComposition(TfEditCookie ec, ITfContext *pContext);
    HRESULT DoCommit(TfEditCookie ec, ITfContext *pContext);

    HimeContext *GetHimeContext() { return m_himeCtx; }
    ITfComposition *GetComposition() { return m_pComposition; }
    void SetComposition(ITfComposition *pComp) { m_pComposition = pComp; }
    TfClientId GetClientId() { return m_tfClientId; }

private:
    HRESULT _InitKeystrokeSink();
    void _UninitKeystrokeSink();
    HRESULT _RequestEditSession(ITfContext *pContext, int action);
    void _EndComposition();

    LONG m_cRef;
    ITfThreadMgr *m_pThreadMgr;
    TfClientId m_tfClientId;
    ITfKeystrokeMgr *m_pKeystrokeMgr;
    ITfComposition *m_pComposition;
    HimeContext *m_himeCtx;
    DWORD m_dwCookie;
    WCHAR m_commitBuf[256];
    int m_commitLen;
};

/* ========== HimeClassFactory Class ========== */

class HimeClassFactory : public IClassFactory {
public:
    /* IUnknown */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /* IClassFactory */
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
    STDMETHODIMP LockServer(BOOL fLock);
};

/* ========== HimeTextService Implementation ========== */

HimeTextService::HimeTextService()
    : m_cRef(1),
      m_pThreadMgr(NULL),
      m_tfClientId(TF_CLIENTID_NULL),
      m_pKeystrokeMgr(NULL),
      m_pComposition(NULL),
      m_himeCtx(NULL),
      m_dwCookie(0),
      m_commitLen(0) {
    memset(m_commitBuf, 0, sizeof(m_commitBuf));
    InterlockedIncrement(&g_cRefDll);
}

HimeTextService::~HimeTextService() {
    if (m_himeCtx) {
        hime_context_free(m_himeCtx);
    }
    InterlockedDecrement(&g_cRefDll);
}

STDMETHODIMP HimeTextService::QueryInterface(REFIID riid, void **ppvObj) {
    if (ppvObj == NULL) return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor)) {
        *ppvObj = (ITfTextInputProcessor *)this;
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = (ITfKeyEventSink *)this;
    } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
        *ppvObj = (ITfCompositionSink *)this;
    }

    if (*ppvObj) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) HimeTextService::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) HimeTextService::Release() {
    LONG cr = InterlockedDecrement(&m_cRef);
    if (cr == 0) {
        delete this;
    }
    return cr;
}

STDMETHODIMP HimeTextService::Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId) {
    m_pThreadMgr = pThreadMgr;
    m_pThreadMgr->AddRef();
    m_tfClientId = tfClientId;

    /* Initialize HIME core */
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(g_hInst, path, MAX_PATH);
    WCHAR *lastSlash = wcsrchr(path, L'\\');
    if (lastSlash) {
        wcscpy(lastSlash + 1, L"data");
    }

    char dataDir[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, path, -1, dataDir, MAX_PATH, NULL, NULL);

    if (hime_init(dataDir) == 0) {
        m_himeCtx = hime_context_new();
    }

    /* Initialize keystroke sink */
    _InitKeystrokeSink();

    return S_OK;
}

STDMETHODIMP HimeTextService::Deactivate() {
    _UninitKeystrokeSink();

    if (m_himeCtx) {
        hime_context_free(m_himeCtx);
        m_himeCtx = NULL;
    }

    if (m_pThreadMgr) {
        m_pThreadMgr->Release();
        m_pThreadMgr = NULL;
    }

    m_tfClientId = TF_CLIENTID_NULL;

    return S_OK;
}

HRESULT HimeTextService::_InitKeystrokeSink() {
    HRESULT hr;
    ITfKeystrokeMgr *pKeystrokeMgr;

    hr = m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr);
    if (FAILED(hr)) return hr;

    hr = pKeystrokeMgr->AdviseKeyEventSink(m_tfClientId, (ITfKeyEventSink *)this, TRUE);
    if (FAILED(hr)) {
        pKeystrokeMgr->Release();
        return hr;
    }

    m_pKeystrokeMgr = pKeystrokeMgr;
    return S_OK;
}

void HimeTextService::_UninitKeystrokeSink() {
    if (m_pKeystrokeMgr) {
        m_pKeystrokeMgr->UnadviseKeyEventSink(m_tfClientId);
        m_pKeystrokeMgr->Release();
        m_pKeystrokeMgr = NULL;
    }
}

HRESULT HimeTextService::_RequestEditSession(ITfContext *pContext, int action) {
    HimeEditSession *pEditSession = new HimeEditSession(this, pContext, action);
    if (!pEditSession) return E_OUTOFMEMORY;

    HRESULT hr;
    hr = pContext->RequestEditSession(m_tfClientId, pEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
    pEditSession->Release();
    return hr;
}

/* Edit session implementation */
STDMETHODIMP HimeEditSession::DoEditSession(TfEditCookie ec) {
    switch (m_action) {
        case ACTION_START_COMPOSITION:
            return m_pService->DoStartComposition(ec, m_pContext);
        case ACTION_UPDATE_COMPOSITION:
            return m_pService->DoUpdateComposition(ec, m_pContext);
        case ACTION_COMMIT:
            return m_pService->DoCommit(ec, m_pContext);
    }
    return E_UNEXPECTED;
}

HRESULT HimeTextService::DoStartComposition(TfEditCookie ec, ITfContext *pContext) {
    HRESULT hr;
    ITfInsertAtSelection *pInsertAtSelection = NULL;
    ITfRange *pRangeInsert = NULL;
    ITfContextComposition *pContextComposition = NULL;

    /* Get insertion point */
    hr = pContext->QueryInterface(IID_ITfInsertAtSelection, (void **)&pInsertAtSelection);
    if (FAILED(hr)) goto Exit;

    hr = pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRangeInsert);
    if (FAILED(hr)) goto Exit;

    /* Start composition */
    hr = pContext->QueryInterface(IID_ITfContextComposition, (void **)&pContextComposition);
    if (FAILED(hr)) goto Exit;

    hr = pContextComposition->StartComposition(ec, pRangeInsert, (ITfCompositionSink *)this, &m_pComposition);

Exit:
    if (pContextComposition) pContextComposition->Release();
    if (pRangeInsert) pRangeInsert->Release();
    if (pInsertAtSelection) pInsertAtSelection->Release();
    return hr;
}

HRESULT HimeTextService::DoUpdateComposition(TfEditCookie ec, ITfContext *pContext) {
    if (!m_pComposition) {
        /* Start composition if not started */
        HRESULT hr = DoStartComposition(ec, pContext);
        if (FAILED(hr)) return hr;
    }

    if (!m_pComposition) return E_FAIL;

    /* Get preedit string */
    char preeditUtf8[HIME_MAX_PREEDIT];
    int len = hime_get_preedit(m_himeCtx, preeditUtf8, sizeof(preeditUtf8));
    if (len <= 0) return S_OK;

    /* Convert to wide string */
    WCHAR preeditW[HIME_MAX_PREEDIT];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, preeditUtf8, len, preeditW, HIME_MAX_PREEDIT);
    if (wlen <= 0) return E_FAIL;

    /* Get composition range and set text */
    ITfRange *pRange = NULL;
    HRESULT hr = m_pComposition->GetRange(&pRange);
    if (SUCCEEDED(hr) && pRange) {
        hr = pRange->SetText(ec, 0, preeditW, wlen);
        pRange->Release();
    }

    return hr;
}

HRESULT HimeTextService::DoCommit(TfEditCookie ec, ITfContext *pContext) {
    if (m_commitLen <= 0) return S_OK;

    if (m_pComposition) {
        /* Replace composition with commit text */
        ITfRange *pRange = NULL;
        HRESULT hr = m_pComposition->GetRange(&pRange);
        if (SUCCEEDED(hr) && pRange) {
            pRange->SetText(ec, 0, m_commitBuf, m_commitLen);
            pRange->Release();
        }

        /* End composition */
        m_pComposition->EndComposition(ec);
        m_pComposition->Release();
        m_pComposition = NULL;
    } else {
        /* Insert directly at selection */
        ITfInsertAtSelection *pInsertAtSelection = NULL;
        HRESULT hr = pContext->QueryInterface(IID_ITfInsertAtSelection, (void **)&pInsertAtSelection);
        if (SUCCEEDED(hr)) {
            ITfRange *pRange = NULL;
            hr = pInsertAtSelection->InsertTextAtSelection(ec, 0, m_commitBuf, m_commitLen, &pRange);
            if (pRange) pRange->Release();
            pInsertAtSelection->Release();
        }
    }

    m_commitLen = 0;
    return S_OK;
}

STDMETHODIMP HimeTextService::OnSetFocus(BOOL fForeground) {
    return S_OK;
}

STDMETHODIMP HimeTextService::OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;

    if (!m_himeCtx || !hime_is_chinese_mode(m_himeCtx)) {
        return S_OK;
    }

    /* Check if this is a key we handle */
    if (wParam >= 'A' && wParam <= 'Z') *pfEaten = TRUE;
    else if (wParam >= '0' && wParam <= '9') *pfEaten = TRUE;
    else if (wParam == VK_SPACE) *pfEaten = TRUE;
    else if (wParam == VK_BACK) *pfEaten = TRUE;
    else if (wParam == VK_ESCAPE) *pfEaten = TRUE;
    else if (wParam == VK_RETURN) *pfEaten = TRUE;
    else if (wParam == VK_OEM_1) *pfEaten = TRUE;  /* ; */
    else if (wParam == VK_OEM_COMMA) *pfEaten = TRUE;
    else if (wParam == VK_OEM_PERIOD) *pfEaten = TRUE;
    else if (wParam == VK_OEM_2) *pfEaten = TRUE;  /* / */
    else if (wParam == VK_OEM_MINUS) *pfEaten = TRUE;

    return S_OK;
}

STDMETHODIMP HimeTextService::OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP HimeTextService::OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;

    if (!m_himeCtx) return S_OK;

    /* Convert virtual key to character */
    BYTE keyState[256];
    GetKeyboardState(keyState);

    WCHAR wch[3] = {0};
    int result = ToUnicode((UINT)wParam, (lParam >> 16) & 0xFF, keyState, wch, 2, 0);

    UINT charCode = 0;
    if (result == 1) {
        charCode = wch[0];
    }

    UINT modifiers = 0;
    if (keyState[VK_SHIFT] & 0x80) modifiers |= HIME_MOD_SHIFT;
    if (keyState[VK_CONTROL] & 0x80) modifiers |= HIME_MOD_CONTROL;
    if (keyState[VK_MENU] & 0x80) modifiers |= HIME_MOD_ALT;

    /* Handle Shift for mode toggle (only on key release for clean toggle) */
    if (wParam == VK_SHIFT && !(modifiers & (HIME_MOD_CONTROL | HIME_MOD_ALT))) {
        /* Toggle handled in OnKeyUp */
        return S_OK;
    }

    /* Process key through HIME */
    HimeKeyResult kr = hime_process_key(m_himeCtx, (uint32_t)wParam, charCode, modifiers);

    switch (kr) {
        case HIME_KEY_COMMIT: {
            char commitUtf8[256];
            int len = hime_get_commit(m_himeCtx, commitUtf8, sizeof(commitUtf8));
            if (len > 0) {
                /* Convert to wide string and request edit session */
                m_commitLen = MultiByteToWideChar(CP_UTF8, 0, commitUtf8, len,
                                                  m_commitBuf, sizeof(m_commitBuf)/sizeof(WCHAR));
                if (m_commitLen > 0) {
                    _RequestEditSession(pContext, HimeEditSession::ACTION_COMMIT);
                }
            }
            hime_clear_commit(m_himeCtx);
            *pfEaten = TRUE;
            break;
        }
        case HIME_KEY_PREEDIT: {
            _RequestEditSession(pContext, HimeEditSession::ACTION_UPDATE_COMPOSITION);
            *pfEaten = TRUE;
            break;
        }
        case HIME_KEY_ABSORBED: {
            _EndComposition();
            *pfEaten = TRUE;
            break;
        }
        default:
            break;
    }

    return S_OK;
}

STDMETHODIMP HimeTextService::OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;

    if (!m_himeCtx) return S_OK;

    /* Handle Shift key release for mode toggle */
    if (wParam == VK_SHIFT) {
        BYTE keyState[256];
        GetKeyboardState(keyState);

        /* Only toggle if no other modifiers are pressed */
        if (!(keyState[VK_CONTROL] & 0x80) && !(keyState[VK_MENU] & 0x80)) {
            hime_toggle_chinese_mode(m_himeCtx);
            *pfEaten = TRUE;
        }
    }

    return S_OK;
}

STDMETHODIMP HimeTextService::OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP HimeTextService::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition) {
    if (m_himeCtx) {
        hime_context_reset(m_himeCtx);
    }
    if (m_pComposition) {
        m_pComposition->Release();
        m_pComposition = NULL;
    }
    return S_OK;
}

void HimeTextService::_EndComposition() {
    if (m_pComposition) {
        /* Note: Can't call EndComposition here without edit cookie */
        /* The composition will be ended when we commit or when terminated */
        m_pComposition->Release();
        m_pComposition = NULL;
    }
    if (m_himeCtx) {
        hime_context_reset(m_himeCtx);
    }
}

/* ========== HimeClassFactory Implementation ========== */

STDMETHODIMP HimeClassFactory::QueryInterface(REFIID riid, void **ppvObj) {
    if (ppvObj == NULL) return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObj = (IClassFactory *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) HimeClassFactory::AddRef() {
    InterlockedIncrement(&g_cRefDll);
    return 2;
}

STDMETHODIMP_(ULONG) HimeClassFactory::Release() {
    InterlockedDecrement(&g_cRefDll);
    return 1;
}

STDMETHODIMP HimeClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj) {
    if (ppvObj == NULL) return E_INVALIDARG;
    *ppvObj = NULL;

    if (pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;

    HimeTextService *pService = new HimeTextService();
    if (pService == NULL) return E_OUTOFMEMORY;

    HRESULT hr = pService->QueryInterface(riid, ppvObj);
    pService->Release();

    return hr;
}

STDMETHODIMP HimeClassFactory::LockServer(BOOL fLock) {
    if (fLock) {
        InterlockedIncrement(&g_cRefDll);
    } else {
        InterlockedDecrement(&g_cRefDll);
    }
    return S_OK;
}

/* ========== DLL Entry Points ========== */

static HimeClassFactory g_ClassFactory;

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj) {
    if (ppvObj == NULL) return E_INVALIDARG;
    *ppvObj = NULL;

    if (!IsEqualCLSID(rclsid, CLSID_HimeTextService)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return g_ClassFactory.QueryInterface(riid, ppvObj);
}

STDAPI DllCanUnloadNow() {
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}

/* Helper to convert GUID to string */
static BOOL GuidToString(REFGUID guid, WCHAR *buf, int bufSize) {
    return StringFromGUID2(guid, buf, bufSize) > 0;
}

/* Helper to create registry key */
static LONG CreateRegKeyAndValue(HKEY hKeyRoot, const WCHAR *subKey, const WCHAR *valueName,
                                  const WCHAR *value) {
    HKEY hKey;
    LONG result = RegCreateKeyExW(hKeyRoot, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE, NULL, &hKey, NULL);
    if (result == ERROR_SUCCESS) {
        if (value) {
            result = RegSetValueExW(hKey, valueName, 0, REG_SZ,
                                    (const BYTE *)value, (wcslen(value) + 1) * sizeof(WCHAR));
        }
        RegCloseKey(hKey);
    }
    return result;
}

/* Helper to delete registry key tree */
static LONG DeleteRegKeyTree(HKEY hKeyRoot, const WCHAR *subKey) {
    return RegDeleteTreeW(hKeyRoot, subKey);
}

STDAPI DllRegisterServer() {
    HRESULT hr = S_OK;
    WCHAR modulePath[MAX_PATH];
    WCHAR clsidStr[64];
    WCHAR subKey[256];

    /* Get module path */
    GetModuleFileNameW(g_hInst, modulePath, MAX_PATH);

    /* Convert CLSID to string */
    if (!GuidToString(CLSID_HimeTextService, clsidStr, 64)) {
        return E_FAIL;
    }

    /* Register CLSID under HKCR\CLSID */
    swprintf(subKey, 256, L"CLSID\\%s", clsidStr);
    if (CreateRegKeyAndValue(HKEY_CLASSES_ROOT, subKey, NULL, TEXTSERVICE_DESC) != ERROR_SUCCESS) {
        return E_FAIL;
    }

    /* Register InprocServer32 */
    swprintf(subKey, 256, L"CLSID\\%s\\InprocServer32", clsidStr);
    if (CreateRegKeyAndValue(HKEY_CLASSES_ROOT, subKey, NULL, modulePath) != ERROR_SUCCESS) {
        return E_FAIL;
    }
    if (CreateRegKeyAndValue(HKEY_CLASSES_ROOT, subKey, L"ThreadingModel", TEXTSERVICE_MODEL) != ERROR_SUCCESS) {
        return E_FAIL;
    }

    /* Register with TSF using ITfInputProcessorProfiles */
    ITfInputProcessorProfiles *pProfiles = NULL;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, (void **)&pProfiles);
    if (SUCCEEDED(hr) && pProfiles) {
        /* Register language profile */
        hr = pProfiles->Register(CLSID_HimeTextService);
        if (SUCCEEDED(hr)) {
            /* Add language profile for Traditional Chinese (zh-TW) */
            hr = pProfiles->AddLanguageProfile(
                CLSID_HimeTextService,
                MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL),  /* 0x0404 */
                GUID_HimeProfile,
                TEXTSERVICE_DESC,
                (ULONG)wcslen(TEXTSERVICE_DESC),
                modulePath,
                (ULONG)wcslen(modulePath),
                0  /* Icon index */
            );
        }
        pProfiles->Release();
    }

    /* Register with ITfCategoryMgr for keyboard input */
    ITfCategoryMgr *pCategoryMgr = NULL;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr, (void **)&pCategoryMgr);
    if (SUCCEEDED(hr) && pCategoryMgr) {
        /* Register as keyboard */
        pCategoryMgr->RegisterCategory(CLSID_HimeTextService,
                                       GUID_TFCAT_TIP_KEYBOARD,
                                       CLSID_HimeTextService);
        pCategoryMgr->Release();
    }

    return hr;
}

STDAPI DllUnregisterServer() {
    HRESULT hr = S_OK;
    WCHAR clsidStr[64];
    WCHAR subKey[256];

    /* Convert CLSID to string */
    if (!GuidToString(CLSID_HimeTextService, clsidStr, 64)) {
        return E_FAIL;
    }

    /* Unregister from TSF */
    ITfInputProcessorProfiles *pProfiles = NULL;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, (void **)&pProfiles);
    if (SUCCEEDED(hr) && pProfiles) {
        pProfiles->Unregister(CLSID_HimeTextService);
        pProfiles->Release();
    }

    /* Unregister from category */
    ITfCategoryMgr *pCategoryMgr = NULL;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr, (void **)&pCategoryMgr);
    if (SUCCEEDED(hr) && pCategoryMgr) {
        pCategoryMgr->UnregisterCategory(CLSID_HimeTextService,
                                         GUID_TFCAT_TIP_KEYBOARD,
                                         CLSID_HimeTextService);
        pCategoryMgr->Release();
    }

    /* Delete CLSID registry keys */
    swprintf(subKey, 256, L"CLSID\\%s", clsidStr);
    DeleteRegKeyTree(HKEY_CLASSES_ROOT, subKey);

    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInst = hInstance;
            DisableThreadLibraryCalls(hInstance);
            break;
        case DLL_PROCESS_DETACH:
            hime_cleanup();
            break;
    }
    return TRUE;
}
