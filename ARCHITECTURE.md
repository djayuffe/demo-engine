# Architecture

The engine is a single-threaded render loop plus one audio callback thread. A demo supplies a wire mesh each frame; the engine uploads it, draws it additively into an HDR target, and a full-screen post shader produces the final picture.

```
 CLI / Config ----> de::run ---------------------------------------------------------+
                      |  window + GL 4.1 core, signal handling, asset search          |
                      v                                                               |
 Timeline  (seconds,BPM -> beat/bar/pulse)      AudioPlayer (SDL2 callback thread)    |
      \                                          libopenmpt + bass/mid/treble split   |
       \                                            | atomics                         |
        v                                           v                                 |
   FrameInfo { time, dt, size, sync, music } + Input (keys)                           |
                      |                                                               |
                      v                                                               |
              Demo::update(FrameInfo, Input, Frame&)   <-- your code                  |
                      |  Frame { mesh, revision, sceneIndex, scale, lineWidth,        |
                      |          travelers, ticker }                                  |
                      v                                                               |
   Renderer (OpenGL 4.1 Core)                                                         |
     upload(mesh) -> draw(): logo backdrop + additive wires -> HDR FBO (RGBA16F)      |
                  -> drawTravelers() -> HDR FBO                                       |
                  -> finishLogoFrame(): post.frag -> screen                           |
                  -> drawScroller(): ticker band, alpha blended                       |
                  -> screenshot() when requested                                      |
```

## Layers
| Layer | Library target | Needs GL | Contents |
|---|---|---|---|
| Core | `de_core` | no | `Geometry` (meshes, polytopes, validation), `Timeline`, `Effects`, `TextScroller` (font), `Image` (stb decode), `WindowSpec` |
| Runtime | `de_engine` | yes | `Engine` (run loop and CLI), `Renderer` (all GL state), `Audio` (SDL2 + libopenmpt) |
| Demo content | e.g. `wireframe_content` | no | A demo's own scenes and generators, testable without a GPU |
| Demos | `impossible_wireframe`, `hello_demo`, `ripple_demo` | yes | `main()` calling `de::run` |

Keeping GL-free code in separate libraries lets tests and CI run headless: `geometry_tests` links only `wireframe_content` and `de_core`.

## The run loop (`engine/src/Engine.cpp`)
1. Parse options. The demo gets first refusal on each argument through `Demo::parseArg`; anything unrecognised is an error (exit code 2).
2. Resolve assets: every `Config` path is tried relative to the CWD, the executable directory and each of its ancestors. Music defaults to the first existing `Config::musicFiles` entry.
3. Open a GLFW window with a 4.1 core (forward-compatible on macOS) context, start audio, initialise the `Renderer` from the shader directory, load logo cards, then call `Demo::init`.
4. Each frame: skip while minimised, build `FrameInfo` (show time is the music clock when music plays, else wall time; `sync.pulse` is `max(beat pulse, music level)`), poll `Input`, handle the engine key (T toggles the ticker), call `Demo::update`.
5. Upload the mesh unless `Frame::revision` is non-zero and unchanged. The scale is `min(1.48, 1.92 / radius) * Frame::scale`, so any mesh auto-fits.
6. Draw: wires into the HDR target, travellers, post pass, ticker. Take the screenshot if requested, honour `--frames`, swap, poll events.
7. On exit (Esc, window close, Ctrl-C, SIGTERM, `Demo::update` returning false or a fatal renderer error) shut down GL, audio, SDL and GLFW in order. Exit codes: 0 ok, 1 GLFW, 2 bad option or context, 3 renderer init, 4 demo init, 5 mesh rejected, 6 draw failed.

## The Demo contract
`Demo::update` is called once per frame on the main thread and must fill a `Frame`:
- `mesh` is a `Mesh3` (vertices plus index-pair edges). It must have finite vertices, in-range indices and no self or duplicate edges, otherwise `Renderer::upload` rejects it and the engine exits with code 5. Run it through `geo::validate` in tests and `Effects::sanitize` after warping.
- `revision` lets a static mesh skip the upload: set a non-zero counter that changes only when the mesh does. `0` means "re-upload every frame", which is always correct.
- `sceneIndex` selects which logo card shows and varies the post pass per scene.
- `ticker` is shown in the bottom band, if non-empty and the ticker is on.

## Rendering pipeline (`engine/src/Renderer.cpp`, `engine/shaders/`)
- **HDR target**: RGBA16F colour plus depth, sized by `--quality`. Lines use additive `GL_ONE, GL_ONE` blending, so overlaps glow instead of occluding. Wire gain is `clamp(0.80 * sqrt(500 / lines), 0.05, 1)` so dense meshes do not burn out.
- **`bg.frag`**: draws the logo card (two slots cross-fade when `sceneIndex` changes), zoomed to cover the screen, scaled by the logo-show opacity.
- **Logo show** (`Renderer::drawBackground`): a time-driven 14.5 s cycle (fade in 1 s, hold 3.5 s, fade out 1 s, black 9 s) producing `logoVis_`, which scales the card, trims wire gain and is sent to the post shader as `uLogoVis`. With no logo cards the show is simply absent.
- **`wire.*`**: object-space colour cycling and music-reactive highlights, or (`Frame::wireStyle = 1`) golden glimmer: twinkling gold dashes drawn without depth test so clouds accumulate light, with `uGold` in the post pass adding gold cross glints, tinting the light rays gold and turning warps and colour grading off.
- **`traveler.*`**: a small octahedron mesh with a per-vertex lane index; the vertex shader moves each traveller on its own path.
- **`post.frag`**, in order: UV warps (lensing, shockwave, barrel, wild event, chromatic aberration); bloom, anamorphic streak and light-ray taps; the procedural art background; logo-aware compositing; SDF sculpture, holographic stripes, glitch slices; scanlines, vignette, shadow, grain; colour grading; ACES tone map. Two uniforms gate the look: `hasLogo = max(uHasLogo * uLogoVis, 1 - uFxAmount)`. `uFxAmount` comes from `Config::fxAmount`; at 0 only clean tone-mapped wires remain.
- **`scroller.*`**: full-screen triangle; for each pixel it finds the glyph cell, reads the character code from a 512-texel strip texture, extracts the glyph bit with integer ops and writes premultiplied alpha.

### Platform notes
Windows only exports OpenGL 1.1 from `opengl32.dll`, so `GLWin32.hpp` declares the GL 4.1 functions the renderer uses as function pointers and `de::loadGL` fills them through `glfwGetProcAddress` right after the context is made current. macOS stops at OpenGL 4.1, so `glTexStorage2D` (4.2) and 1D textures are avoided, `glLineWidth > 1` is not used in the core profile, and stale GL errors are flushed before `glReadPixels`. Shaders are loaded at run time from a `shaders/` directory beside the binary (copied by `de_add_demo` after every link), so a shader edit needs a `make`. `Config::shaderDir` points a demo at its own shader set.

## Audio and timing
`AudioPlayer` renders a tracker module through libopenmpt in the SDL2 callback, splits the mix with one-pole IIR filters into bass (<350 Hz), mid and treble (>3 kHz), and publishes level, bands, order, pattern, row, tempo and speed through atomics read by the render thread. Without SDL2 and libopenmpt (`IW_HAS_AUDIO` undefined) the engine runs on `Timeline`, a pure function from seconds and BPM to beat, bar, phase and pulse.

## Effects library (`engine/src/Effects.cpp`)
`applyEffect(mesh, id, ctx, secondary)` runs one of 80 vertex warpers. An `EffectRecipe` chains up to six `EffectStage`s; `recipe(id)` gives 30 curated ones and `mutateRecipe(seed, depth)` derives deterministic procedural ones. `applyRecipe` restores the mesh if any stage yields an empty or invalid result, and edge growth is capped at 50 000. Demos opt in; the engine itself never calls it.

## Build system
`CMakeLists.txt` defines the targets above and the `de_add_demo(name sources...)` function, which links `de_engine` and copies the shaders next to the binary. Audio is optional and detected automatically. The `Makefile` wraps CMake with a stamp file so a changed toolchain triggers reconfiguration. Platform workarounds (macOS SDK pinning, `.tbd` stub avoidance, GLFW static-link fallback) are documented inline.

## Testing
`tests/geometry_tests.cpp` is a single executable of `req(cond, message)` checks that exits non-zero on the first failure. Visual layers are verified with `--screenshot PATH --frames N` captures, which also work in CI.
