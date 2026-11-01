// Windows runtime regression: loading/unloading Weasel TSF must not replace
// the embedding process's global unhandled-exception filter.
#include <windows.h>
#include <stdio.h>

static LONG WINAPI host_exception_filter(EXCEPTION_POINTERS*) {
  return EXCEPTION_CONTINUE_SEARCH;
}

int wmain(int argc, wchar_t** argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: tsf_host_filter_test <absolute TSF DLL path>\n");
    return 2;
  }

  wchar_t dll_path[MAX_PATH] = {};
  const DWORD length = GetFullPathNameW(argv[1], MAX_PATH, dll_path, NULL);
  if (length == 0 || length >= MAX_PATH) {
    fprintf(stderr, "unable to resolve TSF DLL path\n");
    return 2;
  }

  const LPTOP_LEVEL_EXCEPTION_FILTER original =
      SetUnhandledExceptionFilter(host_exception_filter);
  const HMODULE module =
      LoadLibraryExW(dll_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (module == NULL) {
    const DWORD error = GetLastError();
    SetUnhandledExceptionFilter(original);
    fprintf(stderr, "LoadLibraryExW TSF failed: Win32 %lu\n", error);
    return 3;
  }

  const LPTOP_LEVEL_EXCEPTION_FILTER after_load =
      SetUnhandledExceptionFilter(host_exception_filter);
  const BOOL unloaded = FreeLibrary(module);
  const LPTOP_LEVEL_EXCEPTION_FILTER after_unload =
      SetUnhandledExceptionFilter(original);

  if (after_load != host_exception_filter || !unloaded ||
      after_unload != host_exception_filter) {
    fprintf(stderr,
            "TSF changed the host exception filter or failed to unload\n");
    return 1;
  }

  printf("PASS: TSF load/unload preserved host exception filter\n");
  return 0;
}
