# IMPOSSIBLE WIREFRAME — Complete OpenGL Design

**Version:** 1.0  
**Target:** OpenGL 4.1 Core / C++20 / GLFW  
**Primary platform:** macOS 11+ arm64/x86_64; portable to Linux/Windows  
**Aesthetic:** 1990s wireframe demoscene engineering applied to mathematically unusual geometry.

## 1. Goal

Render mathematically coherent objects that are substantially less familiar than cubes, toruses and conventional platonic solids. The production progresses from exact 4D regular polychora into slices, minimal surfaces, fibrations, hyperbolic structures and deterministic discovered implicit surfaces. Geometry must remain reproducible from source; no opaque model assets are required.

The engine has two truth classes:

1. **Canonical** — vertices/edges follow a known mathematical construction exactly up to floating-point representation.
2. **Visual approximation** — sampled implicit surfaces, reconstructed slice edges, fractals and artistic fields. These are explicitly tagged and never described as exact topology.

## 2. Scene order

| Time | Scene | Source | Technique |
|---:|---|---|---|
| 0–12 s | 600-cell reveal | canonical | 4D rotation + perspective projection |
| 12–24 s | 600-cell surgery | canonical vertices/intersections; reconstructed display edges | moving 4D hyperplane |
| 24–36 s | Gyroid chamber | implicit approximation | sampled TPMS wire lattice |
| 36–48 s | 24-cell | canonical | dual-plane 4D rotation |
| 48–60 s | Hopf fibres | procedural mathematical | stereographic projection |
| 60–72 s | Boy surface | parametric | immersion grid |
| 72–84 s | Hyperbolic chamber | procedural | Poincaré/Klein projection |
| 84–96 s | Quaternion Julia | implicit/fractal | deterministic field extraction |
| 96+ | Unknown object search | procedural | seeded implicit equation bank |

The included executable implements the first three as the minimal running core. The following sections specify the complete extension path.

## 3. Repository

```text
impossible_wireframe/
├── CMakeLists.txt
├── README.md
├── design.md
├── include/
│   ├── Geometry.hpp
│   └── Renderer.hpp
├── src/
│   ├── main.cpp
│   ├── Geometry.cpp
│   └── Renderer.cpp
├── shaders/
│   ├── wire.vert
│   └── wire.frag
├── data/
│   └── object_catalog.csv
├── docs/
└── tests/
```

Recommended production expansion:

```text
src/{AudioSync,Timeline,Camera,Polytope4D,ImplicitSurface,Hopf,
     Hyperbolic,Julia4D,PostFX,Capture}.cpp
shaders/{wire,point,trail,bloom_composite,feedback,fade}.vert/.frag
```

## 4. Coordinate conventions

Right-handed world coordinates. Camera looks toward `-Z`. OpenGL NDC is used unchanged. 4D coordinates are `(x,y,z,w)`. CPU geometry uses `float` for display; construction/tests may use `double`.

### 4.1 4D plane rotations

For axes `a,b`:

```text
x'a = cos(theta) xa - sin(theta) xb
x'b = sin(theta) xa + cos(theta) xb
```

Use at least two simultaneous planes. `XY + ZW` is the baseline. Better choreography uses `XW + YZ` and smoothly cross-fades angular velocity.

### 4.2 4D perspective

For camera coordinate `w_c`:

```text
k = wCamera / (wCamera - w)
p3 = k * (x,y,z)
```

Never allow `abs(wCamera-w) < epsilon`. Clip the vertex/edge or cap the projection deliberately; do not permit NaNs.

## 5. Canonical object data

### 5.1 Tesseract / 8-cell

Vertices are all sign combinations:

```text
(±1, ±1, ±1, ±1)
```

16 vertices, 32 edges. Two vertices share an edge iff they differ in exactly one sign.

### 5.2 16-cell

Vertices:

```text
(±1,0,0,0)
(0,±1,0,0)
(0,0,±1,0)
(0,0,0,±1)
```

8 vertices. Infer edges using the minimum non-zero pair distance.

### 5.3 24-cell

All permutations of:

```text
(±1, ±1, 0, 0)
```

There are `C(4,2)*4 = 24` vertices. Infer edges from the minimum non-zero pair distance. This construction is particularly useful because the 24-cell has no direct 3D regular-polyhedron analogue.

### 5.4 600-cell

Use the standard 120-vertex coordinate construction, represented in the project by:

```text
8 vertices:  permutations of (±2,0,0,0)
16 vertices: (±1,±1,±1,±1)
96 vertices: even permutations of (0, ±1, ±phi, ±1/phi)
phi = (1+sqrt(5))/2
```

The implementation generates the set, removes floating duplicates, then connects pairs at the minimum non-zero distance. Expected invariant: **120 unique vertices**. This avoids a brittle 120-entry hand-coded array while retaining the canonical coordinate construction.

### 5.5 Production lookup record

```cpp
enum class TruthClass { Canonical, Approximation };
enum class Primitive { Edges, Fibres, IsoLines, Points };
struct ObjectDesc {
    const char* id;
    TruthClass truth;
    Primitive primitive;
    uint32_t expectedVertices;
    float defaultScale;
    float cameraW;
};
```

Recommended table:

```text
600cell       Canonical      Edges      120  0.75  3.4
24cell        Canonical      Edges       24  1.00  3.0
tesseract     Canonical      Edges       16  1.00  3.2
gyroid        Approximation  IsoLines     0  1.00  n/a
hopf          Approximation* Fibres       0  1.00  n/a
boy           Approximation  IsoLines     0  1.00  n/a
qjulia        Approximation  IsoLines     0  0.85  n/a
```

`*` The mathematical mapping can be exact while its finite sampling is necessarily approximate.

## 6. 4D slicing

Plane:

```text
n dot p = h
```

Normalize `n`. For every canonical edge `(A,B)` compute:

```text
da = n·A - h
db = n·B - h
```

The edge crosses the plane when the signs differ or an endpoint lies on it. Intersection parameter:

```text
t = da / (da-db)
P = A + t(B-A)
```

### Correct production topology

The sample implementation connects intersection points by nearest-neighbour distance for an immediately visible demo. **That is a visualization heuristic, not canonical section topology.**

The final implementation should store the source polychoron's 2-faces. Intersect every face polygon with the hyperplane; two intersection points generated by the same face form an exact section edge. Deduplicate intersections by `(source edge ID, quantized t)` or a stable source-edge map. This is the required method for mathematically faithful slices.

## 7. Gyroid

Implicit field:

```text
G(x,y,z) = sin(x)cos(y) + sin(y)cos(z) + sin(z)cos(x)
G(x,y,z) = c
```

The included core uses a deterministic near-isosurface lattice for portability. Production should replace this with Marching Cubes or dual contouring.

### Marching Cubes lookup data

Use the conventional 256-case `edgeTable[256]` and `triTable[256][16]`, but derive wireframe edges from emitted triangles and deduplicate unordered pairs `(min(i,j),max(i,j))`. Keep these tables in `data/marching_cubes.hpp`. Tests must verify every triangle index is in `[0,11]` and terminated correctly.

For an artistic wireframe, do not necessarily render every triangle edge. Three useful modes:

```text
FULL       all unique triangle edges
FEATURE    edges with dihedral angle > threshold
CONTOUR    silhouette/front-back transition edges
```

`FEATURE` produces the cleanest unfamiliar-object look.

## 8. Hopf fibration scene

Represent `S^3` using complex coordinates `(z1,z2)` with `|z1|²+|z2|²=1`. A useful finite family is generated by choosing base points on `S²`, lifting each to a circle in `S³`, then stereographically projecting to `R³`.

Finite sampling defaults:

```text
base fibres       96
samples/fibre    128
vertices       12288
segments       12288
```

Animate the global phase without destroying linkage. Music transients may perturb line brightness and camera position, but should not randomly displace the geometry.

## 9. Boy surface

Use a documented Boy-surface immersion implementation as a separate generator. Sample `(u,v)` on a rectangular parameter grid, generate positions, reject singular/non-finite samples, and connect parameter-neighbours. Because several parameterizations exist, the generator source must state which equation it implements rather than claiming an unspecified “canonical mesh.”

## 10. Hyperbolic scene

Preferred model: Poincaré ball for the recognizable infinite-boundary effect. Geometry approaching radius 1 becomes visually compressed.

For a point `p` inside the ball:

```text
|p| < 1
```

Never place vertices exactly on the boundary. Clamp to `1-epsilon`. Generate cells by reflection from a fundamental domain rather than random Euclidean duplication. Maintain a hash of quantized transformed vertices/cells to terminate expansion.

## 11. Quaternion Julia

Quaternion iteration:

```text
q(n+1) = q(n)^2 + c
```

For display, fix or scan one dimension and evaluate a 3D field. Escape-time alone gives a point cloud; a higher-quality implementation estimates a scalar distance field and extracts an isosurface. The demo should cache geometry and morph `c` only at controlled keyframes; remeshing every frame is wasteful.

Deterministic preset bank:

```text
QJ00 c=(-0.20, 0.72, 0.00, 0.00)
QJ01 c=(-0.45, 0.40, 0.15, 0.00)
QJ02 c=(-0.10, 0.65, 0.22,-0.08)
```

These are **art presets**, not claims of uniquely significant Julia sets.

## 12. Unknown-object generator

Seeded equation grammar:

```text
F = a0
  + a1*sin(k1*x)*cos(k2*y)
  + a2*sin(k3*y)*cos(k4*z)
  + a3*sin(k5*z)*cos(k6*x)
  + a4*x*y*z
  + a5*(x*x+y*y+z*z)
  + a6*(x*x*y*y + y*y*z*z + z*z*x*x)
```

Seed controls all coefficients and integer frequencies. Search offline, not during the show. Reject fields that produce zero geometry, exceed vertex budget, contain NaN/Inf, or have a bounding-box aspect ratio beyond a configured threshold.

Store winning forms as:

```cpp
struct ImplicitPreset {
 uint64_t seed;
 float a[7];
 int k[6];
 float iso;
};
```

This gives genuinely production-specific objects while retaining complete reproducibility.

## 13. GPU data model

Static/cached geometry:

```cpp
struct GPUWireMesh {
 GLuint vao;
 GLuint positionVBO;
 GLuint edgeEBO;
 GLsizei vertexCount;
 GLsizei indexCount;
};
```

Dynamic slice geometry uses `GL_DYNAMIC_DRAW` initially. If profiling shows stalls, use a ring of 3 VBO/EBO sets with fences. OpenGL 4.1 does not guarantee persistent mapped buffers, so do not make them a baseline requirement.

Index type: `uint32_t / GL_UNSIGNED_INT`. Convert to 16-bit only as an optional optimization when vertex count is proven below 65536.

## 14. Shader contract

### `wire.vert`

```glsl
#version 410 core
layout(location=0) in vec3 aPosition;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vWorld;
void main() {
    vec4 w = uModel * vec4(aPosition,1.0);
    vWorld = w.xyz;
    gl_Position = uProjection * uView * w;
}
```

### `wire.frag`

```glsl
#version 410 core
in vec3 vWorld;
out vec4 outColor;
uniform vec3 uColor;
uniform float uTime;
void main() {
    float pulse = 0.75 + 0.25*sin(uTime*2.0 + length(vWorld)*4.0);
    outColor = vec4(uColor*pulse,1.0);
}
```

### Production line quality

Core OpenGL line width is implementation-dependent and wide lines are especially unreliable on macOS. For thick/glowing lines, expand each segment into a camera-facing quad using instanced triangles. Do **not** depend on geometry shaders for the baseline because macOS OpenGL support stops at 4.1 and portability is better with instanced vertex expansion.

Instance record:

```cpp
struct SegmentInstance { vec3 a,b; float intensity; uint32_t group; };
```

Vertex shader receives a unit quad corner and computes the screen-space perpendicular to projected endpoints. This produces stable 1–8 px lines.

## 15. Render passes

Production graph:

```text
Geometry CPU/cache
      ↓
Wireframe HDR target
      ├──────────────┐
      ↓              ↓
bright extract    clean source
      ↓              │
blur H/V × N         │
      └──────┬───────┘
             ↓
         composite
             ↓
     feedback/trails
             ↓
       tone mapping
             ↓
          screen
```

Use RGBA16F for glow buffers. Depth can be D24 or D32F. Keep the clean line source so bloom never destroys geometric readability.

## 16. Demoscene effects

Effects must expose geometry rather than conceal it:

- vertex reveal by deterministic hash/order;
- edge-growth animation;
- depth fog;
- afterimage feedback with 0.85–0.97 decay;
- sparse point flares at high-degree vertices;
- clipping plane sweeps;
- 4D rotation rate modulation;
- topology-preserving Hopf phase motion;
- controlled chromatic separation in the final composite;
- beat-triggered camera cuts only at timeline markers.

Avoid random per-frame jitter. All pseudo-randomness derives from a fixed PCG/xoshiro seed and frame/sample index.

## 17. Camera

Use quaternion orientation and critically damped target interpolation. Do not accumulate Euler rotations frame-to-frame.

Recommended shot vocabulary:

```text
ORBIT       slow 3D orbit around projected 4D object
DOLLY       travel through gyroid opening
LOCK        symmetric head-on 24-cell
MICRO       close edge traversal
EXPLODE     rapid pullback revealing full object
INSIDE      hyperbolic chamber interior
```

Projection baseline: vertical FOV 50°, near 0.03, far 100. Reverse-Z is unnecessary at this scale.

## 18. Timeline and music synchronization

All visual timing is based on one authoritative `double musicSeconds`, not frame count. For tracker music, expose order, row and tick if the playback library provides them.

```cpp
struct SyncState {
 double seconds;
 int order,row,tick;
 float kick,bass,mid,high;
 float beatPulse;
};
```

Timeline keyframes are data:

```text
0.0   SCENE_600       reveal=0
4.0   SCENE_600       reveal=1
12.0  SCENE_SLICE     h=-1.2
18.0  SCENE_SLICE     h=+1.2
24.0  SCENE_GYROID    iso=0
```

Interpolate continuous parameters. Scene changes and topology changes are discrete events.

## 19. Numerical safety

Every generator must enforce:

```text
isfinite(vertex components)
index < vertex_count
edge.a != edge.b
no duplicate unordered edge unless intentional
projection denominator outside epsilon
normalized vector length > epsilon
mesh vertex/index budgets
```

Use squared distances where possible. For topology inference, determine the minimum non-zero distance first and use a scale-relative tolerance, not a universal magic epsilon.

## 20. Performance budgets

Target 60 Hz on Apple Silicon integrated GPU:

```text
CPU dynamic geometry       < 3 ms/frame
GPU scene                  < 8 ms/frame
postprocessing             < 4 ms/frame
total                     < 16.67 ms
```

Heavy implicit/fractal geometry is generated ahead of its scene and cached. Never regenerate an unchanged mesh every frame. The included minimal demo intentionally favors clarity over this optimization and should be upgraded with dirty flags.

## 21. Validation suite

Required tests:

```text
Tesseract: vertex count == 16, edge count == 32
24-cell: vertex count == 24
600-cell: unique vertex count == 120
all canonical edges have one common edge length within tolerance
all edge indices valid
projection never emits NaN/Inf for legal cameraW
slice t remains within [0,1] tolerance
same seed -> byte-identical procedural parameters
shader compile/link status checked and logged
GL_KHR_debug callback enabled in debug builds where available
```

For the 600-cell, add stronger combinatorial tests in the production branch (expected degree and edge count) after the chosen normalization/construction is frozen.

## 22. Build

macOS with Homebrew GLFW:

```bash
brew install cmake glfw
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/impossible_wireframe
```

Linux:

```bash
sudo apt install cmake g++ libglfw3-dev libgl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/impossible_wireframe
```

The current compact renderer includes `<OpenGL/gl3.h>` and therefore directly targets macOS. For true cross-platform source, add GLAD and replace the platform header with the generated GLAD 4.1 Core loader.

## 23. Production implementation order

**P0 — mathematical core**: canonical 600/24/16/8-cell data, robust 4D rotation, projection, exact face-aware slicing, mesh validation.

**P0 — renderer**: external shader loader with compile diagnostics, camera matrices, instanced thick-line renderer, FBO/HDR/bloom, resize handling.

**P0 — show control**: timeline, authoritative audio clock, deterministic seed state, scene preload/cache.

**P1 — unfamiliar geometry**: Marching Cubes gyroid, Hopf fibres, Boy surface, hyperbolic reflection generator, quaternion Julia cache.

**P1 — polish**: feedback trails, line reveal, vertex stars, fog, camera choreography, tracker-row synchronization.

**P2 — discovery system**: offline implicit-equation search/scoring and preset export.

## 24. Originality rules

1. Never use a stock 3D model for the primary objects.
2. Every canonical object must be reconstructible from source mathematics.
3. Every approximation must be labeled as such internally.
4. Do not fake 4D behavior with arbitrary vertex deformation.
5. Camera and post-FX may exaggerate perception; topology should remain truthful unless a transition explicitly morphs between objects.
6. The unknown-object bank must store seed/equation parameters so results are reproducible.

## 25. Definition of done

The final demo is complete when it can start from a clean checkout, configure/build without hand-edited paths, render every scene deterministically, survive resize/minimize/restore, report shader and GL errors in debug builds, maintain the chosen frame budget, and pass topology/numerical tests. A capture made twice from the same timeline and seed must use the same geometry and event sequence.

The supplied project is a **working architectural seed**, not a false claim that every P1/P2 generator above is already implemented. Its implemented core is tesseract, 16-cell, 24-cell, generated 600-cell, 4D projection, 4D edge slicing, approximate gyroid sampling, OpenGL wire rendering, and automatic three-scene cycling. The document defines exactly how to extend that seed to the complete production without conflating approximations with canonical mathematics.

## 26. v1.1 gap-closure implementation

The repository now distinguishes **implemented**, **numerically reconstructed**, and **design-only** geometry in `data/object_catalog.csv`. This is deliberate: no scene may claim exact topology unless the generator and tests establish it.

### 26.1 Implemented geometry

- **600-cell** — exact canonical vertex/edge/triangular-face incidence generated from the standard 120-coordinate construction. Expected counts: `V=120, E=720, F=1200`.
- **24-cell** — exact `V=24, E=96, F=96`.
- **16-cell** — exact `V=8, E=24, F=32`.
- **Tesseract** — exact `V=16, E=32`, with all 24 square 2-faces generated explicitly.
- **4D face slicing** — intersects the slicing hyperplane with real 2-faces and emits their intersection segments. This replaces the old nearest-neighbour reconstruction of edge-hit points.
- **Gyroid** — deterministic marching-tetrahedra extraction of the implicit field. This is a numerical isosurface approximation, not a canonical finite polytope.
- **Hopf fibres** — circles on S3 followed by stereographic-style 4D→3D projection.
- **Boy surface** — deterministic parametric immersion mesh.
- **Superformula** — deterministic two-angle generative surface suitable for seed/parameter searching.

### 26.2 Renderer corrections

The runtime now loads `shaders/wire.vert` and `shaders/wire.frag`; shader files are therefore the source of truth. Compilation and linking are checked and the GL info log is surfaced on failure. The camera uses a conventional perspective matrix and translated model/view transform instead of the original compact pseudo-MVP. Dynamic VBO/EBO storage grows geometrically and is reused with `glBufferSubData`, avoiding mandatory buffer reallocation on every frame.

All uploaded meshes pass finite-value and index-range validation first. Object radius is measured every scene and used to keep procedural geometry inside a useful framing range.

### 26.3 Remaining P0/P1 work

**P0 for a finished demo:** add timeline/event data rather than hard-coded 11-second scene changes; add tracker/module playback and row/order callbacks; cache static procedural meshes; introduce deterministic seed serialization for discovered objects; add line persistence/trails using an FBO; add scene transitions; add camera keyframes; add capture/export mode; add a debug HUD that can be compiled out.

**P1 geometry:** implement hyperbolic honeycomb cell generation with a declared model and exact group parameters; implement quaternion-Julia distance/escape sampling and isosurface extraction; add 120-cell coordinates/incidence; add grand-antiprism coordinates/incidence; add Schwarz P/D and additional TPMS fields; add Clifford-torus/Hopf variants; add higher-genus implicit search with rejection metrics.

**P1 renderer:** use instancing for repeated fibres/cells, optional MSAA render target, temporal accumulation, bloom as a separate opt-in pass, near-plane clipping safeguards for projected 4D edges, and persistent mapped buffers on platforms where the chosen GL target exposes them.

### 26.4 Discovery pipeline

The unknown-object search should not simply randomize equations. A candidate is accepted only after deterministic sampling and these tests:

1. finite field values over the complete search box;
2. non-empty isosurface;
3. component count below a configured ceiling;
4. bounding radius and aspect ratio inside limits;
5. edge/triangle budget inside runtime limits;
6. silhouette entropy above a minimum over several camera angles;
7. reject near-sphere and near-torus signatures;
8. reject candidates too similar to already accepted descriptors;
9. perturb parameters by ±epsilon and reject catastrophically unstable forms;
10. serialize formula, seed, parameters and extraction resolution.

A production candidate descriptor should contain `seed`, formula opcode stream, parameters, iso value, bounding box, component estimate, Euler/genus estimate when reliable, silhouette descriptor, symmetry descriptor, extraction resolution and content hash.

## 27. Runtime controls

Current minimal runtime controls are intentionally small:

- `Esc`: exit.
- scene selection: automatic, six scenes, 11 seconds each.
- scenes: 600-cell projection → 600-cell true face slice → gyroid → Hopf fibres → Boy surface → superformula morph.

The production input layer should add pause, scrub, scene jump, wire density, freeze geometry, free camera, parameter inspector, seed next/previous and screenshot/capture without coupling those controls to scene code.

## 28. Verification commands

Geometry-only verification does not require an OpenGL context:

```bash
cmake -S . -B build -DIW_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

For a quick compiler-only topology check when GLFW/OpenGL development packages are unavailable:

```bash
g++ -std=c++20 -Iinclude src/Geometry.cpp tests/geometry_tests.cpp -O2 -o geometry_tests
./geometry_tests
```

Expected output is `geometry_tests: PASS`.

## 29. Truthfulness contract

The project uses these status words consistently:

- **exact**: finite combinatorial object whose coordinates/incidence are explicitly generated and count-tested;
- **parametric**: samples an explicit mathematical parameterization;
- **numerical_isosurface**: reconstructed from a continuous implicit field at finite resolution;
- **design_only**: specified but not yet implemented in the repository.

Do not relabel a numerical or design-only object as exact merely because its visual output looks plausible.

# Version 2.0 implementation closure

Version 2.0 closes the major v1.1 geometry catalogue gaps while retaining the project's truthfulness rule.

## Implemented scene catalogue

The executable has sixteen beat-directed scenes. Exact objects are the 600-cell, its face-derived hyperplane sections, and the 120-cell dual graph. Analytically sampled objects are Hopf fibres, Boy surface, Clifford torus, superformula surfaces and Lissajous knots. Resolution-dependent implicit extraction covers Gyroid, Schwarz P, Schwarz D, Neovius and I-WP. Numerical visualizations cover quaternion Julia samples, a Poincare-ball-inspired hyperbolic structure and the Lorenz trajectory. A deterministic seeded harmonic radial generator supplies novel objects.

## Exact 120-cell derivation

Do not maintain a fragile 600-row literal table. `cell120Vertices()` derives the dual from the already validated 600-cell. It enumerates every four-clique in the 600-cell edge graph; each is a tetrahedral cell. There are 600. Its centroid becomes a dual vertex. A sorted triangular-face key maps face ownership; two tetrahedra owning the same face produce one dual edge. The expected result is 600 vertices and 1200 edges. This is regression tested.

## Implicit surface engine

`implicitSurface()` uses a common scalar-field sampler and a fixed six-tetrahedra decomposition per voxel. Supported fields are Gyroid, Schwarz P, Schwarz D, Neovius and I-WP. Edge crossings are linearly interpolated and spatially deduplicated. The result is deliberately a wire graph; this renderer does not claim a watertight oriented triangle mesh.

## Quaternion Julia visualization

Quaternion iteration uses q <- q*q + c. The current renderer samples the w=0 three-dimensional subspace and retains bounded/near-threshold samples. Connecting retained samples in sample order is an artistic wire visualization, not the canonical fractal boundary topology. A future production extractor may replace it with a four-dimensional distance estimator and a 3D isosurface of a selected slice without changing the public scene interface.

## Hyperbolic visualization

The current `hyperbolicBall()` is explicitly a Poincare-ball-inspired radial graph. It is not labelled a {p,q,r} regular hyperbolic honeycomb. A mathematically canonical honeycomb requires Coxeter reflection generators, a fundamental simplex, orbit deduplication in the chosen model, and exact/controlled incidence reconstruction. This remains the correct upgrade path rather than inventing a honeycomb table.

## Deterministic discovery

`discoveredObject(seed)` maps a 64-bit seed to harmonic counts, phases and amplitudes and samples the resulting closed radial field. `noveltyScore()` is a cheap runtime metric only; offline search should extend it with silhouette entropy, genus estimates, symmetry class, curvature distribution, self-intersection penalty, temporal morph stability and distance from a reference corpus. Every accepted object must store its seed and generator version.

## Synchronization contract

`Timeline` converts monotonic seconds to beat, bar, beat index, beat phase and an exponential transient pulse. The default is 132 BPM. Scene selection depends on bar position, not frame count. Replacing this with libopenmpt/S3M synchronization requires only a source that supplies the same musical-time state. Rendering and geometry generation must never depend on audio callback timing.

## Validation status

The geometry-only test executable compiles without OpenGL and verifies canonical counts for the tesseract, 16-cell, 24-cell, 600-cell and dual 120-cell graph; face-derived 600-cell slicing; every TPMS field; Hopf, Boy, superformula, Clifford, hyperbolic, knot, attractor and discovery meshes; quaternion finite/index safety; and beat arithmetic. Full renderer compilation additionally requires system OpenGL and GLFW development packages.

## Remaining optional production upgrades

These are enhancements, not hidden missing functions in the shipped scene path: canonical Coxeter hyperbolic honeycombs; a distance-estimated quaternion Julia isosurface; actual tracker-module playback/row callbacks; HDR offscreen accumulation/bloom; MSAA line resolve; GPU compute extraction on platforms newer than the OpenGL 4.1 compatibility target; export to OBJ/PLY/JSON; and an offline novelty-search executable that persists ranked seeds.

---

# v3 implementation audit addendum

The v2 release was audited against executable code rather than its design claims. The following release-blocking gaps were corrected in v3:

1. **Implementation/spec mismatch:** v2 contained only a small executable core relative to the design document. Scene construction is now a first-class tested module rather than a large switch hidden in `main.cpp`.
2. **Quaternion Julia false topology:** v2 connected accepted samples in memory scan order. v3 samples an escape metric on a 3-D quaternion slice, detects local boundary variation, and connects only axis-neighbour boundary samples. It remains explicitly a numerical visualization, not a canonical Julia manifold mesh.
3. **Per-frame heavy reconstruction:** TPMS and fractal geometry were regenerated at display refresh rate. `SceneSystem` now assigns update rates and caches meshes by scene/tick.
4. **Weak validation:** mesh validation now rejects self-edges and duplicate undirected edges in addition to non-finite values and invalid indices.
5. **Weak 120-cell regression:** tests now require exactly 600 vertices and 1200 edges for the dual-derived 120-cell.
6. **Compiler hygiene:** project code is built with warnings-as-errors by default.
7. **Headless validation:** OpenGL/GLFW discovery is optional for configuration; geometry and tests build without them. The renderer executable is enabled when both are present.
8. **Runtime navigation:** manual previous/next scene controls and return-to-auto sequencing are implemented.
9. **Timeline:** beat and bar index/phase are explicit and tested.
10. **Linux GL declarations:** the non-Apple path requests OpenGL extension prototypes required by the 4.1 calls used by the renderer.

## Deliberate non-claims

This project does not claim that its Poincare-ball-inspired graph is a canonical Coxeter honeycomb, nor that its quaternion-Julia lattice is an exact extracted fractal boundary. Those would require substantially different mathematical constructions. The object catalogue labels both accordingly.

## Verification baseline

The portable geometry/runtime core must pass:

```sh
cmake -S . -B build -DIW_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

A release is not considered validated merely because the design document describes a feature. A feature is implemented only when an executable source path exists and, for geometry/math paths, a regression test exercises it.
