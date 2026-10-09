#!/usr/bin/env python3
"""Fetch the selected freely licensed tracker module for local runs.

Target: Drozerix — Silicon Dancer (MOD), listed by the Quinlight Audio project
as Public Domain. The binary module remains ignored by Git; this script makes a
local runtime copy for the OpenGL demo.
"""
from pathlib import Path
from urllib.request import urlopen

URL = "https://media.githubusercontent.com/media/Kind-Computers/quinlight-audio/main/mods/drozerix_-_silicon_dancer.mod"
OUT = Path(__file__).resolve().parent / "drozerix_-_silicon_dancer.mod"

data = urlopen(URL, timeout=30).read()
if len(data) < 1084:
    raise SystemExit("download too small to be a ProTracker-compatible module")
OUT.write_bytes(data)
print(f"wrote {OUT} ({len(data)} bytes)")
