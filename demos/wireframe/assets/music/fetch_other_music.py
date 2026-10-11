#!/usr/bin/env python3
"""Re-fetch the bundled tracker modules and verify them against The Mod Archive's MD5s.

- Stars (4ch. version) by cs127, CC BY 4.0, Mod Archive 201917 (the default track).
- Digital Rendezvous by Drozerix, Public Domain, Mod Archive 180821 (second in the playlist).
- Silicon Dancer by Drozerix, Public Domain, Mod Archive 209692.
"""
from hashlib import md5
from pathlib import Path
from urllib.request import Request, urlopen

HERE = Path(__file__).resolve().parent
TRACKS = [
    ("cs127_-_stars_4ch.mod", "https://api.modarchive.org/downloads.php?moduleid=201917", "8d4fa931ab5f55de7695b0898e7dcac9"),
    ("drozerix_-_digital_rendezvous.xm", "https://api.modarchive.org/downloads.php?moduleid=180821", "6e8306d6c451188265880279c170bdb9"),
    ("drozerix_-_silicon_dancer.mod", "https://api.modarchive.org/downloads.php?moduleid=209692", "04318458581491be90cc0c7e51e2b1a7"),
]

for name, url, want in TRACKS:
    data = urlopen(Request(url, headers={"User-Agent": "Mozilla/5.0"}), timeout=60).read()
    got = md5(data).hexdigest()
    if got != want:
        raise SystemExit(f"{name}: MD5 {got} does not match The Mod Archive's {want}")
    (HERE / name).write_bytes(data)
    print(f"wrote {name} ({len(data)} bytes, MD5 ok)")
