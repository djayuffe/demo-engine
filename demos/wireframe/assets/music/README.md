# Optional music

The demo can run from its internal BPM clock and does not require bundled audio.
Eight tracks are bundled. By default the demo plays **Stars, Digital Rendezvous, then Silicon Dancer, Wireframe Pulse and the four-track variety pack**, looping (key S skips to the next track, `--track N` starts on track N):

- **Stars (4ch. version)** — **cs127**, ProTracker module (`cs127_-_stars_4ch.mod`, 4 channels,
  4:21, 125 BPM, genre "Demo Style"). Source: The Mod Archive, module 201917,
  https://modarchive.org/index.php?request=view_by_moduleid&query=201917 (MD5
  8d4fa931ab5f55de7695b0898e7dcac9). Licence: Creative Commons Attribution 4.0
  (CC BY 4.0), https://creativecommons.org/licenses/by/4.0/ . Bundled unmodified.
  **This is the default track.** cs127 later finished it as a longer IT version:
  https://modarchive.org/index.php?request=view_by_moduleid&query=201894
- **Digital Rendezvous** — **Drozerix**, FastTracker 2 module (`drozerix_-_digital_rendezvous.xm`,
  20 channels, 2:44, 138 BPM, genre Chiptune). Source: The Mod Archive, module 180821,
  https://modarchive.org/index.php?request=view_by_moduleid&query=180821 (MD5
  6e8306d6c451188265880279c170bdb9). Licence: **Public Domain**. Bundled unmodified.
  Second track of the default playlist.
- **Silicon Dancer** — Drozerix, ProTracker module
  (`drozerix_-_silicon_dancer.mod`), Public Domain (The Mod Archive, module 209692).
- **Wireframe Pulse** — an original 30 s synth loop
  (`wireframe_pulse.wav`, 44.1 kHz stereo PCM), generated for this project and
  released to the **public domain (CC0)**. See the generator note below.

Suggested local layout:

```text
assets/music/cs127_-_stars_4ch.mod           # default (CC BY 4.0, credit cs127)
assets/music/drozerix_-_digital_rendezvous.xm # default playlist, second (public domain)
assets/music/drozerix_-_silicon_dancer.mod   # alternative (public domain)
assets/music/wireframe_pulse.wav             # alternative (CC0)
```

Run:

```sh
./build/impossible_wireframe                                   # Stars, 125 BPM read from the module
./build/impossible_wireframe --music assets/music/drozerix_-_silicon_dancer.mod
./build/impossible_wireframe --music assets/music/wireframe_pulse.wav --bpm 128
```

Tracker modules (`.mod .xm .it .s3m` ...) play through libopenmpt; `.wav` files are decoded by SDL, converted to float stereo at the device rate and looped. (Before this, the `.wav` was silently rejected by libopenmpt and the demo fell back to the BPM clock.)

`Wireframe Pulse` was synthesized from a small Python script (square-lead bass,
32-note two-octave lead over a 4-chord Am/F/G/Em loop, and a noise-hat layer)
and is looped to the demo's length. It is provided as a public-domain (CC0)
alternative so the scroller has a dependency-free, reproducible soundtrack.

When audio support is compiled in, libopenmpt/SDL2 play the module and its RMS
energy drives wire brightness. Without audio dependencies, the renderer falls
back to deterministic BPM timing.


## Downloaded variety pack (2026-10-24)

The following original files are bundled unmodified. All four source pages label
them CC0. Titles and creator credits also appear in the demo ticker.

| Preset | Track / artist | Style | Tempo |
|---|---|---|---|
| `edm` | [Melodic EDM / Fupi](https://opengameart.org/content/melodic-edm-loops) | Melodic dance loop | 140 BPM, published |
| `dnb` | [Black Diamond / Joth](https://opengameart.org/content/black-diamond) | Drum and bass | 143 BPM, published |
| `jazz` | [Moil / Ruskerdax](https://opengameart.org/content/moil) | Dark jazz | Automatic/fallback |
| `ambient` | [Synthwave 15k / The Cynic Project](https://opengameart.org/content/calm-ambient-2-synthwave-15k) | Ambient synth | Free-time |

The Cynic Project credit includes cynicmusic.com and pixelsphere.org as requested
on the source page. The MP3s require the optional libsndfile backend; the EDM WAV
also works through the standard SDL decoder. No re-encoding or trimming was applied.

`additional_music.json` records exact source/download URLs, authors, licence URLs,
file sizes and pinned SHA-256 checksums. Verify the actual files or re-fetch missing
ones using:

```sh
python3 demos/wireframe/assets/music/fetch_additional_music.py --verify-only
python3 demos/wireframe/assets/music/fetch_additional_music.py
./build/hold_infinity --music-preset new --beat-source hybrid
./build/hold_infinity --music-preset jazz --beat-source audio
```

A changed upstream checksum stops the fetch rather than silently replacing a track.
