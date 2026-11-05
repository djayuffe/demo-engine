#pragma once
#include <algorithm>
#include <cmath>

namespace infinity_composition {
struct Shot {
  float yaw = 0, pitch = 0;
  float eyeX = 0, eyeY = 0;
  float dolly = 1;
};

// A bounded, 128-flow-unit camera phrase shared by every 3D renderer. Integer
// harmonics make position and velocity continuous across the wrap, while the
// golden-angle scene offset prevents consecutive compositions from repeating.
inline Shot shot(int scene, double flow, float energy, bool frozen) {
  flow = std::isfinite(flow) ? flow : 0.0;
  energy = std::isfinite(energy) ? std::clamp(energy, 0.f, 1.5f) : 0.f;
  if (frozen) energy = 0; // A frozen pose must not respond to changing audio drive.
  const double local = frozen ? 0.0 : std::fmod(std::max(0.0, flow), 128.0);
  const double phase = local * 0.04908738521234052 + double(std::max(0, scene)) * 2.399963229728653;
  const float drive = .72f + .18f * energy;
  Shot out;
  out.yaw = drive * (.075f * float(std::sin(phase)) + .026f * float(std::sin(phase * 3.0 + .4)));
  out.pitch = drive * (.043f * float(std::sin(phase * 2.0 + .8)) + .014f * float(std::cos(phase * 4.0)));
  out.eyeX = drive * .045f * float(std::sin(phase + 1.1));
  out.eyeY = drive * .030f * float(std::cos(phase * 2.0 - .3));
  out.dolly = 1.f + drive * (.028f * float(std::sin(phase - .6)) + .010f * float(std::sin(phase * 3.0)));
  return out;
}
} // namespace infinity_composition
