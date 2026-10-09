# Changelog

## 1.1.0 - Windows support
- In-tree OpenGL 4.1 function loader for Windows (`GLWin32.hpp/.cpp`): declares exactly the functions and constants the renderer uses and fills them via `glfwGetProcAddress`. Other platforms are unchanged.
- MSVC definitions (`NOMINMAX`, `WIN32_LEAN_AND_MEAN`, `_CRT_SECURE_NO_WARNINGS`) and warning policy; Windows CI job restored.
- Verification: the same loader and build changes pass build + tests on macOS, Linux and Windows in the public wireframe-scroller CI. This repo's CI could not start jobs (GitHub billing limit), and nothing has been run on physical Windows hardware yet.

## 1.0.1 - CI and build fixes
- Fixed a parallel-build race: engine shaders are now copied once by a shared `de_shaders` target instead of by every demo.
- Fixed a misleading-indentation error (GCC `-Werror`) and the vendored `stb_image_write` `sprintf` deprecation (Apple clang `-Werror`).
- MSVC warnings C4244/C4267 (intentional narrowing) silenced.
- Windows is documented as unsupported: the renderer needs an OpenGL function loader. The Windows CI job, which never built, was removed.
- CI is green on macOS 15 and Ubuntu 24.04.

## 1.0.0 - Initial engine release
Extracted from [Impossible Wireframe](https://github.com/djayuffe/wireframe-scroller) v4.34.

### Engine
- `de::Demo` interface and `de::run` run loop: window, GL 4.1 core context, clean shutdown, minimise handling, asset search, per-frame `FrameInfo` and `Input`.
- Generic command line (`--bpm --music --logos --window --fullscreen --quality --no-post --no-ticker --screenshot --frames --version --help`) that demos can extend; unknown options are now an error.
- `Config::fxAmount` to dial the signature post look from full to clean wires, `Config::shaderDir` for custom shader sets, `Frame::revision` to skip uploads of static meshes.
- Build split into `de_core` (no GL), `de_engine` (runtime) and demo targets, with a `de_add_demo()` CMake helper.

### Demos
- `hello_demo`: minimal cube-in-cube.
- `ripple_demo`: the README example, compiled.
- `impossible_wireframe` v5.0: the original demo reduced to a `de::Demo` (57 scenes, effect recipes, logo show). Behaviour unchanged.

### Docs
- README, ARCHITECTURE.md and docs/WRITING_A_DEMO.md.
