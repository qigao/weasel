#pragma once
#include <msctf.h>
#include <openatl/com_server.h>

class CEnumDisplayAttributeInfo;
using EnumDisplayAttributeInfoComBase = OpenATL::COM::Object<
    CEnumDisplayAttributeInfo,
    OpenATL::COM::Interface<IEnumTfDisplayAttributeInfo,
                            &IID_IEnumTfDisplayAttributeInfo>>;

class CEnumDisplayAttributeInfo final : public EnumDisplayAttributeInfoComBase {
 public:
  explicit CEnumDisplayAttributeInfo(OpenATL::COM::Module& module);
  ~CEnumDisplayAttributeInfo();

  // IEnumTfDisplayAttributeInfo
  STDMETHODIMP Clone(_Out_ IEnumTfDisplayAttributeInfo** ppEnum);
  STDMETHODIMP Next(ULONG ulCount,
                    __RPC__out_ecount_part(ulCount, *pcFetched)
                        ITfDisplayAttributeInfo** rgInfo,
                    __RPC__out ULONG* pcFetched);
  STDMETHODIMP Reset();
  STDMETHODIMP Skip(ULONG ulCount);

 private:
  OpenATL::COM::Module& _module;
  LONG _index;  // next display attribute to enum
};
