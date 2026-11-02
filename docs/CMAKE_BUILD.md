# CMake product build (issue #7)

The `msvc-x64` preset builds `WeaselServer.exe`, `WeaselDeployer.exe`,
`WeaselSetup.exe` and `weaselx64.dll`, including the existing IPC client,
IPC server, Rime adapter and candidate UI source libraries. All Weasel targets
consume the pinned `external/OpenWTL` source through `OpenWTL::WTL`.
C++ and resource include paths exclude Microsoft ATL/MFC; the TSF and Server
resources use OpenWTL's `atlres.h`.

`WeaselTargetVersion.h` selects the Windows 8.1 minimum before SDK headers
are loaded, including DirectComposition declarations. Boost remains required
by the current IPC protocol and thread implementation.

The ARM64 preset explicitly disables `WEASEL_BUILD_APPLICATIONS`, retaining
its TSF/UI/IPC client/Setup graph qualified in
[PR #16 CI](https://github.com/qigao/weasel/actions/runs/41426017622).
Server and Deployer currently depend on x64 Rime and WinSparkle bundles;
requesting application builds with another architecture fails configuration.

## Dependencies

Use CMake 3.25 or newer, Ninja, Visual Studio C++ tools and Windows SDK.
`VCPKG_ROOT` names the selected vcpkg checkout. The versioned manifest restores
static Boost, with no `BOOST_ROOT` or b2 invocation for CMake builds.
Initialize OpenWTL with `git submodule update --init external/OpenWTL`.

The x64 preset sets `WEASEL_DEPENDENCY_ROOT` to the source directory, using
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

## Build and test

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

Products are isolated in `build/<preset>/bin`; install staging is
`build/install/<preset>`. CTest passes `$<TARGET_FILE:WeaselTSF>` to the
existing COM factory/identity/lifetime and host exception filter tests,
loading the corresponding CMake DLL without installation or registration.

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
local ARM64 tools are unavailable.

LLVM-MinGW, ARM64 application dependencies, live Server/Deployer and candidate
GUI behavior, engine integration, TSF host and installer acceptance remain
open. Build and DLL-test results do not establish those runtime gates.
