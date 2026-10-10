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

## Unknown effects (64-79)

Added in engine 1.2 / Impossible Wireframe v4.36. Each is deterministic, finite and fail-safe like the rest, and each is tested to actually change a mesh.

| Id | Name | What it does |
|---|---|---|
| 64 | Hopf Rotation | Lifts the mesh onto the 3-sphere by inverse stereographic projection, rotates isoclinically (moving points along Hopf fibres) and projects back |
| 65 | Klein Fold | The +x half twists by up to pi about the x axis with a smooth seam, a non-orientable fold |
| 66 | Hyperbolic Drift | Moebius translation of the Poincare ball (gyrovector addition); the mesh crowds toward a moving ideal point |
| 67 | Thomas Flow | Advects vertices through the bounded Thomas attractor field |
| 68 | Galactic Disk | Keplerian shear (angular speed ~ r^-1.5), disk flattening and a two-arm logarithmic density wave |
| 69 | Harmonic Bloom | Radial displacement by a breathing spherical-harmonic-like pattern |
| 70 | FCC Crystal Snap | Pulls vertices toward the nearest face-centred-cubic lattice point |
| 71 | Tendril Growth | New geometry: curling 10-segment tendrils sprout outward from up to 48 vertices |
| 72 | Shell Cage | New geometry: a counter-rotating outer copy tied to the core by struts (skipped on very large meshes) |
| 73 | Barbed Wire | New geometry: spinning crossbars at edge midpoints (at most about 3000) |
| 74 | Torus Attractor | Pulls vertices onto a ring torus around the y axis, breathing |
| 75 | Mobius Map | The radial and height offset from a ring rotates by theta/2 around it |
| 76 | Soliton Wave | A sech-shaped breather packet travels across the object |
| 77 | Quaternion Square | Treats each point as a quaternion w + yi + zj and squares it, doubling angles |
| 78 | Lissajous Satellites | Three small copies orbit on a 3:4:5 Lissajous knot |
| 79 | Spectral Harmonics | Bass, mid and treble each drive their own harmonic band of radial displacement |

New recipes: 24 Hopf Fibration Storm (64, 73, 14), 25 Hyperbolic Drift (66, 75, 41), 26 Keplerian Galaxy (68, 71, 5), 27 Barbed Crystal (70, 73, 65), 28 Soliton Garden (76, 69, 71), 29 Quaternion Bloom (77, 79, 72, 78). The procedural mutation draws from all 80 effects.
