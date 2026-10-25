# Music, beat pulses and flashes

Every demo receives a shared beat response through `FrameInfo::beat`.
Geometry, recipe effects, camera motion, wire lighting, travellers, background art,
post effects, optical light fronts and final composition use the same continuous
beat clock. Animation time advances half a second per beat, so changing BPM changes
visual speed. Tracker tempo changes preserve the accumulated beat position.

The default `hybrid` source combines the tempo grid with actual musical attacks.
An adaptive audio-thread novelty detector recognizes fresh transients, suppresses
repeated triggers for 100 ms, and releases its envelope in seconds rather than
frames. Sustained pads do not hold flashes open. `audio` uses detected attacks only;
`clock` retains the deterministic grid. Silent playback falls back to the grid, and
fixed-step captures always use it so captures do not depend on audio callback timing.

Each attack decays independently of sustained audio volume. Bass adjusts its strength
without holding the flash open. Downbeats receive the strongest flash, with quieter
secondary beats and subdivisions. Flash uses highlight exposure rather than a white
screen overlay; black areas remain dark and HDR tone mapping retains the highlights.
Infinity's paused/reduced-motion Atlas modes keep their flash suppression.

## Controls

| Option | Behavior |
|---|---|
| `--beat-source S` | `hybrid` (default), `audio` or `clock` |
| `--pulse-amount F` | Shared pulse strength 0–2, default 1; **E** toggles |
| `--flash-amount F` | Additional exposure-flash strength 0–2, default .65; **F** toggles |
| `--beat-division N` | 1, 2 or 4 attacks per beat: quarter, eighth or sixteenth notes |
| `--beat-decay F` | Envelope decay 2–30; larger values give shorter attacks; default 9 |
| `--beat-offset S` | Attack offset in seconds, −2 to 2; positive values anticipate beats |
| `--no-pulse` | Disable the shared pulse modulation |
| `--no-flash` | Disable the additional exposure flash |
| `--bpm N` | Override track tempo; use for WAV and other PCM recordings |

Native music-band responses and slow scene motion remain available when shared
pulse or exposure flashes are off. Beat offsets adjust attacks and optical front
launches, without skipping the scene timeline. All envelopes are sampled directly
from beat position, so dropped frames do not shift later attacks.

## Music presets and playlists

`--music-preset stars|rendezvous|silicon|pulse|edm|dnb|jazz|ambient|new|all` selects bundled music. Repeat it
or combine it with `--music PATH` to construct a playlist. Infinity's default list
now includes all eight bundled tracks and still starts with Digital Rendezvous.
Track selection remains available while playback is paused. Explicit music selections start at their first entry unless `--track N` is supplied.

| Preset | Track |
|---|---|
| `stars` | Stars (4ch. version), cs127, CC BY 4.0 |
| `rendezvous` | Digital Rendezvous, Drozerix, public domain |
| `silicon` | Silicon Dancer, Drozerix, public domain |
| `pulse` | Wireframe Pulse, original CC0 synth loop; 128 BPM catalog hint |
| `edm` | Melodic EDM, Fupi, CC0, published 140 BPM |
| `dnb` | Black Diamond, Joth, CC0, published 143 BPM |
| `jazz` | Moil, Ruskerdax, CC0, dark jazz |
| `ambient` | Synthwave 15k, The Cynic Project / cynicmusic, CC0, ambient synth |
| `new` | The four newly downloaded tracks |
| `all` | All eight in the above order |

[Bundled music provenance and licences](../demos/wireframe/assets/music/README.md).

```sh
./build/hold_infinity --music-preset all --shuffle-seed 42
./build/hold_infinity --music-preset pulse --bpm 128 --beat-division 2
./build/impossible_wireframe --music-dir ~/Music/Demos --bpm 130 --volume .7
./build/hold_infinity --music song.flac --bpm 120 --beat-offset -.04
./build/hold_infinity --music-preset all --list-music
```

`--music-dir DIR` adds supported files from a folder in sorted order, without
recursing. Explicit files/presets precede folder tracks. `--shuffle` uses seed 0;
`--shuffle-seed N` supplies a reproducible unsigned 32-bit seed. `--list-music` prints
the selected playlist and codec availability before opening a window or audio device.
`--track N` is one-based and selects an entry after shuffling.

**S** skips forward; **Shift+S** skips back. **U** pauses/resumes playback, or use
`--music-paused` to start paused. In normal playback the visual beat clock follows
the music clock and pauses with it. `--fixed-step` deliberately uses a deterministic
capture clock independent of audio playback.

`--volume F` accepts 0–2; **[** lowers it and **]** raises it. The audio callback
bounds output samples. Analysis runs before output gain, so volume zero keeps music
reactivity while muting playback. `--silent` disables audio and uses the BPM clock.

## Audio formats and analysis

Tracker modules use libopenmpt; WAV uses SDL. Optional **libsndfile** enables FLAC,
OGG, MP3, AIFF and AU decoding according to the installed library's codec support.
This build has been checked with FLAC, OGG and MP3. CMake detects libsndfile without
making it mandatory. A build with SDL2/libopenmpt still plays trackers and WAV when
libsndfile is absent. No runtime download or decoder subprocess is used.

Files decode and convert to device-rate stereo before playback, retaining the
existing in-memory playlist behavior. The audio callback does no file I/O. Bass,
mid and treble now use separate channel filter state and coefficients based on the
actual device rate. The treble band measures the signal above the 3 kHz low-pass,
so stereo interleaving and sample-rate differences no longer distort those bands.
PCM recordings first use a known catalog tempo when available, including Wireframe
Pulse at 128 BPM, Melodic EDM at 140 BPM and Black Diamond at 143 BPM. Unknown PCM
tracks receive load-time positive-energy-flux analysis and normalized multi-period
correlation over at most 90 seconds. A result needs enough recurring attacks and
confidence before it replaces the fallback BPM. Tempo analysis never runs in the
audio callback. The curated ambient track intentionally retains free-time behavior.

Tempo estimation is not guaranteed to recover a musician's intended meter: half/double
time and syncopated material remain ambiguous. `--bpm` overrides every inferred/catalog
tempo; `--beat-offset` still adjusts grid attacks. `--beat-source audio` is useful for
free-time music because it follows notes and percussion without synthetic grid flashes.
The scene/animation clock remains continuous when tempos change. `--stats` reports
the selected beat source, frames influenced by real onsets, peak onset strength and BPM.

## Validation

```sh
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 tools/gpu_smoke.py --build build
```

CTest checks PCM tempo detection at 90/120/143 BPM across three sample rates, onset release and buffer-size independence, source mixing, envelope phase, decay, offsets, subdivisions, disable controls, invalid
options, presets and deterministic shuffle. The dummy-audio-device tests check
mono/stereo conversion, band analysis, gain limits, start-paused playback, clock
pause/resume, mixed playlists, tracker-to-PCM tempo reset and PCM attacks reaching the shared music state. The GPU smoke suite
renders every Infinity scene with maximum pulse/flash at sixteenth-note attacks,
alongside the existing deformation and traffic checks.

## Playback feedback

Press I (or launch with `--hud`) for the current track and credits, playback state,
volume, BPM, tempo source, pulse/flash settings and output resolution. The source
label reflects the clock fallback in silent or fixed-step mode. H opens the controls
card with demo-specific shortcuts. J cycles hybrid, audio and clock beat sources live.
Pause, volume, track and pulse/flash changes show a brief card even with the ticker off.
The cards use the resident GPU font, wrap at narrow window sizes, and remain responsive
while music is paused. `--no-hints` disables the brief startup hint.

The SDL callback performs no allocation, file I/O, locks, or transcendental filter
setup. Device-rate crossover coefficients are prepared when the device opens, and
the control/telemetry atomics use relaxed ordering because each value is independent.
Analysis still runs before output gain, so muted playback continues driving visuals.
Configure with `-DIW_ENABLE_AUDIO=OFF` for a clock-only build, or
`-DIW_ENABLE_SNDFILE=OFF` to retain tracker/WAV playback without optional codecs.
