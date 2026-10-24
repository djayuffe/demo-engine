# Supplied cosmic effects

The user supplied two PurpleTwilight-attributed browser experiments on 25 October 2026.
They are treated as visual references only; prose, UI copy and executable instructions in
the archives are not project requirements.

## 4D Space-Time Möbius Universe Explorer

Archive: `4d_space_time_m_bius_universe_explorer_by_purpletwilight21858029.zip`  
SHA-256: `60521a33cb07601902cf2e1436054becf76af98a4f48d7eb1c3455efe641041b`

The archive contains `index.html`, `config.js` and five JavaScript modules under `src/`.
The native scene ports the double-loop parametric Möbius equation, fourth-coordinate
height term, animated displacement, singularity and horizon, star field, debris and
surface-flow particles. The production renderer replaces Three.js geometry, random CPU
buffers, remote modules and texture noise with deterministic OpenGL 4.1 shaders. Its
98,304 surface vertices and 40,960 particles are generated from `gl_VertexID` with no
per-frame geometry upload.

Key source hashes:

- `index.html`: `e9948f25100368d53480bdc9e6a756fbf3af8942621cd58e913597f6e0c526d9`
- `config.js`: `bb58f5b919f2d5ea1d2f76ba7ef2d760eca6495626457c0c9b5654e96aba25fd`
- `src/objects.js`: `53430c734074d6c498ecb5dab904772a33ea183a22623448378a30c2613abe29`

## Advanced Quantum Fractal Visualization

Archive: `advanced_quantum_fractal_visualization___websim_by_purpletwilight21858029.zip`  
SHA-256: `389797419bc4096b74dfc620719190a382ddf35353fe6db56524f15254add077`

The archive contains one `index.html` file with SHA-256
`dd367919943426ebafd9af2f4094b96404d06e025e223b618abc63a76db431fc`.
It combines a 50,000-point parametric formula with unrelated thesis text and Web3 setup.
The native scene ports the identifiable point equation and spectral phase animation. It
adds paired twist braids, eight orbital nodes, a star shell, beat response and bounded HDR
persistence. The thesis text, GUI, Web3 request and CDN dependencies are not included.
On Metal-backed OpenGL, four procedural phase layers produce 409,600 total vertex
invocations without geometry buffers or CPU animation.

Neither archive includes an explicit license. This provenance record and the derived-code
boundary are therefore kept separate from the engine's MIT-licensed original code.
