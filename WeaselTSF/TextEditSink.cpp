#include "stdafx.h"
#include "WeaselTSF.h"

static BOOL IsRangeCovered(TfEditCookie ec,
                           ITfRange* pRangeTest,
                           ITfRange* pRangeCover) {
  LONG lResult;

  if (pRangeCover->CompareStart(ec, pRangeTest, TF_ANCHOR_START, &lResult) !=
          S_OK ||
      lResult > 0)
    return FALSE;
  if (pRangeCover->CompareEnd(ec, pRangeTest, TF_ANCHOR_END, &lResult) !=
          S_OK ||
      lResult < 0)
    return FALSE;
  return TRUE;
}

STDMETHODIMP WeaselTSF::OnEndEdit(ITfContext* pContext,
                                  TfEditCookie ecReadOnly,
                                  ITfEditRecord* pEditRecord) {
  BOOL fSelectionChanged;
  IEnumTfRanges* pEnumTextChanges;
  ITfRange* pRange;

  /* did the selection change? */
  if (pEditRecord->GetSelectionStatus(&fSelectionChanged) == S_OK &&
      fSelectionChanged) {
    if (_IsComposing()) {
      /* if the caret moves out of composition range, stop the composition */
      TF_SELECTION tfSelection;
      ULONG cFetched;

      if (pContext->GetSelection(ecReadOnly, TF_DEFAULT_SELECTION, 1,
                                 &tfSelection, &cFetched) == S_OK &&
          cFetched == 1) {
        ITfRange* pRangeComposition;
        if (_pComposition->GetRange(&pRangeComposition) == S_OK) {
          if (!IsRangeCovered(ecReadOnly, tfSelection.range, pRangeComposition))
            _EndComposition(pContext, true);
          pRangeComposition->Release();
        }
      }
    }
  }

  /* text modification? */
  if (pEditRecord->GetTextAndPropertyUpdates(TF_GTP_INCL_TEXT, NULL, 0,
                                             &pEnumTextChanges) == S_OK) {
    if (pEnumTextChanges->Next(1, &pRange, NULL) == S_OK) {
      pRange->Release();
    }
    pEnumTextChanges->Release();
  }
  return S_OK;
}

STDMETHODIMP WeaselTSF::OnLayoutChange(ITfContext* pContext,
                                       TfLayoutCode lcode,
                                       ITfContextView* pContextView) {
  if (!_IsComposing())
    return S_OK;

  if (pContext != _pTextEditSinkContext)
    return S_OK;

  if (lcode == TF_LC_CHANGE)
    _UpdateCompositionWindow(pContext);
  return S_OK;
}

BOOL WeaselTSF::_InitTextEditSink(com_ptr<ITfDocumentMgr> pDocMgr) {
  com_ptr<ITfSource> pSource;

  // Release every advised cookie, including a partially installed pair.
  // Failed Unadvise retains its context and cookie for the next owner call.
  if (_pTextEditSinkContext != nullptr) {
    if (_dwTextEditSinkCookie != TF_INVALID_COOKIE ||
        _dwTextLayoutSinkCookie != TF_INVALID_COOKIE) {
      if (FAILED(_pTextEditSinkContext->QueryInterface(&pSource)))
        return FALSE;
      if (_dwTextEditSinkCookie != TF_INVALID_COOKIE) {
        const HRESULT hr = pSource->UnadviseSink(_dwTextEditSinkCookie);
        if (FAILED(hr) && hr != CONNECT_E_NOCONNECTION)
          return FALSE;
        _dwTextEditSinkCookie = TF_INVALID_COOKIE;
      }
      if (_dwTextLayoutSinkCookie != TF_INVALID_COOKIE) {
        const HRESULT hr = pSource->UnadviseSink(_dwTextLayoutSinkCookie);
        if (FAILED(hr) && hr != CONNECT_E_NOCONNECTION)
          return FALSE;
        _dwTextLayoutSinkCookie = TF_INVALID_COOKIE;
      }
    }
    _pTextEditSinkContext = nullptr;
  }

  if (pDocMgr == nullptr)
    return TRUE;
  if (pDocMgr->GetTop(&_pTextEditSinkContext) != S_OK) {
    _pTextEditSinkContext = nullptr;
    return FALSE;
  }
  if (_pTextEditSinkContext == nullptr)
    return TRUE;

  pSource.Release();
  if (FAILED(_pTextEditSinkContext->QueryInterface(&pSource))) {
    _pTextEditSinkContext = nullptr;
    return FALSE;
  }
  if (FAILED(pSource->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink*)this,
                                 &_dwTextEditSinkCookie))) {
    _dwTextEditSinkCookie = TF_INVALID_COOKIE;
    _pTextEditSinkContext = nullptr;
    return FALSE;
  }
  if (FAILED(pSource->AdviseSink(IID_ITfTextLayoutSink,
                                 (ITfTextLayoutSink*)this,
                                 &_dwTextLayoutSinkCookie))) {
    _dwTextLayoutSinkCookie = TF_INVALID_COOKIE;
    (void)_InitTextEditSink(com_ptr<ITfDocumentMgr>());
    return FALSE;
  }
  return TRUE;
}
