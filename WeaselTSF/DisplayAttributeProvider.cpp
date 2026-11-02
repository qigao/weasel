#include "stdafx.h"

#include "WeaselTSF.h"
#include "EnumDisplayAttributeInfo.h"
#include "DisplayAttributeInfo.h"

STDMETHODIMP WeaselTSF::EnumDisplayAttributeInfo(
    __RPC__deref_out_opt IEnumTfDisplayAttributeInfo** ppEnum) {
  if (ppEnum == nullptr) {
    return E_INVALIDARG;
  }

  *ppEnum = nullptr;

  return CEnumDisplayAttributeInfo::CreateInstance(
      _module, IID_IEnumTfDisplayAttributeInfo, reinterpret_cast<void**>(ppEnum));
}

STDMETHODIMP WeaselTSF::GetDisplayAttributeInfo(
    __RPC__in REFGUID guidInfo,
    __RPC__deref_out_opt ITfDisplayAttributeInfo** ppInfo) {
  if (ppInfo == nullptr) {
    return E_INVALIDARG;
  }

  *ppInfo = nullptr;

  if (!IsEqualGUID(guidInfo, c_guidDisplayAttributeInput))
    return E_INVALIDARG;
  return CDisplayAttributeInfoInput::CreateInstance(
      _module, IID_ITfDisplayAttributeInfo, reinterpret_cast<void**>(ppInfo));
}
