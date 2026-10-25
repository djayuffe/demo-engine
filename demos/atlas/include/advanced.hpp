#pragma once
#include "geometry.hpp"
#include <limits>
namespace dmt::advanced {
struct Vec3 {
  double x{}, y{}, z{};
  Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
  Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
  Vec3 operator*(double a) const { return {x * a, y * a, z * a}; }
};
inline double dot3(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Vec3 normalized(Vec3 a) {
  double n = std::hypot(a.x, a.y, a.z);
  if (!std::isfinite(n) || n < 1e-14) throw std::domain_error("zero direction");
  return a * (1 / n);
}
inline V4 tangentAtOrigin(Vec3 v) {
  v = normalized(v);
  return {0, v.x, v.y, v.z};
}
inline V4 geodesicRay(V4 origin, V4 tangent, double t) {
  if (!finite(origin) || !finite(tangent) || !std::isfinite(t) || std::abs(t) > 40 ||
      std::abs(lorentz(origin, origin) + 1) > 1e-7 || std::abs(lorentz(origin, tangent)) > 1e-7 ||
      std::abs(lorentz(tangent, tangent) - 1) > 1e-7)
    throw std::domain_error("invalid geodesic initial conditions");
  V4 p{};
  for (int i = 0; i < 4; i++) p[i] = origin[i] * std::cosh(t) + tangent[i] * std::sinh(t);
  return p;
}
inline V4 tangentFromOrigin(V4 origin, Vec3 direction) {
  if (!finite(origin) || std::abs(lorentz(origin, origin) + 1) > 1e-7 || origin[0] < 1)
    throw std::domain_error("invalid camera origin");
  direction = normalized(direction);
  double projection = origin[1] * direction.x + origin[2] * direction.y + origin[3] * direction.z;
  return {projection, direction.x + origin[1] * projection / (origin[0] + 1),
          direction.y + origin[2] * projection / (origin[0] + 1),
          direction.z + origin[3] * projection / (origin[0] + 1)};
}
struct Sphere {
  V4 center;
  double radius;
};
// Ray/sphere intersection in H3. cosh(distance(ray(t),center)) = A*cosh(t)+B*sinh(t).
inline double intersectSphere(V4 o, V4 d, Sphere s) {
  if (!finite(o) || !finite(d) || !finite(s.center) || !std::isfinite(s.radius) || s.radius <= 0 || s.radius > 40)
    throw std::domain_error("invalid sphere radius");
  double A = -lorentz(o, s.center), B = -lorentz(d, s.center), C = std::cosh(s.radius);
  // In u=e^t: (A+B)u^2 - 2 C u + (A-B)=0.
  double aa = A + B, cc = A - B, disc = C * C - aa * cc;
  if (disc < 0) return std::numeric_limits<double>::infinity();
  double root = std::sqrt(std::max(0., disc));
  double best = std::numeric_limits<double>::infinity();
  if (std::abs(aa) > 1e-13) {
    for (double u : {cc / (C + root), (C + root) / aa})
      if (u >= 1.0 + 1e-9) best = std::min(best, std::log(u));
  } else if (C > 0) {
    double u = cc / (2 * C);
    if (u >= 1.0 + 1e-9) best = std::log(u);
  }
  return best;
}
struct GeodesicPlane {
  V4 normal;
};
inline double intersectPlane(V4 origin, V4 direction, GeodesicPlane plane) {
  (void)geodesicRay(origin, direction, 0);
  if (!finite(plane.normal) || std::abs(lorentz(plane.normal, plane.normal) - 1) > 1e-7)
    throw std::domain_error("invalid geodesic plane");
  double denominator = lorentz(direction, plane.normal);
  if (std::abs(denominator) < 1e-14) return std::numeric_limits<double>::infinity();
  double ratio = -lorentz(origin, plane.normal) / denominator;
  if (ratio <= 1e-9 || ratio >= 1) return std::numeric_limits<double>::infinity();
  return std::atanh(ratio);
}
inline V4 reflectInPlane(V4 vector, GeodesicPlane plane) {
  if (!finite(vector) || !finite(plane.normal) || std::abs(lorentz(plane.normal, plane.normal) - 1) > 1e-7)
    throw std::domain_error("invalid Lorentz reflection");
  const double projection = 2 * lorentz(vector, plane.normal);
  for (size_t i = 0; i < 4; ++i) vector[i] -= projection * plane.normal[i];
  return vector;
}
inline Vec3 poincareBall(V4 p) {
  if (!finite(p) || p[0] < 1) throw std::domain_error("invalid hyperboloid projection");
  double d = p[0] + 1;
  return {p[1] / d, p[2] / d, p[3] / d};
}
inline double radialLod(double pixelsPerUnit, double projectedScale) {
  if (!std::isfinite(pixelsPerUnit) || !std::isfinite(projectedScale) || pixelsPerUnit <= 0 || projectedScale <= 0)
    return 0;
  return std::max(0., std::log2(pixelsPerUnit) + std::log2(projectedScale));
}
inline double smoothstep(double a, double b, double x) {
  if (a == b) return x < a ? 0. : 1.;
  double t = std::clamp((x - a) / (b - a), 0., 1.);
  return t * t * (3 - 2 * t);
}
} // namespace dmt::advanced
