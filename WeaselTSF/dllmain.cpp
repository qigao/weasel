#include "stdafx.h"
#include "Globals.h"

// Weasel TSF is an in-process server hosted by third-party applications.
// The loader entry must not install a host-wide crash handler, register COM,
// initialize an apartment, or build a second module-level COM runtime.
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH)
    g_hInst = module;
  return TRUE;
}
