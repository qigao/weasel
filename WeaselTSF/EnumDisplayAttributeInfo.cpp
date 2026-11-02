#include "stdafx.h"

#include "EnumDisplayAttributeInfo.h"
#include "Globals.h"
#include "DisplayAttributeInfo.h"

CEnumDisplayAttributeInfo::CEnumDisplayAttributeInfo(OpenATL::COM::Module& module)
    : EnumDisplayAttributeInfoComBase(module), _module(module), _index(0) {}

CEnumDisplayAttributeInfo::~CEnumDisplayAttributeInfo() = default;

STDMETHODIMP CEnumDisplayAttributeInfo::Clone(
    _Out_ IEnumTfDisplayAttributeInfo** ppEnum) {
  com_ptr<IEnumTfDisplayAttributeInfo> clone;

  if (ppEnum == nullptr) {
    return E_INVALIDARG;
  }

  *ppEnum = nullptr;

  const HRESULT hr = CreateInstance(_module, IID_IEnumTfDisplayAttributeInfo,
                                    reinterpret_cast<void**>(&clone.p));
  if (FAILED(hr))
    return hr;

  // This is the concrete instance just created by our own checked factory.
  static_cast<CEnumDisplayAttributeInfo*>(clone.p)->_index = _index;
  *ppEnum = clone.Detach();

  return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Next(
    ULONG ulCount,
    __RPC__out_ecount_part(ulCount, *pcFetched)
        ITfDisplayAttributeInfo** rgInfo,
    __RPC__out ULONG* pcFetched) {
  ULONG fetched;

  fetched = 0;
  if (pcFetched)
    *pcFetched = 0;

  if (ulCount == 0) {
    return S_OK;
  }
  if (rgInfo == nullptr) {
    return E_INVALIDARG;
  }
  *rgInfo = nullptr;

  while (fetched < ulCount) {
    ITfDisplayAttributeInfo* pDisplayAttributeInfo = nullptr;

    if (_index == 0) {
      const HRESULT hr = CDisplayAttributeInfoInput::CreateInstance(
          _module, IID_ITfDisplayAttributeInfo,
          reinterpret_cast<void**>(&pDisplayAttributeInfo));
      if (FAILED(hr))
        return hr;
    } else {
      break;
    }

    *rgInfo = pDisplayAttributeInfo;
    rgInfo++;
    fetched++;
    _index++;
  }

  if (pcFetched != nullptr) {
    *pcFetched = fetched;
  }

  return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Reset() {
  _index = 0;
  return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Skip(ULONG ulCount) {
  if ((ulCount + _index) > 1 || (ulCount + _index) < ulCount) {
    _index = 1;
    return S_FALSE;
  }
  _index += ulCount;
  return S_OK;
}
