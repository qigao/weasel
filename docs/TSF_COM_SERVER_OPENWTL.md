# Weasel TSF COM Server / OpenWTL 0.1.2 cutover (Issue #14)

The only source of reusable COM server infrastructure is pinned
[qigao/OpenWTL](https://github.com/qigao/OpenWTL) at
`311b2dc361eea42cf6c821289133a99713298a74` (Git tree
`2e5a3f4b49dd27b18d4c89470aad5df7e258dfe6`), under `external/OpenWTL`,
the OpenWTL 0.1.2 release candidate in
[PR #134](https://github.com/qigao/OpenWTL/pull/134).
[OpenWTL PR #111](https://github.com/qigao/OpenWTL/pull/111) adds the standard
`CWindow::GetWindowText(CString&)` overload required by Weasel Deployer.
Together with [OpenWTL PR #110](https://github.com/qigao/OpenWTL/pull/110),
it fixes standalone `atlcomcli.h` namespace and SDK/runtime dependency order.
Both compatibility fixes are merged and included in 0.1.2. The existing
Windows workflow checks out the actual Weasel commit, verifies this dependency
pin and builds the ARM64/x64 product before running the x64 DLL tests.

The immutable [OpenWTL v0.1.0](https://github.com/qigao/OpenWTL/releases/tag/v0.1.0)
release remains at `cf2b4443ef068c002fa781ab96aaafe22a5f96e7`.
The new pin also includes the `CSize::SetSize(int,int)` fix from PR #108.
OpenWTL's single public CMake target is `OpenWTL::WTL`; the existing MSBuild
product consumes its source headers directly. Product CMake conversion remains
[Weasel #7](https://github.com/qigao/weasel/issues/7).

[Weasel CI 41339194723](https://github.com/qigao/weasel/actions/runs/41339194723)
on `88a5b4f2deede82fec3ec9eafcd7da30dfc09f9a` compiled the TSF migration;
the full product then exposed the missing window-text overload and residual
`wtl/atlapp.h` / `wtl/atlframe.h` includes in IPC Server.
The inherited test/TestWeaselIPC include is corrected to the same canonical
source path. A scan of all 162 tracked native source/build files found no other
old WTL include paths. No project, workflow or test target is added.
Upstream results never substitute for actual Weasel Windows DLL tests.

Weasel's own TSF interfaces, CLSID, session, profile/category registration,
candidate UI and Rime business belong to Weasel. OpenWTL's
`OpenATL::COM::Module` and `Object` own the generic class factory,
IUnknown identity, object references and module/server lock lifetime.
The Weasel TSF DLL publishes only the one class in its immutable class
table, and never modifies its host-wide crash exception filter.

The [SDK C++ declaration](https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/um/msctf.h#L12010)
makes `ITfTextInputProcessorEx` derive from `ITfTextInputProcessor`.
Expose the legacy IID through OpenWTL `InterfaceAlias`, avoiding a duplicate
legacy subobject. The interface descriptor checks this inheritance at compile time.
The final concrete TSF remains strongly retained by `CComPtr`; business calls use
its public `.p` pointer because ATL's arrow guard derives from its pointed-to type.

Display-attribute elements and enumerators now use `COM::Object` factories,
canonical identity, atomic references and module pins. Clones and returned elements
keep the DLL busy independently after the TSF has been released. The language bar
uses the same `ModuleReference` and checked atomic reference arithmetic; its initial
reference is adopted with `Attach`. Removed `DllAddRef/DllRelease` are not restored.
The existing native DLL test checks helper lifetime, cloned cursor state and final
unload readiness, using the actual product DLL without registration.

The TSF owns one initial reference to `CCandidateList` using
`CComPtr::Attach`. The candidate holds a borrowed owner pointer and
a separate `OpenATL::COM::ModuleReference` for COM UIElement lifetime.
The TSF destructor invalidates the owner and callback. A UIElement
retained after its TSF owner is released must refuse owner-dependent
operations, yet keep the containing DLL loaded until the final
UIElement reference is released. COM callers must follow the apartment
marshalling contract; this is not a cross-thread raw-pointer protocol.

The original Weasel-specific `RegisterServer/RegisterProfiles/`
`RegisterCategories` operations remain pending independent review.
Do **not** confuse the OpenWTL class factory cutover with full ownership
verification of the Rime/TSF Windows registry installation.

## Qualification

- `test/tsf_com_server_test.cpp` is built and executed against the real
  Weasel DLL in the existing Windows x64 build job, **not** via an extra
  installer or consumer project.
- Validate unknown CLSID, incorrect IID and null outputs, the inherited
  `ITfTextInputProcessorEx` / `ITfTextInputProcessor` QI identities
  (the legacy interface aliases the SDK extended interface), nonaggregation, LockServer and
  actual `DllCanUnloadNow == S_OK` after releasing the final object/factory.
- Existing [Draft #12](https://github.com/qigao/weasel/pull/12)
  independently handles ThreadMgr/ThreadFocus/KeyEvent sink cleanup and
  the unbounded detached reconnect callback; this branch alone does not
  establish safe deactivation under live Windows TSF hosts.
- Still required before merging: Windows native activation/deactivation
  under a real Text Services Framework manager, in-progress UIElement
  callback teardown, concurrent release, scoped registry operations,
  MSVC/LLVM-MinGW x64/ARM64 and exact-head CI evidence.

The current exact-HEAD build/runtime results are recorded in [PR #15](https://github.com/qigao/weasel/pull/15).
