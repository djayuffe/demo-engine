# GPU optimization pass — 2026-10-21

The optical integrators, mirror tracing, spectral sampling and instanced ray ribbons already execute on the GPU. This pass reduces unnecessary GPU work and CPU/driver submission overhead without lowering ray-step, bounce, wavelength or sample limits.

## Changes

- Compile state-specialized glass kernels: medium, horizon, face shape, illumination, inside/outside, surrounding index and printed layers become shader constants. The compiler can eliminate inactive branches in the integration loop. The six show presets are prepared at startup; a supplied seven-bit override prepares just its kernel. The unused generic glass kernel is only compiled in reference mode.
- Upload immutable face normals, edge endpoints and hyperbolic planes once. Cache uniform locations, including inactive uniforms, per program. Do not submit optical switches that the specialized program has already compiled in.
- Keep the original two-stage RK4 integration on the GPU, but dispatch only the active ray rows. Cache its texture result by exact aspect, curvature, ray count and wavelength count. A fixed-curvature lab solves once while its moving light fronts and music-reactive intensity continue every frame.
- Once a stationary instrument reaches 400 samples, reuse its bloom mip chain until the accumulation changes. A 205-frame test issues 200 mip builds, not 205. Moving/resized/new scenes still reset accumulation.
- Add an eight-slot non-blocking GPU timer ring. Only read available results; skip busy slots, never force a GPU finish. Ignore the first ten warmup frames, cap retained measurements at 8,192 per scene, and skip final unavailable queries at shutdown.
- Add `--silent` for deterministic BPM-clock captures and comparisons without initializing audio playback.

The measurements below precede the 1.3 dimensional light presentation. They do not measure the additional ray sheets or establish its performance. Reference mode now shares the dimensional presentation for current workload comparisons.

## Measured results

Apple M1 Pro, OpenGL 4.1 Metal driver 91.7. Each reference/optimized pair uses 90 frames, two alternating-order repeats, 120,000-pixel budget (one third for glass), logical 640×360 window / 1280×720 framebuffer, fixed 0.0166667-second clock, silent 132 BPM and vsync off. Values are the mean of both runs' GPU-query averages; CPU submission, presentation/vsync, and startup compilation are outside the timer.

| Scene | Reference GPU ms | Optimized GPU ms | Reduction |
|---|---:|---:|---:|
| Mirrored exterior | 9.938 | 9.869 | 0.7% |
| Continuous GRIN glass | 5.561 | 4.397 | 20.9% |
| Seven printed layers | 42.493 | 16.591 | 61.0% |
| Monochrome ray lab, fixed curvature | 5.476 | 3.735 | 31.8% |
| Spectral ray lab, changing curvature | 6.894 | 6.827 | 1.0% |

Chamber and changing spectral-ray GPU times are effectively unchanged within measurement variation. These are measurements on one Mac at one budget, not universal frame-rate guarantees. Clock/thermal/load differences affect results.

Driver uniform lookups drop from 1,800 to 45 for 90 glass frames, 2,430 to 54 for chambers, and 2,970 to 63 for the ray lab. The fixed-curvature ray lab reduces 90 repeated solves to one. [Raw runs and image comparisons](GPU_BENCHMARK.json).

## Image and behavior checks

Chamber and ray-lab A/B captures are pixel-identical. Continuous glass scores 54.03 dB PSNR / 0.999412 SSIM. Printed layers score 39.19 dB / 0.985584: constant folding changes floating-point rounding at grazing Fresnel crossings, which can change individual stochastic paths. The geometry and optical model are unchanged; the comparison is intentionally not advertised as bit-identical for these paths. The benchmark checks both signal level and structural similarity, allowing the measured stochastic sensitivity while rejecting larger image changes.

All 12 optimized scenes rendered on the actual GPU. State 127, combining all instrument switches, also passed. Headless tests cover specialization of all 128 masks and cache invalidation of every ray-solve input, alongside the original geometry/sync suites. The strict-warning build passes. Windows timer entry points are included, but this pass was executed on macOS only.

## Reproduce

```sh
make
python3 tools/benchmark_infinity.py
./build/hold_infinity --gpu-profile
./build/hold_infinity --gpu-profile --infinity-reference
./build/hold_infinity --silent --fixed-step 0.0166667 --frames 120
```

`build/infinity-benchmark/` receives captures, full driver logs and results JSON. FFmpeg, when available, provides PSNR/SSIM comparisons. Reference mode retains the original shader switches, uniform/geometry uploads, integration dispatches and mip rebuilds for A/B checks.

The implementation stays within the engine's OpenGL 4.1 contract; it does not pretend to enable OpenGL 4.3 compute shaders on macOS. GPU integration remains parallel fragment/FBO work. Primary API references: [Khronos timer query specification](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_timer_query.txt) and [OpenGL 4.3 additions](https://www.khronos.org/news/press/khronos-releases-opengl-4.3-specification-with-major-enhancements).
