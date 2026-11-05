# DMT Atlas geometry core — verified reference implementation

This package is a **compilable, dependency-free C++20 mathematical kernel**, not a finished OpenGL application or a scientifically validated reconstruction of subjective perception.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Implemented

- Poincaré disk distance, Möbius translation, geodesic circles/diameters and geodesic reflection.
- Regular {p,q} hyperbolic fundamental-polygon circumradius.
- H³ hyperboloid points, Lorentz metric, distance and x-axis Lorentz boosts.
- Tesseract vertices/edges, arbitrary six-plane 4D rotations (call rotatePlane for each plane), perspective clipping and projection.
- Stable hierarchical deterministic seeds.
- Numerical invariant tests.

## Algorithms to integrate next

1. **2D hyperbolic tessellation:** compute regular polygon vertices using polygonDiskRadius; reflect polygons across their geodesic edges; store a canonical combinatorial cell ID; cull by hyperbolic distance and projected pixel footprint. Use geodesic midpoint interpolation, not straight-line edge interpolation. Floating-point hashing alone must not be treated as exact topology.
2. **H³ camera:** represent camera as Lorentz matrix L satisfying L^T eta L = eta; apply boost and spatial rotations; re-orthonormalize numerically as needed; map visible rays using exp-map and project through a chosen optical model. H³ is not a Poincaré disk texture.
3. **4D geometry:** compose six plane rotations, clip edges against 4D eye plane, project to 3D, then ordinary camera-project to screen. For slicing, intersect *faces/cells* with a 3D hyperplane; edge intersections alone are insufficient to recover surface topology.
4. **Recursive detail:** subdivide using stable childSeed; stop when projected feature width falls below two pixels; bandlimit procedural noise using screen-space derivatives.
5. **Temporal objects:** store event graph nodes (position, time, duration, causality links); display time as an authored axis or parallel slices. This is a visual metaphor, not a physical extra spatial dimension.
6. **Face emergence:** create parametric face landmark fields, smoothly blend their signed-distance contributions into filigree, then perform visual studies to tune recognition threshold.
7. **Non-Euclidean perception:** blend mathematically coherent H³ navigation with multiperspective projection; treat perceptual resemblance as an experimental design metric, not a proof.
8. **Color:** use HDR plus opponent contrast and tone mapping; impossible-to-display spectral stimuli remain impossible on ordinary RGB hardware. Avoid claims of creating new colors.
9. **Safety:** provide reduced motion, flash-rate controls, exposure cap, pause, and no mandatory strobing.

## Unsolved by this package

Renderer/windowing, shader integration, actual polygon BFS tessellation, complete H³ camera, 4D face/cell topology, audio engine, glyph grammar, volumetric effects, temporal compositor, user study, GPU profiling and macOS app packaging.

## Numerical caveats

All floating-point algorithms need explicit domain checks and production-grade near-boundary precision management. Reflection inversion becomes poorly conditioned near a geodesic center. Lorentz matrices can drift after repeated multiplication. The tests here are invariant smoke tests, not a formal proof or exhaustive robustness suite.

## v0.2 executable geometry visualizer

The project now also builds `atlas_render`, a CPU reference renderer of four phenomena. See `docs/IMPLEMENTATION_STATUS.md`. It renders an actual 4D hyperplane cross-section, Poincaré polygon reflections, deterministic radial geometry, and coordinate folding. The command is `./build/atlas_render atlas.ppm 0.9 480`.
