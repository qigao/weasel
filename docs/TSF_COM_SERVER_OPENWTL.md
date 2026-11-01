# Weasel TSF COM Server / OpenWTL cutover (Draft #14)

The only source of reusable COM server infrastructure is pinned
[qigao/OpenWTL](https://github.com/qigao/OpenWTL) at
`7e9a25aa646ed163a337a8338dbbd9b8437b65e8`, under
`external/OpenWTL`. OpenWTL's **single public CMake target** is
`OpenWTL::WTL`; the existing MSBuild/xmake graphs consume the exact same
source include directories directly while their conversion to CMake is
tracked in [Weasel #7](https://github.com/qigao/weasel/issues/7).

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
- Validate unknown CLSID, incorrect IID and null outputs, inherited
  `ITfTextInputProcessor` QI identity, nonaggregation, LockServer and
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
