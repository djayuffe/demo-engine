# Hold Infinity — native music-directed optical demo

`make run` now launches this demo. It renders the supplied optical code directly through OpenGL 4.1, without the old wireframe recipes, travellers, logo cards or procedural backgrounds. `make run-wireframe` opens the previous show.

```sh
make
./build/hold_infinity
./build/hold_infinity --infinity-scene 3 --window 1280x720
./build/hold_infinity --infinity-scene 5 --still
./build/hold_infinity --infinity-scene 11 --grin-curvature 0.85
```

Drag to orbit. Left/right select scenes; Space returns to the automatic show; Q or Escape quits. T toggles the credit ticker. Standard engine `--music`, `--bpm`, `--window`, `--fullscreen`, `--frames`, `--fixed-step`, `--stats`, and `--screenshot` options work. The imported optical shaders own their bloom and grading; generic background/post options do not affect this custom renderer.

## Scenes

| Index | Optical scene |
|---|---|
| 0 | Exterior mirrored dodecahedron / Klein-model interior |
| 1 | Euclidean mirrored cube |
| 2 | Euclidean mirrored dodecahedron |
| 3 | Hyperbolic right-angle honeycomb / hyperboloid reflections |
| 4 | Uniform index, flat glass faces |
| 5 | Continuous hyperbolic GRIN, curved faces |
| 6 | Schwarzschild optical horizon |
| 7 | Seven printed refractive-index layers |
| 8 | Hero-wavelength spectral glass, water surround |
| 9 | Camera inside the optical instrument |
| 10 | Gradient-index ray lab, monochrome |
| 11 | Gradient-index ray lab, seven wavelengths |

## Music direction

The default playlist reuses Stars by cs127 and Digital Rendezvous by Drozerix with their existing credits. Music playback supplies show time, native tempo and bass/mid/treble energy. Without a playable track, the engine BPM clock still drives the show.

A scene lasts 32 beats (eight bars). The cut fades through black at the bar boundary. Six deterministic optical fronts launch on beat crossings; their radii derive from fractional beat age, so dropped frames do not accumulate timing drift. Bass drives chamber glow, front strength and glass bloom; treble drives ray intensity; mid energy drives chamber bloom. Camera and glyph flow derive from beat position rather than independent wall-clock timers. Live module tempo changes use the existing engine timeline continuity contract.

## Optical controls and quality

`--instrument-state MASK` selects any combination of the source instrument's seven controls and defaults to scene 5 when no manual scene was selected:

| Bit / value | Enabled state |
|---|---|
| 0 / 1 | Hyperbolic medium (otherwise uniform) |
| 1 / 2 | Optical horizon |
| 2 / 4 | Curved faces (otherwise flat) |
| 3 / 8 | White/spectral illumination |
| 4 / 16 | Inside camera |
| 5 / 32 | Water surround (otherwise air) |
| 6 / 64 | Printed index layers |

For example, `--instrument-state 13` uses hyperbolic, curved, spectral glass in air. `--grin-curvature F` sets the ray-lab gradient in [0,1]; otherwise it breathes with the clock and bass.

The GPU pixel budget defaults to 400,000, with a third of that for the costly spectral instrument. Use `--infinity-pixels N` to choose 10,000–4,000,000. Intermediate targets retain floating point precision and bloom mipmaps; the final image upscales with linear filtering. The ray lab retains the original 512-step, checkpoint-then-fill RK4 integration and 182-row float state texture, including all seven spectral wavelengths.

Moving glass uses two stochastic samples per frame. `--still` fixes automatic camera motion and accumulates up to 400 samples; dragging, resizing, or changing scenes clears accumulation to avoid ghosting. The full numerical ray integrator remains enabled. Resolution is capped rather than silently reducing bounce/step accuracy. Large budgets can be expensive, especially with horizon and printed layers enabled together.

## Validation and provenance

```sh
ctest --test-dir build --output-on-failure
python3 tools/smoke_infinity.py
```

Headless tests cover 20 vertices, 30 edges, unit face normals, hyperbolic face orthogonality, bar cuts, fade timing, clock sanitization, deterministic beat fronts and audio response. The GPU smoke tool runs all 12 scenes and saves screenshots/logs under `build/infinity-smoke/`. On the development Mac, all scenes and the combined 127 instrument state rendered without shader/FBO/OpenGL errors. Strict-warning Release builds and the existing geometry suite pass. A 200-frame accelerated clock run covers a full automatic cycle; a 205-frame still run exercises the 400-sample accumulation cap. Windows entry points are declared in the engine loader, but this pass was run on macOS only.

[Source attribution, original archive hash and port boundaries](SOURCE.md).
