# Changelog

## Unreleased
- The beat clock locks to the playing module's own tempo (tempo x 6 / speed) unless `--bpm` is given; `RunInfo::musicPath` tells the demo which track is playing.
- Wireframe demo: new default soundtrack **"Stars (4ch. version)" by cs127** (CC BY 4.0, The Mod Archive 201917), credited in its README, NOTICE, music README and in the ticker while it plays. `make run-silicon` plays Silicon Dancer.
- `AudioPlayer` plays `.wav` files through SDL (decoded once, converted to float stereo, looped), with the same band analysis and music clock as modules. `--music file.wav` used to be rejected by libopenmpt.
- The music clock is now total played time, not the position in the track, so the show keeps going when a track loops instead of restarting its scene sequence and logo show (a 30 s loop never got past scene 8).

## 1.3.0 - Golden glimmer and golden orbits
- Engine: `Frame::wireStyle` and `Renderer::setWireStyle`. Style 1, golden glimmer, draws wires additively without depth test (point and orbit clouds accumulate light), twinkles each dash in gold with a sweeping shimmer band, and makes the post pass add gold horizontal-plus-vertical glints, tint the light rays gold, quiet the art background and switch off warps, colour grading and the rainbow tint.
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
