# Weasel TSF COM Server / OpenWTL v0.1.0 cutover (Draft #14)

The only source of reusable COM server infrastructure is pinned
[qigao/OpenWTL](https://github.com/qigao/OpenWTL) at
`cf2b4443ef068c002fa781ab96aaafe22a5f96e7`, under
`external/OpenWTL`. OpenWTL's **single public CMake target** is
`OpenWTL::WTL`; the existing MSBuild product graph consumes its source headers directly;
conversion to the single CMake target remains tracked in [Weasel #7](https://github.com/qigao/weasel/issues/7).

The Git submodule is pinned to the immutable [OpenWTL v0.1.0](https://github.com/qigao/OpenWTL/releases/tag/v0.1.0) release commit
`cf2b4443ef068c002fa781ab96aaafe22a5f96e7` (tree `09065079ddaeb098f29cd15791c961d7af6fe2be`).
The release has exact-HEAD [master CI #41320380332](https://github.com/qigao/OpenWTL/actions/runs/41320380332): 6/6 successful jobs, including MSVC and llvm-mingw x64/ARM64 and real native Windows ARM64. It includes the upstream counted-Unicode CString resource fix formerly tracked in OpenWTL #75 / PR #76.
**This verifies OpenWTL itself, not Weasel's Windows TSF DLL or its fork Actions.**

Weasel's own TSF interfaces, CLSID, session, profile/category registration,
candidate UI and Rime business belong to Weasel. OpenWTL's
`OpenATL::COM::Module` and `Object` own the generic class factory,
IUnknown identity, object references and module/server lock lifetime.
The Weasel TSF DLL publishes only the one class in its immutable class
table, and never modifies its host-wide crash exception filter.

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
  identities (both independently inherit IUnknown), nonaggregation, LockServer and
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
