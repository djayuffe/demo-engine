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
#include "OpticalMath.hpp"
#include "MatrixLife.hpp"
#include "AntTraffic.hpp"
#include "BouncingBall.hpp"
#include "BallShader.hpp"
#include "GpuTimer.hpp"
#include "SequentialRays.hpp"
#include "WorldSheets.hpp"
#include "Motion.hpp"
#include "OpticalPost.hpp"
#include "../atlas/AtlasShaders.hpp"
#include "Slice.hpp"
#include "HyperAtlas.hpp"
#include <unordered_map>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

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
                                 "PRISMATIC HYPERSPACE THREADS",
                                 "HARMONIC CHRYSANTHEMUM",
                                 "HEPTAGONAL POINCARE GATES",
                                 "SADDLED WORLD SHEETS",
                                 "BRANCHING CRYSTAL GARDEN",
                                 "ATLAS / TRUE FOUR-DIMENSIONAL SECTION",
                                 "ATLAS / SIX-PLANE HYPERCUBE",
                                 "ATLAS / HIERARCHICAL RADIAL BLOOM",
                                 "ATLAS / RECURSIVE COORDINATE FOLDS",
                                 "ATLAS / HYPERBOLIC SPHERE CONSTELLATION",
                                 "ATLAS / MULTI-CHART IRIS PORTAL",
                                 "ATLAS / EUCLIDEAN ORNAMENT",
                                 "ATLAS / CAUSAL EVENT VOLUME",
                                 "ATLAS / FILIGREE FACE EMERGENCE",
                                 "ATLAS / THREE HYPERBOLIC VIEWPOINTS"};
static_assert(std::size(names) == infinity_sync::sceneCount);
struct Target {
  Target() = default;
  Target(const Target&) = delete;
  Target& operator=(const Target&) = delete;
  GLuint texture = 0, fbo = 0;
  int w = 0, h = 0;
  bool mip = false;
  GLint storageFormat = 0;
  void release() {
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (texture) glDeleteTextures(1, &texture);
    texture = fbo = 0;
    w = h = 0;
    mip = false;
    storageFormat = 0;
  }
  bool resize(int width, int height, GLint format, bool mips, std::string& error) {
    if (w == width && h == height && texture && fbo && storageFormat == format && mip == mips) return true;
    GLint maximum = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
    if (width < 1 || height < 1 || width > maximum || height > maximum) {
      error = "Infinity target exceeds GPU texture limits";
      return false;
    }
    GLuint nextTexture = 0, nextFbo = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &nextTexture);
    glBindTexture(GL_TEXTURE_2D, nextTexture);
    int x = width, y = height, level = 0;
    do {
      glTexImage2D(GL_TEXTURE_2D, level++, format, x, y, 0, GL_RGBA, GL_FLOAT, nullptr);
      if (!mips || (x == 1 && y == 1)) break;
      x = std::max(1, x / 2);
      y = std::max(1, y / 2);
    } while (true);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    mips ? GL_LINEAR_MIPMAP_LINEAR : (format == GL_RGBA16F ? GL_LINEAR : GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mips || format == GL_RGBA16F) ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level - 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &nextFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, nextFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nextTexture, 0);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    const GLenum apiError = glGetError();
    if (status != GL_FRAMEBUFFER_COMPLETE || apiError != GL_NO_ERROR) {
      glDeleteFramebuffers(1, &nextFbo);
      glDeleteTextures(1, &nextTexture);
      error =
          "Infinity target allocation failed (FBO " + std::to_string(status) + ", GL " + std::to_string(apiError) + ")";
      return false;
    }
    release();
    texture = nextTexture;
    fbo = nextFbo;
    w = width;
    h = height;
    mip = mips;
    storageFormat = format;
    return true;
  }
  void bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
  }
  void sample(int unit = 0) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
  }
  void generate() const {
    sample();
    glGenerateMipmap(GL_TEXTURE_2D);
  }
};
using infinity_math::Camera;
using infinity_math::camera;
class Infinity final : public de::Demo {
  std::array<GLuint, 4> chamber_{};
  std::array<GLuint, 128> specialized_{};
  std::unordered_map<GLuint, std::unordered_map<std::string, GLint>> uniforms_;
  InfinityGpuTimer timer_;
  bool profile_ = false, reference_ = false, adaptive_ = true, opticalBloom_ = true;
  GLuint bloomExtract_ = 0;
  infinity_motion::Bands bandMotion_;
  infinity_motion::Clock atlasMotion_;
  size_t bloomBuilds_ = 0;
  double targetMs_ = 1000.0 / 60.0;
  std::array<infinity_opt::SceneQuality, infinity_sync::sceneCount> quality_;
  infinity_opt::RaySolveKey solved_;
  size_t integrations_ = 0, uniformLookups_ = 0, submitFrames_ = 0, mipBuilds_ = 0;

  GLuint post_ = 0, trace_ = 0, show_ = 0, integrate_ = 0, rays_ = 0, compose_ = 0, vao_ = 0, vbo_ = 0;
  std::array<GLuint, 2> sequential_{};
  std::array<GLuint, 4> worlds_{};
  GLuint worldReference_ = 0;
  std::array<GLuint, dmt::gpu::modes> atlasPrograms_{};
  GLuint atlasReference_ = 0, atlasTemporal_ = 0;
  std::array<Target, 2> atlasHistory_;
  int atlasHistoryIndex_ = 0;
  bool atlasHistoryValid_ = false;
  dmt::atlas::Atlas atlasGeometry_;
  bool atlasReduced_ = false, atlasPaused_ = false, atlasPauseKey_ = false;
  float atlasExposure_ = 1.f, atlasFlash_ = 1.f;

  bool rasterIntegrator_ = false, forceSequential_ = false, validateRays_ = false, validatedRays_ = false,
       validateTargets_ = false, validateOutput_ = false;
  std::array<bool, infinity_sync::sceneCount> outputChecked_{};
  Target image_, output_, state_, checkpoints_, bloom_;
  de::FrameInfo frame_;
  infinity_sync::Cue cue_;
  int manual_ = -1, previous_ = -1, announced_ = -1, pixels_ = 400000, samples_ = 0;
  bool still_ = false, dragging_ = false, viewDirty_ = false;
  double mouseX_ = 0, mouseY_ = 0;
  float yawOffset_ = 0, pitchOffset_ = 0, grinCurvature_ = -1;
  GLuint antTexture_ = 0;
  infinity_ants::Traffic traffic_;
  infinity_ball::World ballWorld_;
  infinity_ants::Traffic::Image antImage_;
  int instrumentState_ = -1;
  float pointerX_ = 0, pointerY_ = 0;
  bool fixedEye_ = false;
  infinity_parallax::Eye eye_, atlasEye_;
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
  void sampler(GLuint p, const char* n, int unit) {
    if (reference_) integer(p, n, unit);
  }
  void two(GLuint p, const char* n, float a, float b) { glUniform2f(loc(p, n), a, b); }
  void three(GLuint p, const char* n, float a, float b, float c) { glUniform3f(loc(p, n), a, b, c); }
  static void quad() { glDrawArrays(GL_TRIANGLES, 0, 3); }
  GLuint program(const char* vs, const char* fs) {
    const auto vertex = infinity_render::orderedRamps(vs);
    const auto fragment = infinity_render::orderedRamps(infinity_ball::shader(infinity_matrix::expand(fs)));
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
    for (int i = 0; i < 12; ++i) {
      const auto plane = infinity_math::hyperplane(faces[3 * i], faces[3 * i + 1], faces[3 * i + 2], kleinT);
      std::copy(plane.begin(), plane.end(), hn + 4 * i);
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
    traffic_.raster(frame_.sync.beat, antImage_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, antTexture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGBA, GL_FLOAT, antImage_.data());
    sampler(p, "uAntField", 2);
    glActiveTexture(GL_TEXTURE0);
    const auto flatBall = ballWorld_.sample(scene == 1 ? ballWorld_.cube : ballWorld_.flat, frame_.sync.beat);
    const auto hyperBall = ballWorld_.sample(ballWorld_.hyper, frame_.sync.beat);
    glUniform4f(loc(p, "uBallE"), float(flatBall.p[0]), float(flatBall.p[1]), float(flatBall.p[2]),
                float(infinity_ball::Ball::radius));
    glUniform4f(loc(p, "uBallH"), float(hyperBall.p[0]), float(hyperBall.p[1]), float(hyperBall.p[2]),
                float(hyperBall.p[3]));
    one(p, "uBallImpact", float(scene == 1 || scene == 2 ? flatBall.flash : hyperBall.flash));
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
    const float cameraFlow = still_ ? 0.f : float(cue_.flow);
    if (scene == 1 || scene == 2)
      origin(p, cam, .15f * std::sin(cameraFlow * .2f), .08f, .15f * std::cos(cameraFlow * .13f));
    glUniform4f(loc(p, "uBoost"), std::cos(cameraFlow * .11f), 0, std::sin(cameraFlow * .11f),
                .28f * std::sin(cameraFlow * .23f));
    quad();
    image_.generate();
    ++mipBuilds_;
    output_.bind();
    glUseProgram(post_);
    sampler(post_, "uImg", 0);
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
    sampler(show_, "uAcc", 0);
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
  bool validateField(float aspect, float k, int count, int nw) {
    std::vector<float> actual(size_t(512 * count * nw * 4)), expected(actual.size());
    state_.bind();
    glReadPixels(0, 0, 512, count * nw, GL_RGBA, GL_FLOAT, actual.data());
    Target referenceState, referenceCheck;
    if (!referenceState.resize(512, 182, GL_RGBA32F, false, error_) ||
        !referenceCheck.resize(16, 182, GL_RGBA32F, false, error_)) {
      referenceState.release();
      referenceCheck.release();
      return false;
    }
    glUseProgram(integrate_);
    common(integrate_, aspect, k, count, nw);
    two(integrate_, "uSource", .055f * aspect, .6f);
    one(integrate_, "uSpread", 1.22f);
    one(integrate_, "uDs", aspect * 1.7f / 512);
    sampler(integrate_, "uCk", 0);
    state_.sample();
    referenceCheck.bind();
    integer(integrate_, "uPass", 0);
    glViewport(0, 0, 16, count * nw);
    quad();
    referenceCheck.sample();
    referenceState.bind();
    integer(integrate_, "uPass", 1);
    glViewport(0, 0, 512, count * nw);
    quad();
    glReadPixels(0, 0, 512, count * nw, GL_RGBA, GL_FLOAT, expected.data());
    referenceState.release();
    referenceCheck.release();
    float maximum = 0;
    for (size_t i = 0; i < actual.size(); ++i) {
      float delta = std::abs(actual[i] - expected[i]);
      maximum = std::max(maximum, delta);
      if (!std::isfinite(actual[i]) || !std::isfinite(expected[i]) || delta > 3e-5f * (1 + std::abs(expected[i]))) {
        error_ = "Ray-state mismatch at component " + std::to_string(i);
        return false;
      }
    }
    for (size_t i = 0; i < actual.size(); i += 4) {
      float norm = actual[i + 2] * actual[i + 2] + actual[i + 3] * actual[i + 3];
      if (std::abs(norm - 1) > 5e-4f) {
        error_ = "Ray tangent lost unit length";
        return false;
      }
    }
    std::printf("Ray-state validation: %d points, max error %.9g, all finite/unit tangents\n", 512 * count * nw,
                double(maximum));
    return true;
  }
  void grin(int scene, int w, int h) {
    const bool white = scene == 11;
    int count = white ? 26 : 46, nw = white ? 7 : 1;
    float aspect = float(w) / float(h),
          k = grinCurvature_ >= 0 ? grinCurvature_ : .57f + .13f * std::sin(float(cue_.flow) * .12f) + .06f * cue_.bass;
    infinity_opt::RaySolveKey key{aspect, k, count, nw};
    if (reference_ || !(key == solved_)) {
      glDisable(GL_BLEND);
      if (!reference_ && !rasterIntegrator_ && (!white || forceSequential_)) {
        for (int pass = 0; pass < 2; ++pass) {
          const auto p = sequential_[size_t(pass)];
          glUseProgram(p);
          common(p, aspect, k, count, nw);
          two(p, "uSource", .055f * aspect, .6f);
          one(p, "uSpread", 1.22f);
          one(p, "uDs", aspect * 1.7f / 512);
          sampler(p, "uCk", 0);
          if (pass == 0)
            checkpoints_.bind();
          else {
            checkpoints_.sample();
            state_.bind();
          }
          glViewport(0, 0, pass == 0 ? 16 : 512, count * nw);
          glDrawArrays(GL_POINTS, 0, count * nw * (pass == 0 ? 1 : 16));
        }
      } else {
        glUseProgram(integrate_);
        common(integrate_, aspect, k, count, nw);
        two(integrate_, "uSource", .055f * aspect, .6f);
        one(integrate_, "uSpread", 1.22f);
        one(integrate_, "uDs", aspect * 1.7f / 512);
        sampler(integrate_, "uCk", 0);
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
      }
      if (validateRays_ && !validatedRays_) {
        if (!validateField(aspect, k, count, nw)) throw std::runtime_error(error_);
        validatedRays_ = true;
      }
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
    sampler(rays_, "uState", 0);
    common(rays_, aspect, k, count, nw);
    two(rays_, "uRes", float(w), float(h));
    one(rays_, "uBeat", float(std::fmod(std::max(0.0, frame_.sync.beat), 16.0)));
    one(rays_, "uPulse", cue_.strike);
    one(rays_, "uBass", cue_.bass);
    three(rays_, "uEye", eye_.x, eye_.y, eye_.z);
    const auto rayCamera =
        camera(yawOffset_ + (still_ ? 0.f : 0.18f * std::sin(float(cue_.flow) * 0.08f)), pitchOffset_ + 0.12f);
    glUniformMatrix3fv(loc(rays_, "uCam"), 1, GL_FALSE, rayCamera.m);
    float turns[12];
    for (int sheet = 0; sheet < 3; ++sheet) {
      const auto turn = infinity_math::turn(cue_.flow, sheet);
      std::copy(turn.begin(), turn.end(), turns + 4 * sheet);
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

  void atlas(int scene, int w, int h) {
    const int mode = scene - 16;
    const auto p = reference_ ? atlasReference_ : atlasPrograms_[size_t(mode)];
    output_.bind();
    glUseProgram(p);
    two(p, "uRes", float(w), float(h));
    const double time = still_ ? 0.0 : atlasMotion_.time();
    const float breath = infinity_motion::breath(time);
    one(p, "uBreath", breath);
    one(p, "uTime", float(time));
    one(p, "uPulse", atlasPaused_ || atlasReduced_ ? 0.f : cue_.strike * atlasFlash_);
    one(p, "uBass", atlasPaused_ || atlasReduced_ ? 0.f : cue_.bass * atlasFlash_);
    one(p, "uExposure", atlasExposure_);
    integer(p, "uUseBvh", reference_ ? 0 : 1);
    three(p, "uEye", atlasEye_.x, atlasEye_.y, atlasEye_.z);
    const auto cam = camera(yawOffset_, pitchOffset_);
    glUniformMatrix3fv(loc(p, "uCam"), 1, GL_FALSE, cam.m);
    if (reference_) integer(p, "uMode", mode);
    if (mode == 0) {
      const auto slice = dmt::slice::section({.31, .48, .25, .77}, .16 * std::sin(time * .2), time * .12, false,
                                             1 + .025 * (2 * breath - 1));
      float planes[32];
      for (size_t i = 0; i < 8; ++i)
        for (size_t j = 0; j < 4; ++j) planes[i * 4 + j] = float(slice.planes[i][j]);
      glUniform4fv(loc(p, "uPlanes"), 8, planes);
      integer(p, "uEmpty", slice.empty ? 1 : 0);
    }
    if (mode == 1) {
      float rotation[16];
      for (int column = 0; column < 4; ++column) {
        dmt::V4 v{};
        v[column] = 1;
        v = dmt::slice::rotated(v, time * .12);
        for (int row = 0; row < 4; ++row) rotation[column * 4 + row] = float(v[row]);
      }
      glUniformMatrix4fv(loc(p, "uRotation"), 1, GL_FALSE, rotation);
    }
    quad();
  }
  void atlasConstants(GLuint p) {
    glUseProgram(p);
    float bounds[112 * 4]{}, meta[56 * 4]{}, levels[9 * 4]{}, events[24 * 4]{};
    for (size_t i = 0; i < atlasGeometry_.nodes.size(); ++i) {
      const auto& node = atlasGeometry_.nodes[i];
      for (int j = 0; j < 3; ++j) bounds[i * 4 + size_t(j)] = float(node.bound.center[size_t(j + 1)]);
      // Round outward for float GPU bounds built from double CPU geometry.
      bounds[i * 4 + 3] = float(node.bound.radius + .008);
      meta[i * 2] = float(node.escape);
      meta[i * 2 + 1] = float(node.leaf + 1);
    }
    glUniform4fv(loc(p, "uBounds"), 112, bounds);
    glUniform4fv(loc(p, "uMeta"), 56, meta);
    uint64_t seed = 0xD47A51;
    for (int i = 0; i < 9; ++i) {
      seed = dmt::childSeed(seed, uint32_t(i));
      levels[i * 4] = float(6 + 2 * (i % 5));
      levels[i * 4 + 1] = float(double(seed % 100000) / 100000.0 * 2 * dmt::pi);
      levels[i * 4 + 2] = float(std::pow(1.8, i));
      levels[i * 4 + 3] = float(std::pow(.64, i));
    }
    glUniform4fv(loc(p, "uLevels"), 9, levels);
    for (int i = 0; i < 24; ++i) {
      double a = i * 2.399963229728653;
      events[i * 4] = float(1.4 * std::cos(a));
      events[i * 4 + 1] = float(1.1 * std::sin(a));
      events[i * 4 + 2] = float(-1.2 + 2.4 * i / 23.0);
      events[i * 4 + 3] = float(i) * .8f;
    }
    glUniform4fv(loc(p, "uEvents"), 24, events);
  }

  void worldSheet(int scene, int w, int h) {
    const int mode = scene - 12;
    const auto p = reference_ ? worldReference_ : worlds_[size_t(mode)];
    output_.bind();
    glUseProgram(p);
    two(p, "uRes", float(w), float(h));
    one(p, "uBeat", float(std::fmod(std::max(0.0, frame_.sync.beat), 256.0)));
    one(p, "uPulse", cue_.strike);
    one(p, "uBass", cue_.bass);
    three(p, "uEye", eye_.x, eye_.y, eye_.z);
    const float phase = still_ ? 0.f : float(cue_.flow);
    const float baseYaw = mode == 3 ? .6f : (mode == 2 ? .25f : 0.f);
    const float basePitch = mode == 3 ? .3f : .13f;
    const auto cam = camera(yawOffset_ + baseYaw + .25f * std::sin(phase * .1f),
                            pitchOffset_ + basePitch + .12f * std::sin(phase * .07f));
    glUniformMatrix3fv(loc(p, "uCam"), 1, GL_FALSE, cam.m);
    if (reference_) integer(p, "uMode", mode);
    quad();
  }

public:
  de::Config config() const override {
    de::Config c;
    c.title = "HOLD INFINITY / music-driven optical geometry";
    c.binaryName = "hold_infinity";
    c.version = "4.1";
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
    if (a == "--infinity-orbit") {
      if (i + 2 >= argc) throw std::runtime_error(a + " needs YAW PITCH in radians");
      float values[2];
      for (int j = 0; j < 2; ++j) {
        std::string text = argv[++i];
        size_t end = 0;
        values[j] = std::stof(text, &end);
        if (end != text.size() || !std::isfinite(values[j]) || std::abs(values[j]) > (j == 0 ? 3.141593f : 1.2f))
          throw std::runtime_error("orbit exceeds yaw/pitch limits");
      }
      yawOffset_ = values[0];
      pitchOffset_ = values[1];
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
    if (a == "--infinity-raster-integrator") {
      rasterIntegrator_ = true;
      forceSequential_ = false;
      return true;
    }
    if (a == "--infinity-sequential-integrator") {
      forceSequential_ = true;
      rasterIntegrator_ = false;
      return true;
    }
    if (a == "--validate-output") {
      validateOutput_ = true;
      return true;
    }
    if (a == "--validate-targets") {
      validateTargets_ = true;
      return true;
    }
    if (a == "--validate-ray-state") {
      validateRays_ = true;
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
    if (a == "--no-optical-bloom") {
      opticalBloom_ = false;
      return true;
    }
    if (a == "--atlas-reduced-motion") {
      atlasReduced_ = true;
      return true;
    }
    if (a == "--atlas-paused") {
      atlasPaused_ = true;
      return true;
    }
    if (a == "--atlas-exposure" || a == "--atlas-flash-gain") {
      if (i + 1 >= argc) throw std::runtime_error(a + " needs a value");
      std::string text = argv[++i];
      size_t end = 0;
      float value = std::stof(text, &end);
      if (end != text.size() || !std::isfinite(value) || value < 0 || value > (a == "--atlas-exposure" ? 2.f : 1.f))
        throw std::runtime_error("invalid " + a);
      if (a == "--atlas-exposure")
        atlasExposure_ = value;
      else
        atlasFlash_ = value;
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
      if (end != value.size() || (a == "--infinity-scene"     ? (n < 0 || n >= infinity_sync::sceneCount)
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
    return "  --infinity-scene N  Hold one optical scene (0..25)\n"
           "  --no-optical-bloom Disable shared GPU highlight pyramid for A/B timing\n"
           "  --atlas-reduced-motion Slow new Atlas effects; suppress beat flashes\n"
           "  --atlas-paused     Freeze Atlas animation; P toggles pause\n"
           "  --atlas-exposure F Atlas exposure cap 0..2 (default 1)\n"
           "  --atlas-flash-gain F Atlas beat light gain 0..1 (default 1)\n"
           "  --infinity-pixels N GPU budget (10000..4000000; default 400000)\n"
           "  --infinity-orbit Y P Fixed initial yaw/pitch in radians (yaw +/-pi, pitch +/-1.2)\n"
           "  --infinity-eye X Y  Fixed camera offset (-1..1); disables automatic dolly\n"
           "  --still             Fixed camera; accumulate up to 400 samples\n"
           "  --instrument-state N Seven-bit optical state (0..127; see demo README)\n"
           "  --grin-curvature F   Index gradient 0..1 (selects ray lab if auto)\n"
           "  --infinity-fps N    Adaptive GPU target 24..144 FPS (default 60)\n"
           "  --infinity-fixed-budget Disable automatic resolution adjustment\n"
           "  --infinity-adaptive Re-enable adaptation after an explicit pixel budget\n"
           "  --infinity-sequential-integrator Force sequential RK4 on all ray scenes\n"
           "  --infinity-raster-integrator Original raster RK4 for isolated GPU comparison\n"
           "  --validate-output Check HDR target values for non-finite results\n"
           "  --validate-targets Test GPU target storage and failed-resize recovery\n"
           "  --validate-ray-state Compare every active float ray sample against raster RK4\n"
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
    if (!reference_ && !rasterIntegrator_)
      for (int pass = 0; pass < 2; ++pass) {
        const auto gs = infinity_sequential::geometry(INTEGRATE_FS, pass == 0);
        sequential_[size_t(pass)] =
            glutil::linkProgram(infinity_sequential::vertex, gs, infinity_sequential::fragment, error_);
      }
    if (!reference_ && !rasterIntegrator_ && (!sequential_[0] || !sequential_[1])) {
      std::fprintf(stderr, "Sequential ray kernels unavailable; using raster RK4: %s\n", error_.c_str());
      for (auto& p : sequential_)
        if (p) {
          glDeleteProgram(p);
          p = 0;
        }
      rasterIntegrator_ = true;
      forceSequential_ = false;
      if (integrate_) error_.clear();
    }
    if (reference_)
      worldReference_ = program(FULLSCREEN_VS, infinity_worlds::fragment);
    else
      for (int mode = 0; mode < 4; ++mode) {
        const auto source = infinity_worlds::specialized(mode);
        worlds_[size_t(mode)] = program(FULLSCREEN_VS, source.c_str());
      }
    if (reference_) {
      atlasReference_ = program(FULLSCREEN_VS, dmt::gpu::fragment);
      if (atlasReference_) atlasConstants(atlasReference_);
    } else
      for (int mode = 0; mode < dmt::gpu::modes; ++mode) {
        const auto source = dmt::gpu::specialized(mode);
        atlasPrograms_[size_t(mode)] = program(FULLSCREEN_VS, source.c_str());
        if (atlasPrograms_[size_t(mode)]) atlasConstants(atlasPrograms_[size_t(mode)]);
      }
    atlasTemporal_ = program(FULLSCREEN_VS, dmt::gpu::temporal);
    const auto rayVertex = infinity_dimension::vertex(RAY_VS);
    const auto rayFragment = infinity_dimension::fragment(RAY_FS);
    rays_ = program(rayVertex.c_str(), rayFragment.c_str());
    compose_ = program(FULLSCREEN_VS, infinity_post::composite);
    bloomExtract_ = program(FULLSCREEN_VS, infinity_post::extract);
    if (std::any_of(chamber_.begin(), chamber_.end(), [](GLuint p) { return !p; }) || !post_ ||
        (reference_ && !trace_) || !show_ || !integrate_ || !rays_ || !compose_ || !bloomExtract_ ||
        (validateRays_ && !integrate_) ||
        (reference_ ? !worldReference_ : std::any_of(worlds_.begin(), worlds_.end(), [](GLuint p) { return !p; }))) {
      std::fprintf(stderr, "Infinity shader failure: %s\n", error_.c_str());
      return false;
    }
    if (!atlasTemporal_ ||
        (reference_ ? !atlasReference_
                    : std::any_of(atlasPrograms_.begin(), atlasPrograms_.end(), [](GLuint p) { return !p; }))) {
      std::fprintf(stderr, "Atlas shader failure: %s\n", error_.c_str());
      return false;
    }
    const auto polygon = infinity_worlds::heptagon();
    if (reference_) {
      glUseProgram(worldReference_);
      two(worldReference_, "uPolygon", polygon[0], polygon[1]);
    } else
      for (auto p : worlds_) {
        glUseProgram(p);
        two(p, "uPolygon", polygon[0], polygon[1]);
      }
    if (!reference_) {
      const auto assign = [&](GLuint p, const char* name, int unit) {
        if (p) {
          glUseProgram(p);
          integer(p, name, unit);
        }
      };
      assign(post_, "uImg", 0);
      assign(show_, "uAcc", 0);
      assign(integrate_, "uCk", 0);
      assign(rays_, "uState", 0);
      assign(compose_, "uImg", 0);
      assign(compose_, "uBloom", 1);
      assign(bloomExtract_, "uImg", 0);
      assign(sequential_[1], "uCk", 0);
      for (auto p : chamber_) assign(p, "uAntField", 2);
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
    glGenTextures(1, &antTexture_);
    glBindTexture(GL_TEXTURE_2D, antTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 64, 64, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    timer_.init(profile_ || (adaptive_ && !still_ && !reference_), profile_);
    glutil::setupFullscreenVao(vao_, vbo_);
    if (validateTargets_) {
      Target test;
      std::string expectedError;
      bool valid = test.resize(17, 11, GL_RGBA16F, false, error_);
      const auto first = test.texture;
      valid = valid && test.resize(17, 11, GL_RGBA32F, false, error_) && test.texture != first;
      GLint format = 0, maximum = 0;
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
      valid = valid && format == GL_RGBA32F;
      const auto changedFormat = test.texture;
      valid = valid && test.resize(17, 11, GL_RGBA32F, true, error_) && test.texture != changedFormat;
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
      glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maximum);
      const auto retained = test.texture;
      valid = valid && test.resize(17, 11, GL_RGBA32F, true, error_) && test.texture == retained;
      valid = valid && format == GL_RGBA32F && maximum == 4 && !test.resize(0, 11, GL_RGBA16F, false, expectedError) &&
              test.texture == retained;
      test.bind();
      valid = valid && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
      test.release();
      if (!valid) {
        std::fprintf(stderr, "GPU target validation failed: %s\n", error_.c_str());
        return false;
      }
      std::printf("GPU target validation: format/mip changes and invalid resize recovery passed\n");
    }
    return state_.resize(512, 182, GL_RGBA32F, false, error_) &&
           checkpoints_.resize(16, 182, GL_RGBA32F, false, error_);
  }
  bool update(const de::FrameInfo& f, const de::Input& in, de::Frame& out) override {
    frame_ = f;
    atlasMotion_.advance(f.sync.beat, atlasPaused_, atlasReduced_);
    traffic_.advance(f.sync.beat);
    ballWorld_.advance(f.sync.beat);
    viewDirty_ = false;
    if (auto* window = glfwGetCurrentContext()) {
      const bool pauseKey = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
      if (pauseKey && !atlasPauseKey_) {
        atlasPaused_ = !atlasPaused_;
        if (atlasPaused_ && cue_.scene >= 16) manual_ = cue_.scene;
      }
      atlasPauseKey_ = pauseKey;
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
    if (!atlasPaused_)
      atlasEye_ = atlasReduced_ ? infinity_parallax::eye(f.sync.beat, pointerX_, pointerY_, true) : eye_;
    int current = infinity_sync::sample(f.sync.beat, 0, false, 0, 0, 0, manual_).scene;
    if (in.pressed(de::Key::Right)) manual_ = (current + 1) % infinity_sync::sceneCount;
    if (in.pressed(de::Key::Left)) manual_ = (current + infinity_sync::sceneCount - 1) % infinity_sync::sceneCount;
    if (in.pressed(de::Key::Space)) manual_ = -1;
    cue_ = infinity_sync::sample(f.sync.beat, f.sync.pulse, f.music.active, f.music.bass, f.music.mid, f.music.treble,
                                 manual_);
    const auto& bands = bandMotion_.update(cue_.bass, cue_.mid, cue_.treble, f.dt);
    cue_.bass = bands[0];
    cue_.mid = bands[1];
    cue_.treble = bands[2];
    if (cue_.scene != announced_) {
      std::fprintf(stdout, "infinity scene %02d: %s / beat %.2f\n", cue_.scene, names[cue_.scene], f.sync.beat);
      announced_ = cue_.scene;
    }
    out.travelers = false;
    out.ticker = std::string(names[cue_.scene]) +
                 (cue_.scene < 12   ? "  /  HOLD INFINITY BY PURPLETWILIGHT  /  "
                  : cue_.scene < 16 ? "  /  QRI ESSAY INSPIRED GEOMETRY STUDIES  /  "
                                    : "  /  DMT ATLAS / GEOMETRY STUDIES  /  ") +
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
    int budget = (cue_.scene >= 4 && cue_.scene < 10) ? pixels_ / 3 : pixels_;
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
    else if (cue_.scene < 12)
      grin(cue_.scene, w, h);
    else if (cue_.scene < 16)
      worldSheet(cue_.scene, w, h);
    else
      atlas(cue_.scene, w, h);
    if (validateOutput_ && !outputChecked_[size_t(cue_.scene)]) {
      std::vector<float> values(size_t(w * h * 4));
      output_.bind();
      glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, values.data());
      for (float value : values)
        if (!std::isfinite(value)) {
          error = "Non-finite HDR output in scene " + std::to_string(cue_.scene);
          timer_.end();
          return false;
        }
      outputChecked_[size_t(cue_.scene)] = true;
      std::printf("HDR output validation: scene %02d finite (%d pixels)\n", cue_.scene, w * h);
    }
    const Target* presented = &output_;
    if (cue_.scene >= 16) {
      bool reset = !atlasHistoryValid_ || previous_ != cue_.scene || viewDirty_ || atlasPaused_ || still_;
      for (auto& history : atlasHistory_) {
        reset = reset || history.w != w || history.h != h;
        if (!history.resize(w, h, GL_RGBA16F, false, error)) {
          timer_.end();
          return false;
        }
      }
      auto& destination = atlasHistory_[size_t(atlasHistoryIndex_)];
      auto& history = atlasHistory_[size_t(1 - atlasHistoryIndex_)];
      destination.bind();
      glUseProgram(atlasTemporal_);
      output_.sample(0);
      history.sample(1);
      integer(atlasTemporal_, "uCurrent", 0);
      integer(atlasTemporal_, "uHistory", 1);
      two(atlasTemporal_, "uTexel", 1.f / float(w), 1.f / float(h));
      one(atlasTemporal_, "uWeight", reset ? 0.f : float(std::exp(-std::max(0.0, double(f.dt)) * 16.0)));
      quad();
      if (validateOutput_) {
        std::vector<float> values(size_t(w * h * 4));
        glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, values.data());
        if (std::any_of(values.begin(), values.end(), [](float x) { return !std::isfinite(x); })) {
          error = "Non-finite Atlas temporal output";
          timer_.end();
          return false;
        }
      }
      presented = &destination;
      atlasHistoryIndex_ = 1 - atlasHistoryIndex_;
      atlasHistoryValid_ = true;
    } else
      atlasHistoryValid_ = false;
    if (opticalBloom_) {
      if (!bloom_.resize(std::max(1, w / 2), std::max(1, h / 2), GL_RGBA16F, true, error)) {
        timer_.end();
        return false;
      }
      bloom_.bind();
      glUseProgram(bloomExtract_);
      presented->sample();
      sampler(bloomExtract_, "uImg", 0);
      two(bloomExtract_, "uTexel", 1.f / float(w), 1.f / float(h));
      quad();
      bloom_.generate();
      ++bloomBuilds_;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, f.width, f.height);
    glDisable(GL_BLEND);
    glUseProgram(compose_);
    presented->sample();
    sampler(compose_, "uImg", 0);
    if (opticalBloom_) bloom_.sample(1);
    sampler(compose_, "uBloom", 1);
    const bool frozen = cue_.scene >= 16 && (atlasPaused_ || still_);
    const float breath = cue_.scene >= 16 ? infinity_motion::breath(still_ ? 0 : atlasMotion_.time()) : cue_.breath;
    one(compose_, "uGlow",
        opticalBloom_ ? (cue_.scene < 12 ? .07f : .14f) * (frozen ? 1.f : (.94f + .12f * breath)) : 0.f);
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
    if (antTexture_) {
      glDeleteTextures(1, &antTexture_);
      antTexture_ = 0;
    }
    if (profile_)
      std::printf("GPU workload: frames=%zu ray-solves=%zu uniform-lookups=%zu mipmaps=%zu mode=%s\n", submitFrames_,
                  integrations_, uniformLookups_, mipBuilds_, reference_ ? "reference" : "optimized");
    for (auto& p : specialized_)
      if (p) {
        glDeleteProgram(p);
        p = 0;
      }
    for (auto& p : atlasPrograms_)
      if (p) {
        glDeleteProgram(p);
        p = 0;
      }
    if (atlasReference_) {
      glDeleteProgram(atlasReference_);
      atlasReference_ = 0;
    }
    if (atlasTemporal_) {
      glDeleteProgram(atlasTemporal_);
      atlasTemporal_ = 0;
    }
    for (auto& history : atlasHistory_) history.release();
    for (auto& p : worlds_)
      if (p) {
        glDeleteProgram(p);
        p = 0;
      }
    if (worldReference_) {
      glDeleteProgram(worldReference_);
      worldReference_ = 0;
    }
    for (auto& p : sequential_)
      if (p) {
        glDeleteProgram(p);
        p = 0;
      }
    uniforms_.clear();

    if (profile_) std::printf("GPU highlight pyramids: %zu\n", bloomBuilds_);
    bloom_.release();
    image_.release();
    output_.release();
    state_.release();
    checkpoints_.release();
    for (auto& p : chamber_)
      if (p) {
        glDeleteProgram(p);
        p = 0;
      }
    for (auto* p : {&post_, &trace_, &show_, &integrate_, &rays_, &compose_, &bloomExtract_})
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
