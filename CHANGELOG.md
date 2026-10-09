# Changelog

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
