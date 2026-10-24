#!/usr/bin/env python3
"""Fetch the CC0 variety pack from recorded sources and verify pinned SHA-256 hashes."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
from urllib.request import Request, urlopen

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--verify-only", action="store_true")
options = parser.parse_args()
for track in json.loads((root / "additional_music.json").read_text())["tracks"]:
    path = root / track["filename"]
    valid = path.is_file() and sha256(path.read_bytes()).hexdigest() == track["sha256"]
    if not valid:
        if options.verify_only:
            raise SystemExit(f"Missing or changed asset: {path.name}")
        with urlopen(Request(track["url"], headers={"User-Agent": "DemoEngine music asset fetcher"}), timeout=60) as source:
            data = source.read()
        if sha256(data).hexdigest() != track["sha256"]:
            raise SystemExit(f"Source checksum changed: {path.name}; review before replacing")
        temporary = path.with_suffix(path.suffix + ".download")
        temporary.write_bytes(data)
        temporary.replace(path)
    print(f"{path.name}: SHA-256 verified")
