#include "stdafx.h"

#include <WeaselIPCData.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include "WeaselTSF.h"
#include "CandidateList.h"
#include "LanguageBar.h"
#include "Compartment.h"
#include "ResponseParser.h"

static void error_message(const WCHAR* msg) {
  static DWORD next_tick = 0;
  DWORD now = GetTickCount();
  if (now > next_tick) {
    next_tick = now + 10000;  // (ms)
    MessageBox(NULL, msg, get_weasel_ime_name().c_str(), MB_ICONERROR | MB_OK);
  }
}

WeaselTSF::WeaselTSF() {
  _cRef = 1;

  _dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;

  _dwTextEditSinkCookie = TF_INVALID_COOKIE;
  _dwTextLayoutSinkCookie = TF_INVALID_COOKIE;
  _dwThreadFocusSinkCookie = TF_INVALID_COOKIE;
  _fTestKeyDownPending = FALSE;
  _fTestKeyUpPending = FALSE;

  _fCUASWorkaroundTested = _fCUASWorkaroundEnabled = FALSE;

  _cand = new CCandidateList(this);

  DllAddRef();
}

WeaselTSF::~WeaselTSF() {
  DllRelease();
}

STDMETHODIMP WeaselTSF::QueryInterface(REFIID riid, void** ppvObject) {
  if (ppvObject == NULL)
    return E_INVALIDARG;

  *ppvObject = NULL;

  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfTextInputProcessor))
    *ppvObject = (ITfTextInputProcessor*)this;
  else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
    *ppvObject = (ITfTextInputProcessorEx*)this;
  else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    *ppvObject = (ITfThreadMgrEventSink*)this;
  else if (IsEqualIID(riid, IID_ITfTextEditSink))
    *ppvObject = (ITfTextEditSink*)this;
  else if (IsEqualIID(riid, IID_ITfTextLayoutSink))
    *ppvObject = (ITfTextLayoutSink*)this;
  else if (IsEqualIID(riid, IID_ITfKeyEventSink))
    *ppvObject = (ITfKeyEventSink*)this;
  else if (IsEqualIID(riid, IID_ITfCompositionSink))
    *ppvObject = (ITfCompositionSink*)this;
  else if (IsEqualIID(riid, IID_ITfEditSession))
    *ppvObject = (ITfEditSession*)this;
  else if (IsEqualIID(riid, IID_ITfThreadFocusSink))
    *ppvObject = (ITfThreadFocusSink*)this;
  else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
    *ppvObject = (ITfDisplayAttributeProvider*)this;

  if (*ppvObject) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) WeaselTSF::AddRef() {
  return ++_cRef;
}

STDMETHODIMP_(ULONG) WeaselTSF::Release() {
  LONG cr = --_cRef;

  assert(_cRef >= 0);

  if (_cRef == 0)
    delete this;

  return cr;
}

STDMETHODIMP WeaselTSF::Activate(ITfThreadMgr* pThreadMgr,
                                 TfClientId tfClientId) {
  return ActivateEx(pThreadMgr, tfClientId, 0U);
}

STDMETHODIMP WeaselTSF::Deactivate() {
  // Unadvise on the owning TSF apartment before releasing the thread manager.
  m_client.Disconnect();
  if (_pThreadMgr == nullptr) {
    _tfClientId = TF_CLIENTID_NULL;
    _prevKeyEvent = weasel::KeyEvent();
    _prevfEaten = FALSE;
    _keyCountToSimulate = 0;
    _fTestKeyDownPending = FALSE;
    _fTestKeyUpPending = FALSE;
    _serviceLaunchAttempted = false;
    return S_OK;
  }

  const BOOL textSinkReleased = _InitTextEditSink(com_ptr<ITfDocumentMgr>());
  _UninitKeyEventSink();
  _UninitThreadFocusSink();
  _UninitThreadMgrEventSink();
  _UninitPreservedKey();
  _UninitLanguageBar();
  _UninitCompartment();

  _prevKeyEvent = weasel::KeyEvent();
  _prevfEaten = FALSE;
  _keyCountToSimulate = 0;
  _fTestKeyDownPending = FALSE;
  _fTestKeyUpPending = FALSE;
  _serviceLaunchAttempted = false;

  // Retain the COM manager and any outstanding cookies if Unadvise fails.
  // A subsequent Deactivate() may retry, without touching freed TSF state.
  if (!textSinkReleased || _keyEventSinkAdvised ||
      _dwThreadFocusSinkCookie != TF_INVALID_COOKIE ||
      _dwThreadMgrEventSinkCookie != TF_INVALID_COOKIE)
    return E_FAIL;

  _pThreadMgr = nullptr;
  _tfClientId = TF_CLIENTID_NULL;
  _cand->DestroyAll();
  return S_OK;
}

STDMETHODIMP WeaselTSF::ActivateEx(ITfThreadMgr* pThreadMgr,
                                   TfClientId tfClientId,
                                   DWORD dwFlags) {
  if (pThreadMgr == nullptr || tfClientId == TF_CLIENTID_NULL)
    return E_INVALIDARG;
  if (_pThreadMgr != nullptr)
    return E_UNEXPECTED;

  LoadWeaselLogLevelFromRegistry();
  com_ptr<ITfDocumentMgr> pDocMgrFocus;
  _activateFlags = dwFlags;

  _pThreadMgr = pThreadMgr;
  _tfClientId = tfClientId;

  if (!_InitThreadMgrEventSink())
    goto ExitError;

  if ((_pThreadMgr->GetFocus(&pDocMgrFocus) == S_OK) &&
      (pDocMgrFocus != NULL)) {
    _InitTextEditSink(pDocMgrFocus);
  }

  if (!_InitKeyEventSink())
    goto ExitError;

  // if (!_InitDisplayAttributeGuidAtom())
  //	goto ExitError;
  //	some app might init failed because it not provide DisplayAttributeInfo,
  // like some opengl stuff
  _InitDisplayAttributeGuidAtom();

  if (!_InitPreservedKey())
    goto ExitError;

  if (!_InitLanguageBar())
    goto ExitError;

  if (!_IsKeyboardOpen())
    _SetKeyboardOpen(TRUE);

  if (!_InitCompartment())
    goto ExitError;
  if (!_InitThreadFocusSink())
    goto ExitError;

  _EnsureServerConnected();

  return S_OK;

ExitError:
  Deactivate();
  return E_FAIL;
}

STDMETHODIMP WeaselTSF::OnSetThreadFocus() {
  std::wstring _ToggleImeOnOpenClose{};
  RegGetStringValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                    L"ToggleImeOnOpenClose", _ToggleImeOnOpenClose);
  _isToOpenClose = (_ToggleImeOnOpenClose == L"yes");
  if (m_client.Echo()) {
    m_client.ProcessKeyEvent(0);
    weasel::ResponseParser parser(NULL, NULL, &_status, NULL, &_cand->style());
    bool ok = m_client.GetResponseData(std::ref(parser));
    if (ok)
      _UpdateLanguageBar(_status);
  }
  return S_OK;
}
STDMETHODIMP WeaselTSF::OnKillThreadFocus() {
  _AbortComposition();
  return S_OK;
}
BOOL WeaselTSF::_InitThreadFocusSink() {
  if (_pThreadMgr == nullptr ||
      _dwThreadFocusSinkCookie != TF_INVALID_COOKIE)
    return FALSE;
  com_ptr<ITfSource> pSource;
  if (FAILED(_pThreadMgr->QueryInterface(&pSource)))
    return FALSE;
  if (FAILED(pSource->AdviseSink(IID_ITfThreadFocusSink,
                                 (ITfThreadFocusSink*)this,
                                 &_dwThreadFocusSinkCookie))) {
    _dwThreadFocusSinkCookie = TF_INVALID_COOKIE;
    return FALSE;
  }
  return TRUE;
}

void WeaselTSF::_UninitThreadFocusSink() {
  if (_pThreadMgr == nullptr ||
      _dwThreadFocusSinkCookie == TF_INVALID_COOKIE)
    return;
  com_ptr<ITfSource> pSource;
  if (FAILED(_pThreadMgr->QueryInterface(&pSource)))
    return;
  const HRESULT hr = pSource->UnadviseSink(_dwThreadFocusSinkCookie);
  if (SUCCEEDED(hr) || hr == CONNECT_E_NOCONNECTION)
    _dwThreadFocusSinkCookie = TF_INVALID_COOKIE;
}

STDMETHODIMP WeaselTSF::OnActivated(REFCLSID clsid,
                                    REFGUID guidProfile,
                                    BOOL isActivated) {
  if (!IsEqualCLSID(clsid, c_clsidTextService)) {
    return S_OK;
  }

  if (isActivated) {
    _ShowLanguageBar(TRUE);
    _UpdateLanguageBar(_status);
  } else {
    _DeleteCandidateList();
    _ShowLanguageBar(FALSE);
  }
  return S_OK;
}

void WeaselTSF::_Reconnect() {
  m_client.Disconnect();
  if (!m_client.Connect(NULL))
    return;
  m_client.StartSession();
  weasel::ResponseParser parser(NULL, NULL, &_status, NULL, &_cand->style());
  bool ok = m_client.GetResponseData(std::ref(parser));
  if (ok) {
    _UpdateLanguageBar(_status);
  }
}

bool WeaselTSF::_EnsureServerConnected() {
  if (m_client.Echo())
    return true;

  // One connection attempt on the calling TSF apartment. Do not replay keys.
  _Reconnect();
  if (m_client.Echo())
    return true;

  // Launch at most once per activation. Never capture this in a detached
  // thread or call COM/IPC from a non-TSF apartment.
  if (_serviceLaunchAttempted)
    return false;
  _serviceLaunchAttempted = true;

  HANDLE hMutex = CreateMutexW(NULL, TRUE, L"WeaselDeployerExclusiveMutex");
  if (hMutex == NULL)
    return false;
  const DWORD creationError = GetLastError();
  if (creationError != ERROR_ALREADY_EXISTS) {
    const auto countServerProcesses = []() -> int {
      int count = 0;
      HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
      if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
      PROCESSENTRY32 entry = {};
      entry.dwSize = sizeof(entry);
      if (Process32First(snapshot, &entry)) {
        do {
          if (_wcsicmp(entry.szExeFile, L"WeaselServer.exe") == 0)
            ++count;
        } while (Process32Next(snapshot, &entry));
      }
      CloseHandle(snapshot);
      return count;
    };
    if (countServerProcesses() == 0) {
      const std::wstring dir = _GetRootDir();
      ShellExecuteW(NULL, L"open", (dir + L"\\start_service.bat").c_str(),
                    NULL, dir.c_str(), SW_HIDE);
    }
    ReleaseMutex(hMutex);
  }
  CloseHandle(hMutex);

  // The launch does not prove readiness. A later TSF event may make a new
  // connection attempt; this call never waits or retries the original key.
  return false;
}
