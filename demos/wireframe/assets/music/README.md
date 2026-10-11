# Optional music

The demo can run from its internal BPM clock and does not require bundled audio.
Three tracks are bundled:

- **Stars (4ch. version)** — **cs127**, ProTracker module (`cs127_-_stars_4ch.mod`, 4 channels,
  4:21, 125 BPM, genre "Demo Style"). Source: The Mod Archive, module 201917,
  https://modarchive.org/index.php?request=view_by_moduleid&query=201917 (MD5
  8d4fa931ab5f55de7695b0898e7dcac9). Licence: Creative Commons Attribution 4.0
  (CC BY 4.0), https://creativecommons.org/licenses/by/4.0/ . Bundled unmodified.
  **This is the default track.** cs127 later finished it as a longer IT version:
  https://modarchive.org/index.php?request=view_by_moduleid&query=201894
- **Silicon Dancer** — Drozerix, ProTracker module
  (`drozerix_-_silicon_dancer.mod`), Public Domain (The Mod Archive, module 209692).
- **Wireframe Pulse** — an original 30 s synth loop
  (`wireframe_pulse.wav`, 44.1 kHz stereo PCM), generated for this project and
  released to the **public domain (CC0)**. See the generator note below.

Suggested local layout:

```text
assets/music/cs127_-_stars_4ch.mod           # default (CC BY 4.0, credit cs127)
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
