#pragma once
// Demo engine public API.
//
// A demo is a class derived from de::Demo. The engine owns everything else: the window and GL
// context, the HDR render target and post pass, audio playback and analysis, tempo sync, the logo
// show, the text ticker, screenshots and the command line. Each frame the engine asks the demo
// for a wire mesh (de::Frame) and draws it through the shared pipeline.
//
//   struct MyDemo : de::Demo {
//     de::Config config() const override { de::Config c; c.title="My Demo"; return c; }
//     void update(const de::FrameInfo& f, const de::Input& in, de::Frame& out) override { ... }
//   };
//   int main(int argc,char**argv){ MyDemo d; return de::run(d,argc,argv); }
#include "Audio.hpp"
#include "Geometry.hpp"
#include "Timeline.hpp"
#include <cstdint>
#include <string>
#include <vector>
struct GLFWwindow;
namespace de {

// GLFW key codes (stable across GLFW 3.x) so demos need not include GLFW.
namespace Key {
constexpr int Space=32, Escape=256, Right=262, Left=263, Down=264, Up=265;
constexpr int A=65, B=66, C=67, D=68, E=69, F=70, G=71, H=72, I=73, J=74, K=75, L=76, M=77, N=78, O=79, P=80,
              Q=81, R=82, S=83, T=84, U=85, V=86, W=87, X=88, Y=89, Z=90;
constexpr int Num0=48, Num1=49, Num2=50, Num3=51, Num4=52, Num5=53, Num6=54, Num7=55, Num8=56, Num9=57;
}

// Static description of a demo. Returned by Demo::config().
struct Config {
  std::string title="Demo";            // window title
  std::string version="0.0";           // printed by --version
  std::string binaryName="demo";       // used in --help
  double defaultBpm=132.0;            // used when no --bpm is given and the music has no tempo of its own (WAV, none)
  // Asset search: each entry is tried relative to the CWD, the executable directory and every
  // ancestor of it, so a demo works from the repo root or from build/.
  std::vector<std::string> logoDirs;   // directories holding *_1920x1080.jpg logo cards (optional)
  std::vector<std::string> musicFiles; // default playlist: every one found plays, in order, looping
  std::string shaderDir;               // empty = the engine's shaders next to the executable
  bool logoShow=true;                  // false = never draw logo cards even if found
  float fxAmount=1.f;                  // 0..1 strength of the signature post look (art, warps, lensing)
};

// Everything the engine knows about the current frame.
struct FrameInfo {
  double time=0;          // show time in seconds: music clock when music plays, wall clock otherwise
  double dt=0;            // seconds since the previous frame
  int width=0, height=0;  // framebuffer size in pixels
  int frame=0;            // frames rendered so far
  double bpm=132.0;
  SyncState sync;         // beat/bar position; sync.pulse is already max(beat pulse, music level)
  MusicState music;       // real band energies when music is active; music.track = playlist index
  std::string musicPath;  // file of the track playing now (changes as the playlist advances; use it for credits)
};

// Keyboard state. Key codes are de::Key::* (GLFW codes).
class Input {
public:
  bool down(int key) const { return key>=0&&key<kKeys&&cur_[key]; }
  bool pressed(int key) const { return key>=0&&key<kKeys&&cur_[key]&&!prev_[key]; }   // rising edge this frame
  void poll(GLFWwindow* w);   // engine use
private:
  static constexpr int kKeys=349;
  bool cur_[kKeys]={}, prev_[kKeys]={};
};

// What a demo hands back each frame.
struct Frame {
  Mesh3 mesh;                  // the wire mesh to draw this frame
  uint64_t revision=0;         // 0 = always re-upload; otherwise only upload when it changes
  int sceneIndex=0;            // selects the logo card; also seeds per-scene variation in the post pass
  float scale=1.f;             // extra scale on top of the engine's auto-fit
  float lineWidth=1.f;
  bool travelers=true;         // draw the octahedra weaving through the mesh
  int wireStyle=0;             // 0 = electric palette, 1 = golden glimmer (additive, no depth test, gold glints)
  std::string ticker;          // text for the ticker band; empty = nothing
};

// Information passed to Demo::init.
struct RunInfo {
  double bpm=132.0;          // --bpm, else the module's own tempo, else Config::defaultBpm
  std::string executableDir;
  std::string musicPath;     // the first track; empty when silent. FrameInfo::musicPath follows the playlist
};

class Demo {
public:
  virtual ~Demo() = default;
  virtual Config config() const { return {}; }
  // Consume a demo-specific command-line option. Return true if argv[i] (and any value, advancing
  // i) was handled. Return false to let the engine report it as unknown.
  virtual bool parseArg(const std::string& arg, int& i, int argc, char** argv) { (void)arg;(void)i;(void)argc;(void)argv; return false; }
  // Extra lines appended to --help (one option per line, already formatted).
  virtual std::string usage() const { return {}; }
  virtual bool init(const RunInfo&) { return true; }
  // Called once per frame. Fill `out`. Return false to end the demo.
  virtual bool update(const FrameInfo& f, const Input& in, Frame& out) = 0;
};

// Parses the command line, opens the window and runs until the user quits.
// Returns a process exit code. Engine options: --bpm --music --no-ticker --no-post --quality
// --logos --window --fullscreen --screenshot --frames --version --help.
int run(Demo& demo, int argc, char** argv);

}  // namespace de
