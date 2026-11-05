# Response parser tests

This C++17 TinyTest suite links the production `WeaselIPC` library and the
canonical `Salts::TinyTest` static runtime from the pinned SaltsCore submodule.
It runs without a server, an installed IME, user configuration, or input prompts.
The former Boost lightweight-test runner and its Visual Studio project are
replaced by the CMake target `TestResponseParser`.

Use the project presets from the matching Windows developer environment:

```bat
cmake --preset msvc-x64
cmake --build --preset msvc-x64 --target TestResponseParser
ctest --preset msvc-x64 -R weasel.response_parser --verbose
```

The `llvm-x64` presets also run the suite. Both ARM64 profiles build it;
run ARM64 CTest only on native ARM64 Windows.

The four original sample behaviors are retained. The candidate sample uses
the current Server's serialized `ctx.cand` format, replacing the obsolete
`ctx.cand.length`/index/page fields. Additional cases cover CJK and emoji,
escaped text, multiple candidates with labels and comments, empty candidates,
optional output destinations, terminators, incomplete input, malformed lines,
inactive actions and incomplete cursor fields. Valid responses include the
protocol's `.\n` terminator, and tests check the parser's completion result.
The suite now has 31 cases, including rollback of every output on incomplete or
malformed responses, malformed archive headers and extraction failures,
recovery and action isolation on parser reuse, independent optional destinations,
and exact/over-limit receive lengths. See the
[publication contract](../../docs/RESPONSE_PARSER.md).

TinyTest supplies `main()` and reports each case in TAP. CTest also requests
`response-parser.xml` in the profile's `test/TestResponseParser` build directory;
the Windows workflow uploads these JUnit reports even when a test fails.
Fatal checks remain enabled in Release and return a failing process status.

This suite tests the existing text/archive protocol. Parsing is transactional
on its owner thread and archive errors return failure without a dialog. The
outer input limit does not bound allocations from malicious archive counts;
the replacement wire schema and its field limits remain part of issue #2.
The manual `TestWeaselIPC` console is a separate tool; isolated IPC,
authorization and TSF integration tests remain tracked by issues #1–#4 and #6.
