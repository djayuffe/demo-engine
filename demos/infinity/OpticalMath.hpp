#pragma once
#include <array>
#include <cmath>
namespace infinity_math {
struct Camera {
  float m[9];
  float back[3];
};
inline Camera camera(float yaw, float pitch) {
  float cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
  return {{cy, 0, -sy, -sy * sp, cp, -cy * sp, sy * cp, sp, cy * cp}, {sy * cp, sp, cy * cp}};
}
inline std::array<float, 4> hyperplane(float x, float y, float z, float t) {
  const float scale = 1.f / std::sqrt(1.f - t * t);
  return {x * scale, y * scale, z * scale, t * scale};
}
inline std::array<float, 4> turn(double flow, int sheet) {
  const double a = std::remainder(flow * .11 + sheet * .34, 6.283185307179586);
  const double b = std::remainder(flow * .07 + .25, 6.283185307179586);
  return {float(std::cos(a)), float(std::sin(a)), float(std::cos(b)), float(std::sin(b))};
}
} // namespace infinity_math
