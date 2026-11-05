# Response parser publication contract

`ResponseParser` borrows its output pointers for a synchronous call on their
owner thread. It does not synchronize a parser or its destinations across
threads, call UI code, reconnect, or replay requests.

Each call admits at most 32,768 UTF-16 code units, matching the existing
64 KiB pipe buffer on Windows. Null, empty, or larger input is rejected before
reading it. Input is borrowed only until the call returns.

The parser copies only the requested destinations into private staging storage.
Actions are activated separately for each response. Complete lines update that
staging storage. A newline-terminated `.` publishes the commit, context, status,
configuration and style with non-throwing swaps, then returns `true`. Bytes after
that terminator remain outside the response. The call has no callbacks or
suspension points during publication; this is a single-owner transaction, not
an atomic operation for concurrent readers.

Missing terminators, truncated lines, invalid archives and standard allocation
or decoding exceptions return `false` without publishing any staged data.
Archive construction and extraction share this boundary. The decoder never
opens a message box. A later call starts a new transaction and cannot inherit
enabled actions from either a failed or successful earlier response. TSF callers
must stop applying a response when the callback returns `false`.

Unknown actions, inactive fields, comments and non-assignment lines retain the
existing ignore behavior. Optional destinations remain optional, including
configuration without context and context without configuration.

This is a correction to the current text/archive decoder. It does not replace
Boost.Serialization or validate all of its declared container sizes before
allocation. The outer input limit does not establish a hard heap bound for
malicious archive counts. The versioned, bounded wire schema, enum/Unicode
validation and framing/session IDs remain required by issue #2. Transport owner,
cancellation and peer authorization remain separate work in issues #1 and #3.

Regression scenarios belong to the production-linked TinyTest parser suite:
rollback across every destination, malformed headers and truncated archive
objects, independent optional destinations, action isolation, recovery after
failure, and exact/over-limit input boundaries. Existing valid response cases
must continue to pass.

On 2026-11-05, all 31 parser cases and both existing TSF DLL tests passed
locally with MSVC x64 and LLVM-MinGW 20261020 x64. Both complete x64 product
graphs built, and LLVM ARM64 compiled the changed IPC/TSF code and parser
suite. Native ARM64 execution and live TSF-host error handling remain untested.
