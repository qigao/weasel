// Real Windows native COM server acceptance for the Weasel TSF DLL.
// This runs against the built product DLL; it does not install/register it.
#include <windows.h>
#include <msctf.h>
#include <stdio.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

namespace {
const CLSID kWeaselTsf = {
    0xa3f4cded, 0xb1e9, 0x41ee,
    {0x9c, 0xa6, 0x7b, 0x4d, 0x0d, 0xe6, 0xcb, 0x0a}};
using GetClassObject = HRESULT(WINAPI*)(REFCLSID, REFIID, void**);
using CanUnloadNow = HRESULT(WINAPI*)();

bool Check(bool value, const char* name) {
  if (!value)
    fprintf(stderr, "FAIL: %s\n", name);
  return value;
}

bool TestCycle(GetClassObject get, CanUnloadNow canUnload) {
  if (!Check(canUnload() == S_OK, "initial unload readiness"))
    return false;

  void* output = reinterpret_cast<void*>(1);
  if (!Check(get(CLSID_NULL, IID_IClassFactory, &output) ==
                     CLASS_E_CLASSNOTAVAILABLE &&
                 output == nullptr, "reject unknown CLSID"))
    return false;
  output = reinterpret_cast<void*>(1);
  if (!Check(get(kWeaselTsf, IID_ITfKeyEventSink, &output) ==
                     E_NOINTERFACE && output == nullptr,
             "reject factory IID mismatch"))
    return false;
  if (!Check(get(kWeaselTsf, IID_IClassFactory, nullptr) == E_POINTER,
             "reject null output pointer"))
    return false;

  IClassFactory* factory = nullptr;
  if (!Check(get(kWeaselTsf, IID_IClassFactory,
                 reinterpret_cast<void**>(&factory)) == S_OK && factory,
             "create correct factory"))
    return false;
  bool success = true;
  success &= Check(canUnload() == S_FALSE, "factory pins module");

  ITfTextInputProcessorEx* service = nullptr;
  success &= Check(
      factory->CreateInstance(nullptr, IID_ITfTextInputProcessorEx,
                              reinterpret_cast<void**>(&service)) == S_OK &&
          service, "create TSF processor");

  if (service) {
    ITfTextInputProcessor* legacy = nullptr;
    IUnknown* canonical = nullptr;
    IUnknown* alias = nullptr;
    success &= Check(service->QueryInterface(IID_ITfTextInputProcessor,
                      reinterpret_cast<void**>(&legacy)) == S_OK &&
                         legacy, "legacy TSF interface");
    success &= Check(service->QueryInterface(IID_IUnknown,
                      reinterpret_cast<void**>(&canonical)) == S_OK &&
                         canonical, "canonical IUnknown");
    if (legacy) {
      success &= Check(legacy == static_cast<ITfTextInputProcessor*>(service),
                       "SDK-inherited legacy interface alias");
      success &= Check(legacy->QueryInterface(IID_IUnknown,
                        reinterpret_cast<void**>(&alias)) == S_OK &&
                           alias, "legacy interface IUnknown");
      success &= Check(canonical && alias && canonical == alias,
                       "single canonical IUnknown identity");
    }
    if (alias)
      alias->Release();
    if (canonical)
      canonical->Release();
    if (legacy)
      legacy->Release();
    success &= Check(canUnload() == S_FALSE, "TSF object pins module");
  }

  output = reinterpret_cast<void*>(1);
  success &= Check(factory->CreateInstance(factory,
                    IID_ITfTextInputProcessorEx, &output) ==
                       CLASS_E_NOAGGREGATION && output == nullptr,
                   "reject aggregation");

  if (service)
    service->Release();
  success &= Check(canUnload() == S_FALSE, "factory still owns module");

  success &= Check(factory->LockServer(TRUE) == S_OK, "LockServer");
  factory->Release();
  success &= Check(canUnload() == S_FALSE, "server lock retains module");

  IClassFactory* unlockFactory = nullptr;
  const HRESULT hr = get(kWeaselTsf, IID_IClassFactory,
                          reinterpret_cast<void**>(&unlockFactory));
  if (hr == S_OK && unlockFactory) {
    success &= Check(unlockFactory->LockServer(FALSE) == S_OK,
                     "UnlockServer");
    success &= Check(unlockFactory->LockServer(FALSE) == E_UNEXPECTED,
                     "reject unbalanced UnlockServer");
    unlockFactory->Release();
  } else {
    success = false;
  }
  success &= Check(canUnload() == S_OK,
                   "all references retired and DLL unloadable");
  return success;
}

bool TestDisplayHelperLifetime(GetClassObject get, CanUnloadNow canUnload) {
  IClassFactory* factory = nullptr;
  HRESULT hr = get(kWeaselTsf, IID_IClassFactory,
                   reinterpret_cast<void**>(&factory));
  bool success = Check(hr == S_OK && factory, "helper test factory");
  ITfDisplayAttributeProvider* provider = nullptr;
  if (factory) {
    hr = factory->CreateInstance(nullptr, IID_ITfDisplayAttributeProvider,
                                 reinterpret_cast<void**>(&provider));
    success &= Check(hr == S_OK && provider, "display attribute provider");
    factory->Release();
  }
  IEnumTfDisplayAttributeInfo* enumerator = nullptr;
  IEnumTfDisplayAttributeInfo* clone = nullptr;
  ITfDisplayAttributeInfo* attribute = nullptr;
  ITfDisplayAttributeInfo* fromClone = nullptr;
  if (provider) {
    hr = provider->EnumDisplayAttributeInfo(&enumerator);
    success &= Check(hr == S_OK && enumerator, "display attribute enumerator");
  }
  if (enumerator) {
    ULONG fetched = 0;
    hr = enumerator->Next(1, &attribute, &fetched);
    success &= Check(hr == S_OK && fetched == 1 && attribute,
                     "display attribute element");
    hr = enumerator->Clone(&clone);
    success &= Check(hr == S_OK && clone, "clone enumerator state");
  }
  if (provider)
    provider->Release();
  if (enumerator)
    enumerator->Release();
  if (clone) {
    success &= Check(canUnload() == S_FALSE,
                     "retained helpers pin module after TSF release");
    ITfDisplayAttributeInfo* exhausted = nullptr;
    ULONG fetched = 99;
    hr = clone->Next(1, &exhausted, &fetched);
    success &= Check(hr == S_FALSE && fetched == 0 && exhausted == nullptr,
                     "clone preserves exhausted position");
    if (exhausted)
      exhausted->Release();
    success &= Check(clone->Reset() == S_OK, "reset retained clone");
    fetched = 0;
    hr = clone->Next(1, &fromClone, &fetched);
    success &= Check(hr == S_OK && fetched == 1 && fromClone,
                     "clone creates element after TSF destruction");
    clone->Release();
  }
  GUID first{}, second{};
  bool firstGuid = false;
  if (attribute) {
    firstGuid = attribute->GetGUID(&first) == S_OK;
    success &= Check(firstGuid, "retained attribute remains callable");
    attribute->Release();
  }
  if (fromClone) {
    success &= Check(canUnload() == S_FALSE, "last attribute pins module");
    success &= Check(fromClone->GetGUID(&second) == S_OK && firstGuid &&
                         IsEqualGUID(first, second),
                     "independent attributes share their native GUID");
    fromClone->Release();
  }
  success &= Check(canUnload() == S_OK, "helper references retire before unload");
  return success;
}
}  // namespace

int wmain(int argc, wchar_t** argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: tsf_com_server_test <absolute DLL path>\n");
    return 2;
  }
  const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(init)) {
    fprintf(stderr, "CoInitializeEx failed: 0x%08lx\n",
            static_cast<unsigned long>(init));
    return 3;
  }
  wchar_t fullPath[MAX_PATH] = {};
  const DWORD length = GetFullPathNameW(argv[1], MAX_PATH, fullPath, nullptr);
  if (!length || length >= MAX_PATH) {
    CoUninitialize();
    return 2;
  }
  const HMODULE dll = LoadLibraryExW(fullPath, nullptr,
                                    LOAD_WITH_ALTERED_SEARCH_PATH);
  if (!dll) {
    fprintf(stderr, "LoadLibraryExW failed: Win32 %lu\n", GetLastError());
    CoUninitialize();
    return 3;
  }
  auto get = reinterpret_cast<GetClassObject>(
      GetProcAddress(dll, "DllGetClassObject"));
  auto canUnload = reinterpret_cast<CanUnloadNow>(
      GetProcAddress(dll, "DllCanUnloadNow"));
  bool success = Check(get && canUnload, "required COM exports");
  for (int i = 0; success && i != 8; ++i)
    success = TestCycle(get, canUnload);
  if (success)
    success = TestDisplayHelperLifetime(get, canUnload);
  if (canUnload)
    success &= Check(canUnload() == S_OK, "safe final unload");
  success &= Check(FreeLibrary(dll) != 0, "FreeLibrary");
  CoUninitialize();
  if (success)
    printf("PASS: Weasel OpenWTL COM Server class/identity/lifetime\n");
  return success ? 0 : 1;
}
