# Effects and scene catalogue

This demo renders mathematically unusual wireframe geometry with a deterministic
beat timeline. Exact canonical constructions are labelled separately from
sampled or artistic numerical visualizations.

## Canonical / derived scenes

- **600-cell projection** — generated from the standard 120-vertex coordinate
  construction, rotated in 4D and projected into 3D.
- **600-cell face slice** — moving 4D hyperplane intersection using face-aware
  section edges.
- **120-cell dual graph** — derived from the validated 600-cell tetrahedral
  cell incidence.
- **24-cell / 16-cell / tesseract family** — canonical polychora used by the
  geometry core and tests.

## Approximation / visualization scenes

- **Gyroid, Schwarz P, Schwarz D, Neovius, I-WP** — sampled TPMS wire lattices.
- **Hopf fibres** — stereographic-style fibre projection.
- **Boy surface** — analytic immersion grid.
- **Superformula and Clifford torus** — parametric wire surfaces.
- **Poincare-ball-inspired hyperbolic chamber** — visual hyperbolic graph; not
  claimed to be a Coxeter honeycomb.
- **Quaternion Julia boundary lattice** — deterministic escape-boundary sample;
  not claimed to be an exact extracted fractal manifold.
- **Lissajous knot and Lorenz attractor** — motion-led mathematical trails.
- **Discovered surface bank** — deterministic seeded harmonic radial generator.
- **v4 exotic family pack** — Mobius strip, Klein bottle, Enneper, helicoid,
  catenoid, Dini, pseudosphere, Roman surface, cross-cap, torus/viviani/
  hypotrochoid curves, Duffing/Rossler/Thomas attractors, Sierpinski
  tetrahedron and eight algebraic implicit visualizations.
- **Unknown-lab procedural objects** — curated supercage, knot-bundle,
  phyllotaxis, ruled and star-cage variants ported from the 49-object lab pack.

## Music/timeline

The show uses a BPM clock (`--bpm`, default 132) and can optionally play the
Public Domain `Silicon Dancer` MOD through SDL2/libopenmpt. The timeline emits
beat, bar, beat phase and pulse values; decoded module RMS is folded into the
visual pulse for wire brightness and background color. Geometry generation never
depends on audio callback timing.

## HDR, glimmer and background pass

Wire geometry is rendered into an RGBA16F offscreen framebuffer before a shader
composite pass. The composite adds a procedural nebula/star background, glimmer
streaks, local bloom-like sampling around bright wire pixels, music-reactive
color lift, vignette, grain and exponential tone mapping. This keeps the core
geometry mathematically simple while giving the final image a brighter
high-dynamic-range demoscene finish.

## Size, zoom and flyover choreography

The renderer now applies a slow breathing scale, music-lifted line width,
animated field-of-view, z-axis zoom and lateral camera flyover. The object stays
centered enough for manual browsing, but the framing constantly changes so large
surfaces feel like fly-through sculptures instead of static turntables.

## Wire lighting matrix

The wire shader now derives color from object-space position instead of using a
flat line tint. Every edge participates in a moving color cycle, so the color
appears to travel through the object as it rotates. Three high-frequency
procedural lattice masks add “lighting matrix” flashes across x/y/z object
coordinates, while three soft light sheets sweep through the mesh from different
directions. Music RMS raises the sheet intensity, line bloom and blue electric
lift without changing the deterministic geometry.

## Shader background optimization

The background remains fully procedural and single-pass, but the fractal noise
octave count is kept low and reused for nebula, aurora and matrix-line layers.
This avoids uploading textures or spawning extra framebuffers while still adding
depth behind the wireframe: sparse stars, diagonal glimmer, aurora bands,
music-reactive matrix streaks, a layered pseudo-3D tunnel, vignette and HDR tone
mapping all happen in the same composite shader.

## Showpiece scenes (51-56)

Clean, readable 3D objects added in v4.34. They skip the procedural mutation overlay and get a "Pure form" window (about 40 % of every ~12 s cycle) where no effect recipe is applied at all:

- **Geodesic dome** - subdivided icosahedron with a breathing surface bump.
- **Tube trefoil** - trefoil knot swept into a wire tube with twisting cross-section rings.
- **DNA helix** - two backbones with base-pair rungs and an animated twist.
- **Atom orbits** - geodesic nucleus, three tilted electron rings and octahedral electrons.
- **Wave terrain** - rippling wire ocean (radial wave plus a cross-wave), tilted toward the camera.
- **Platonic compound** - icosahedron, its dual dodecahedron and a pulsing stella octangula.
