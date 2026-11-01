#pragma once

#include <msctf.h>
#include <openatl/com_server.h>

class CDisplayAttributeInfoInput;
using DisplayAttributeInfoComBase = OpenATL::COM::Object<
    CDisplayAttributeInfoInput,
    OpenATL::COM::Interface<ITfDisplayAttributeInfo, &IID_ITfDisplayAttributeInfo>>;

class CDisplayAttributeInfoInput final : public DisplayAttributeInfoComBase {
 public:
  explicit CDisplayAttributeInfoInput(OpenATL::COM::Module& module);
  ~CDisplayAttributeInfoInput();

  // ITfDisplayAttributeInfo
  STDMETHODIMP GetGUID(_Out_ GUID* pguid);
  STDMETHODIMP GetDescription(_Out_ BSTR* pbstrDesc);
  STDMETHODIMP GetAttributeInfo(_Out_ TF_DISPLAYATTRIBUTE* pTSFDisplayAttr);
  STDMETHODIMP SetAttributeInfo(_In_ const TF_DISPLAYATTRIBUTE* ptfDisplayAttr);
  STDMETHODIMP Reset();

 private:
  const GUID* _pguid;
  const TF_DISPLAYATTRIBUTE* _pDisplayAttribute;
  const WCHAR* _pDescription;
  const WCHAR* _pValueName;
};
