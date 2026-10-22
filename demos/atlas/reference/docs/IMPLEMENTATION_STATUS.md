# DMT Atlas — experimental geometry implementation v0.2

## What is actually executable

`atlas_render` is a standalone, dependency-free C++20 CPU renderer producing a four-panel PPM image. The panels show: (top left) reflected Poincaré {7,3} cells with geodesic edges; (top right) a genuine hyperplane cross-section of the *eight cubic cells* of a rotated 4D tesseract, reconstructed as polygonal 3D faces; (bottom left) deterministic multi-octave radial chrysanthemum field; (bottom right) iterative coordinate-fold visualization. This is not an OpenGL renderer and does not recreate subjective perception.

## Build and render

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/atlas_render atlas.ppm 0.9 480
# macOS: sips -s format png atlas.ppm --out atlas.png
# or: ffmpeg -i atlas.ppm atlas.png
```

`phase` (second argument) controls geometry; `tile` (third argument) is 64..2048; output is a square image of size 2*tile. Render multiple frames by invoking with increasing phase.

## Previously unknown / difficult cases concretely addressed

- Hyperbolic geodesic reflection and breadth-first polygon expansion are implemented. The duplicate suppression uses quantized centroids and is **not exact group-word canonicalization**; it is a bounded visualization algorithm.
- Four-dimensional slicing now intersects the **3D cubic boundary cells**, not only the 4D edges, with an arbitrary hyperplane. The 3D slice polygons are sorted in a local planar basis and rendered as wireframes. Degenerate hyperplanes need further robust topology handling.
- Scale-stable hierarchical seed derivation is implemented, and procedural radial structures remain deterministic across frames. Infinite resolution remains physically impossible.
- Recursive coordinate folding has a rendered demonstration. It is not a signed-distance estimator, nor proof of a non-Euclidean manifold.
- Lorentz hyperboloid distance and boosts, 4D plane rotation, clipping, and 4D perspective projection remain available in `include/geometry.hpp`.

## Known limitations / P0-P2 remaining

P0: full H3 ray tracing with analytic geodesic primitive intersections; numerical error budgets and property-based testing; correct canonical Coxeter group cell identities; robust degenerate 4D slicing; scene composition and clipping across coordinate charts.

P1: SDL2/OpenGL 4.1 GPU implementation; temporal buffers; signed-distance field render graph; audio engine; physically bounded color pipeline; view-dependent multi-camera compositor; glyph grammar and circuitry simulations; adaptive pixel-footprint refinement.

P2: performance measurements on Apple M1 Pro; VR experiments; psychophysical user study; offline video export; artistic scene transitions.

Claims of creating new physically impossible colors, genuinely infinite detail, literal simultaneous perception, or a uniquely unprecedented effect are **not supported**. The project implements mathematical visual analogues and novel combinations, not a validated simulation of drug experiences.

## Validation

Built in this execution environment using CMake and a C++20 compiler. `ctest`: 2/2 tests passed (invariant test and renderer smoke test). The test suite is not exhaustive; CPU image output inspected programmatically, no macOS GPU testing performed.
