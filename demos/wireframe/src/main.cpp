// Impossible Wireframe: the flagship demo built on the engine.
// Scenes, effect recipes and the ticker text live here; everything else is the engine.
#include "Engine.hpp"
#include "MusicCatalog.hpp"
#include "Effects.hpp"
#include "Scene.hpp"
#include "Choreography.hpp"
#include "Describe.hpp"
#include "Transition.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <tuple>
namespace {
constexpr const char* kVersion = "v5.1";

class WireframeDemo final : public de::Demo {
public:
  de::Config config() const override {
    de::Config c;
    c.title = "Impossible Wireframe";
    c.version = kVersion;
    c.binaryName = "impossible_wireframe";
    c.logoDirs = {"demos/wireframe/assets/logos", "assets/logos", "UBER_Fullscreen_Logo_Pack"};
    // Default soundtrack: "Stars (4ch. version)" by cs127, CC BY 4.0 (see assets/music/README.md).
    // Playlist: "Stars (4ch. version)" by cs127 (CC BY 4.0), then "Digital Rendezvous" by Drozerix (Public Domain).
    c.musicFiles = musiccat::playlist();   // S skips to the next track
    return c;
  }
  bool parseArg(const std::string& arg, int& i, int argc, char** argv) override {
    if (arg == "--cpu-effects") { cpuEffects_ = true; return true; }
    if (arg == "--scene" && i + 1 < argc) {
      manual_ = std::atoi(argv[++i]);
      return true;
    }
    if (arg == "--recipe" && i + 1 < argc) {
      recipeArg_ = std::atoi(argv[++i]);
      haveRecipe_ = true;
      if (recipeArg_ < 0 || recipeArg_ >= recipeCount()) {
        std::fprintf(stderr, "--recipe N must be in [0,%d] (got %d)\n", recipeCount() - 1, recipeArg_);
        std::exit(2);
      }
      return true;
    }
    return false;
  }
  std::string usage() const override {
    return "  --cpu-effects   Use CPU deformation reference path\n"
           "  --scene N       Start on scene N in manual mode (0-72; Space returns to auto)\n"
           "  --recipe N      Start in recipe mode with curated recipe N (0-29)\n"
           "Keys: Left/Right cycle scenes, Space back to auto, R recipe mode,\n"
           "      Up/Down cycle recipe, P re-roll mutation,\n"
           "      N noncommutative camera, Y temporal-refraction sphere, M memory layer,\n"
           "      1-5 chromatic gravity / prediction failure / colorless matter / specular mutation / negative reflection\n";
  }
  bool init(const de::RunInfo& ri) override {
    recipeMode_ = haveRecipe_;
    recipeChoice_ = haveRecipe_ ? recipeArg_ : 0;
    (void)ri;
    return true;
  }

  bool update(const de::FrameInfo& f, const de::Input& in, de::Frame& out) override {
    const int n = scenes_.count();
    const int autoScene = int(f.sync.barIndex / 2) % n;
    // Normalise the current scene before applying the delta so Left on scene 0 wraps to the last one.
    const int cur = manual_ < 0 ? autoScene : ((manual_ % n) + n) % n;
    for (auto [key, flag, name] : {std::tuple{de::Key::N, &forceShear_, "noncommutative camera"},
                                    std::tuple{de::Key::Y, &forceTemporal_, "temporal refraction"},
                                    std::tuple{de::Key::M, &forceMemory_, "memory layer"}})
      if (in.pressed(key)) {
        *flag = !*flag;
        std::fprintf(stdout, "%s: %s\n", name, *flag ? "on" : "off");
      }
    for (int i = 0; i < 5; ++i)
      if (in.pressed(de::Key::Num1 + i)) {
        forceLook_[i] = !forceLook_[i];
        static const char* const names[5] = {"chromatic gravity", "prediction failure", "colorless matter",
                                             "specular mutation", "negative reflection"};
        std::fprintf(stdout, "%s: %s\n", names[i], forceLook_[i] ? "on" : "off");
      }
    if (in.pressed(de::Key::Left)) manual_ = (cur - 1 + n) % n;
    if (in.pressed(de::Key::Right)) manual_ = (cur + 1) % n;
    if (in.down(de::Key::Space)) manual_ = -1;
    const int scene = manual_ < 0 ? autoScene : manual_;
    // Never wait for a rebuild: the newest finished mesh is shown while the next tick builds off-thread,
    // and the scenes that can come next start building before they are needed.
    Mesh3 m = scenes_.meshAsync(scene, f.effectTime, seed_);
    for (int nb : {(autoScene + 1) % n, (scene + 1) % n, (scene - 1 + n) % n}) scenes_.prefetch(nb, f.effectTime, seed_);
    if (scene != lastScene_) {
      sceneChangedAt_ = f.time;
      auto& si = scenes_.info(scene);
      std::fprintf(stdout, "scene %02d: %.*s [%.*s]\n", scene, int(si.name.size()), si.name.data(),
                   int(si.provenance.size()), si.provenance.data());
      lastScene_ = scene;
    }
    if (in.pressed(de::Key::R)) {
      recipeMode_ = !recipeMode_;
      std::fprintf(stdout, "effects: %s\n",
                   recipeMode_ ? "recipe mode (curated, Up/Down to cycle)" : "auto (per-scene curated + mutation)");
    }
    if (recipeMode_) {
      if (in.pressed(de::Key::Up)) recipeChoice_ = (recipeChoice_ + 1) % recipeCount();
      if (in.pressed(de::Key::Down)) recipeChoice_ = (recipeChoice_ - 1 + recipeCount()) % recipeCount();
    }
    if (in.pressed(de::Key::P)) {
      baseSeed_ ^= 0x9e3779b9u;
      std::fprintf(stdout, "effects: mutation seed re-rolled (0x%08x)\n", baseSeed_);
    }

    EffectContext ec;
    ec.time = float(f.effectTime);
    ec.amount = 1.0f + .6f * f.sync.pulse;
    ec.beat = f.sync.pulse;
    if (f.music.active) {
      ec.bass = f.music.bass;
      ec.mid = f.music.mid;
      ec.treble = f.music.treble;
    } else {
      ec.bass = .5f + .5f * std::sin(float(f.time) * 2.0f);
      ec.mid = .5f + .5f * std::sin(float(f.time) * 3.3f + 1.1f);
      ec.treble = .5f + .5f * std::sin(float(f.time) * 5.7f + 2.3f);
    }
    ec.seed = baseSeed_ + uint32_t(scene) * 131u;
    EffectRecipe rc;
    bool pureForm = false;
    if (recipeMode_)
      rc = recipe(recipeChoice_);
    else if (haveRecipe_)
      rc = recipe(recipeArg_ % recipeCount());
    else {
      // Auto: the scene's curated recipe plus a two-stage deterministic mutation, in a SEPARATE buffer.
      auto base = recipe(scene % recipeCount());
      auto mut = mutateRecipe(ec.seed, 2);
      rc = EffectRecipe{};
      rc.name = "Auto";
      int i = 0;
      for (int k = 0; k < base.stageCount && i < 6; k++) rc.stages[i++] = base.stages[k];
      // Showpiece objects (scene >= 51) are meant to be read as clean shapes: no mutation overlay,
      // and the warp swells in and out so the pure object shows for part of every ~12 s cycle.
      if (scene >= 51) {
        float env = .5f - .5f * std::cos(float(f.effectTime) * .52f);
        pureForm = env < .45f;
        ec.amount *= .25f + .75f * env;
      } else
        for (int k = 0; k < mut.stageCount && i < 6; k++) rc.stages[i++] = mut.stages[k];
      rc.stageCount = i;
    }
    std::string recipeName = rc.name;
    // The secondary mesh (next scene) is only built when a stage actually bridges to it.
    const Mesh3* secondary = nullptr;
    for (int k = 0; k < rc.stageCount; k++)
      if (rc.stages[k].useSecondary) {
        secondary = m.v.empty() ? nullptr : &scenes_.meshAsync((scene + 1) % n, f.effectTime, seed_);
        break;
      }
    // The golden orbit clouds (57-58) are never warped in auto mode: their motion is the attractor's own morph.
    if ((scene == 57 || scene == 58) && !recipeMode_ && !haveRecipe_)
      recipeName = "Golden glimmer";
    else if (pureForm && !recipeMode_ && !haveRecipe_)
      recipeName = "Pure form";
    else if (cpuEffects_)
      applyRecipe(m, rc, ec, secondary);
    else
      applyRecipeGpu(m, rc, ec, out.gpuEffect, secondary);

    out.mesh = std::move(m);
    out.sceneIndex = scene;
    {
      const choreo::Look look = choreo::forScene(scene, f.effectTime, forceShear_, forceTemporal_, forceMemory_, forceLook_);
      std::copy(look.shear.m, look.shear.m + 9, out.modelTransform);
      out.temporalRefraction = look.temporal;
      out.memory = look.memory;
      std::copy(look.look, look.look + 5, out.look);
    }
    out.scale =
        1.f + .11f * std::sin(float(f.effectTime) * .41f + float(scene) * .37f) + .07f * f.sync.pulse; // breathing size cycle
    out.lineWidth = 1.f + f.sync.pulse;
    {   // scene flow: exit rush toward the camera, entry grow-in from 80% and a thicker stroke on the downbeat
      const transition::Cut cut = transition::at(f.sync.barIndex, f.sync.barPhase, 2, manual_ < 0, f.time - sceneChangedAt_,
                                                 60.0 / std::max(1.0, double(f.bpm)));
      out.cutIn = cut.in;
      out.cutOut = cut.out;
      out.scale *= (1.f + .35f * cut.out * cut.out) * (1.f - .2f * cut.in);
      out.lineWidth += 1.5f * cut.in;
    }
    if (scene == 57 || scene == 58) {
      out.wireStyle = 1;
      out.travelers = false;
      out.scale *= .8f;
    } // golden orbit clouds: dense right out to the rim
    auto& si = scenes_.info(scene);
    out.ticker = describeScene(scene, si.name, si.provenance, recipeName, f.bpm, creditFor(f.musicPath));
    return true;
  }

private:
  SceneSystem scenes_;
  uint64_t seed_ = 0x49574f424a454354ull;
  int manual_ = -1, lastScene_ = -1;
  double sceneChangedAt_ = -100.0;
  bool forceShear_ = false, forceTemporal_ = false, forceMemory_ = false;
  bool forceLook_[5] = {false, false, false, false, false};
  bool cpuEffects_ = false;
  bool haveRecipe_ = false, recipeMode_ = false;
  int recipeArg_ = 0, recipeChoice_ = 0;
  uint32_t baseSeed_ = 0x50454646u;
  // On-screen credit for the bundled tracks, keyed by file name.
  static std::string creditFor(const std::string& path) {
    return musiccat::credit(path);
  }
};
} // namespace

int main(int argc, char** argv) {
  WireframeDemo d;
  return de::run(d, argc, argv);
}
