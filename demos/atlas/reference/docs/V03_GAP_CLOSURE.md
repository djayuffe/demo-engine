# v0.3 implementation and remaining gaps

## Verified implemented in this revision

- Exact hyperboloid-model H3 geodesic ray: `o cosh(t) + d sinh(t)` with Minkowski normalization.
- Analytic ray intersection against geodesic H3 spheres. Derivation: `-<ray(t),center> = A cosh(t)+B sinh(t) = cosh(radius)`; substitute `u=exp(t)` to solve quadratic. Only positive forward hits are retained.
- H3 camera tangent construction for a camera displaced by an x-axis Lorentz boost.
- Poincare ball projection for surface shading, with normals used as an artistic approximation (not a rigorous H3 shading model).
- Multi-chart portal composite: Euclidean ornamented screen exterior, analytically intersected H3 interior, antialiased iris blend. This is a visual composite, not a continuous metric-preserving manifold transition.
- Four extra CTest checks including analytic intersection, ray normalization, displaced camera tangent, and a render smoke test.

## Known limitations and priority

P0: H3 renderer only intersects spheres, not arbitrary hyperbolic surfaces; no hyperbolic occlusion hierarchy/BVH; 56 spheres per pixel is O(N) and CPU-only.
P0: Previous polygon tiling uses quantized-center deduplication, not canonical Coxeter words. Previous 4D slice may generate duplicate/coplanar faces at degeneracies.
P0: No SDL2/OpenGL app or HDR render graph; this is a deterministic offline CPU reference renderer.
P1: Portal currently composites different scene charts in image space; it does not construct mathematically smooth transition maps between manifolds of different curvature.
P1: Hyperbolic surface normals and illumination need Lorentz-tangent-space derivation.
P1: Audio engine, glyph grammars, temporal event volume, multiperspective and 21-system registry remain absent.
P1: Adaptive detail and anti-aliasing for small-scale structures; no temporal filtering.
P2: Color gamut limitations and human perceptual validation cannot be solved with code alone.

## Build

`cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
`cmake --build build -j`
`ctest --test-dir build --output-on-failure`
`./build/atlas_advanced portal.ppm 640 0.8 0`

Mode 0: portal composite; mode 1: H3 only; mode 2: Euclidean ornament only.

## Why this is a concrete gap closure

The previous renderer generated a 2D hyperbolic tiling, but had no ray intersection in 3D hyperbolic space. This revision computes forward geodesic intersections using the actual H3 metric, making an analytic H3 ray renderer rather than a Euclidean scene warped to resemble one.

## Novelty

No claim of world-first invention is made. The implementation combines established mathematics into a specific experimental visual renderer.
