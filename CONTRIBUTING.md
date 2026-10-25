# Contributing

Report reproducible bugs and propose focused changes through GitHub issues and pull requests. Search existing issues first. For larger changes, describe the intended behavior and performance budget before implementing them.

## Build and check

Requirements: C++20, CMake 3.20+, OpenGL 4.1 and GLFW. SDL2/libopenmpt enable audio; optional codec support is described in [music documentation](docs/MUSIC_AND_BEATS.md).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DIW_BUILD_TESTS=ON -DIW_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For rendering changes, run the relevant native checks on a machine with a GPU/display:

```sh
python3 tools/gpu_smoke.py --build build
python3 tools/validate_infinity.py --out build/infinity-validation
python3 tools/validate_atlas.py --out build/atlas-validation
```

The full graphics checks can take several minutes. Include the OS, GPU/driver, scene, command, frame budget and before/after captures in rendering reports. Measure performance with matching render budgets and settings; do not infer speedups from code changes alone.

## Code and assets

Follow the surrounding C++/GLSL style, keep work bounded in shader loops, validate numerical domains and preserve deterministic capture options. Add meaningful regression coverage for behavior changes. Document new controls and scene mappings in the appropriate demo README.

Keep changes focused. Preserve imported reference files and their recorded hashes. Attribute imported code and assets, identify their license and update the relevant source record. Do not assume the engine MIT license covers an imported contribution; see [LICENSES.md](LICENSES.md).

## Pull request checklist

- Explain the problem and resulting behavior.
- State which checks ran and any hardware limitations.
- Include screenshots for visible changes and measurements for performance claims.
- Update controls, credits and documentation when affected.
- Exclude build output, local music and incidental capture files.
