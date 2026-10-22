#pragma once
#include <stdexcept>
#include <string>

namespace infinity_opt {
inline void replace(std::string& source, const std::string& from, const std::string& to) {
  auto at = source.find(from);
  if (at == std::string::npos) throw std::runtime_error("Infinity shader contract missing: " + from);
  source.replace(at, from.size(), to);
}
// The numerical integrator is unchanged. Constant switches let the compiler
// remove unused medium/face/horizon/spectrum/layer branches from its hot loop.
inline std::string specialize(std::string source, int state) {
  if (state < 0 || state > 127) throw std::runtime_error("Invalid optical specialization");
  const char* names[] = {"uMedium", "uHole", "uCurved", "uWhite", "uInside", "uSteps"};
  const int bits[] = {1, 2, 4, 8, 16, 64};
  for (int i = 0; i < 6; ++i)
    replace(source, std::string("uniform int ") + names[i] + ";",
            std::string("const int ") + names[i] + " = " + (state & bits[i] ? "1;" : "0;"));
  replace(source, "uniform float uNout;", std::string("const float uNout = ") + (state & 32 ? "1.33;" : "1.0;"));
  return source;
}
inline int stateForScene(int scene, int overrideState = -1) {
  return overrideState >= 0 ? overrideState
         : scene == 4
             ? 0
             : 5 | (scene == 6 ? 2 : 0) | (scene == 7 ? 64 : 0) | (scene == 8 ? 40 : 0) | (scene == 9 ? 16 : 0);
}
struct RaySolveKey {
  float aspect = -1, curvature = -1;
  int rays = 0, wavelengths = 0;
  bool operator==(const RaySolveKey&) const = default;
};
} // namespace infinity_opt
