#pragma once
#include <algorithm>
#include <cmath>
namespace infinity_parallax {
struct Eye {
  float x = 0, y = 0, z = 0;
};
inline float smooth(float current, float target, double dt) {
  if (!std::isfinite(current)) current = 0;
  if (!std::isfinite(target)) target = 0;
  dt = std::isfinite(dt) ? std::clamp(dt, 0.0, 0.1) : 0.0;
  return current + (std::clamp(target, -1.f, 1.f) - current) * float(1.0 - std::exp(-dt * 10.0));
}
inline Eye eye(double beat, float x, float y, bool fixed) {
  beat = std::isfinite(beat) ? std::max(0.0, beat) : 0.0;
  x = std::isfinite(x) ? std::clamp(x, -1.f, 1.f) : 0.f;
  y = std::isfinite(y) ? std::clamp(y, -1.f, 1.f) : 0.f;
  const double phase = std::fmod(beat, 64.0) * 0.098174770424681;
  return {.24f * x + (fixed ? 0.f : .12f * float(std::sin(phase))),
          .16f * y + (fixed ? 0.f : .06f * float(std::sin(phase * 2.0))), fixed ? 0.f : .08f * float(std::sin(phase))};
}
} // namespace infinity_parallax
