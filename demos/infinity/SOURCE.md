# Imported source and attribution

This native demo ports the GPU geometry and optical renderers from the user-supplied `hold_infinity_by_purpletwilight21858029.zip` archive, attributed to **PurpleTwilight** by its supplied filename. The archive SHA-256 is `71deb5a0cd1dd051b7add885657de14b317f120cf68f884d5dda3fd18910a866`.

The eight original files are preserved in `reference/`; `source.json` records their exact hashes. No explicit license is present in the supplied files. Their attribution is retained separately from the engine's MIT license; this port does not relicense the imported source.

`tools/import_hold_infinity.py` makes the native shader header and dodecahedron constants from that archive. It changes WebGL ES shader declarations into GLSL 4.10 and substitutes the same numeric constants computed by the JavaScript. The ray integrators, mirror intersections, Lorentz reflections, spectral functions, Fresnel/TIR, optical index layers, horizon and bloom shaders are retained. The native fullscreen composition shader and C++ host are new integration code.

The browser page, CSS, visibility observers and DOM controls remain reference material. The C++ host replaces their event loop, GPU resource allocation, orbit input, parameter UI and random light scheduler. The six light fronts and scene changes are driven by the engine music/beat clock.

Bundled soundtrack licensing and credits remain in the repository `NOTICE.md` and `demos/wireframe/assets/music/README.md`; the live ticker credits the active bundled track.

The GPU optimization pass adds constant-specialized program variants in `Optimize.hpp`; it preserves the source integrator and all numerical limits. The generic uniform-driven source remains available with `--infinity-reference` for image/workload comparisons.

The native 1.3 presentation adds `DimensionalRays.hpp`: four-coordinate embedding, plane rotations, camera projection, layered instancing and beat-driven light packets for the last two scenes. These artistic transformations are separate from the preserved source shaders and do not replace their RK4 solver.

Native `RenderCorrections.hpp` normalizes decreasing smooth ramps before compilation to avoid undefined reversed GLSL smoothstep bounds. Dimensional projection corrections remain in the separate native presentation layer. Archived sources and generated import headers are preserved.

The 2.3 `SequentialRays.hpp` geometry kernels extract the optical definitions, derivative and RK4 update from the imported integration shader at startup. They change scheduling and prefix reuse while retaining those source equations and their attribution. Generated `Shaders.hpp`, geometry constants and archived reference files are not rewritten by this audit.

## QRI-inspired original geometry studies

Scenes 12–15 in `WorldSheets.hpp` are original procedural GPU implementations inspired by the user-supplied text of Andrés Gómez-Emilsson (2016, December 12), *The Hyperbolic Geometry of DMT Experiences: Symmetries, Sheets, and Saddled Scenes*, Qualia Research Institute: https://www.qri.org/blog/hyperbolic-geometry-DMT .

The visual themes used are harmonic symmetry/chrysanthemum forms, heptagonal disk tilings, folded world sheets and self-similar branching. The supplied essay is treated as artistic reference, including its distinction between subjective reports and proposed models; its phenomenological hypotheses are not asserted as established science by the demo. No article prose, images or embedded media are redistributed. These new effects are separate from PurpleTwilight’s imported shaders and keep that source’s attribution intact. No QRI affiliation or endorsement is implied.
