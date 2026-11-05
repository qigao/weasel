# CMake product build (issue #7)

The `msvc-x64` and `llvm-x64` presets build `WeaselServer.exe`, `WeaselDeployer.exe`,
`WeaselSetup.exe` and `weaselx64.dll`, including the existing IPC client,
IPC server, Rime adapter and candidate UI source libraries. All Weasel targets
consume the pinned `external/OpenWTL` source through `OpenWTL::WTL`.
C++ and resource include paths exclude Microsoft ATL/MFC; the TSF and Server
resources use OpenWTL's `atlres.h`.

`WeaselTargetVersion.h` selects the Windows 8.1 minimum before SDK headers
are loaded, including DirectComposition declarations. Boost remains required
by the current IPC protocol and thread implementation.

The ARM64 presets explicitly disable `WEASEL_BUILD_APPLICATIONS`, retaining
the TSF/UI/IPC client/Setup graph. Its MSVC build was qualified in
[PR #16 CI](https://github.com/qigao/weasel/actions/runs/41426017622).
Server and Deployer currently depend on x64 Rime and WinSparkle bundles;
requesting application builds with another architecture fails configuration.

## Dependencies

Use CMake 3.25 or newer, Ninja, Visual Studio C++ tools and Windows SDK.
`VCPKG_ROOT` names the selected vcpkg checkout. The versioned manifest restores
static Boost, with no `BOOST_ROOT` or b2 invocation for CMake builds.
Initialize the source dependencies with
`git submodule update --init external/OpenWTL external/SaltsCore`.
When tests are enabled, only SaltsCore's canonical `tinytest` module is added:
its `Salts::TinyTest` static C runtime backs the C++17 response-parser suite.
The submodule is pinned to `c96696affa3feaa62ec6052aee99a8f2cc596bb1`;
the remaining SaltsCore modules are not built or installed by this graph.

The x64 presets set `WEASEL_DEPENDENCY_ROOT` to the source directory, using
the existing acquisition layout:

| Files | Existing producer |
| --- | --- |
| `include/rime_api.h`, `include/rime_levers_api.h`, other `rime_*.h` | `get-rime.ps1 -use dev` |
| `lib64/rime.lib`, `output/rime.dll` | `get-rime.ps1 -use dev` |
| `lib64/WinSparkle.lib`, `output/WinSparkle.dll` | Patched WinSparkle 0.9.2 build in the Windows workflow |

The same workflow already acquires these files for MSBuild. CMake imports
them as `Rime::Rime` and `WinSparkle::WinSparkle`; missing files stop configure.
It does not discover another SDK or build a second Rime/WinSparkle copy.
Rime source migration and its remaining Boost/logging dependencies belong
to issues #9 and #10. This graph does not establish a source build of Rime.
LLVM-MinGW x64 links the same acquired import libraries: the Rime and
WinSparkle entry points used here have C linkage, and library-owned objects
remain behind their existing API handles and release functions. This does
not qualify live engine, updater or GUI behavior with the LLVM-built apps.

## Build and test

| Preset | Compiler / target | Product graph |
| --- | --- | --- |
| `msvc-x64` | MSVC x64 | Server, Deployer, TSF, Setup and supporting libraries |
| `msvc-arm64` | MSVC ARM64 | TSF, Setup, candidate UI and IPC client |
| `llvm-x64` | LLVM-MinGW x64 | Server, Deployer, TSF, Setup and supporting libraries |
| `llvm-arm64` | LLVM-MinGW ARM64 | TSF, Setup, candidate UI and IPC client |

Enter a developer prompt with `VsDevCmd.bat -arch=x64 -host_arch=x64`, then:

```bat
cmake --preset msvc-x64
cmake --build --preset msvc-x64
ctest --preset msvc-x64
cmake --build --preset install-msvc-x64
```

Keep `VCPKG_ROOT` set to the chosen checkout after entering VsDevCmd, which
can otherwise select Visual Studio's bundled vcpkg. Run configure, build and
CTest in the same developer environment. The preset's `PATH` includes the
profile's `bin`, the acquired dependency `output` directory, and inherited
`PATH`. No dependency DLLs are copied into the build directory.

For ARM64, use `VsDevCmd.bat -arch=arm64 -host_arch=x64` and the
`msvc-arm64` configure/build/install presets. Run `ctest --preset msvc-arm64`
only on native ARM64 Windows; cross compilation establishes build results.

For LLVM-MinGW, set `OPENATL_LLVM_MINGW_BIN` to the compiler's `bin` directory
and `WEASEL_WINDOWS_SDK_BIN` to the selected Windows SDK's host `bin/<version>/x64`
directory containing `rc.exe`. Use `llvm-x64` or `llvm-arm64` with the same
configure/build/test/install commands. These profiles chainload the pinned OpenWTL source toolchain through
vcpkg and use architecture-specific Boost triplets. The compiler release is
LLVM-MinGW 20261020, archive SHA256
`fd5462b7f5d93b2ff981fcd3f9c7e427e47f3203c64ea91c27bcc444eebd74cd`,
matching OpenWTL 0.1.2 qualification. CI downloads and checks that archive.
Compiler C++ runtimes are statically linked in the LLVM profiles.

The selected SDK supplies the complete TSF ABI declarations absent from the
MinGW headers and its architecture-matched UUID library. Only four SDK headers
(`msctf.h`, `ctfutb.h`, `textstor.h`, `ctffunc.h`) are materialized in the build
tree, with MinGW UUID bindings derived from their COM interface annotations
using OpenWTL's GUID formatter. They are not installed or redistributed.
MinGW continues to supply the standard Win32 and CRT headers. Native SDK RC
handles the original UTF-16 resource files and excludes Microsoft ATL/MFC.

The LLVM ARM64 profile disables Server/Deployer until matching application
dependencies are available. Setup, Server and Deployer embed their process
manifests through resource files on LLVM-MinGW. The existing native DLL tests
use their Unicode console entry points with either compiler. Run ARM64 CTest only on an
ARM64 Windows host; the x64 CI runner builds and stages the ARM64 products.

Products are isolated in `build/<preset>/bin`; install staging is
`build/install/<preset>`. CTest passes `$<TARGET_FILE:WeaselTSF>` to the
existing COM factory/identity/lifetime and host exception filter tests,
loading the corresponding CMake DLL without installation or registration.
CTest also runs the production response parser through the 16-case
[TinyTest suite](../test/TestResponseParser/README.md), without starting a
server. It emits TAP and a profile-local JUnit report; CI uploads these reports
and fails on test errors. Both ARM64 profiles compile the suite for native
execution later. The former parser MSBuild project has been replaced by this
CMake target; the manual `TestWeaselIPC` console remains separate.

Install presets stage the Weasel binaries only. Rime/WinSparkle runtime DLLs,
Rime data, installer generation and TSF registration remain under the existing
acquisition and installer workflow. Staging is not a complete IME installer.

## Validation and remaining work

The full x64 CMake product graph built locally on 2026-11-02 against the
existing WinSparkle import library and the Rime 1.17.0 release bundle
(`33e7814`). Windows CI uses its existing patched WinSparkle build.
Both x64 CTest DLL tests and staging of all four Weasel products passed locally.
The existing workflow builds this graph, runs the two x64 CTest DLL tests
and stages all four Weasel products. ARM64 retains the graph passed in #16;
the local MSVC ARM64 toolset is unavailable.

LLVM-MinGW x64 compilation and linking of all four products, both native DLL
tests and staging passed locally on 2026-11-05 with LLVM-MinGW 20261020 and
Windows SDK 10.0.26100.0, against the same acquired dependency bundle.
Extracted Server/Deployer manifests contain Common Controls v6 and per-monitor
DPI awareness. Import inspection confirms the expected Rime/WinSparkle C
entry points and no LLVM C++ runtime DLL dependency for those applications.
The full MSVC x64 graph, both DLL tests and staging also passed after the
shared source portability fixes. LLVM ARM64 TSF/Setup and both test executables
also compiled and linked, and install staging passed. ARM64 tests have not
been executed on a native ARM64 host.

ARM64 application dependencies, live Server/Deployer and candidate
GUI behavior, engine integration, TSF host and installer acceptance remain
open. Build and DLL-test results do not establish those runtime gates.

The response-parser TinyTest migration also passed locally on 2026-11-05:
all 16 cases passed with MSVC x64 and LLVM-MinGW x64, alongside the two
existing DLL tests. A temporary wrong candidate-selection expectation caused
the parser test to fail with CTest status 8; the expectation was restored and
the suite passed again. LLVM ARM64 compiled and linked the parser suite and
its static TinyTest runtime, without native execution. MSVC ARM64 remains a
CI build requirement because the local ARM64 compiler is unavailable.
