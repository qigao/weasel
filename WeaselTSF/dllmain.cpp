#include "stdafx.h"
#include "Globals.h"

// A TSF DLL is loaded into third-party host processes. It must never replace
// the process-wide unhandled-exception filter or write host crash dumps.
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      g_hInst = hInstance;
      if (!InitializeCriticalSectionAndSpinCount(&g_cs, 0))
        return FALSE;
      break;
    case DLL_PROCESS_DETACH:
      DeleteCriticalSection(&g_cs);
      break;
  }
  return TRUE;
}
