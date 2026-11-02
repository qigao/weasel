#include "stdafx.h"

#include "Globals.h"
#include "Register.h"
#include "WeaselTSF.h"
#include <VersionHelpers.hpp>

// OpenWTL is the sole COM server factory and module-refcount owner.
// The static table contains only this TSF CLSID: other classes are rejected.
namespace {
const OpenATL::COM::ClassEntry kWeaselTsfClasses[] = {
    {c_clsidTextService, &WeaselTSF::CreateInstance}};
OpenATL::COM::Module& TsfModule() noexcept {
  static OpenATL::COM::Module module(kWeaselTsfClasses, 1);
  return module;
}
}  // namespace

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void** output) {
  return TsfModule().GetClassObject(clsid, iid, output);
}

STDAPI DllCanUnloadNow() {
  return TsfModule().CanUnloadNow();
}

STDAPI DllRegisterServer() {
  if (!RegisterServer() || !RegisterProfiles() || !RegisterCategories()) {
    DllUnregisterServer();
    return E_FAIL;
  }
  return S_OK;
}

STDAPI DllUnregisterServer() {
  UnregisterProfiles();
  UnregisterCategories();
  UnregisterServer();
  return S_OK;
}
