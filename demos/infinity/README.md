# Hold Infinity — native music-directed optical demo

`make run` now launches this demo. It renders the supplied optical code, four original geometry studies, ten Atlas scenes and a native GPU parametric point field directly through OpenGL 4.1, without the old wireframe recipes, travellers or logo cards. `make run-wireframe` opens the previous show.

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
| 12 | Harmonic chrysanthemum, sevenfold radial surface |
| 13 | Heptagonal Poincaré gates, regular {7,3} disk tiling |
| 14 | Saddled world sheets, five translucent intersecting patches |
| 15 | Branching crystal garden, seven-level tetrahedral folds |
| 16 | True 4D tesseract section |
| 17 | Six-plane perspective hypercube |
| 18 | Stable hierarchical radial bloom |
| 19 | Recursive coordinate folds |
| 20 | Analytic H³ sphere constellation |
| 21 | Euclidean/H³ iris portal |
| 22 | Euclidean ornament |
| 23 | Causal event volume and procedural glyphs |
| 24 | Filigree face emergence |
| 25 | Three independent H³ viewpoints |
| 26 | Spectral parametric point field, moon companion and bouncing marker |

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

Headless tests cover 20 vertices, 30 edges, unit face normals, hyperbolic face orthogonality, bar cuts, fade timing, clock sanitization, deterministic beat fronts and audio response. The GPU smoke tool runs all 30 scenes and saves screenshots/logs under `build/infinity-smoke/`. On the development Mac, all scenes and the combined 127 instrument state rendered without shader/FBO/OpenGL errors. Strict-warning Release builds and the existing geometry suite pass. A dynamically sized accelerated clock run covers the full automatic cycle; a 205-frame still run exercises the 400-sample accumulation cap. Windows entry points are declared in the engine loader, but this pass was run on macOS only.

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


## Physics and optical frames (2.2)

Sphere shading now uses a rough GGX highlight, Schlick Fresnel response and a restrained diffuse/ambient body under the existing HDR shoulder. Hyperbolic hits transport the ray to the surface and transform the tangent-space normal, viewing ray and light into the hit point's orthonormal rest frame. This replaces the earlier XYZ-only approximation, preserving local angles in curved space.

Euclidean sphere roots use a conjugate quadratic form. Hyperbolic roots solve for the exponential of geodesic distance with a conjugate form and one logarithm, avoiding the previous closest-point inverse-hyperbolic sequence. Both use the uploaded physical sphere radius. The hyperbolic camera now translates its origin and orthonormal tangent frame onto the hyperboloid before the scene boost, so pointer parallax changes the actual ray origin in the curved chamber too.

Physics caches normalized plane sets. Impact emission decays over the time after contact rather than the entire enclosing step; invalid/nonpositive step lengths are ignored. Added checks cover simultaneous cube-corner contacts, exact post-impact decay, elastic time reversal in flat/curved chambers and preservation of local lighting angles, alongside the long-run invariant suite. Surface shading is a local lighting model; it does not add full global illumination or ball-to-ball dynamics.


## Low-level audit and sequential RK4 (2.3)

The monochrome ray lab now emits checkpoints and path samples sequentially in two OpenGL 4.1 geometry-shader passes. It derives the index function, gradient, derivative and RK4 update directly from the preserved imported shader. Each ray still has 512 samples with the same step size and normalization. Sharing prefixes reduces RK4 updates per active ray from 11,776 to 976. This is a work-count reduction, not a 12× frame-rate claim.

The 46-ray monochrome workload uses sequential emission by default. The 182-row spectral workload retains raster integration because longer alternating trials did not establish a consistent sequential benefit. `--infinity-sequential-integrator` and `--infinity-raster-integrator` force either path; reference mode keeps the original raster workload. If sequential shader compilation fails, the renderer reports the error and falls back to raster RK4. Immutable sampler bindings are assigned once in optimized mode.

Target replacement now validates device size limits, format, mip configuration, allocation errors and framebuffer completeness before releasing working storage. Timer polling is no longer repeated within a frame. Large seeks recover the colony at the requested tick instead of repeatedly resetting, physics catch-up has a smaller bounded budget, and interpolation handles extreme clocks without NaNs. Still-camera origins/boosts remain fixed; periodic light phases are reduced before conversion to float.

```sh
python3 tools/validate_infinity.py
python3 tools/benchmark_infinity.py --integration-only --scenes 10,11 --frames 120 --repeats 3
```

`--validate-ray-state` reads every active float sample and compares it against the original raster solver, also checking finite values and unit tangents. `--validate-targets` exercises format changes, mip changes, storage reuse and invalid-resize recovery. These are explicit diagnostics with synchronous readback; ordinary playback has no ray-field readback. [Audit findings and validation](LOW_LEVEL_AUDIT.md), [GPU measurements](GPU_AUDIT_BENCHMARK.json).


## Geometry studies from the QRI essay (3.0)

Four new scenes draw on the geometric imagery in Andrés Gómez-Emilsson’s 2016 essay, *The Hyperbolic Geometry of DMT Experiences: Symmetries, Sheets, and Saddled Scenes*. They are original procedural graphics inspired by the supplied text, not a scientific model of altered perception. No article images, audio, dosage material or third-party visual assets are bundled.

- **12 — Harmonic chrysanthemum:** a shaded sevenfold radial surface with interleaved harmonic ridges, travelling luminous contours and beat-driven petal depth.
- **13 — Heptagonal Poincaré gates:** the regular {7,3} tiling, generated by inversions in side circles orthogonal to the disk boundary. Möbius navigation moves through the disk without changing its hyperbolic geometry; coloured motifs reveal cells crowding toward the rim. This is a conformal 2D model displayed on a plane in the 3D view.
- **14 — Saddled world sheets:** five finite hyperbolic-paraboloid patches, independently rotated and breathing through the music phrase. Analytic quadratics find both intersections per patch and sort all valid hits for translucent composition. Their Gaussian curvature is negative but not constant, so they are not exact constant-curvature H2 embeddings.
- **15 — Branching crystal garden:** a seven-level tetrahedral fold fractal with nested openings, illuminated facets and travelling contours. It is a bounded distance-estimator rendering, not an exact knot-complement manifold.

All four share translated camera parallax, mouse orbit, restrained HDR grading and beat/bar envelopes. Each has a specialized GPU shader in normal playback, while reference mode retains a generic version for image comparison. They use the full adaptive pixel ceiling; the costly glass scenes retain their existing reduced budget. Scene timing, GPU statistics and quality state now cover all sixteen slots. The smoke/benchmark tools discover the supported range from the binary.

```sh
./build/hold_infinity --infinity-scene 12
./build/hold_infinity --infinity-scene 13 --infinity-eye 0.5 -0.2
./build/hold_infinity --infinity-scene 14 --validate-output
./build/hold_infinity --infinity-scene 15
```

Headless checks verify disk-circle orthogonality, three-polygon vertex angles, disk containment/reflection involution and scene specialization/director coverage. Native tests compare each new specialized shader with its generic reference. `--validate-output` optionally reads the HDR target once per scene and rejects non-finite values; ordinary playback does not read the HDR target back. [Inspiration and attribution](SOURCE.md).

## DMT Atlas integration

Scenes 16–23 and 25 port the supplied v0.3 Atlas archive and extend its concrete rendering roadmap. The figurative face-emergence scene was removed from live playback. See the [feature matrix, controls, geometry conventions, validation and research boundaries](../atlas/README.md). P pauses Atlas animation; `--atlas-reduced-motion`, `--atlas-exposure` and `--atlas-flash-gain` control the Atlas effects. Original source bytes and hashes are preserved separately from the production derivatives.

## Möbius universe and quantum lattice

Scene 24 replaces the retired face effect with the supplied four-dimensional Möbius
universe. A 256 × 64 procedural grid emits 98,304 triangle vertices for the double-loop
strip, including its fourth-coordinate height, analytic normals, animated micro-displacement
and metallic Fresnel flow. A second procedural draw adds 40,960 singularity, horizon, star,
debris and surface-flow particles. The full scene uses GPU temporal persistence and bloom;
an M1 Pro measured 4.639 ms average and 6.446 ms p95 at the 400,000-pixel budget.

Scene 27 ports the supplied advanced quantum-fractal point equation as a volumetric field,
then adds paired twist braids, eight alternating orbital nodes and a temperature-varied star
shell. Metal-backed OpenGL uses four delayed instances for 409,600 vertex invocations each
frame. Bass changes volume, mid energy winds the braids, treble activates the star shell and
beat attacks travel through the node pairs. An M1 Pro measured 4.913 ms average and 7.039 ms
p95 at the same budget. Both scenes generate all geometry from `gl_VertexID`, with zero
streaming geometry uploads. [Port boundaries and source hashes](../cosmic/SOURCE.md).

Scene 28 is a bounded gravitational ray field derived from the supplied advanced fractal
black-hole study. Ninety-two fragment steps bend rays around a dark event horizon and
accumulate a tilted, turbulent accretion disk, Doppler-colored photon rings, polar Hawking
jets and a lensed temperature-varied starfield. Bass energizes the disk, mid energy changes
its spiral turbulence, treble wakes distant stars and beat attacks strike the photon ring.
An M1 Pro measured 5.663 ms average and 7.092 ms p95 at 400,000 pixels.

Scene 29 moves the supplied hyperfractal's four-component probabilistic recurrence entirely
onto the GPU. Each of 65,536 seeds executes 28 deterministic sine/product iterations per
phase layer; Metal runs four layers plus a 16,384-point projected tesseract and star shell,
for 282,624 vertex invocations with zero streaming geometry uploads. The fourth coordinate
controls perspective and hue while music changes branch balance, rotation and emission. An
M1 Pro measured 5.155 ms average and 7.271 ms p95 at the same pixel budget.

## Rendering refinement (4.1)

All 30 scenes share a half-resolution RGBA16F highlight pyramid: soft-knee extraction, weighted downsampling, GPU mip generation and explicit-LOD composition. Bloom adds a restrained halo without raising black input. `--no-optical-bloom` disables this pass for A/B measurements. GPU timing includes the new passes, so the existing adaptive pixel governor accounts for their cost.

Audio bands use frame-independent exponential attack/release smoothing. Beat attacks retain their phase-locked clock; smooth envelopes supply the sustained motion. Atlas motion now handles backward seeks, pause/resume and a consistent 12% reduced-motion rate. Sphere radii and 4D section extents breathe geometrically, with conservative hierarchy padding for the maximum radius.

The H³ renderer uses finite point-light geodesics for occlusion and local-rest-frame GGX/Fresnel shading. Radial, folded and ornament scenes intersect bounded height fields, using bracketed roots at ordinary angles and conservative Lipschitz steps at grazing angles. Material footprints follow surface ray differentials. Face shading avoids derivatives inside divergent ray marching, rejects rays outside its enclosing sphere and adds local occlusion. Saddle sheets use angle-dependent Fresnel transmission; event packets arrive when their child events activate.

`--infinity-orbit YAW PITCH` sets a reproducible initial view in radians (yaw ±π, pitch ±1.2). This is useful for inspecting actual relief and comparing landscape, portrait and grazing-angle renders. All changes remain within OpenGL 4.1; no compute-shader support is required.

## Parametric point field (4.2)

Scene 26 ports the supplied browser experiment's four phase-shifted parametric lobes,
spectral point colour, moon-like companion and falling marker, then adds six orbital light
guides, a three-dimensional temperature-varied star shell and a spiral dust volume. The
103,044-point base composition is synthesized from `gl_VertexID`; no position buffer is
rebuilt or uploaded per frame. On Metal-backed OpenGL, four instanced phase echoes produce
327,696 vertex invocations without a geometry shader; other drivers use two echoes by
default.

Analytic parametric derivatives give the central field a stable curved normal, including a
bounded fallback at its singular poles. Per-particle micro-sphere shading separates crystal,
translucent membrane, cratered stone, hot ember, brushed orbital metal, stellar and dust
materials. Depth fog, Fresnel edges, caustics, anisotropic highlights and star flares provide
spatial cues without textures. Two slow narrative envelopes move the palette between cool
wonder and warm tension while the faster audio envelopes retain precise musical response:
bass breathes the shell and dust, mid energy twists the echoes and orbit guides, treble wakes
the stars, and beat attacks send a brief warm highlight through the field.

A GPU ping-pong pass rotates, expands and decays prior HDR light for curved,
resolution-independent chromatic persistence. It bounds retained radiance before the shared
highlight pyramid, keeping long trails finite while preserving bright detail.

Mouse orbit, translated parallax, adaptive resolution, fixed-step capture and finite-HDR
validation work through the existing native host. A 360-frame, 400,000-pixel M1 Pro run at
an oblique orbit measured 5.856 ms average and 7.410 ms p95 GPU time for the denser field,
material shading, temporal pass, bloom and final composition. [Port boundary and source
hashes](../aviation/SOURCE.md).
## Shared beat and music controls

All scenes use the engine’s shared beat pulse and exposure-flash envelopes. Use `--pulse-amount`, `--flash-amount`, `--beat-division`, `--beat-decay` and `--beat-offset`; E/F toggle pulse/flash. Music presets, folder playlists, shuffle, pause and volume are described in [Music and beats](../../docs/MUSIC_AND_BEATS.md). The default playlist now includes Silicon Dancer and Wireframe Pulse as well as the original two tracks. Atlas reduced-motion and pause modes suppress the new composition flash.

The CC0 variety pack adds melodic EDM, drum and bass, dark jazz and ambient synth. Use `--music-preset new`, or select `edm`, `dnb`, `jazz` or `ambient`. The default hybrid beat source combines the tempo grid with real musical attacks; `--beat-source audio` follows attacks alone. See [music provenance](../wireframe/assets/music/README.md) for original sources and checksum verification.
