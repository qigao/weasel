# CMake TSF and Setup product build (issue #7)

This first CMake slice builds the existing `WeaselTSF` DLL, candidate UI,
IPC client and `WeaselSetup` executable. Every product target consumes the
pinned `external/OpenWTL` source through `OpenWTL::WTL`. Microsoft ATL/MFC
is excluded from the compiler and resource include paths. The TSF resource
uses OpenWTL's `atlres.h`.

`WeaselTargetVersion.h` selects the product's Windows 8.1 minimum before SDK
headers are loaded, including DirectComposition declarations. Setup uses the
Unicode `CString` already supplied by OpenWTL 0.1.2 for its directory fields.

Server, Deployer, Rime and WinSparkle are still built by the existing MSBuild
workflow. This CMake graph is not a complete replacement for `build.bat`,
and its install preset stages only the TSF DLL and Setup executable, not a
usable IME installer. Rime data, registration and live TSF acceptance remain
separate work in issues #7 and #14. Boost remains required by the current IPC
protocol and thread implementation.

## Prerequisites

- CMake 3.25 or newer, Ninja, Visual Studio C++ tools and Windows SDK.
- `VCPKG_ROOT` pointing to a vcpkg checkout. The versioned manifest declares
  Boost dependencies and a baseline; no `BOOST_ROOT` or b2 invocation is used.
- Initialize the pinned source with
  `git submodule update --init external/OpenWTL`.

## Build and test

Enter a developer command prompt using `VsDevCmd.bat -arch=x64 -host_arch=x64`,
then run:

```bat
cmake --preset msvc-x64
cmake --build --preset msvc-x64
ctest --preset msvc-x64
cmake --build --preset install-msvc-x64
```

Keep `VCPKG_ROOT` set to the selected checkout after entering VsDevCmd;
Visual Studio can set it to its bundled vcpkg. Run configure, build and CTest
in the same developer environment. Static Boost libraries and the static
MSVC CRT match the existing TSF deployment model.

For ARM64, use `VsDevCmd.bat -arch=arm64 -host_arch=x64` and the
`msvc-arm64` configure/build/install presets. Execute `ctest --preset
msvc-arm64` only on native ARM64 Windows with an ARM64 developer environment.
Cross compilation on x64 proves compilation and linking only.

Artifacts are isolated in `build/<preset>/bin`; install staging is
`build/install/<preset>`. Tests receive the corresponding DLL through
`$<TARGET_FILE:WeaselTSF>`, so they cannot silently test MSBuild's `output/`
DLL. CTest runs the existing COM factory/identity/lifetime and host exception
filter regressions without installing or registering the IME.

LLVM-MinGW support and the remaining Server/Deployer dependency graph are
follow-up work. Unsupported compilers and architectures fail configuration.

## Validation

On 2026-11-02, the MSVC x64 Release product build, both CTest DLL tests and
the install preset passed locally with OpenWTL 0.1.2 and manifest Boost 1.88.0.
Local ARM64 configure stopped because the installed Visual Studio instance
has no ARM64 C++ toolchain; ARM64 compilation is requested by the existing
Windows CI workflow. Neither result establishes live TSF host, GUI or
installation acceptance.
