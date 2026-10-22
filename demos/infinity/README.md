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
| 10 | Four-dimensional light loom, three monochrome sheets |
| 11 | Prismatic hyperspace threads, two seven-wavelength sheets |

## Music direction

The default playlist reuses Stars by cs127 and Digital Rendezvous by Drozerix with their existing credits. Playback starts with the second song, Digital Rendezvous; `--track 1` starts with Stars. Music playback supplies show time, native tempo and bass/mid/treble energy. Without a playable track, the engine BPM clock still drives the show.

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


## GPU optimization and profiling

The 1.1 renderer uses specialized optical kernels, cached immutable uniforms, active ray rows, exact-input GPU field reuse and converged bloom reuse. `--gpu-profile` reports non-blocking GPU timings; `--infinity-reference` retains the original workload for A/B checks. `--silent` uses the BPM clock without playing audio. [Measured performance, image comparisons and reproduction commands](GPU_PERFORMANCE.md).

## Adaptive GPU budget (1.2)

Normal playback now adjusts each scene's rendering resolution from completed GPU timer queries, targeting 16.67 ms of GPU work by default. It changes the pixel count, not the optical model, ray steps, bounce count or wavelength count. The ceiling remains 400,000 pixels (one third for glass); the lower bound is 25% of that scene's ceiling, or half its linear resolution. A scene's learned quality is retained separately from the other scenes.

Eight readings establish the timing estimate. A smoothed measurement, a dead band around the target, and a 30-frame cooldown prevent repeated resize chatter. Busy queries are never waited on. Without `--gpu-profile`, the feedback path stores only the latest reading and prints no timing statistics.

```sh
./build/hold_infinity --infinity-fps 60
./build/hold_infinity --infinity-fixed-budget
./build/hold_infinity --infinity-pixels 600000 --infinity-adaptive
```

`--infinity-fps N` accepts 24–144 and sets a GPU-time target, not a hard frame-rate cap or guarantee. Explicit `--infinity-pixels N` pins the budget, making existing captures and benchmarks reproducible; put `--infinity-adaptive` after it to use that value as an adaptive ceiling. Reference mode and `--still` always bypass adaptation. Stationary accumulation therefore still reaches 400 samples without being reset by quality changes.

On the development M1 Pro, a 160-frame printed-layer run reduced its pixel fraction to 0.8 and then 0.64, reporting a 17.44 ms mean GPU time across the run. This includes the higher-cost settling frames and is not an A/B speedup claim. Tests cover budget limits, recovery with headroom, stale/invalid query rejection and near-target stability. Actual GPU checks verify pinned budgets, stationary convergence, quiet ordinary playback and all 12 scenes.


## Contrast and glare

The default grade now uses darker midtones and a hue-preserving highlight shoulder. Bloom, star glare and music-driven light boosts are restrained, and glass exposure is lowered before display mapping. The ray lab keeps a narrow luminous core with a quieter halo. These presentation changes preserve the optical calculations and music timing.


## Dimensional light (1.3)

The final two scenes lift the integrated ray paths into four coordinates, rotate them through XW and YZ planes, then project through a three-dimensional camera. Mouse orbit changes the view of the layered curves. Three monochrome sheets form the light loom; two seven-wavelength sheets form the prismatic threads. Screen-space tangents keep the ribbons aligned after projection, and depth controls their width and brightness.

Traveling luminous packets follow musical beat position, with staggered phases across sheets and a restrained beat pulse. Narrow cores, attenuated distant sheets and a darker field preserve contrast. The underlying 512-step RK4 paths and spectral integration are unchanged: this is an artistic four-dimensional embedding, not an additional physical optical solver. Both optimized and reference modes share this presentation. The extra instanced sheets add rendering work; the adaptive pixel budget remains available.


## Black and HDR light (1.4)

The final two light scenes clear to pure black, without the former index-field backdrop. A small display black threshold and deeper midtones increase separation; the hue-preserving shoulder retains up to 0.98 display intensity while compressing HDR light energy. This is tone-mapped HDR on the existing display output, not an HDR-monitor output mode. Narrow cores are brighter, with wider but lower-energy halos.

Packets complete one circuit every four beats, with phase offsets between dimensional sheets and circular distance across the seam. A fast exponential envelope attacks on the exact beat and decays within the beat, keeping light accents aligned with the continuous music clock. Band energy adds restrained detail. Startup tempo follows the selected track, and `--track` still overrides the default.


## Projection and light corrections (1.5)

All native shader stages now use defined increasing or decreasing smooth ramps. The imported LED coverage, reflected underside light and distance-detail fades previously used reversed GLSL `smoothstep` bounds, whose results are undefined. Native preprocessing retains the intended decreasing fade as the complement of an increasing ramp; archived source files remain unchanged.

Dimensional rays use the inverse camera basis for world-to-view projection and real homogeneous clip W for perspective-correct path, depth and packet interpolation. Centered neighboring samples follow bends more smoothly, with one-sided endpoint tangents. Derivative-filtered ribbon edges fade to zero at their boundaries rather than ending at a visible Gaussian cutoff. Transparent light sheets continue to add together at crossings; they do not behave as opaque surfaces. Existing physical reflection, refraction and RK4 calculations remain intact.


## Organic light and rhythmic dynamics (1.6)

The final light sheets carry bounded traveling waves through their four-dimensional embedding. The deformation is anchored at both path endpoints; bass and beat accents flex the body without modifying the RK4 optical solver. Fine moving striations and an eight-beat breathing envelope give the filaments an organic character.

Beat attacks now depend only on beat phase, so sustained loud passages cannot hold them open. A four-beat accent pattern emphasizes the downbeat, while half-beat transients gate treble detail. All these envelopes derive directly from the existing tempo-aware music clock, with deterministic fallback timing. This adds rhythmic direction rather than audio onset detection or a new tempo estimator.

Three additive HDR ribbon layers separate the narrow hot core, coloured inner glow and faint broad halo. Only the central packet is slightly desaturated toward its peak colour intensity, preserving spectral edges. Chamber glow and glass bloom also use the bar accents. Output remains tone-mapped to the existing display framebuffer; extra ribbon layers add GPU drawing work and remain covered by adaptive resolution.


## Translational parallax (1.7)

All twelve scenes now translate the camera origin in addition to orbiting it. Moving the pointer shifts the eye along camera-local right/up axes, smoothly and independently of frame rate. A small 64-beat horizontal/depth dolly and vertical figure-eight make parallax visible during unattended playback. Near geometry shifts more than distant geometry through the actual perspective projection; no flat image displacement is applied.

Chamber and refractive scenes trace from the translated world-space origin. Dimensional lights subtract the same camera-local offset before perspective projection. Mouse drag still orbits. Leaving the window smoothly recenters the pointer contribution. `--still` disables pointer translation and automatic dolly to preserve accumulation. `--infinity-eye X Y` pins a normalized eye offset in [-1,1] and disables automatic dolly, useful for reproducible left/right comparisons; combine with `--still` for stationary glass accumulation.


## Detail, math and GPU cleanup (1.8)

Fine filament striations now fade with their screen-space derivative before they become subpixel, reducing distant shimmer. The XW/YZ sine/cosine rotation coefficients are computed once per sheet on the host and uploaded as uniforms instead of being evaluated repeatedly during ribbon projection. Light-only scenes skip allocating/resizing the unused mipmapped optical image target; the actual output and full ray integration remain unchanged.

Parallax helpers recover from non-finite pointer state and bound eye inputs. Benchmarks pin the eye at the center so pointer motion and automatic dolly cannot contaminate A/B images. Both headless suites and native GPU scene checks cover the final changes. An experimental per-point GPU projection pass was tested and removed because isolated alternating timings did not establish a consistent gain; no frame-rate improvement is claimed from that experiment.


## Matrix walkers, geometry and HDR (1.9)

Native chamber surface detail now includes deterministic ant-like packets along the existing geodesic lanes. Segmented bodies, per-lane direction, gait and beat accents are evaluated in intrinsic surface coordinates, so reflections and hyperbolic bending carry the detail with the geometry. They are procedural walkers, not an agent or pheromone simulation. Walkers fade with pixel footprint and skip their fine-detail calculations in distant rooms.

Chamber and glass highlights now use a shared hue-preserving exponential shoulder before the existing display grade, retaining coloured highlights instead of separately saturating RGB channels. Float optical targets and restrained bloom remain in use. Texture maximum mip levels are explicitly bounded to their allocated chains.

The camera and Lorentz mirror-plane construction now live in tested shared math helpers. Geometry checks verify every edge endpoint, edge uniqueness/equal length, three-edge vertex valence, convex face bounds and five vertices per face. Mirror checks validate spacelike unit normals, hyperboloid norm preservation and reflection involution. Sampled camera bases remain orthonormal, and plane rotations preserve length even for large clock values; their angles are reduced in double precision before trigonometry. These checks validate the transformations and mesh invariants, alongside native rendering checks; they are not a proof of every stochastic optical path.


## Interacting ant traffic (2.0)

Matrix walkers now come from a persistent sixteen-agent colony on an 8×8 wrapping junction grid. Every half beat they complete a step and reserve a new destination. Occupied nodes and reserved targets block conflicting moves, preventing duplicate arrivals and head-on swaps. Rotating planning priority avoids assigning permanent priority to the same ant. Route scoring favors less-visited cells, discourages immediate reversal and uses deterministic tie breaking; visit memory decays every 32 decisions. A blocked neighborhood produces a warm contact flash while the ant turns or waits.

The simulation runs on the CPU at half-beat boundaries; a small 64×64 float texture carries smoothly interpolated bodies and contact energy to the GPU. This replaces the independent lane formulas with actual shared traffic state. The texture is repeated in intrinsic surface coordinates with room-dependent offsets, so the colony pattern bends and reflects with the Matrix grid. Rooms share the colony pattern; they are not separate simulated worlds. Fine detail still fades at distance.

Tests cover 500 decision steps, unique positions/target reservations, no head-on swaps, valid grid movement, deterministic catch-up under different frame cadences, path changes and finite raster output. Invalid clocks are ignored. Backward seeks or jumps over 4,096 decisions reset the colony instead of performing unbounded catch-up.


## Bouncing chamber ball (2.1)

Scenes 0–3 now contain a lit sphere, including its mirror images. The cube and flat dodecahedron use three-dimensional continuous sphere/plane collision times with finite-radius wall clearance. The hyperbolic chamber and exterior sculpture use a geodesic sphere: its center follows the hyperboloid, contacts a mirror at geodesic distance 0.14, and reflects in the local tangent-space wall normal. These are three spatial dimensions in Euclidean or hyperbolic geometry; the ball is not a four-dimensional hypersphere.

Physics advances in fixed 1/120-beat steps, preserving elastic speed, with collision-time subdivision within a step. Rendering predicts the fractional remainder for smooth motion. The GPU tests sphere intersections on each ray segment before its next wall, so depth, occlusion and repeated mirror images follow the scene tracing rather than a screen overlay. Impact energy adds a brief warm glint through the existing HDR grade. Glass/instrument and final light scenes remain separate from the chamber ball.

Long-run tests cover 24,000 steps for all three chamber models, checking wall clearance, unit speed, hyperboloid position norm, tangent velocity and repeated impacts. Different update cadences produce the same state. Large or backward clock seeks reset at the new clock position with bounded work, and playback can resume normally.
