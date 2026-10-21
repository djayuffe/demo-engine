#include "Sync.hpp"
#include "Dodecahedron.hpp"
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
int main() {
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
  std::puts("Infinity geometry, clock, beat fronts and band response passed");
}
