# Weasel TSF COM Server / OpenWTL post-v0.1.0 cutover (Issue #14)

The only source of reusable COM server infrastructure is pinned
[qigao/OpenWTL](https://github.com/qigao/OpenWTL) at
`c0b077e37e5ca23dbcf316d7a2f94d30c90734ed` ([OpenWTL PR #110](https://github.com/qigao/OpenWTL/pull/110) standalone-header namespace candidate), under
`external/OpenWTL`. OpenWTL's **single public CMake target** is
`OpenWTL::WTL`; the existing MSBuild product graph consumes its source headers directly;
conversion to the single CMake target remains tracked in [Weasel #7](https://github.com/qigao/weasel/issues/7).

The immutable [OpenWTL v0.1.0](https://github.com/qigao/OpenWTL/releases/tag/v0.1.0) release tag remains at
`cf2b4443ef068c002fa781ab96aaafe22a5f96e7`. The Weasel gitlink now tests the proposed standalone `atlcomcli.h` namespace fix
`c0b077e37e5ca23dbcf316d7a2f94d30c90734ed` (Git tree `27233e1c63c62a5475d7c9d2027cb59d482ecdd9`).
It is based on merged `107147834e7c82ddf44016a56b4ed9d331e25e64` and retains
`CSize::SetSize(int,int)` from PR #108. PR #110 is Ready, unmerged/unreleased.
[Its exact-HEAD CI #41335463962](https://github.com/qigao/OpenWTL/actions/runs/41335463962) passed all six jobs, including
MSVC/llvm-mingw x64 and real ARM64 normal/full-original source subsets x5.
This downstream product build and its real TSF tests remain a separate gate.
Its [OpenWTL PR #108 CI #41332552933](https://github.com/qigao/OpenWTL/actions/runs/41332552933)
passed 6/6 Windows source/native jobs on exact PR HEAD `b19159b684c322f2318aae76126ad271f1bf4a40`.
The merged master exact-HEAD qualification is tracked separately at
[CI #41333224581](https://github.com/qigao/OpenWTL/actions/runs/41333224581).
**Neither upstream CI substitutes for Weasel's real Windows TSF DLL/COM tests.**

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
- Validate unknown CLSID, incorrect IID and null outputs, the separately
  implemented `ITfTextInputProcessorEx` / `ITfTextInputProcessor` QI
  identities (the legacy interface aliases the SDK extended interface), nonaggregation, LockServer and
  actual `DllCanUnloadNow == S_OK` after releasing the final object/factory.
- Existing [Draft #12](https://github.com/qigao/weasel/pull/12)
  independently handles ThreadMgr/ThreadFocus/KeyEvent sink cleanup and
  the unbounded detached reconnect callback; this branch alone does not
  establish safe deactivation under live Windows TSF hosts.
- Still required before merging: Windows native activation/deactivation
  under a real Text Services Framework manager, in-progress UIElement
  callback teardown, concurrent release, scoped registry operations,
  MSVC/LLVM-MinGW x64/ARM64 and exact-head CI evidence.

**No claims of passing Windows CI are made by this source-only commit.**
