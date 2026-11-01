#!/usr/bin/env python3
"""Fast structural regression gate; real Windows TSF runtime tests are separate."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
def source(path: str) -> str:
    return (root / path).read_text(encoding="utf-8-sig")

tsf = source("WeaselTSF/WeaselTSF.cpp")
keys = source("WeaselTSF/KeyEventSink.cpp")
header = source("WeaselTSF/WeaselTSF.h")
dll = source("WeaselTSF/dllmain.cpp")
edit = source("WeaselTSF/TextEditSink.cpp")

assert "SetUnhandledExceptionFilter" not in dll
assert "MiniDumpWriteDump" not in dll
assert "std::thread" not in tsf
assert ".detach()" not in tsf
assert "static unsigned int retry" not in tsf
assert tsf.count("_UninitThreadMgrEventSink();") == 1
assert tsf.count("_UninitThreadFocusSink();") == 1
assert "static weasel::KeyEvent prevKeyEvent" not in keys
assert "static BOOL prevfEaten" not in keys
assert "static int keyCountToSimulate" not in keys
for owned in ("_prevKeyEvent", "_prevfEaten", "_keyCountToSimulate"):
    assert owned in header
for marker in ("_keyEventSinkAdvised", "_dwThreadFocusSinkCookie",
               "_dwThreadMgrEventSinkCookie"):
    assert marker in tsf
assert "CONNECT_E_NOCONNECTION" in edit
print("PASS: TSF host/filter, apartment ownership and sink-source contracts")
