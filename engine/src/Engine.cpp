#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath
#elif defined(_WIN32)
#include "GLWin32.hpp" // windows.h (GetModuleFileNameA) + GL 1.1 + the GL 4.1 function pointers
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include "Engine.hpp"
#include "AdaptiveRes.hpp"
#include "Director.hpp"
#include "BackgroundStyle.hpp"
#include "RenderScale.hpp"
#include "Renderer.hpp"
#include "WindowSpec.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
namespace de {
namespace fs = std::filesystem;
namespace {
// Set by the SIGINT/SIGTERM handler; the render loop checks it each frame so Ctrl-C / kill
// produces a clean shutdown instead of a hard kill that leaks GL resources.
std::atomic<bool> g_shutdown{false};
void onSignal(int) {
  g_shutdown.store(true);
}

fs::path executablePath() {
  fs::path exe;
#if defined(__APPLE__)
  {
    char buf[4096];
    uint32_t sz = sizeof(buf);
    if (_NSGetExecutablePath(buf, &sz) == 0) {
      std::error_code ec;
      exe = fs::canonical(buf, ec);
    }
  }
#elif defined(__linux__)
  {
    std::error_code ec;
    exe = fs::canonical("/proc/self/exe", ec);
  }
#elif defined(_WIN32)
  {
    char buf[4096];
    DWORD n = GetModuleFileNameA(nullptr, buf, sizeof buf);
    if (n > 0 && n < sizeof buf) {
      std::error_code ec;
      exe = fs::canonical(buf, ec);
    }
  }
#endif
  return exe;
}

// Every place an asset named `rel` could live: CWD, the executable dir, then each ancestor of it.
std::vector<fs::path> assetCandidates(const std::string& rel, const fs::path& exeDir) {
  std::vector<fs::path> c;
  c.push_back(fs::current_path() / rel);
  if (!exeDir.empty()) {
    c.push_back(exeDir / rel);
    auto p = exeDir;
    while (p.has_parent_path() && p.parent_path() != p) {
      p = p.parent_path();
      c.push_back(p / rel);
    }
  }
  return c;
}
} // namespace

void Input::poll(GLFWwindow* w) {
  for (int k = 0; k < kKeys; ++k) {
    prev_[k] = cur_[k];
    // Only query real GLFW key codes; the gaps in the code space would raise GLFW_INVALID_ENUM.
    const bool valid = k == GLFW_KEY_SPACE || k == GLFW_KEY_APOSTROPHE || (k >= GLFW_KEY_COMMA && k <= GLFW_KEY_9) ||
                       k == GLFW_KEY_SEMICOLON || k == GLFW_KEY_EQUAL ||
                       (k >= GLFW_KEY_A && k <= GLFW_KEY_RIGHT_BRACKET) || k == GLFW_KEY_GRAVE_ACCENT ||
                       k >= GLFW_KEY_ESCAPE;
    cur_[k] = valid && glfwGetKey(w, k) == GLFW_PRESS;
  }
}

int run(Demo& demo, int argc, char** argv) {
  const Config cfg = demo.config();
  double bpm = cfg.defaultBpm;
  bool bpmGiven = false;
  std::vector<fs::path> musicPaths;
  fs::path logoOverride;
  int bgStyle = 0;
  bool fixedPointer = false;   // --pointer X,Y: pin the pointer (screenshots, parallax checks)
  float fixedX = 0, fixedY = 0;
  std::string hideList;       // --hide: objects to start switched off
  bool noGloss = false;
  double fixedStep = 0;   // --fixed-step S: advance the show clock S seconds per frame
  bool adaptive = true;      // frame-time governor for the background resolution (--no-adaptive)
  bool captureNow = false;   // key C: save the next frame as demo-<date>-<time>.png
  bool noDepth = false, autoStyle = false;   // autoStyle: cycle the background styles by themselves
  bool parallaxGiven = false;
  double parallax = 1.6;   // gain on every pointer-driven depth offset (--parallax, key P)
  bool noTicker = false, noPost = false, noBackground = false, noLogo = false;
  double quality = 1.0;
  bool qualityGiven = false;
  int winW = 0, winH = 0;
  bool fullscreen = false;
  std::string screenshotPath;
  int maxFrames = 0;
  bool vsync = true, showStats = false;
  const auto helpText = [&]() {
    std::fprintf(stdout,
                 "%s %s\n"
                 "Usage: %s [options]\n"
                 "  --bpm N         Tempo (default: the module's own tempo, else %g)\n"
                 "  --music PATH    Module or WAV to play; repeat for a playlist\n"
                 "  --no-ticker     Start with the text ticker off (toggle with T)\n"
                 "  --no-post       Skip the screen-space FX post pass (faster; debugging)\n"
                 "  --no-background Start with the procedural background off (toggle with B)\n"
                 "  --background-style S  auto|classic|synthwave|nebula|mandala|blackhole|hyperspace (cycle with V)\n"
                 "  --parallax F    Depth-parallax gain 0-3 (default 1.6; cycle with G)\n"
                 "  --pointer X,Y   Pin the pointer, -1..1 (screenshots, parallax checks)\n"
                 "  --fixed-step S  Deterministic clock: each frame advances the show by S s (captures, A/B)\n"
                 "  --no-gloss      Start with the gloss finish (specular sweep, glints) off (toggle with O)\n"
                 "  --hide A,B,..   Start with background objects off, e.g. sun,grid,jets (F1-F12 toggle live)\n"
                 "  --no-adaptive   Keep the background at fixed half resolution (default: governed by frame time)\n"
                 "  --no-depth      Start with the depth-mote layer off (toggle with D)\n"
                 "  --no-logo       Start with the logo cards off (toggle with L)\n"
                 "  --quality F     Render the HDR target at F of screen resolution (0.25-1.0)\n"
                 "  --logos DIR     Directory of *_1920x1080.jpg logo cards\n"
                 "  --window WxH    Initial window size (320x200 .. 3840x2160, default 1440x900)\n"
                 "  --fullscreen    Full-screen on the primary monitor (overrides --window)\n"
                 "  --screenshot P  Save one PNG frame to P (on frame N when --frames N is given)\n"
                 "  --frames N      Exit after N frames (0 = run forever)\n"
                 "  --stats         Print fps and CPU time per frame (demo update + upload) every 5 s\n"
                 "  --no-vsync      Do not wait for the display refresh (benchmarking)\n"
                 "  --version       Print version and exit\n"
                 "  -h, --help      Show this help\n"
                 "%s"
                 "Keys: T ticker, B background, V background style, D depth layer, O gloss, F1-F12 background objects, G parallax gain, C capture PNG, H key list, mouse steers the backgrounds (click = shockwave), L logo cards, Esc quits\n",
                 cfg.title.c_str(), cfg.version.c_str(), cfg.binaryName.c_str(), cfg.defaultBpm, demo.usage().c_str());
  };
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (demo.parseArg(arg, i, argc, argv)) continue;
    if (arg == "--bpm" && i + 1 < argc) {
      double v = std::atof(argv[++i]);
      if (!std::isfinite(v)) {
        std::fprintf(stderr, "--bpm: not a finite number\n");
        return 2;
      }
      bpm = std::max(1.0, v);
      bpmGiven = true;
    } else if (arg == "--music" && i + 1 < argc)
      musicPaths.push_back(argv[++i]); // repeat for a playlist
    else if (arg == "--no-ticker" || arg == "--no-scroller")
      noTicker = true;
    else if (arg == "--no-post")
      noPost = true;
    else if (arg == "--no-background")
      noBackground = true;
    else if (arg == "--parallax" && i + 1 < argc) {
      const double v = std::atof(argv[++i]);
      if (!std::isfinite(v)) {
        std::fprintf(stderr, "--parallax: not a finite number\n");
        return 2;
      }
      parallax = std::clamp(v, 0.0, 3.0);
      parallaxGiven = true;
    } else if (arg == "--pointer" && i + 1 < argc) {
      float px = 0, py = 0;
      if (std::sscanf(argv[++i], "%f,%f", &px, &py) != 2 || !std::isfinite(px) || !std::isfinite(py)) {
        std::fprintf(stderr, "--pointer: expected X,Y in -1..1\n");
        return 2;
      }
      fixedPointer = true;
      fixedX = std::clamp(px, -1.f, 1.f);
      fixedY = std::clamp(py, -1.f, 1.f);
    } else if (arg == "--fixed-step" && i + 1 < argc) {
      const double v = std::atof(argv[++i]);
      if (!std::isfinite(v) || v <= 0 || v > 1) {
        std::fprintf(stderr, "--fixed-step: seconds per frame in (0, 1]\n");
        return 2;
      }
      fixedStep = v;
    } else if (arg == "--no-gloss")
      noGloss = true;
    else if (arg == "--hide" && i + 1 < argc)
      hideList = argv[++i];
    else if (arg == "--no-adaptive")
      adaptive = false;
    else if (arg == "--no-depth")
      noDepth = true;
    else if (arg == "--background-style" && i + 1 < argc) {
      const std::string v = argv[++i];
      const int st = bgstyle::parse(v);
      if (st == -1) autoStyle = true;
      else if (st >= 0) bgStyle = st;
      else {
        std::fprintf(stderr, "--background-style: use auto|classic|synthwave|nebula|mandala|blackhole|hyperspace (or 0-5)\n");
        return 2;
      }
    } else if (arg == "--no-logo")
      noLogo = true;
    else if (arg == "--quality" && i + 1 < argc) {
      double v = std::atof(argv[++i]);
      if (!std::isfinite(v)) {
        std::fprintf(stderr, "--quality: not a finite number\n");
        return 2;
      }
      quality = std::clamp(v, 0.25, 1.0);
      qualityGiven = true;
    } else if (arg == "--logos" && i + 1 < argc)
      logoOverride = argv[++i];
    else if (arg == "--window" && i + 1 < argc) {
      std::string werr;
      int ww, wh;
      if (!parseWindowSpec(argv[++i], ww, wh, werr)) {
        std::fprintf(stderr, "--window: %s\n", werr.c_str());
        return 2;
      }
      winW = ww;
      winH = wh;
    } else if (arg == "--fullscreen")
      fullscreen = true;
    else if (arg == "--no-vsync")
      vsync = false;
    else if (arg == "--stats")
      showStats = true;
    else if (arg == "--version") {
      std::fprintf(stdout, "%s %s\n", cfg.title.c_str(), cfg.version.c_str());
      return 0;
    } else if (arg == "--screenshot" && i + 1 < argc)
      screenshotPath = argv[++i];
    else if (arg == "--frames" && i + 1 < argc)
      maxFrames = std::max(0, std::atoi(argv[++i]));
    else if (arg == "--help" || arg == "-h") {
      helpText();
      return 0;
    } else {
      std::fprintf(stderr, "unknown or incomplete option: %s (try --help)\n", arg.c_str());
      return 2;
    }
  }
  std::signal(SIGINT, onSignal);
  std::signal(SIGTERM, onSignal);

  const fs::path exe = executablePath();
  const fs::path exeDir = exe.has_parent_path() ? exe.parent_path() : fs::path();
  if (musicPaths.empty()) {
    for (auto& m : cfg.musicFiles) {
      for (auto& c : assetCandidates(m, exeDir)) {
        std::error_code ec;
        if (fs::exists(c, ec)) {
          musicPaths.push_back(c);
          break;
        }
      }
    }
  }

  if (!glfwInit()) {
    std::fprintf(stderr, "GLFW initialization failed\n");
    return 1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  GLFWmonitor* mon = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
  const GLFWvidmode* vm = mon ? glfwGetVideoMode(mon) : nullptr;
  int cw, ch;
  if (fullscreen) {
    cw = vm ? vm->width : 1920;
    ch = vm ? vm->height : 1080;
  } else {
    cw = winW > 0 ? winW : 1440;
    ch = winH > 0 ? winH : 900;
  }
  const std::string windowTitle = cfg.title + " " + cfg.version;
  GLFWwindow* w = glfwCreateWindow(cw, ch, windowTitle.c_str(), mon, nullptr);
  if (!w) {
    std::fprintf(stderr, "OpenGL 4.1 context creation failed\n");
    glfwTerminate();
    return 2;
  }
  if (fullscreen) glfwSetWindowMonitor(w, mon, 0, 0, cw, ch, vm ? vm->refreshRate : 0);
  glfwMakeContextCurrent(w);
  glfwSwapInterval(vsync ? 1 : 0); // vsync in full-screen too: uncapped only burned the GPU and tore
#ifdef _WIN32
  {
    const char* missing = nullptr;
    if (!loadGL(reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress), &missing)) {
      std::fprintf(stderr, "OpenGL loader: missing %s (needs an OpenGL 4.1 driver)\n", missing ? missing : "?");
      glfwDestroyWindow(w);
      glfwTerminate();
      return 2;
    }
  }
#endif

  AudioPlayer audio;
#if defined(IW_HAS_AUDIO)
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) std::fprintf(stderr, "SDL audio init failed: %s\n", SDL_GetError());
#endif
  if (!musicPaths.empty() && audio.open(musicPaths)) {
    // Lock the visuals to the first module's own tempo unless --bpm was given.
    if (!bpmGiven && audio.nativeBpm(0) > 0) bpm = audio.nativeBpm(0);
    for (int i = 0; i < audio.trackCount(); ++i)
      std::fprintf(stdout, "music %d/%d: %s (%.0f BPM)\n", i + 1, audio.trackCount(),
                   audio.trackPath(i).string().c_str(), audio.nativeBpm(i) > 0 ? audio.nativeBpm(i) : bpm);
  }

  std::string shaderDir = cfg.shaderDir.empty() ? "shaders" : cfg.shaderDir;
  if (!exeDir.empty() && cfg.shaderDir.empty()) {
    auto cand = exeDir / "shaders";
    if (fs::exists(cand / "wire.vert")) shaderDir = cand.string();
  }
  const auto cleanup = [&](Renderer* r) {
    if (r) r->shutdown();
    audio.close();
#if defined(IW_HAS_AUDIO)
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
#endif
    glfwDestroyWindow(w);
    glfwTerminate();
  };
  Renderer r;
  if (!r.init(w, shaderDir)) {
    std::fprintf(stderr, "Renderer init failed: %s\n", r.error().c_str());
    cleanup(nullptr);
    return 3;
  }
  r.setRenderScale(float(quality));
  // Default: cap the HDR target to a pixel budget (Retina/4K); an explicit --quality means "exactly this".
  r.setMaxHdrPixels(qualityGiven ? 0.0 : rscale::kDefaultBudget);
  r.setPostEnabled(!noPost);
  r.setShowBackground(!noBackground);
  r.setBackgroundStyle(bgStyle);
  r.setShowDepth(!noDepth);
  r.setShowGloss(!noGloss);
  if (!hideList.empty())
    for (const auto& u : r.objects().hide(hideList)) std::fprintf(stderr, "--hide: no background object named '%s'\n", u.c_str());
  adaptive::Governor governor;
  double ptrT = fixedStep > 0 ? 1.0 : glfwGetTime();
  director::Director dj;
  if (autoStyle) r.setBackgroundStyle(1);
  bool djStarted = false;
  dj.objectCount = &bgstyle::elementCount;
  bgstyle::Objects userObjects = r.objects();   // the --hide / F-key choices; the director composes on top of them
  float ptrX = 0, ptrY = 0, ptrPulse = 0;
  bool mouseWas = false;
  r.setShowLogos(!noLogo);
  r.setFxAmount(cfg.fxAmount);
  if (cfg.logoShow) {
    std::vector<fs::path> cands;
    if (!logoOverride.empty()) cands.push_back(logoOverride);
    for (auto& d : cfg.logoDirs)
      for (auto& c : assetCandidates(d, exeDir)) cands.push_back(c);
    bool loaded = false;
    for (auto& c : cands) {
      if (r.loadLogos(c.string())) {
        loaded = true;
        std::fprintf(stdout, "logos: %d cards from %s\n", r.logoCount(), c.string().c_str());
        break;
      }
    }
    if (!loaded && !cands.empty()) {
      std::fprintf(stderr, "note: no logo cards found; tried:\n");
      for (auto& c : cands) std::fprintf(stderr, "  %s\n", c.string().c_str());
      std::fprintf(stderr, "  -> running without logo background\n");
    }
  }

  RunInfo ri;
  ri.bpm = bpm;
  ri.executableDir = exeDir.string();
  ri.musicPath = audio.state().active ? audio.trackPath(0).string() : std::string();
  int lastTrack = 0;
  if (!demo.init(ri)) {
    std::fprintf(stderr, "demo init failed\n");
    cleanup(&r);
    return 4;
  }

  Timeline timeline(bpm);
  Input input;
  Frame frame;
  uint64_t lastRevision = 0;
  bool haveUpload = false;
  float lastRadius = .1f;
  bool tickerOn = !noTicker, shotTaken = false;
  int frameCount = 0;
  double lastNow = fixedStep > 0 ? 1.0 - fixedStep : glfwGetTime();
  int exitCode = 0;
  while (!glfwWindowShouldClose(w) && !g_shutdown.load()) {
    // Minimised windows have no visible framebuffer; skip the frame and wait for a restore.
    if (glfwGetWindowAttrib(w, GLFW_ICONIFIED)) {
      glfwWaitEventsTimeout(.05);
      continue;
    }
    // --fixed-step: deterministic show clock (frame N is always at N*step): reproducible captures and A/B checks.
    const double now = fixedStep > 0 ? 1.0 + frameCount * fixedStep : glfwGetTime();
    const MusicState music = audio.state();
    const double showTime = (music.active && fixedStep <= 0) ? music.seconds : now;
    // A new playlist track: lock to its tempo without a jump in the beat count.
    if (music.active && music.track >= 0 && music.track != lastTrack) {
      lastTrack = music.track;
      if (!bpmGiven && audio.nativeBpm(lastTrack) > 0) {
        bpm = audio.nativeBpm(lastTrack);
        timeline.retempo(showTime, bpm);
      }
      std::fprintf(stdout, "music: now playing %s (%.0f BPM)\n", audio.trackPath(lastTrack).filename().string().c_str(),
                   bpm);
    }
    FrameInfo fi;
    fi.time = showTime;
    fi.musicPath = music.active ? audio.trackPath(std::max(0, music.track)).string() : std::string();
    fi.dt = now - lastNow;
    if (adaptive && frameCount > 30 && governor.update(float(fi.dt))) {
      r.setArtDivisor(governor.divisor);
      std::fprintf(stdout, "background resolution: 1/%d (frame time %.1f ms)\n", governor.divisor, governor.ema * 1000.f);
    }
    lastNow = now;
    fi.frame = frameCount;
    fi.bpm = bpm;
    fi.music = music;
    fi.sync = timeline.sample(fi.time);
    fi.sync.pulse = std::max(fi.sync.pulse, music.level);
    int W, H;
    glfwGetFramebufferSize(w, &W, &H);
    if (W <= 0 || H <= 0) {
      glfwWaitEventsTimeout(.05);
      continue;
    }
    fi.width = W;
    fi.height = H;
    input.poll(w);
    if (input.pressed(Key::T)) tickerOn = !tickerOn;
    if (input.pressed(Key::B)) {
      r.setShowBackground(!r.showBackground());
      std::fprintf(stdout, "background: %s\n", r.showBackground() ? "on" : "off");
    }
    {   // pointer: smoothed -1..1 (y up) + click shockwave; drives the backgrounds and the depth layer
      const double now = fixedStep > 0 ? 1.0 + frameCount * fixedStep : glfwGetTime();
      const float dt = float(std::clamp(now - ptrT, 0.0, .1));
      ptrT = now;
      double mx = 0, my = 0;
      int ww = 1, wh = 1;
      glfwGetCursorPos(w, &mx, &my);
      glfwGetWindowSize(w, &ww, &wh);
      const bool inside = glfwGetWindowAttrib(w, GLFW_HOVERED) != 0;
      const float tx = fixedPointer ? fixedX : inside ? std::clamp(float(mx / std::max(1, ww) * 2.0 - 1.0), -1.f, 1.f) : 0.f;
      const float ty = fixedPointer ? fixedY : inside ? std::clamp(float(1.0 - my / std::max(1, wh) * 2.0), -1.f, 1.f) : 0.f;
      const float k = 1.f - std::exp(-dt * 6.f);
      ptrX += (tx - ptrX) * k;
      ptrY += (ty - ptrY) * k;
      const bool mouse = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      if (mouse && !mouseWas) ptrPulse = 1.f;
      mouseWas = mouse;
      ptrPulse *= std::exp(-dt * 1.6f);
      // Idle sway keeps the depth alive when the mouse is still; the gain scales the whole thing.
      float sx, sy;
      bgstyle::sway(now, sx, sy);
      r.setPointer((ptrX + sx) * float(parallax), (ptrY + sy) * float(parallax), ptrPulse);
    }
    for (int k = 0; k < 12; k++)   // F1..F12: switch object k of the current style
      if (input.pressed(Key::F1 + k) && r.objects().toggle(r.backgroundStyle(), k)) {
        userObjects.mask[r.backgroundStyle()] = r.objects().get(r.backgroundStyle());   // what you see is what you chose
        std::fprintf(stdout, "%s: %s\n", bgstyle::name(r.backgroundStyle()), r.objects().describe(r.backgroundStyle()).c_str());
      }
    if (input.pressed(Key::C)) captureNow = true;
    if (input.pressed(Key::H))
      std::fprintf(stdout,
                   "keys: B background  V style  D depth  O gloss  G parallax gain  L logos  T ticker  C capture PNG  H help  Esc quit\n"
                   "      mouse = steer depth, click = shockwave\n");
    if (input.pressed(Key::G)) {
      parallaxGiven = true;
      parallax = bgstyle::nextGain(parallax);
      std::fprintf(stdout, "parallax: %.1f\n", parallax);
    }
    if (autoStyle) {   // the director: cuts on bar lines, reacts to drops/breakdowns, breathes the parallax (V takes over)
      if (!djStarted) {
        dj.start(1, fi.time);
        djStarted = true;
      }
      const int fromStyle = dj.style;
      const director::Decision dd = dj.update(fi.time, music.level, fi.sync.barIndex);
      if (dd.drop) ptrPulse = 1.f;
      if (dd.switchTo >= 0) {
        r.setBackgroundStyle(dd.switchTo);
        r.objects().mask[dd.switchTo] = userObjects.mask[dd.switchTo] & dd.compose;
        std::fprintf(stdout, "director: %s%s -> %s\n", dd.drop ? "drop " : dd.breakdown ? "breakdown " : "", bgstyle::name(fromStyle), bgstyle::name(dd.switchTo));
        if (dd.compose != 0xFFFF) std::fprintf(stdout, "director: composition  %s\n", r.objects().describe(dd.switchTo).c_str());
      }
      if (!parallaxGiven) parallax = dd.gain;
    }
    if (input.pressed(Key::O)) {
      r.setShowGloss(!r.showGloss());
      std::fprintf(stdout, "gloss: %s\n", r.showGloss() ? "on" : "off");
    }
    if (input.pressed(Key::D)) {
      r.setShowDepth(!r.showDepth());
      std::fprintf(stdout, "depth: %s\n", r.showDepth() ? "on" : "off");
    }
    if (input.pressed(Key::V)) {
      autoStyle = false;
      r.setBackgroundStyle(r.backgroundStyle() + 1);
      std::fprintf(stdout, "background style: %s   objects  %s\n", bgstyle::name(r.backgroundStyle()),
                   r.objects().describe(r.backgroundStyle()).c_str());
    }
    if (input.pressed(Key::L)) {
      r.setShowLogos(!r.showLogos());
      std::fprintf(stdout, "logo: %s\n", r.showLogos() ? "on" : "off");
    }

    frame = Frame{};
    const auto cpuStart = std::chrono::steady_clock::now();
    if (!demo.update(fi, input, frame)) break;

    // Mesh upload: skipped only when the demo promises it is unchanged (revision != 0).
    // An empty mesh keeps the previous one on screen (e.g. a scene still building on a worker thread).
    if (!frame.mesh.e.empty() && (!haveUpload || frame.revision == 0 || frame.revision != lastRevision)) {
      // A mesh the GPU cannot take (non-finite, out-of-range or self edges) is skipped, not fatal: the
      // previous mesh stays on screen and the problem is reported once.
      if (r.upload(frame.mesh)) {
        lastRevision = frame.revision;
        haveUpload = true;
        lastRadius = std::max(.1f, geo::stats(frame.mesh).radius);
      } else {
        static bool reported = false;
        if (!reported) {
          std::fprintf(stderr, "Mesh rejected (%s): keeping the previous mesh\n", r.error().c_str());
          reported = true;
        }
      }
    }
    { // --stats: CPU time of the demo's update and the upload, and the frame rate
      static double cpuSum = 0, cpuMax = 0, statT0 = -1;
      static int statFrames = 0;
      const double cpuMs =
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - cpuStart).count();
      cpuSum += cpuMs;
      cpuMax = std::max(cpuMax, cpuMs);
      ++statFrames;
      if (statT0 < 0) statT0 = now;
      if (showStats && now - statT0 >= 5.0) {
        std::fprintf(stdout, "stats: %.1f fps, CPU %.2f ms/frame avg, %.2f max, %zu edges\n",
                     statFrames / (now - statT0), cpuSum / statFrames, cpuMax, frame.mesh.e.size());
        cpuSum = cpuMax = 0;
        statFrames = 0;
        statT0 = now;
      }
    }
    const float scale = std::min(1.48f, 1.92f / lastRadius) * frame.scale;
    r.setWireStyle(frame.wireStyle);
    r.setModelTransform(frame.modelTransform);
    r.setTemporalRefraction(frame.temporalRefraction);
    r.setMemory(frame.memory);
    r.setLook(frame.look);
    // 1) wireframe (+ logo backdrop) into the HDR target, 2) travellers, 3) post pass, 4) ticker.
    if (!r.draw(float(fi.time), W, H, float(W) / float(H), frame.sceneIndex, scale, frame.lineWidth, music.level)) {
      std::fprintf(stderr, "Renderer draw failed: %s\n", r.error().c_str());
      exitCode = 6;
      break;
    }
    if (frame.travelers) r.drawTravelers(float(fi.time), W, H, float(W) / float(H), scale, music.level);
    r.finishLogoFrame(float(fi.time), W, H, frame.sceneIndex, music.level, float(bpm));
    if (tickerOn && !frame.ticker.empty())
      r.drawScroller(float(fi.time), fi.sync.beatPhase, W, H, frame.ticker.c_str(), music.level);

    // --screenshot: taken after the ticker so it is included. With --frames N on the Nth frame.
    if (!screenshotPath.empty() && !shotTaken && (maxFrames <= 0 || frameCount + 1 >= maxFrames)) {
      if (r.screenshot(screenshotPath, W, H)) {
        std::fprintf(stdout, "screenshot: %s (%dx%d) at show time %.2fs\n", screenshotPath.c_str(), W, H, fi.time);
        shotTaken = true;
      } else
        std::fprintf(stderr, "screenshot failed: %s\n", r.error().c_str());
    }
    if (captureNow) {
      captureNow = false;
      char stamp[48];
      const std::time_t tt = std::time(nullptr);
      std::strftime(stamp, sizeof stamp, "demo-%Y%m%d-%H%M%S.png", std::localtime(&tt));
      if (r.screenshot(stamp, W, H))
        std::fprintf(stdout, "captured: %s (%dx%d)\n", stamp, W, H);
      else
        std::fprintf(stderr, "capture failed: %s\n", r.error().c_str());
    }
    ++frameCount;
    if (maxFrames > 0 && frameCount >= maxFrames) {
      std::fprintf(stdout, "--frames %d reached; exiting\n", maxFrames);
      break;
    }
    glfwSwapBuffers(w);
    glfwPollEvents();
    if (input.down(Key::Escape) || glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, 1);
  }
  cleanup(&r);
  return exitCode;
}
} // namespace de
