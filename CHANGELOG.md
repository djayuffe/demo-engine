# Hold Infinity 1.9 Matrix walkers and geometric validation — 2026-10-22

- Add footprint-filtered, beat-accented ant-like walkers on intrinsic geodesic surface lanes.
- Preserve colour ratios in chamber/glass highlight compression and bound texture mip sampling.
- Share and test camera, plane rotation and Lorentz mirror construction; reduce long-running rotation angles in double precision.
- Validate convex mesh topology, face planes, camera orthogonality, rotation length and hyperbolic reflection invariants.

# Hold Infinity 1.8 detail and GPU cleanup — 2026-10-22

- Filter minified filament detail using its screen-space footprint to reduce shimmer.
- Upload shared plane-rotation coefficients once per sheet and skip unused optical image allocation for light-only scenes.
- Recover invalid pointer state, bound parallax inputs and pin benchmark eye position for deterministic comparisons.
- Retain the original integration limits and discard a projection-pass experiment that did not show a consistent GPU benefit.

# Hold Infinity 1.7 translational parallax — 2026-10-22

- Translate the camera origin across every optical scene; apply matching view-space translation to dimensional light sheets.
- Add smoothed pointer-driven eye movement and a subtle beat-clock dolly, preserving mouse orbit and fixed still rendering.
- Add deterministic eye-offset controls and regression coverage for translation direction, still behavior and frame-rate-independent smoothing.

# Hold Infinity 1.6 organic light and rhythm — 2026-10-22

- Add bounded, endpoint-anchored traveling waves and fine moving striations to dimensional light sheets.
- Separate bar accents, half-beat treble detail and eight-beat breathing from sustained audio volume.
- Render hot filaments, coloured inner glow and faint outer halos as three GPU layers; retain deep black backgrounds.
- Drive chamber glow and glass bloom with rhythmic accents; cover beat decay and phrase boundaries in regression tests.

# Hold Infinity 1.5 projection and light corrections — 2026-10-22

- Correct undefined reversed smoothstep fades throughout native LED, reflected-light and distance-detail shaders.
- Use the inverse camera basis and homogeneous perspective interpolation for dimensional light ribbons.
- Follow bends with centered tangents and soften hard ribbon boundaries with derivative-filtered edge coverage.
- Preserve archived shaders, numerical optical solvers, black backgrounds and song-two startup.

# Hold Infinity 1.4 black and HDR light — 2026-10-22

- Clear the final light scenes to pure black and remove the unused field pass.
- Deepen display blacks, expand the hue-preserving HDR shoulder, and contrast brighter narrow cores with softer halos.
- Lock light packets to four-beat circuits and use a sharp phase-derived beat envelope.
- Start with Digital Rendezvous (song two), retain explicit track overrides, and initialize tempo from the selected song.

# Hold Infinity 1.3 dimensional light — 2026-10-22

- Rebuild the final two scenes as a four-dimensional light loom and prismatic hyperspace threads, with layered GPU instances and XW/YZ rotation.
- Project ribbon tangents into screen space and scale width/glow by depth; retain narrow highlights and a quiet background.
- Synchronize traveling light packets to musical beat position and preserve the original GPU RK4 integration.
- Share the artistic presentation between reference and optimized modes for valid workload comparisons.

# Hold Infinity contrast pass — 2026-10-22

- Reduce chamber/glass bloom, star glare and beat-driven light amplification.
- Darken midtones and apply a hue-preserving highlight shoulder so bright edges retain detail instead of approaching display white.
- Reduce the ray-lab halo while keeping its narrow luminous core readable.
- Refresh the twelve-scene native capture sheet; all GPU scenes and both test suites pass.

# Hold Infinity 1.2 adaptive GPU playback — 2026-10-22

- Drive per-scene pixel budgets from non-blocking completed GPU queries, with bounded quality, smoothed timing, hysteresis and resize cooldown.
- Keep query feedback allocation-free during ordinary playback; reserve timing histories and output for explicit profiling.
- Preserve explicit pixel budgets, original-workload comparisons and stationary 400-sample accumulation.
- Add GPU target controls and regression coverage for slow/fast GPUs, recovery, stale/invalid samples and target stability.
- Validate all optical scenes, fixed-budget behavior and stationary convergence on the native renderer.

# Hold Infinity 1.1 GPU optimization — 2026-10-21

- Prepare GPU optical-state kernels and immutable geometry once; cache uniform locations and eliminate inactive parameter uploads.
- Reuse unchanged GPU ray fields, dispatch active rows only, and stop rebuilding bloom after stationary convergence.
- Add asynchronous GPU timing, deterministic silent captures, original-workload reference mode, all-state specialization/cache tests, and an alternating image/performance benchmark.
- Local GPU-query means: continuous glass 21% faster, printed layers 61%, fixed-curvature ray lab 32%; chambers and dynamic spectral rays are effectively unchanged. Full conditions and raw data are in `demos/infinity/GPU_PERFORMANCE.md`.

# Hold Infinity native port — 2026-10-21

- Port all three GPU optical pipelines and dodecahedron geometry from the user-supplied archive to GLSL 4.10/C++.
- Make `hold_infinity` the default show; retain the prior entry point as `make run-wireframe`.
- Direct 12 scenes, six light fronts, camera flow, glow and ray intensity from music beat/band analysis.
- Preserve spectral Fresnel/TIR, horizon, seven printed layers, two-stage RK4 ray integration, mouse orbit and 400-sample still accumulation.
- Add a custom-renderer lifecycle to the engine, bounded native float targets, Windows GL entry points, headless sync tests and a twelve-scene GPU smoke run.

# Changelog

## 1.6.0 - Impossible worlds

Seven selectable background worlds with an auto-director that cuts on the music, interactive
per-layer parallax depth, a gloss finish, cinematic scene flow, 14 new impossible-geometry scenes
(73 in all), new special-effect passes, wide antialiased wires, a bloom pyramid, macOS/Retina tuning
and a deterministic capture clock. Fully additive: existing demos build and run unchanged.

- Scene flow (`Transition.hpp`, test B28): scenes no longer hard-cut. The last beat of every scene is an exit (the form rushes toward the camera, a radial zoom blur builds, the iris tightens and cinema bars close in); the first beat of the next is an entry (the new form lands with an exposure punch on the wires and glow, a chromatic split snaps back, the form grows in from 80% with a thicker stroke, the bars open). Manual scene changes get the entry. Logo cards are untouched.

- Show options moved out of `Engine.cpp` into the pure `ShowOptions.hpp` (test B27) with strict number parsing: `--parallax abc`, `--bpm 12x`, `--track 1.5`, `--pointer 1,y` or `--fixed-step nan` are now errors with a message instead of silently becoming 0. `Engine.cpp` lost ~100 lines of option plumbing.

- Swap music: key **S** jumps to the next track (Shift+S: previous), `--track N` starts on track N, and the default playlist now holds all three modules (Stars, Digital Rendezvous, Silicon Dancer). Tempo re-locks to each track as before.
- Background parallax is independent of the effects: the background has its own gain (`--bg-parallax F`, key **K**), the effects plane (wires, depth motes) keeps `--parallax`/G, and the director's energy breathing now moves only the effects plane.

- Seventh style `fractal`: a ray-marched fly-through of an infinite, slowly twisting and breathing Menger-sponge cathedral: orbit-trap tinted stone, step-count ambient occlusion, headlight and specular gloss, travelling light orbs that light the walls and pulse on the beat, volumetric glow and depth fog. Speed follows the music; the mouse steers the gaze. Objects: structure, orbs, glow, fog. Costs about the same as the black hole; the director favours it for driving and peak music.

- Wire exposure accounts for wide-wire width (`WireGain.hpp`, test B26): the density gain was calibrated for 1-pixel lines, but the geometry-shader wires are 1.4-9 px wide (wider on Retina), so dense scenes deposited several times the light. Dense scenes are now pulled back toward 1-pixel energy; sparse scenes keep their bright strokes. At 1440x900: scene 3 pure-white pixels 1.7% -> 1.3%, scene 4 0.6% -> 0.4%; the gain grows with resolution.
- Quality audit: all 73 scenes rendered on the deterministic clock and ranked by blow-out and darkness; no scene now exceeds 9% pure-white pixels at intro time (the dark outliers are scenes still growing in).

- Fixed white blow-out in dense scenes: measured with a deterministic clock, the gloss glints were the main cause (scene 4: 16.8% of pixels pure white -> 7.1%, scene 64: 12.0% -> 4.5%, now equal to the gloss-off picture). Glints now only spike from a bright point into darker surroundings and are soft-saturated; a hue-preserving highlight knee on the wire layer keeps dense bundles coloured.
- `--fixed-step S`: deterministic show clock (frame N is at N*S s), bit-identical repeat renders for captures and A/B checks.
- Director compositions: on about one cut in three (more often in calm music) the director leaves out one secondary object of the new world (never its centrepiece), on top of your own `--hide`/F-key choices, so return visits differ.

- Auto-director (`--background-style auto`, `Director.hpp`, test B25): replaces the fixed 40 s timer. It follows the music energy (fast/slow followers), detects drops and breakdowns, cuts only on bar lines after a 24 s minimum (60 s maximum) hold, fires a shockwave on a drop, picks the next world by how well it suits the current energy and how long ago it was shown, and breathes the parallax gain with the energy (1.0-2.2) unless `--parallax`/G is set.

- Gloss finish (key **O** / `--no-gloss`): a specular sweep that glides over mid-bright detail every 7.5 s, four-point diagonal glints thrown by the brightest highlights (sampled from the bloom), and a pulsing glass-bezel rim light. It never pushes already-blown pixels further.

- Per-object switches in every background style: F1..F12 toggle the current style's objects (classic: organism, ink, glass, rose, julia, moire, turing, contour, poincare, kifs, ribbons, ambient; synthwave: sun, mountains, grid, stars, aurora, reflection, sky; nebula: gas, stars, flares, supernova, comets, dust; mandala: crystal, neon, folds, mirror, rings; blackhole: disk, photonring, jets, galaxy, stars; hyperspace: walls, streaks, glow, flash). `--hide a,b` starts with objects off; V and F-keys print the current switches. Test B24.

- Adaptive background resolution: a frame-time governor (`AdaptiveRes.hpp`, test B23) steps the background pass from 1/2 to 1/3 to 1/4 of the HDR size when the demo cannot hold ~45 fps, and tries finer again after 20 s of headroom. `--no-adaptive` pins it at 1/2.

- Robustness: the style pass sanitises NaN/Inf and clamps HDR output before it can reach bloom; `--pointer X,Y` pins the pointer for screenshots and parallax checks.

- Sixth style `hyperspace`: warp tunnel with six depth shells of streaking stars, ribbed energy walls, a pointer-steered vanishing point, speed and flashes driven by the music.

- Key **C** saves the current frame as `demo-<date>-<time>.png` in the working directory; key **H** prints the key list.

- Polish: background-style parsing, the fade state machine, parallax-gain cycle and idle sway moved into the pure, unit-tested `BackgroundStyle.hpp` (test B22); black hole ray-march exits early once the disk is opaque; calmer synthwave aurora.

- Eye-candy pass: synthwave aurora curtains and sun reflection on the floor; nebula beat-driven supernova shells; mandala counter-rotating second fold; black hole spiral arms and orbiting hot spots; style changes now fade through black; `--background-style auto` cycles a new world every 40 s (V takes over).

- Maximised per-layer parallax: 5 depth-mote planes (near planes slide opposite to far ones), every classic art layer and every style layer at its own depth, the wires as a mid plane, idle sway so depth lives without the mouse. `--parallax F` (0-3, default 1.6) / key **G** sets the gain.

- Interactive depth: the mouse steers every background (black-hole camera, synthwave steering, per-layer nebula parallax, mandala centre, classic parallax); a click fires a shockwave. New independent depth-mote layer (3 parallax planes, near bokeh, pointer repels them) drawn in front of the wires: key **D** / `--no-depth`.

- Three selectable background styles: `synthwave` (striped sun, neon grid, ridgeline), `nebula` (parallax self-shadowed gas, flared stars, shooting stars), `mandala` (folded orbit-trap kaleidoscope). `--background-style classic|synthwave|nebula|mandala`, key **V** cycles.
- Fourth style `blackhole`: Schwarzschild ray-marched black hole (lensed galaxy sky, Doppler-beamed turbulent accretion disk, photon ring, polar jets).
- Styled backgrounds keep their own palette: the post hue-drift/solarize/posterize grade is held back to 12% when a style is selected.
- Final output is TPDF-dithered (±1 LSB) to remove 8-bit banding in dark gradients.
- Background and logo switches: keys B (procedural background) and L (logo cards) toggle at runtime; `--no-background` and `--no-logo` start with them off.
- Signal packets: with the geometry stage every wire knows its position along itself, so each carries a travelling pulse (own speed and phase,
  faster with the music): the whole mesh reads as circuitry with data flowing through it.
- Wide antialiased wires: `wire.geom` expands every line into a screen-space quad (~1.4 px at 900 px height, growing with beat/music,
  scaled by render resolution) with a soft edge, so wires are no longer 1 px thin on Retina/macOS core profiles. Falls back to plain
  GL lines if a driver rejects the geometry stage.
- Outstanding fixes: Boy's surface rebuilt on the Bryant-Kusner disk parametrisation (uniform cells, exact antipodal rim weld; the old
  u=pi/2 ring collapsed to a point and stretched the mesh). Scene 65 gets pole vertices (no open caps), 63 gets more agents, a soft
  wall and capped relationship edges, 64 is thinned from a hairball, 61 has denser longer trails. Golden hashes: every effect, recipe and
  mutation hash is unchanged; only scenes 7, 60, 61, 63, 64, 65 differ (intended) plus the new 67-72.
- macOS/Apple Silicon: the HDR target is capped to ~3.1 MP by default (a 2880x1800 Retina framebuffer renders at ~0.77 scale and is upsampled);
  an explicit `--quality F` is honoured exactly. Release builds on arm64 use -O3 and -mcpu=native. See `RenderScale.hpp`.
- Background gallery: four new slow, painterly layers in `lib/art_layers.glsl` that surface in waves: topographic contour flow, a
  Poincare-disk hyperbolic tiling on a sliding Moebius map, a kaleidoscopic IFS lace and auroral ribbons. `tools/artshot.cpp` dumps the
  background at chosen times for review.
- Background refactor: shaders gained `#include` (expanded by `glutil::readText`); shared helpers live in `shaders/lib/common.glsl`,
  the art layers in `lib/art_layers.glsl`, and `art.frag` main is split into named ambient-layer functions. The art pass is now its
  own `ArtPass` class out of `Renderer`. `tools/shaderdiff.cpp` renders old vs new shaders: art.frag and post.frag are bit-identical.
- Refactor: `Camera.hpp` (pure camera/FOV, now keeps the horizontal field of view on windows narrower than 16:9; unchanged at 16:9 and wider),
  `LogoShow.hpp` (pure logo fade/hold/black timing and Ken Burns zoom), `Describe.hpp` (scene description line). Unit-tested.
- Scenes 67-72: hyperbolic city (#81), spherical universe (#82), geodesic lightning (#87/88), time-dilation cathedral (#14),
  discrete-space crystal (#83), conflict geometry (#74). Scene count is now 73.
- `LookFx` pass (one shader, previous-frame aware, zero cost when off): chromatic gravity (#27), prediction failure storm (#33),
  colorless chromatic matter (#23), specular mutation (#30), negative reflection (#24). `Frame::look[5]`; keys 1-5.
- Demo key for the temporal sphere moved from T to Y (T is the engine's ticker toggle).
- New `SpecialFx` pass (GL 4.1, no compute/texture arrays): temporal-refraction sphere (#13, 16-frame history atlas),
  persistent memory layer (#25, advected/diffusing/hue-ageing half-res ping-pong). Returns the scene texture untouched
  (zero cost) when both are off.
- Noncommutative camera (#86): `Frame::modelTransform` 3x3 driven by shear products (`Choreography.hpp`); scene 59.
- `Frame` gains `modelTransform`, `temporalRefraction`, `memory`. Demo keys: N, T, M toggle the effects in any scene.
- Scene 60 holonomy arrows enlarged; choreography unit tests added.
- Post bloom is now a 6-level dual-filter pyramid (`Bloom`, shaders `bloom_down/up.frag`) with a soft knee and Karis averaging, replacing the 6-tap blur; skipped with --no-post.

## 1.5.0 - Optimised, playlists
### Performance
- Effects: sort-based duplicate removal only after the four effects that can create duplicates; a 60,000-edge budget (copy/echo chains reached 404,000 edges); a stage with non-finite output is undone.
- Wireframe demo: scenes build on worker threads (`SceneSystem::meshAsync`, `prefetch`), so the 70-300 ms implicit-surface rebuilds never block a frame.
- Upload: O(edges) `geo::renderable` check, direct index upload, buffer orphaning; `geo::validate` is sort-based.
- GPU: procedural background and art layer moved to a half-resolution pass (`engine/shaders/art.frag`), skipped while a logo card covers the screen (it was about 70% of the frame cost, visible or not). `--quality` now scales it too.
### Fixes
- A rejected mesh no longer ends the run (exit code 5 is gone): it is skipped and reported once.
- Vsync in full-screen too; `--no-vsync` for benchmarks. `--stats` prints fps and CPU time per frame.
- The downbeat solarize flash no longer lifts black to grey.
- Tests: edge budget and renderability of every wireframe scene through the auto recipe, full validation of every effect, async fetch convergence.
- Playlists: `Config::musicFiles` plays every file found, in order, looping; `--music` can be repeated. `FrameInfo::musicPath` follows the track that is playing; the beat clock locks to each track's tempo without a jump (`Timeline::retempo`). Detected tempos are folded into 70-180 BPM.
- Wireframe demo: "Digital Rendezvous" by Drozerix (Public Domain, The Mod Archive 180821) follows Stars by cs127; the ticker credit switches with the track.

## 1.4.0 - Soundtrack and tempo sync
- The beat clock locks to the playing module's own tempo (tempo x 6 / speed) unless `--bpm` is given; `RunInfo::musicPath` tells the demo which track is playing.
- Wireframe demo: new default soundtrack **"Stars (4ch. version)" by cs127** (CC BY 4.0, The Mod Archive 201917), credited in its README, NOTICE, music README and in the ticker while it plays. `make run-silicon` plays Silicon Dancer.
- `AudioPlayer` plays `.wav` files through SDL (decoded once, converted to float stereo, looped), with the same band analysis and music clock as modules. `--music file.wav` used to be rejected by libopenmpt.
- The music clock is now total played time, not the position in the track, so the show keeps going when a track loops instead of restarting its scene sequence and logo show (a 30 s loop never got past scene 8).

## 1.3.0 - Golden glimmer and golden orbits
- Engine: `Frame::wireStyle` and `Renderer::setWireStyle`. Style 1, golden glimmer, draws wires additively without depth test (point and orbit clouds accumulate light), twinkles each dash in gold with a sweeping shimmer band, and makes the post pass add gold horizontal-plus-vertical glints, tint the light rays gold, quiet the art background and switch off warps, colour grading and the rainbow tint.
- Refactor: `.clang-format` plus reformatting of the dense one-line sources (object code verified byte-identical before and after); `applyEffect`'s 280-line switch split into six family functions; `tools/golden.cpp` bit-exact regression hashes.
- Perf: `VertexWeld` hashed-grid welding replaces the O(n) vertex scan in both marching-tetrahedra builders (gyroid n=96: 54 s -> 1.2 s, identical meshes).
- Fix: Mobius strip and Klein bottle seams close along the surface identification (no stray full-width edges); Boy surface seam glued numerically.
- Fix: `bg.frag` cover-fit cropped the wrong axis, stretching logo cards on any window that is not the card's aspect ratio.
- Fix: scroller measures only the characters it uploads (no blank scroll for text over 512 chars); logo upload ignores stale GL errors; mutateRecipe seed cast no longer undefined behaviour; Makefile stamp target compared an always-empty make variable and reconfigured every run.
- Docs: `sceneIndex` is documented as reserved (the logo card is chosen by time).
- Wireframe demo: scenes 59-66, eight "impossible" wire objects adapted from a 100-effect design list to what the CPU-built GL 4.1 wireframe path can show: Noncommutative lattice (#86, Heisenberg commutator), Holonomy sphere (#90, parallel transport), Motion-only sculpture (#55), Atomic collision field (#61, CPU model of atomicAdd scatter), Relational gravity (#77), Data-collision sculpture (#37), Self-refuting object (#48, repelling fixed point, bounded solver) and Contradictory silhouette (#8, visual hull of three incompatible silhouettes). 67 scenes in total; the golden-orbit render style is now limited to scenes 57-58.
- Wireframe demo: scenes 57 Golden Galaxy and 58 Golden Cliff, ports of GALAXY.COM (de Jong attractor) and CLIFF.COM (Clifford attractor) from uber256-lab with the original constants, start points and parameter morph. 59 scenes in total.

## 1.2.0 - Unknown effects
- 16 new effects (64-79): Hopf rotation, Klein fold, hyperbolic drift, Thomas flow, galactic disk, harmonic bloom, FCC crystal snap, tendril growth, shell cage, barbed wire, torus attractor, Moebius map, soliton wave, quaternion square, Lissajous satellites, spectral harmonics.
- 6 new recipes (24-29). `effectCount()` is now 80 and `recipeCount()` 30; the procedural mutation draws from all of them.
- Tests: distinct effect names, and every new effect must change a mesh.

## 1.1.0 - Windows support
- In-tree OpenGL 4.1 function loader for Windows (`GLWin32.hpp/.cpp`): declares exactly the functions and constants the renderer uses and fills them via `glfwGetProcAddress`. Other platforms are unchanged.
- MSVC definitions (`NOMINMAX`, `WIN32_LEAN_AND_MEAN`, `_CRT_SECURE_NO_WARNINGS`) and warning policy; Windows CI job restored.
- Verification: the same loader and build changes pass build + tests on macOS, Linux and Windows in the public wireframe-scroller CI. This repo's CI could not start jobs (GitHub billing limit), and nothing has been run on physical Windows hardware yet.

## 1.0.1 - CI and build fixes
- Fixed a parallel-build race: engine shaders are now copied once by a shared `de_shaders` target instead of by every demo.
- Fixed a misleading-indentation error (GCC `-Werror`) and the vendored `stb_image_write` `sprintf` deprecation (Apple clang `-Werror`).
- MSVC warnings C4244/C4267 (intentional narrowing) silenced.
- Windows is documented as unsupported: the renderer needs an OpenGL function loader. The Windows CI job, which never built, was removed.
- CI is green on macOS 15 and Ubuntu 24.04.

## 1.0.0 - Initial engine release
Extracted from [Impossible Wireframe](https://github.com/djayuffe/wireframe-scroller) v4.34.

### Engine
- `de::Demo` interface and `de::run` run loop: window, GL 4.1 core context, clean shutdown, minimise handling, asset search, per-frame `FrameInfo` and `Input`.
- Generic command line (`--bpm --music --logos --window --fullscreen --quality --no-post --no-ticker --screenshot --frames --version --help`) that demos can extend; unknown options are now an error.
- `Config::fxAmount` to dial the signature post look from full to clean wires, `Config::shaderDir` for custom shader sets, `Frame::revision` to skip uploads of static meshes.
- Build split into `de_core` (no GL), `de_engine` (runtime) and demo targets, with a `de_add_demo()` CMake helper.

### Demos
- `hello_demo`: minimal cube-in-cube.
- `ripple_demo`: the README example, compiled.
- `impossible_wireframe` v5.0: the original demo reduced to a `de::Demo` (57 scenes, effect recipes, logo show). Behaviour unchanged.

### Docs
- README, ARCHITECTURE.md and docs/WRITING_A_DEMO.md.
