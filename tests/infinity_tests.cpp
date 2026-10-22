#include "Sync.hpp"
#include "Parallax.hpp"
#include "OpticalMath.hpp"
#include "MatrixLife.hpp"
#include "RenderCorrections.hpp"
#include "Optimize.hpp"
#include "Shaders.hpp"
#include "Dodecahedron.hpp"
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
int main() {
  const std::string shader =
      "#version 410 core\nvoid main(){float a=smoothstep(1.0,0.0,0.5);float b=smoothstep(0.0,1.0,0.5);}";
  const auto corrected = infinity_render::orderedRamps(shader);
  require(corrected.rfind("#version 410 core\n", 0) == 0, "shader version displaced");
  require(corrected.find("orderedRamp(1.0,0.0,0.5)") != std::string::npos &&
              corrected.find("orderedRamp(0.0,1.0,0.5)") != std::string::npos,
          "increasing or decreasing fade left undefined");
  require(shader.find("orderedRamp") == std::string::npos, "source shader mutated");
  require(std::isfinite(infinity_parallax::smooth(std::numeric_limits<float>::quiet_NaN(), 1, .1)),
          "invalid pointer state did not recover");
  const auto invalidEye = infinity_parallax::eye(0, std::numeric_limits<float>::infinity(), 10, true);
  require(invalidEye.x == 0 && std::abs(invalidEye.y - .16f) < 1e-6, "invalid eye coordinates not bounded");
  const auto leftEye = infinity_parallax::eye(0, -1, 0, true);
  const auto rightEye = infinity_parallax::eye(0, 1, 0, true);
  require(leftEye.x < 0 && rightEye.x > 0, "eye translation direction incorrect");
  const float nearShift = (rightEye.x - leftEye.x) / 2.f, farShift = (rightEye.x - leftEye.x) / 5.f;
  require(nearShift > farShift, "near layer must show larger parallax");
  require(infinity_parallax::eye(20, 0, 0, true).z == 0, "fixed eye still dollies");
  const float oneStep = infinity_parallax::smooth(0, 1, .1);
  const float twoSteps = infinity_parallax::smooth(infinity_parallax::smooth(0, 1, .05), 1, .05);
  require(std::abs(oneStep - twoSteps) < 1e-6, "pointer smoothing depends on frame rate");
  for (int step = 0; step < 80; ++step) {
    const auto cam = infinity_math::camera(float(step) * .31f, float(step - 40) * .03f);
    for (int a = 0; a < 3; ++a)
      for (int b = 0; b < 3; ++b) {
        float dot = 0;
        for (int k = 0; k < 3; ++k) dot += cam.m[a * 3 + k] * cam.m[b * 3 + k];
        require(std::abs(dot - (a == b ? 1.f : 0.f)) < 1e-5, "camera basis loses orthogonality");
      }
    for (int sheet = 0; sheet < 3; ++sheet) {
      const auto turn = infinity_math::turn(double(step) * 1e8, sheet);
      require(std::abs(turn[0] * turn[0] + turn[1] * turn[1] - 1) < 1e-6 &&
                  std::abs(turn[2] * turn[2] + turn[3] * turn[3] - 1) < 1e-6,
              "plane bend changes radius");
    }
  }
  for (const char* chamber : {infinity_shader::CHAMBER_OBJECT, infinity_shader::CHAMBER_CUBE,
                              infinity_shader::CHAMBER_FLAT, infinity_shader::CHAMBER_HYPER})
    require(infinity_matrix::expand(chamber).find("float ants=") != std::string::npos, "matrix walkers missing");
  using namespace infinity_sync;
  require(sample(31.999, 0, false, 0, 0, 0).scene == 0, "scene changes before boundary");
  require(sample(32, 0, false, 0, 0, 0).scene == 1, "bar-boundary cut missing");
  require(sample(384, 0, false, 0, 0, 0).scene == 0, "show does not wrap");
  require(sample(32, 0, false, 0, 0, 0).fade == 0, "scene cut is not dark");
  require(sample(33, 0, false, 0, 0, 0).fade == 1, "scene does not fade in");
  require(sample(100, 1, true, 1, 1, 1, 5).scene == 5, "manual selection ignored");
  require(sample(100, 1, true, 1, 1, 1, 5).fade == 1, "manual view unexpectedly fades");
  require(sample(std::numeric_limits<double>::quiet_NaN(), 0, false, 0, 0, 0).scene == 0, "invalid clock unsafe");
  const auto quiet = sample(10, 0, true, 0, 0, 0), loud = sample(10, 1, true, 1, 1, 1);
  require(loud.energy > quiet.energy, "audio bands do not excite scene");
  require(sample(8, 0, true, 0, 0, 0).beat == 1.f, "downbeat attack missing");
  require(sample(8.5, 0, true, 0, 0, 0).beat < .02f, "beat decay washes out lights");
  require(sample(8.25, 0, false, 0, 0, 0).beat == sample(8.25, 0, true, 0, 0, 0).beat,
          "clock pulse drifts with audio availability");
  require(sample(8, 0, true, 0, 0, 0).strike > sample(9, 0, true, 0, 0, 0).strike, "bar accent missing");
  require(sample(8.5, 1, true, 1, 1, 1).beat < .02f, "sustained audio washes out beat attack");
  require(sample(8.5, 0, true, 0, 0, 0).flutter == 1.f, "offbeat transient missing");
  require(std::abs(sample(8, 0, false, 0, 0, 0).breath - sample(16, 0, false, 0, 0, 0).breath) < 1e-6,
          "organic phrase does not loop");
  float a[24], b[24];
  pulses(12.25, 1, a);
  pulses(12.25, 1, b);
  for (int i = 0; i < 24; ++i) require(a[i] == b[i] && std::isfinite(a[i]), "nondeterministic pulse");
  pulses(12.5, 1, b);
  require(b[0] > a[0], "beat fronts do not propagate");
  pulses(13, 1, b);
  require(b[0] == 0, "new beat does not launch front");
  pulses(0, 1, b);
  require(b[5] == 0, "pre-show beat emitted light");
  using namespace infinity_geometry;
  require(sizeof(vertices) / sizeof(float) == 60, "20 vertices missing");
  require(sizeof(edgeA) / sizeof(float) == 90, "30 edges missing");
  for (int i = 0; i < 12; ++i) {
    float len = 0;
    for (int j = 0; j < 3; ++j) len += faces[3 * i + j] * faces[3 * i + j];
    require(std::abs(len - 1) < 1e-6, "face normal not unit");
  }
  for (int i = 0; i < 30; ++i) {
    float len = 0;
    for (int j = 0; j < 3; ++j) {
      float d = edgeA[3 * i + j] - edgeB[3 * i + j];
      len += d * d;
    }
    require(len > 0, "degenerate edge");
  }
  for (int i = 0; i < 12; ++i)
    for (int j = i + 1; j < 12; ++j) {
      float d = 0;
      for (int k = 0; k < 3; ++k) d += faces[3 * i + k] * faces[3 * j + k];
      if (std::abs(d - 1 / std::sqrt(5.f)) < 1e-6)
        require(std::abs(d - kleinT * kleinT) < 1e-6, "hyperbolic adjacent mirrors not orthogonal");
    }
  for (int f = 0; f < 12; ++f) {
    const auto n = infinity_math::hyperplane(faces[3 * f], faces[3 * f + 1], faces[3 * f + 2], kleinT);
    auto dot = [](const auto& a, const auto& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] - a[3] * b[3]; };
    require(std::abs(dot(n, n) - 1) < 1e-5, "Lorentz mirror normal is not spacelike unit");
    std::array<float, 4> p = {0, 0, 0, 1}, reflected = p;
    const float d = dot(p, n);
    for (int k = 0; k < 4; ++k) reflected[k] -= 2 * d * n[k];
    require(std::abs(dot(reflected, reflected) + 1) < 1e-5, "hyperbolic bend leaves unit hyperboloid");
    const float back = dot(reflected, n);
    for (int k = 0; k < 4; ++k) reflected[k] -= 2 * back * n[k];
    for (int k = 0; k < 4; ++k) require(std::abs(reflected[k] - p[k]) < 1e-5, "mirror bend is not involutive");
  }
  int degrees[20] = {};
  bool edgesSeen[20][20] = {};
  auto vertexIndex = [&](const float* point) {
    for (int v = 0; v < 20; ++v) {
      float d = 0;
      for (int k = 0; k < 3; ++k) d += std::abs(point[k] - vertices[3 * v + k]);
      if (d < 1e-5) return v;
    }
    return -1;
  };
  float firstLength = 0;
  for (int e = 0; e < 30; ++e) {
    int a = vertexIndex(edgeA + 3 * e), b = vertexIndex(edgeB + 3 * e);
    require(a >= 0 && b >= 0 && a != b, "edge endpoint is not a vertex");
    require(!edgesSeen[a][b], "duplicate geometry edge");
    edgesSeen[a][b] = edgesSeen[b][a] = true;
    ++degrees[a];
    ++degrees[b];
    float d = 0;
    for (int k = 0; k < 3; ++k) {
      float x = edgeA[3 * e + k] - edgeB[3 * e + k];
      d += x * x;
    }
    if (e == 0) firstLength = d;
    require(std::abs(d - firstLength) < 1e-5, "dodecahedron edges have unequal lengths");
  }
  for (int degree : degrees) require(degree == 3, "dodecahedron vertex valence must be three");
  for (int f = 0; f < 12; ++f) {
    int count = 0;
    for (int v = 0; v < 20; ++v) {
      float d = 0;
      for (int k = 0; k < 3; ++k) d += faces[3 * f + k] * vertices[3 * v + k];
      require(d <= 1.00001f, "vertex escapes convex face plane");
      if (std::abs(d - 1) < 1e-5) ++count;
    }
    require(count == 5, "face must contain five coplanar vertices");
  }
  using namespace infinity_opt;
  for (int mask = 0; mask < 128; ++mask) {
    auto kernel = specialize(infinity_shader::TRACE_FS, mask);
    require(kernel.find("uniform int uMedium;") == std::string::npos, "specialization kept medium branch uniform");
    require(kernel.find("const int uHole = " + std::string(mask & 2 ? "1;" : "0;")) != std::string::npos,
            "incorrect horizon specialization");
    require(kernel.find(mask & 32 ? "const float uNout = 1.33;" : "const float uNout = 1.0;") != std::string::npos,
            "incorrect surround specialization");
  }
  require(stateForScene(4) == 0 && stateForScene(5) == 5 && stateForScene(7) == 69 && stateForScene(8) == 45,
          "preset states changed");
  require(stateForScene(9, 127) == 127, "optical override dropped");
  RaySolveKey key{1.5f, .65f, 46, 1};
  require(key == RaySolveKey{1.5f, .65f, 46, 1}, "identical GPU field not reused");
  require(!(key == RaySolveKey{1.6f, .65f, 46, 1}) && !(key == RaySolveKey{1.5f, .66f, 46, 1}) &&
              !(key == RaySolveKey{1.5f, .65f, 26, 1}) && !(key == RaySolveKey{1.5f, .65f, 46, 7}),
          "ray solve cache invalidation incomplete");
  SceneQuality slow;
  for (int f = 0; f < 250; ++f) slow.observe(60, f, 16.667);
  require(slow.fraction() >= .25f && slow.fraction() < .4f, "slow GPU budget did not reduce safely");
  float held = slow.fraction();
  slow.observe(1, 249, 16.667);
  require(slow.fraction() == held, "stale GPU query changed budget");
  slow.observe(std::numeric_limits<double>::quiet_NaN(), 250, 16.667);
  require(slow.fraction() == held, "invalid GPU query changed budget");
  for (int f = 251; f < 1000; ++f) slow.observe(4, f, 16.667);
  require(slow.fraction() == 1.f, "GPU budget did not recover with headroom");
  SceneQuality stable;
  for (int f = 0; f < 100; ++f) stable.observe(16, f, 16.667);
  require(stable.fraction() == 1.f, "GPU governor oscillates near target");
  std::puts("Infinity geometry, clock, beat fronts and band response passed");
}
