# Writing a demo

A demo is one class and one `main`. This guide walks from the smallest possible demo to using audio, assets, custom options and the effects library.

## 1. The smallest demo
`demos/hello/main.cpp` is the minimum:
```cpp
#include "Engine.hpp"
struct Hello : de::Demo {
  bool update(const de::FrameInfo&, const de::Input&, de::Frame& out) override {
    out.mesh = geo::cube(1.f);
    return true;
  }
};
int main(int argc, char** argv) { Hello d; return de::run(d, argc, argv); }
```
Add it to `CMakeLists.txt` inside the `if(OpenGL_FOUND AND glfw3_FOUND)` block:
```cmake
de_add_demo(hello demos/hello/main.cpp)
```
`make` then produces `build/hello`. `de_add_demo` links the engine and copies the shaders beside the binary. A demo that has its own content library links it too: `target_link_libraries(my_demo PRIVATE my_content)`.

## 2. Configuration
Override `config()` to describe the demo:

| Field | Default | Meaning |
|---|---|---|
| `title`, `version` | "Demo", "0.0" | Window title and `--version` output |
| `binaryName` | "demo" | Name shown in `--help` |
| `defaultBpm` | 132 | Tempo used until `--bpm` or music takes over |
| `logoDirs` | empty | Directories of `*_1920x1080.jpg` cards for the logo show |
| `musicFiles` | empty | Default tracker modules or wavs; the first one found plays |
| `shaderDir` | engine shaders | Point at your own shader set to replace the look |
| `logoShow` | true | `false` never draws cards |
| `fxAmount` | 1 | 0 to 1 strength of the post look: art background, wild warps, lensing. 0 gives clean wires |

Asset paths are tried relative to the CWD, the executable directory and every ancestor of it, so `"demos/my/assets/music/a.mod"` works from the repo root and from `build/`.

## 3. The frame
`update(const FrameInfo& f, const Input& in, Frame& out)` runs once per frame.

`FrameInfo` gives you:
- `time`: show time in seconds (the music clock when music plays);
- `dt`, `frame`, `width`, `height`, `bpm`;
- `sync`: `beat`, `bar`, `beatPhase`, `barPhase`, `beatIndex`, `barIndex` and `pulse`, a sharp 0 to 1 spike on every beat that is also lifted by the music level;
- `music`: when `music.active`, real `bass`, `mid`, `treble`, `level`, plus `order`, `pattern`, `row`, `tempo`, `speed`.

`Input` has `down(key)` and `pressed(key)` (rising edge) using `de::Key::*` codes. Return `false` from `update` to end the demo.

`Frame` is your output: `mesh`, `revision`, `sceneIndex`, `scale`, `lineWidth`, `travelers`, `ticker`. The engine fits the mesh to the screen, so `scale` is a multiplier around 1, ideal for beat pulses.

### Mesh rules
Vertices must be finite, edge indices in range, no self edges, no duplicate edges. Build meshes with `geo::` helpers (`cube`, `uvSphere`, `torus`, 4D polytopes and `project4D`) or fill `Mesh3::v` and `Mesh3::e` yourself, and check with `geo::validate(mesh)` in a test. A rejected mesh ends the demo with exit code 5.

### Static meshes
If the mesh does not change, set `out.revision` to a non-zero value that you bump only when it changes. The engine then skips the upload. Leave it at 0 for animated meshes.

## 4. Command-line options
The engine handles `--bpm --music --logos --window --fullscreen --quality --no-post --no-ticker --screenshot --frames --version --help`. Add your own by overriding `parseArg` (return true when you consume argv[i], advancing `i` past any value) and `usage` (extra help lines):
```cpp
bool parseArg(const std::string& a, int& i, int argc, char** argv) override {
  if (a == "--level" && i + 1 < argc) { level_ = std::atoi(argv[++i]); return true; }
  return false;
}
std::string usage() const override { return "  --level N       Start on level N\n"; }
```

## 5. Using the effects library
`#include "Effects.hpp"` gives 80 warpers and 30 recipes:
```cpp
EffectContext ec; ec.time = float(f.time); ec.beat = f.sync.pulse; ec.amount = 1.f + .6f * f.sync.pulse;
ec.bass = f.music.bass; ec.mid = f.music.mid; ec.treble = f.music.treble; ec.seed = 1234;
applyRecipe(out.mesh, recipe(3), ec);   // restores the mesh if a stage fails
```
`mutateRecipe(seed, depth)` makes deterministic procedural recipes. See `demos/wireframe/src/main.cpp` for the full pattern.

## 6. Logos and music
Put `*_1920x1080.jpg` cards in a directory, list it in `logoDirs`, and the 14.5 s logo show runs on top of your mesh (1 s fade in, 3.5 s hold, 1 s fade out, 9 s black). List tracker modules (`.mod .xm .it .s3m`) or wavs in `musicFiles`. Both are optional; without them the engine runs on the BPM clock.

## 7. Testing and screenshots
- Put GL-free logic in its own library (see `wireframe_content`) and test it headless.
- Capture any frame in CI or for docs: `./build/my_demo --window 960x540 --frames 300 --screenshot out.png`.
- `--no-post` bypasses the FX chain; `--quality 0.5` renders at half resolution.

## 8. Shaders
Shaders live in `engine/shaders/`. To customise the look for one demo, copy that directory next to your demo, edit `post.frag`, and set `Config::shaderDir`. The shader set must contain `wire`, `bg`, `traveler`, `scroller` and `post` programs (`.vert` and `.frag`). Run `make` after edits so they are copied beside the binary.

## 9. Checklist for a new demo
1. Copy `demos/hello` to `demos/<name>`; edit `config()` and `update()`.
2. Add `de_add_demo(<name> demos/<name>/main.cpp)` to `CMakeLists.txt`.
3. `make && ./build/<name> --window 960x540 --frames 120 --screenshot /tmp/a.png` and look at it.
4. Add a test for any mesh generator, then `make test`.
