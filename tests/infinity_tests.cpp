#include "Sync.hpp"
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
