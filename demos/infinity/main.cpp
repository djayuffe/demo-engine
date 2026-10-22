// Native OpenGL port of the user-supplied Hold Infinity optical renderers.
#include "Engine.hpp"
#include "GLUtil.hpp"
#include "Shaders.hpp"
#include "Dodecahedron.hpp"
#include "Sync.hpp"
#include "Optimize.hpp"
#include "DimensionalRays.hpp"
#include "RenderCorrections.hpp"
#include "Parallax.hpp"
#include "GpuTimer.hpp"
#include <unordered_map>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {
using namespace infinity_shader;
using namespace infinity_geometry;
constexpr const char* names[] = {"HOLD INFINITY / MIRROR SCULPTURE",
                                 "EUCLIDEAN CUBE",
                                 "EUCLIDEAN DODECAHEDRON",
                                 "HYPERBOLIC HONEYCOMB",
                                 "UNIFORM GLASS",
                                 "HYPERBOLIC GRIN GLASS",
                                 "OPTICAL HORIZON",
                                 "SEVEN PRINTED INDEX LAYERS",
                                 "SPECTRAL GLASS / WATER",
                                 "INSIDE THE INSTRUMENT",
                                 "FOUR-DIMENSIONAL LIGHT LOOM",
                                 "PRISMATIC HYPERSPACE THREADS"};
constexpr const char* composite = R"GLSL(#version 410 core
in vec2 vUv; out vec4 outColor; uniform sampler2D uImg; uniform float uFade;
void main(){
  vec3 c=max(texture(uImg,vUv).rgb-vec3(0.008),vec3(0.0));
  c=pow(c,vec3(1.18));
  // Hue-preserving shoulder: keep white-hot cores below display white.
  float peak=max(c.r,max(c.g,c.b));
  float mapped=peak<=0.58?peak:0.58+0.40*(1.0-exp(-(peak-0.58)/0.40));
  c*=mapped/max(peak,1e-6);
  outColor=vec4(c*uFade,1.0);
}
)GLSL";
struct Target {
  GLuint texture = 0, fbo = 0;
  int w = 0, h = 0;
  bool mip = false;
  void release() {
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (texture) glDeleteTextures(1, &texture);
    texture = fbo = 0;
    w = h = 0;
  }
  bool resize(int width, int height, GLint format, bool mips, std::string& error) {
    if (w == width && h == height && texture) return true;
    release();
    w = width;
    h = height;
    mip = mips;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    int x = w, y = h, level = 0;
    do {
      glTexImage2D(GL_TEXTURE_2D, level++, format, x, y, 0, GL_RGBA, GL_FLOAT, nullptr);
      if (!mips || (x == 1 && y == 1)) break;
      x = std::max(1, x / 2);
      y = std::max(1, y / 2);
    } while (true);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    mips ? GL_LINEAR_MIPMAP_LINEAR : (format == GL_RGBA16F ? GL_LINEAR : GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mips || format == GL_RGBA16F) ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      error = "Infinity framebuffer incomplete";
      return false;
    }
    return true;
  }
  void bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
  }
  void sample() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
  }
  void generate() const {
    sample();
    glGenerateMipmap(GL_TEXTURE_2D);
  }
};
struct Camera {
  float m[9];
  float back[3];
};
Camera camera(float yaw, float pitch) {
  float cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
  return {{cy, 0, -sy, -sy * sp, cp, -cy * sp, sy * cp, sp, cy * cp}, {sy * cp, sp, cy * cp}};
}
class Infinity final : public de::Demo {
  std::array<GLuint, 4> chamber_{};
  std::array<GLuint, 128> specialized_{};
  std::unordered_map<GLuint, std::unordered_map<std::string, GLint>> uniforms_;
  InfinityGpuTimer timer_;
  bool profile_ = false, reference_ = false, adaptive_ = true;
  double targetMs_ = 1000.0 / 60.0;
  std::array<infinity_opt::SceneQuality, 12> quality_;
  infinity_opt::RaySolveKey solved_;
  size_t integrations_ = 0, uniformLookups_ = 0, submitFrames_ = 0, mipBuilds_ = 0;

  GLuint post_ = 0, trace_ = 0, show_ = 0, integrate_ = 0, rays_ = 0, compose_ = 0, vao_ = 0, vbo_ = 0;
  Target image_, output_, state_, checkpoints_;
  de::FrameInfo frame_;
  infinity_sync::Cue cue_;
  int manual_ = -1, previous_ = -1, announced_ = -1, pixels_ = 400000, samples_ = 0;
  bool still_ = false, dragging_ = false, viewDirty_ = false;
  double mouseX_ = 0, mouseY_ = 0;
  float yawOffset_ = 0, pitchOffset_ = 0, grinCurvature_ = -1;
  int instrumentState_ = -1;
  float pointerX_ = 0, pointerY_ = 0;
  bool fixedEye_ = false;
  infinity_parallax::Eye eye_;
  void origin(GLuint p, const Camera& c, float x, float y, float z) {
    three(p, "uRo", x + c.m[0] * eye_.x + c.m[3] * eye_.y + c.m[6] * eye_.z,
          y + c.m[1] * eye_.x + c.m[4] * eye_.y + c.m[7] * eye_.z,
          z + c.m[2] * eye_.x + c.m[5] * eye_.y + c.m[8] * eye_.z);
  }
  std::string error_;
  GLint loc(GLuint p, const char* n) {
    if (reference_) {
      ++uniformLookups_;
      return glGetUniformLocation(p, n);
    }
    auto& map = uniforms_[p];
    auto found = map.find(n);
    if (found != map.end()) return found->second;
    ++uniformLookups_;
    auto location = glGetUniformLocation(p, n);
    map.emplace(n, location);
    return location;
  }
  void one(GLuint p, const char* n, float v) { glUniform1f(loc(p, n), v); }
  void integer(GLuint p, const char* n, int v) { glUniform1i(loc(p, n), v); }
  void two(GLuint p, const char* n, float a, float b) { glUniform2f(loc(p, n), a, b); }
  void three(GLuint p, const char* n, float a, float b, float c) { glUniform3f(loc(p, n), a, b, c); }
  static void quad() { glDrawArrays(GL_TRIANGLES, 0, 3); }
  GLuint program(const char* vs, const char* fs) {
    const auto vertex = infinity_render::orderedRamps(vs);
    const auto fragment = infinity_render::orderedRamps(fs);
    return glutil::linkProgram(vertex.c_str(), fragment.c_str(), error_);
  }
  void view(GLuint p, const Camera& c, float distance, float fov, int w, int h) {
    glUniformMatrix3fv(loc(p, "uCam"), 1, GL_FALSE, c.m);
    origin(p, c, c.back[0] * distance, c.back[1] * distance, c.back[2] * distance);
    one(p, "uTanHalf", std::tan(fov * 3.14159265359f / 360));
    two(p, "uRes", float(w), float(h));
  }
  void faceUniforms(GLuint p, int scene) {
    glUniform3fv(loc(p, "uU"), 12, faces);
    if (scene == 1) {
      const float cube[36] = {1, 0, 0, -1, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 1, 0, 0, -1};
      glUniform3fv(loc(p, "uU"), 12, cube);
    }
    float hn[48];
    const float s = 1 / std::sqrt(1 - kleinT * kleinT);
    for (int i = 0; i < 12; ++i) {
      for (int j = 0; j < 3; ++j) hn[i * 4 + j] = faces[i * 3 + j] * s;
      hn[i * 4 + 3] = kleinT * s;
    }
    glUniform4fv(loc(p, "uHN"), 12, hn);
    glUniform3fv(loc(p, "uEA"), 30, edgeA);
    glUniform3fv(loc(p, "uEB"), 30, edgeB);
    integer(p, "uNF", scene == 1 ? 6 : 12);
    one(p, "uSinD", scene == 1 ? 1.f : .894427191f);
    one(p, "uT", kleinT);
  }
  void chamber(int scene, int w, int h, const Camera& cam) {
    image_.bind();
    glDisable(GL_BLEND);
    glUseProgram(chamber_[scene]);
    auto p = chamber_[scene];
    if (reference_) faceUniforms(p, scene);
    view(p, cam, scene == 0 ? 6.4f : 0.f, scene == 0 ? 30.f : 78.f, w, h);
    one(p, "uTime", float(frame_.time));
    one(p, "uFlow", float(cue_.flow));
    one(p, "uEnergy", cue_.energy);
    one(p, "uMirror", .86f);
    one(p, "uFog", .05f);
    one(p, "uGlow", 1.25f + .30f * cue_.bass + .32f * cue_.strike);
    one(p, "uFX", scene == 0 ? 1.6f : .8f);
    float pulses[24];
    infinity_sync::pulses(frame_.sync.beat, cue_.bass, pulses);
    glUniform4fv(loc(p, "uPulse"), 6, pulses);
    if (scene == 1 || scene == 2)
      origin(p, cam, .15f * std::sin(float(cue_.flow) * .2f), .08f, .15f * std::cos(float(cue_.flow) * .13f));
    glUniform4f(loc(p, "uBoost"), std::cos(float(cue_.flow) * .11f), 0, std::sin(float(cue_.flow) * .11f),
                .28f * std::sin(float(cue_.flow) * .23f));
    quad();
    image_.generate();
    ++mipBuilds_;
    output_.bind();
    glUseProgram(post_);
    integer(post_, "uImg", 0);
    two(post_, "uRes", float(w), float(h));
    one(post_, "uBloom", .13f + .06f * cue_.mid + .045f * cue_.strike);
    one(post_, "uStar", scene == 0 ? .035f : .01f);
    one(post_, "uBeat", .65f * cue_.strike);
    quad();
  }
  void instrument(int scene, int w, int h, const Camera& cam, bool resized) {
    image_.bind();
    if (!still_ || viewDirty_ || scene != previous_ || resized) samples_ = 0;
    if (samples_ == 0) {
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
    }
    const int state = infinity_opt::stateForScene(scene, instrumentState_);
    const GLuint p = reference_ ? trace_ : specialized_[size_t(state)];
    glUseProgram(p);
    if (reference_) glUniform3fv(loc(p, "uU"), 12, faces);
    const bool inside = (state & 16) != 0;
    view(p, cam, inside ? .2f : 2.35f, inside ? 82.f : 30.f, w, h);
    if (reference_) {
      integer(p, "uMedium", state & 1 ? 1 : 0);
      integer(p, "uHole", state & 2 ? 1 : 0);
      integer(p, "uCurved", state & 4 ? 1 : 0);
      integer(p, "uWhite", state & 8 ? 1 : 0);
      integer(p, "uInside", inside ? 1 : 0);
      integer(p, "uSteps", state & 64 ? 1 : 0);
      one(p, "uNout", state & 32 ? 1.33f : 1.f);
    }
    const int before = samples_;
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (int i = 0; i < 2 && samples_ < 400; ++i) {
      glUniform1ui(loc(p, "uFrame"), uint32_t(frame_.frame) * 2u + uint32_t(i));
      quad();
      ++samples_;
    }
    glDisable(GL_BLEND);
    if (reference_ || samples_ != before) {
      image_.generate();
      ++mipBuilds_;
    } else
      image_.sample();
    output_.bind();
    glUseProgram(show_);
    integer(show_, "uAcc", 0);
    one(show_, "uExposure", 1.25f * float(samples_));
    two(show_, "uScale", 1, 1);
    one(show_, "uBloom", .12f + .05f * cue_.bass + .04f * cue_.strike);
    quad();
  }
  void common(GLuint p, float aspect, float k, int rays, int nw) {
    one(p, "uK", k);
    one(p, "uAspect", aspect);
    integer(p, "uRays", rays);
    integer(p, "uNW", nw);
  }
  void grin(int scene, int w, int h) {
    const bool white = scene == 11;
    int count = white ? 26 : 46, nw = white ? 7 : 1;
    float aspect = float(w) / float(h),
          k = grinCurvature_ >= 0 ? grinCurvature_ : .57f + .13f * std::sin(float(cue_.flow) * .12f) + .06f * cue_.bass;
    infinity_opt::RaySolveKey key{aspect, k, count, nw};
    if (reference_ || !(key == solved_)) {
      glDisable(GL_BLEND);
      glUseProgram(integrate_);
      common(integrate_, aspect, k, count, nw);
      two(integrate_, "uSource", .055f * aspect, .6f);
      one(integrate_, "uSpread", 1.22f);
      one(integrate_, "uDs", aspect * 1.7f / 512);
      integer(integrate_, "uCk", 0);
      state_.sample();
      checkpoints_.bind();
      integer(integrate_, "uPass", 0);
      glViewport(0, 0, 16, reference_ ? 182 : count * nw);
      quad();
      checkpoints_.sample();
      state_.bind();
      integer(integrate_, "uPass", 1);
      glViewport(0, 0, 512, reference_ ? 182 : count * nw);
      quad();
      solved_ = key;
      ++integrations_;
    }
    output_.bind();
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glUseProgram(rays_);
    state_.sample();
    integer(rays_, "uState", 0);
    common(rays_, aspect, k, count, nw);
    two(rays_, "uRes", float(w), float(h));
    one(rays_, "uTime", float(cue_.flow) * 4.f);
    one(rays_, "uFlow", float(cue_.flow));
    one(rays_, "uBeat", float(frame_.sync.beat));
    one(rays_, "uPulse", cue_.strike);
    one(rays_, "uBass", cue_.bass);
    three(rays_, "uEye", eye_.x, eye_.y, eye_.z);
    const auto rayCamera =
        camera(yawOffset_ + (still_ ? 0.f : 0.18f * std::sin(float(cue_.flow) * 0.08f)), pitchOffset_ + 0.12f);
    glUniformMatrix3fv(loc(rays_, "uCam"), 1, GL_FALSE, rayCamera.m);
    float turns[12];
    for (int sheet = 0; sheet < 3; ++sheet) {
      const float a = float(cue_.flow) * .11f + float(sheet) * .34f, b = float(cue_.flow) * .07f + .25f;
      turns[4 * sheet] = std::cos(a);
      turns[4 * sheet + 1] = std::sin(a);
      turns[4 * sheet + 2] = std::cos(b);
      turns[4 * sheet + 3] = std::sin(b);
    }
    glUniform4fv(loc(rays_, "uTurn"), 3, turns);
    one(rays_, "uNMax", 1 / ((1 - k) + k * .045f / 1.045f));
    constexpr float widths[] = {10.f, 3.2f, .95f};
    constexpr float intensities[] = {.028f, .15f, 1.20f};
    for (int layer = 0; layer < 3; ++layer) {
      one(rays_, "uWidth", widths[layer] * (1.f + .06f * cue_.breath));
      one(rays_, "uIntensity", intensities[layer] * (1 + .14f * cue_.treble * cue_.flutter));
      one(rays_, "uCore", layer == 2 ? 1.f : 0.f);
      one(rays_, "uSharp", layer == 2 ? 2.f : 2.6f);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 1024, count * nw * (white ? 2 : 3));
    }
    glDisable(GL_BLEND);
  }

public:
  de::Config config() const override {
    de::Config c;
    c.title = "HOLD INFINITY / music-driven optical geometry";
    c.binaryName = "hold_infinity";
    c.version = "1.8";
    c.customRendering = true;
    c.defaultTrack = 1;
    c.logoShow = false;
    c.fxAmount = 0;
    c.musicFiles = {"demos/wireframe/assets/music/cs127_-_stars_4ch.mod",
                    "demos/wireframe/assets/music/drozerix_-_digital_rendezvous.xm"};
    return c;
  }
  bool parseArg(const std::string& a, int& i, int argc, char** argv) override {
    if (a == "--infinity-adaptive") {
      adaptive_ = true;
      return true;
    }
    if (a == "--infinity-fixed-budget") {
      adaptive_ = false;
      return true;
    }
    if (a == "--infinity-fps") {
      if (i + 1 >= argc) throw std::runtime_error(a + " needs a value");
      std::string value = argv[++i];
      size_t end = 0;
      double fps = std::stod(value, &end);
      if (end != value.size() || !std::isfinite(fps) || fps < 24 || fps > 144)
        throw std::runtime_error("infinity fps must be 24..144");
      targetMs_ = 1000.0 / fps;
      return true;
    }
    if (a == "--infinity-eye") {
      if (i + 2 >= argc) throw std::runtime_error(a + " needs X Y in -1..1");
      float values[2];
      for (int j = 0; j < 2; ++j) {
        std::string text = argv[++i];
        size_t end = 0;
        values[j] = std::stof(text, &end);
        if (end != text.size() || !std::isfinite(values[j]) || std::abs(values[j]) > 1)
          throw std::runtime_error("eye coordinates must be -1..1");
      }
      pointerX_ = values[0];
      pointerY_ = values[1];
      fixedEye_ = true;
      return true;
    }
    if (a == "--gpu-profile") {
      profile_ = true;
      return true;
    }
    if (a == "--infinity-reference") {
      reference_ = true;
      return true;
    }
    if (a == "--still") {
      still_ = true;
      return true;
    }
    if (a == "--grin-curvature") {
      if (i + 1 >= argc) throw std::runtime_error(a + " needs a value");
      std::string value = argv[++i];
      size_t end = 0;
      float k = std::stof(value, &end);
      if (end != value.size() || !std::isfinite(k) || k < 0 || k > 1)
        throw std::runtime_error("curvature must be 0..1");
      grinCurvature_ = k;
      if (manual_ < 0) manual_ = 10;
      return true;
    }
    if (a == "--infinity-scene" || a == "--infinity-pixels" || a == "--instrument-state") {
      if (i + 1 >= argc) throw std::runtime_error(a + " needs a value");
      std::string value = argv[++i];
      size_t end = 0;
      int n = std::stoi(value, &end);
      if (end != value.size() || (a == "--infinity-scene"     ? (n < 0 || n >= 12)
                                  : a == "--instrument-state" ? (n < 0 || n > 127)
                                                              : (n < 10000 || n > 4000000)))
        throw std::runtime_error("invalid " + a);
      if (a == "--instrument-state") {
        instrumentState_ = n;
        if (manual_ < 0) manual_ = 5;
      } else if (a == "--infinity-scene")
        manual_ = n;
      else {
        pixels_ = n;
        adaptive_ = false;
      }
      return true;
    }
    return false;
  }
  std::string usage() const override {
    return "  --infinity-scene N  Hold one optical scene (0..11)\n"
           "  --infinity-pixels N GPU budget (10000..4000000; default 400000)\n"
           "  --infinity-eye X Y  Fixed camera offset (-1..1); disables automatic dolly\n"
           "  --still             Fixed camera; accumulate up to 400 samples\n"
           "  --instrument-state N Seven-bit optical state (0..127; see demo README)\n"
           "  --grin-curvature F   Index gradient 0..1 (selects ray lab if auto)\n"
           "  --infinity-fps N    Adaptive GPU target 24..144 FPS (default 60)\n"
           "  --infinity-fixed-budget Disable automatic resolution adjustment\n"
           "  --infinity-adaptive Re-enable adaptation after an explicit pixel budget\n"
           "  --gpu-profile       Report non-blocking GPU timings and submission counts\n"
           "  --infinity-reference Original GPU workload for A/B comparisons\n"
           "  Drag to orbit; Left/Right scenes; Space automatic show; Q quit.\n";
  }
  bool init(const de::RunInfo&) override {
    const char* sources[] = {CHAMBER_OBJECT, CHAMBER_CUBE, CHAMBER_FLAT, CHAMBER_HYPER};
    for (int i = 0; i < 4; ++i) chamber_[i] = program(FULLSCREEN_VS, sources[i]);
    post_ = program(FULLSCREEN_VS, CHAMBER_POST);
    if (reference_) trace_ = program(FULLSCREEN_VS, TRACE_FS);
    show_ = program(FULLSCREEN_VS, SHOW_FS);
    integrate_ = program(FULLSCREEN_VS, INTEGRATE_FS);
    const auto rayVertex = infinity_dimension::vertex(RAY_VS);
    const auto rayFragment = infinity_dimension::fragment(RAY_FS);
    rays_ = program(rayVertex.c_str(), rayFragment.c_str());
    compose_ = program(FULLSCREEN_VS, composite);
    if (std::any_of(chamber_.begin(), chamber_.end(), [](GLuint p) { return !p; }) || !post_ ||
        (reference_ && !trace_) || !show_ || !integrate_ || !rays_ || !compose_) {
      std::fprintf(stderr, "Infinity shader failure: %s\n", error_.c_str());
      return false;
    }
    if (!reference_) {
      for (int i = 0; i < 4; ++i) {
        glUseProgram(chamber_[i]);
        faceUniforms(chamber_[i], i);
      }
      for (int scene = 4; scene < 10; ++scene) {
        int state = infinity_opt::stateForScene(scene, instrumentState_);
        auto& p = specialized_[size_t(state)];
        if (!p) {
          auto source = infinity_opt::specialize(TRACE_FS, state);
          p = program(FULLSCREEN_VS, source.c_str());
          if (!p) {
            std::fprintf(stderr, "Optical specialization failed: %s\n", error_.c_str());
            return false;
          }
          glUseProgram(p);
          glUniform3fv(loc(p, "uU"), 12, faces);
        }
      }
    }
    if (profile_) std::printf("GPU device: %s / %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    timer_.init(profile_ || (adaptive_ && !still_ && !reference_), profile_);
    glutil::setupFullscreenVao(vao_, vbo_);
    return state_.resize(512, 182, GL_RGBA32F, false, error_) &&
           checkpoints_.resize(16, 182, GL_RGBA32F, false, error_);
  }
  bool update(const de::FrameInfo& f, const de::Input& in, de::Frame& out) override {
    frame_ = f;
    viewDirty_ = false;
    if (auto* window = glfwGetCurrentContext()) {
      double x = 0, y = 0;
      glfwGetCursorPos(window, &x, &y);
      const bool held = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      if (held && dragging_) {
        int width = 1, height = 1;
        glfwGetWindowSize(window, &width, &height);
        (void)height;
        yawOffset_ -= float(x - mouseX_) * 4.f / float(std::max(1, width));
        pitchOffset_ = std::clamp(pitchOffset_ - float(y - mouseY_) * 4.f / float(std::max(1, width)), -1.2f, 1.2f);
        viewDirty_ = x != mouseX_ || y != mouseY_;
      }
      if (!still_ && !fixedEye_) {
        int width = 1, height = 1;
        glfwGetWindowSize(window, &width, &height);
        const bool inside = glfwGetWindowAttrib(window, GLFW_HOVERED) == GLFW_TRUE;
        pointerX_ = infinity_parallax::smooth(pointerX_, inside ? float(2 * x / std::max(1, width) - 1) : 0.f, f.dt);
        pointerY_ = infinity_parallax::smooth(pointerY_, inside ? float(1 - 2 * y / std::max(1, height)) : 0.f, f.dt);
      }
      mouseX_ = x;
      mouseY_ = y;
      dragging_ = held;
    }
    eye_ = infinity_parallax::eye(f.sync.beat, pointerX_, pointerY_, still_ || fixedEye_);
    int current = infinity_sync::sample(f.sync.beat, 0, false, 0, 0, 0, manual_).scene;
    if (in.pressed(de::Key::Right)) manual_ = (current + 1) % 12;
    if (in.pressed(de::Key::Left)) manual_ = (current + 11) % 12;
    if (in.pressed(de::Key::Space)) manual_ = -1;
    cue_ = infinity_sync::sample(f.sync.beat, f.sync.pulse, f.music.active, f.music.bass, f.music.mid, f.music.treble,
                                 manual_);
    if (cue_.scene != announced_) {
      std::fprintf(stdout, "infinity scene %02d: %s / beat %.2f\n", cue_.scene, names[cue_.scene], f.sync.beat);
      announced_ = cue_.scene;
    }
    out.travelers = false;
    out.ticker = std::string(names[cue_.scene]) + "  /  HOLD INFINITY BY PURPLETWILIGHT  /  " +
                 (f.musicPath.empty()                              ? "BPM CLOCK"
                  : f.musicPath.find("cs127") != std::string::npos ? "STARS (4CH) BY CS127 / CC BY 4.0"
                  : f.musicPath.find("drozerix_-_digital_rendezvous") != std::string::npos
                      ? "DIGITAL RENDEZVOUS BY DROZERIX / PUBLIC DOMAIN"
                      : f.musicPath);
    return !in.pressed(de::Key::Q);
  }
  bool draw(const de::FrameInfo& f, std::string& error) override {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(vao_);
    timer_.poll();
    int budget = cue_.scene >= 4 && cue_.scene < 10 ? pixels_ / 3 : pixels_;
    if (adaptive_ && !still_ && !reference_) {
      const auto& r = timer_.latest();
      auto& q = quality_[size_t(cue_.scene)];
      if (r.scene == cue_.scene && q.observe(r.milliseconds, r.frame, targetMs_) && profile_)
        std::printf("GPU quality scene %02d: %.3f of pixel ceiling (target %.2f ms)\n", cue_.scene, q.fraction(),
                    targetMs_);
      budget = std::max(1, int(float(budget) * q.fraction()));
    }
    double scale = std::min(1.0, std::sqrt(double(budget) / (double(f.width) * f.height)));
    int w = std::max(1, int(f.width * scale)), h = std::max(1, int(f.height * scale));
    bool resized = image_.w != w || image_.h != h;
    if ((cue_.scene < 10 && !image_.resize(w, h, GL_RGBA16F, true, error)) ||
        !output_.resize(w, h, GL_RGBA16F, false, error))
      return false;
    timer_.begin(cue_.scene, f.frame);
    ++submitFrames_;
    float motion = still_ ? 0.f : float(cue_.flow);
    Camera cam = camera(.6f + motion * .09f + yawOffset_, .22f + .1f * std::sin(motion * .07f) + pitchOffset_);
    if (cue_.scene < 4)
      chamber(cue_.scene, w, h, cam);
    else if (cue_.scene < 10)
      instrument(cue_.scene, w, h, cam, resized);
    else
      grin(cue_.scene, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, f.width, f.height);
    glDisable(GL_BLEND);
    glUseProgram(compose_);
    output_.sample();
    integer(compose_, "uImg", 0);
    one(compose_, "uFade", cue_.fade);
    quad();
    timer_.end();
    previous_ = cue_.scene;
    auto status = glGetError();
    if (status != GL_NO_ERROR) {
      error = "Infinity OpenGL error " + std::to_string(status);
      return false;
    }
    return true;
  }
  void shutdown() override {
    timer_.shutdown();
    if (profile_)
      std::printf("GPU workload: frames=%zu ray-solves=%zu uniform-lookups=%zu mipmaps=%zu mode=%s\n", submitFrames_,
                  integrations_, uniformLookups_, mipBuilds_, reference_ ? "reference" : "optimized");
    for (auto& p : specialized_)
      if (p) {
        glDeleteProgram(p);
        p = 0;
      }
    uniforms_.clear();

    image_.release();
    output_.release();
    state_.release();
    checkpoints_.release();
    for (auto& p : chamber_)
      if (p) {
        glDeleteProgram(p);
        p = 0;
      }
    for (auto* p : {&post_, &trace_, &show_, &integrate_, &rays_, &compose_})
      if (*p) {
        glDeleteProgram(*p);
        *p = 0;
      }
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    vao_ = vbo_ = 0;
  }
};
} // namespace
int main(int argc, char** argv) {
  try {
    Infinity demo;
    return de::run(demo, argc, argv);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "hold_infinity: %s\n", e.what());
    return 2;
  }
}
