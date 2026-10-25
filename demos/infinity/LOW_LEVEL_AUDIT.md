# Native optical renderer audit — updated 2026-10-25

Scope: the Hold Infinity host, render targets, shader transformations, ray integration, GPU timing, scene quality, camera math, ant traffic, ball physics and validation tools. The underlying numerical optics and generated reference headers remain preserved. This is an audit of the native optical path, not a claim that every function in the older wireframe engine has been reviewed or mathematically proved correct.

## Corrections and retained optimizations

| Area | Finding | Result |
|---|---|---|
| Ray integration | Raster fragments independently recomputed path prefixes | Two sequential geometry passes reuse prefixes for the 46-ray workload; preserve all 512 samples and RK4 equations |
| Workload selection | Sequential emission had inconsistent timing at 182 spectral rows | Keep raster integration for spectral scenes; retain explicit override and compile-failure fallback |
| Target allocation | Working storage was deleted before replacement succeeded; cache key ignored format/mips | Validate a replacement first, retain working resources on failure, check full storage key and device limits; disable copying GPU ownership |
| Ant clock | Very large seeks reduced the tick modulo a constant, then reset again on the next frame | Bounded integer clock recovers at the requested tick and resumes forward progress |
| Ball clock | Large catch-up could perform 120,000 physics steps in one update | Limit catch-up to 4,096 ticks; reset safely for larger seeks; bound fractional prediction |
| Camera | Chamber origin drift and scene boosts continued under still mode | Freeze both with the camera while allowing scene illumination to animate |
| Light phase | Large beat values lost fractional detail when uploaded as floats | Reduce the periodic ribbon clock modulo its common 16-beat period |
| Samplers | Fixed sampler units were resubmitted every frame | Assign once per optimized program; reference mode retains the comparison workload |
| GPU timers | Two query polls per frame and state retained across reinitialization | One explicit frame poll; clear query/statistics state at init and end active queries at shutdown |
| Regression tools | Ordinary smoke captures depended on live music/pointer state | Silent, centered-eye deterministic GPU checks plus an extended validation command |
| Retina resolution | A fixed 400,000-pixel ceiling left Metal-backed output needlessly soft | Use a tested 2,000,000-pixel Metal ceiling, up to 1.5× linear supersampling and the existing per-scene adaptive fallback; retain 400,000 on other drivers |
| Reconstruction | Cubic upscaling was stable but softened fine filaments | Add neighborhood-clamped local contrast recovery with no HDR overshoot or dark halos |
| Cosmic shader math | View, half-vector and gravity directions assumed nonzero inputs | Use finite fallback directions for degenerate normalization paths |
| Camera direction | Each renderer used an unrelated oscillator, producing inconsistent pacing and framing | Share a bounded, periodic cinematic phrase across world sheets, rays, Atlas and cosmic scenes; test continuity and invalid input containment |
| Black-hole rays | Curvature ignored adaptive step length; disk hits used step endpoints; the star grid illuminated whole cells | Integrate curvature per distance, interpolate disk crossings, correct world-space parallax and filter jittered subpixel stars; see [focused audit](BLACK_HOLE_AUDIT.md) |

The proposed per-point projection cache from an earlier pass remains discarded. Sequential RK4 is a different optimization: it reuses integration prefixes, not projected ribbon coordinates.

## Measurements

Apple M1 Pro, OpenGL 4.1 Metal driver 91.7, 120 frames per run, first ten frames excluded, three alternating-order repeats, 120,000 pixels, logical 640×360 window, fixed 0.0166667-second clock, silent 132 BPM, centered fixed eye and vsync off. Both comparison modes retain all other existing optimizations and solve the changing field 120 times.

| Dynamic workload | Raster GPU mean | Sequential GPU mean | Default |
|---|---:|---:|---|
| 46 monochrome rows, final trial | 2.208 ms | 1.720 ms | Sequential |
| 182 spectral rows, earlier trial | 4.340 ms | 4.688 ms | Raster |

The monochrome result is a 22.1% reduction in measured GPU-query time on this Mac. These numbers include the entire timed drawing path, not only integration, and are not whole-application FPS. Spectral trials varied in both directions, so no spectral improvement is claimed. [Raw data and conditions](GPU_AUDIT_BENCHMARK.json).

At the 2,000,000-pixel Metal ceiling, a 90-frame deterministic run on the same M1 Pro measured scenes 24 and 26–29 at 11.414–16.119 ms mean and 12.501–18.081 ms p95. The black-hole ray field was the slowest and automatically reduced its learned pixel fraction to 0.8 after crossing the 16.67 ms target; the other dense scenes retained supersampling. These measurements include the nine-tap resolve, six procedural depth layers, bloom and full timed draw path and do not claim the same margin on other hardware.

## Validation evidence

- Strict-warning full CMake build and both headless test suites pass.
- AddressSanitizer and UndefinedBehaviorSanitizer run of the geometry/simulation/math suite passes.
- All 30 native GPU scenes render without shader, framebuffer or draw errors and produce finite RGBA16F output.
- Full float readback compares sequential versus original raster integration at curvatures 0, 0.65 and 1 in landscape and portrait windows, for monochrome and all seven wavelengths. All 700,416 tested points match exactly on this GPU and have finite positions/unit tangents.
- GPU target checks separately change format and mip configuration, verify unchanged storage is reused, and confirm that an invalid resize retains a complete working framebuffer.
- Combined optical states 31, 95 and 127 render successfully.
- A 205-frame still run reaches 400 samples and performs exactly 200 mip builds.
- An accelerated-clock run covers all 30 scenes and returns to scene zero.
- Seven invalid/boundary command lines are rejected before rendering.
- Existing long-run ball containment, elastic speed/time reversal, local optical angles, ant reservations and interpolated collision-avoidance checks remain passing.

The extended GPU checks are reproducible with `python3 tools/validate_infinity.py`. The 2026-10-25 pass additionally covered every ray-state curvature/orientation combination, optical states 31/95/127, target storage recovery, stationary convergence and the complete automatic cycle. Full ray-field readback is only performed when explicitly requested. The comparison tolerance allows small cross-driver roundoff even though these captured fields matched exactly.

## Limits

This pass ran on macOS/M1 Pro. Windows OpenGL entry points remain supported by the loader but were not executed here. Driver geometry-shader throughput varies; the raster override remains available. The automatic policy uses the tested 46/182-row workloads, not a universal GPU benchmark. No bounce, wavelength, RK4 step or accumulation limits were reduced.
