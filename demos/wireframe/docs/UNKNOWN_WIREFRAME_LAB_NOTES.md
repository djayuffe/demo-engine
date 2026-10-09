# Unknown Wireframe Laboratory v3 — audited

A deterministic C++20/OpenGL 3.3 core wireframe gallery with 24 animated procedural geometry families.

## Build the gallery (macOS/Homebrew)
```bash
brew install cmake glfw
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/unknown_wireframe
```
macOS uses a forward-compatible 3.3 core context. Linux requires OpenGL and GLFW development packages.

## Headless geometry audit (no OpenGL/GLFW required)
```bash
cmake -S . -B build-headless -DBUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-headless -j
ctest --test-dir build-headless --output-on-failure
./build-headless/geometry_audit
```
Optional Clang/GCC sanitizer build: add `-DENABLE_SANITIZERS=ON`.

## Controls
Left/Right selects a preset; Escape exits. Camera and geometry animate automatically.

## Presets
0 4D Hyperprism — rotated 4-cube perspective projection.
1 Triple Saddle Labyrinth — multi-frequency periodic saddle sheet.
2 Linked Fibre Vortex — Hopf-style linked fibre family.
3 High-Genus Organism — strongly modulated spherical manifold.
4 Quaternion-inspired Coral — irrational high-order radial harmonic construction.
5 Recursive Cell Surrogate — nested phase-offset rings.
6 Non-orientable Ribbon — odd-half-twist Möbius-family strip.
7 Cyclide Knot Architecture — twisted/wobbling toroidal coordinate network.
8 Nodal Harmonic Crystal — spherical nodal field.
9 Bifurcating Toroidal Lattice — nested Lissajous closed trajectories.
10 5D Polytope — rotated 5-cube projected to XYZ.
11 Fibre-Saddle Field — linked fibres deformed by a scalar saddle field.
12 Quasiperiodic Saddle — irrational-frequency nodal sheet.
13 Warped Hopf Field — strongly field-warped fibre family.
14 5D Temporal Ghost Slice — three time-separated 5D projections.
15 Algebraic Cathedral — cusp/arch singularity-inspired field.
16 Nested Genus Cages — nested twisted toroidal cages.
17 Inverted Fibre Bundle — spherical inversion of warped fibres.
18 Irrational Nodal Field — high-order irrational radial nodal structure.
19 Bifurcation Organism — topology-event-inspired loop trajectories.
20 Topology Surgery Ribbon — five-half-twist non-orientable strip.
21 Projection Caustic — curves amplified near a synthetic Jacobian singularity.
22 Geodesic Skeleton — curved closed trajectories of an implicit invisible manifold concept.
23 Negative Geometry Skeleton — branching medial-axis-inspired empty-space skeleton.

The names intentionally distinguish exact constructions (e.g. hypercube projection, Möbius-family ribbon) from artistic mathematical surrogates. This package does not claim an exact quaternion Julia isosurface, exact geodesic solver, exact medial-axis solver, or symbolic topology-surgery engine.

## Audit coverage
`geometry_test` exercises 36,864 generated meshes across wrapped negative/positive preset IDs, 64 animation times, and quality values below/inside/above the supported range. It verifies finite/bounded coordinates, index validity, nonzero physical edge length, deterministic output, quality clamping, ID wrapping, and malformed-mesh rejection. `geometry_audit` prints bounds, radius, duplicate-edge count and zero-length-edge count for every preset.

The v3 audit fixed a previously undetected spherical-pole defect: distinct pole indices occupied identical positions and produced zero-length wire edges. The sphere parameterization now avoids singular sampled poles.

## Renderer hardening
Shader compilation/program linking are checked; partial initialization is cleaned up; uniform locations are cached; VAO attribute state is configured once; dynamic data uses stream buffers; draw-count overflow is guarded; minimized windows are skipped; depth testing and additive alpha blending are explicitly configured; GL resources are released before the GLFW context is destroyed.

Standalone GLSL copies live in `shaders/`; equivalent fallback shader strings are embedded so the executable is independent of its working directory.
