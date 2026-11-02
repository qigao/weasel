#pragma once

#include <WinSDKVer.h>

// Weasel requires Windows 8.1, including DirectComposition. OpenWTL's
// consumer target supplies Windows 7 defaults; select the product's minimum
// before any Windows SDK headers derive NTDDI_VERSION or hide newer APIs.
#undef WINVER
#undef _WIN32_WINNT
#undef NTDDI_VERSION
#define WINVER 0x0603
#define _WIN32_WINNT 0x0603
#define NTDDI_VERSION NTDDI_WINBLUE

#include <SDKDDKVer.h>
