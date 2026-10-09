# Optional music

The demo can run from its internal BPM clock and does not require bundled audio.
Two tracks are available:

- **Silicon Dancer** — Drozerix, ProTracker module
  (`drozerix_-_silicon_dancer.mod`), listed by the Quinlight Audio project as
  Public Domain. This is the default track.
- **Wireframe Pulse** — an original 30 s synth loop
  (`wireframe_pulse.wav`, 44.1 kHz stereo PCM), generated for this project and
  released to the **public domain (CC0)**. See the generator note below.

Suggested local layout:

```text
assets/music/drozerix_-_silicon_dancer.mod   # default (public domain)
assets/music/wireframe_pulse.wav             # alternative (CC0)
```

Run:

```sh
./build/impossible_wireframe --music assets/music/drozerix_-_silicon_dancer.mod --bpm 132
./build/impossible_wireframe --music assets/music/wireframe_pulse.wav --bpm 128
```

`Wireframe Pulse` was synthesized from a small Python script (square-lead bass,
32-note two-octave lead over a 4-chord Am/F/G/Em loop, and a noise-hat layer)
and is looped to the demo's length. It is provided as a public-domain (CC0)
alternative so the scroller has a dependency-free, reproducible soundtrack.

When audio support is compiled in, libopenmpt/SDL2 play the module and its RMS
energy drives wire brightness. Without audio dependencies, the renderer falls
back to deterministic BPM timing.
