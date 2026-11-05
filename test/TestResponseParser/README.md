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

TinyTest supplies `main()` and reports each case in TAP. CTest also requests
`response-parser.xml` in the profile's `test/TestResponseParser` build directory;
the Windows workflow uploads these JUnit reports even when a test fails.
Fatal checks remain enabled in Release and return a failing process status.

This suite tests the existing text/archive protocol. Bounded framing, malformed
archive recovery and atomic publication belong to the protocol migration in
issue #2. The manual `TestWeaselIPC` console is a separate tool; isolated IPC,
authorization and TSF integration tests remain tracked by issues #1–#4 and #6.
