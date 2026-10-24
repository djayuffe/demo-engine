# GPU geometry and HDR rendering

`Frame::gpuWave` moves sine-wave displacement into the GPU. Its four values are
amplitude, spatial frequency, speed and phase. The displacement is
`y += amplitude * sin(x * frequency + time * speed + phase)`.
Use `Frame::retainedMesh` with a shared immutable mesh and a stable nonzero
`revision` to avoid per-frame geometry copies and uploads. Hello now tumbles its
resident cubes with the model matrix; Ripple keeps its torus resident.
The engine includes the wave amplitude in its conservative camera-fit radius.
The ripple demo demonstrates this with a cached 96 by 32 torus.

When wave amplitude is nonzero and no recipe stage is deferred, the renderer uses OpenGL 4.1 tessellation control
and evaluation shaders. Each indexed edge becomes a two-control-point quad patch.
Subdivision follows projected edge length and wave frequency, capped at 16 segments
per edge. Each generated vertex is displaced before projection, and the analytic
projected tangent supplies an antialiased ribbon width. This adds detail along
existing edges; it does not reconstruct a smooth surface from arbitrary wire meshes.

The quad path omits the geometry stage: testing on this Mac exposed driver software
fallbacks with tessellated isolines plus geometry expansion. Undeformed meshes keep
the existing vertex/geometry path. If the optional tessellation shaders fail to link,
the renderer reports it and retains vertex displacement with the original line path.
Windows loads the tessellation entry point through the existing GL loader.

Mesh uploads compare exact vertices and edge indices, retain unchanged GPU buffers,
and skip repeated validation when both match a previously validated mesh. The CPU
cache holds one additional copy of the last uploaded geometry. The revision API
skips these comparisons entirely for static meshes.
Line expansion now clips camera-crossing edges before perspective division.

Bloom obtains source dimensions with GLSL `textureSize`, removing CPU texture-size
queries from each frame. The pyramid stops at 1 by 1 for small targets. First-pass
sampling rejects invalid values and reserves half-float headroom for additive
reconstruction. ACES input sanitization prevents negative grain and non-finite
values from corrupting tone mapping.

Infinity uses four bilinear GPU samples for positive-weight cubic B-spline upscaling
when its scene target is smaller than the display. Native-resolution presentation
keeps the original direct sampling. The reconstruction smooths reduced-resolution
edges without negative-weight highlight ringing, at the cost of extra texture reads.
HDR remains floating-point rendering tone-mapped to the existing display output.

## Recipe effects and instanced copies

The wireframe demo calls `applyRecipeGpu` by default. It executes the CPU prefix in
order, then defers a supported final stage to the vertex shader. Supported effect
IDs are 1, 2, 4, 6, 12, 13, 15, 19, 22, 23, 31–35, 48, 52–54, 58, and 61–63.
Seventeen are vertex deformations; six copy/echo stages use indexed instanced draws
rather than duplicating geometry on the CPU. Copy counts obey the existing
60,000-edge budget. Wire-density compensation includes the instance count.

Topology-changing prefixes, secondary-mesh operations, and unsupported final stages
retain the CPU implementation. The planner computes a conservative bound from the
base mesh and effect envelope, keeping camera fitting independent of GPU readback.
This can frame the object more loosely than the exact CPU vertex radius. Seed
arithmetic, stage time/amount transforms, and stage order follow the CPU path.
`--cpu-effects` selects the wireframe CPU reference path. Recipe deformation uses
ordinary indexed vertex processing so its vertex-index effects and straight edges
retain their semantics; tessellation remains available for the separate wave path.

## GPU traffic texture

Infinity retains the sixteen-agent interaction planner on the CPU and rasterizes
its visual field using one instanced GPU splat draw. Nine periodic images per agent
preserve toroidal wrapping and the original seven-pixel footprint. The draw shades
small sprites, rather than evaluating every agent at every texture pixel.

Per frame, 320 bytes of agent uniforms plus the interpolation phase replace a
65,536-byte RGBA float image upload. The HDR texture remains GPU-resident. Use
`--cpu-ants` for an isolated CPU raster comparison, `--infinity-reference` for the
existing full reference renderer, and `--validate-ants` to compare every GPU traffic
pixel against CPU rasterization. Validation intentionally reads pixels back; normal
playback does not. Traffic profiling reports GPU splats and CPU texture uploads.

## Measured CPU work and transfer savings

On this Apple M1 Pro, two alternating-order 180-frame repeats produced:

| Workload | CPU path | GPU path | Change |
|---|---:|---:|---:|
| Recipe 21, CPU update/upload time | 1.414 ms | 0.682 ms | 52% less CPU time |
| Recipe 1, CPU update/upload time | 0.251 ms | 0.097 ms | 62% less CPU time |
| Recipe 1, total mesh bytes uploaded | 21,813,120 | 3,635,520 | 83% fewer bytes |
| Traffic image uploads in 180 frames | 180 | 0 | Pixel uploads eliminated |

These measurements precede the shared beat-response defaults added on 2026-10-24.
These CPU measurements exclude GPU drawing and presentation. Traffic frame timings
were mixed: the full-frame GPU-query mean was 1.922 ms with CPU rasterization and
2.017 ms with GPU splats across these runs. The traffic path establishes reduced
CPU raster work and transfer volume, not a GPU-time improvement. Conservative
camera bounds can also change the wireframe's screen coverage. See the
[raw measurements and conditions](GPU_OFFLOAD_BENCHMARK.json).

## Validation

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DIW_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 tools/gpu_smoke.py --build build
python3 tools/benchmark_gpu_offload.py --frames 180 --repeats 2
```

The GPU smoke check requires an accessible desktop/GL context. It checks that Hello
and Ripple upload their static meshes once, runs generic demos through post and
bypass paths, validates all 23 GPU effects against CPU vertices, checks traffic
raster agreement, renders all 26 Infinity scenes with finite HDR output, and runs
framebuffer resize/recovery checks. Shader degradation and driver-reported software
fallbacks fail the check. GPU numerical cases cover stage prefixes, negative and
zero amounts, transformed time, seed overflow, translated geometry, camera bounds,
and copy-budget limits. Headless CTest also checks planning/fallback and stage order.

Performance depends on hardware and scene composition. This work stays within
OpenGL 4.1; it does not add a compute-shader or HDR-monitor output backend.
