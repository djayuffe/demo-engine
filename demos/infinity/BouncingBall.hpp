#pragma once
#include "Dodecahedron.hpp"
#include "OpticalMath.hpp"
#include <array>
#include <cmath>
#include <algorithm>
namespace infinity_ball {
using V = std::array<double, 4>;
inline double dot(const V& a, const V& b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] - a[3] * b[3];
}
struct Ball {
  V p = {.12, -.08, .06, 0}, v = {.57, .43, .69, 0};
  V impactPoint = {.12, -.08, .06, 0}, impactNormal = {0, 1, 0, 0};
  bool hyper = false, cube = false;
  int impacts = 0;
  double flash = 0, travelled = 0;
  static constexpr double radius = .14;
  Ball(bool h = false, bool c = false) : hyper(h), cube(c) {
    if (hyper) {
      p[3] = std::sqrt(1 + p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
      v[3] = (p[0] * v[0] + p[1] * v[1] + p[2] * v[2]) / p[3];
    }
    impactPoint = p;
    const double length = std::sqrt(dot(v, v));
    for (auto& x : v) x /= length;
  }
  V normal(int face) const {
    if (cube) {
      V n = {0, 0, 0, 0};
      n[face / 2] = (face % 2) ? -1 : 1;
      return n;
    }
    static const auto euclidean = []() {
      std::array<V, 12> planes{};
      for (int i = 0; i < 12; ++i) {
        const float* f = infinity_geometry::faces + 3 * i;
        V n = {f[0], f[1], f[2], 0};
        const double length = std::sqrt(dot(n, n));
        for (auto& x : n) x /= length;
        planes[i] = n;
      }
      return planes;
    }();
    static const auto curved = []() {
      std::array<V, 12> planes{};
      for (int i = 0; i < 12; ++i) {
        const V& f = euclidean[i];
        const double t = infinity_geometry::kleinT;
        const double scale = 1 / std::sqrt(1 - t * t);
        planes[i] = {f[0] * scale, f[1] * scale, f[2] * scale, t * scale};
      }
      return planes;
    }();
    return hyper ? curved[face] : euclidean[face];
  }
  void travel(double distance) {
    flash *= std::exp(-distance * 16);
    travelled += distance;
    if (!hyper) {
      for (int k = 0; k < 3; ++k) p[k] += v[k] * distance;
      return;
    }
    const double ch = std::cosh(distance), sh = std::sinh(distance);
    V q = p;
    for (int k = 0; k < 4; ++k) {
      p[k] = ch * q[k] + sh * v[k];
      v[k] = sh * q[k] + ch * v[k];
    }
  }
  void step(double distance) {
    if (!std::isfinite(distance) || distance <= 0) return;
    for (int event = 0; distance > 1e-10 && event < 12; ++event) {
      double first = distance;
      int hit = -1;
      for (int f = 0; f < (cube ? 6 : 12); ++f) {
        auto n = normal(f);
        const double a = dot(p, n), b = dot(v, n);
        if (!hyper) {
          if (b > 1e-10) {
            double t = (1 - radius - a) / b;
            if (t >= -1e-9 && t <= first) {
              first = std::max(0.0, t);
              hit = f;
            }
          }
        } else {
          const double wall = -std::sinh(radius);
          if (a * std::cosh(first) + b * std::sinh(first) > wall && b > 0) {
            double lo = 0, hi = first;
            for (int j = 0; j < 40; ++j) {
              double mid = (lo + hi) * .5;
              if (a * std::cosh(mid) + b * std::sinh(mid) > wall)
                hi = mid;
              else
                lo = mid;
            }
            first = (lo + hi) * .5;
            hit = f;
          }
        }
      }
      travel(first);
      distance -= first;
      if (hit < 0) break;
      V n = normal(hit);
      if (hyper) {
        double a = dot(p, n);
        for (int k = 0; k < 4; ++k) n[k] += a * p[k];
        const double norm = std::sqrt(dot(n, n));
        for (auto& x : n) x /= norm;
      }
      const double vn = dot(v, n);
      impactPoint = p;
      impactNormal = n;
      for (int k = 0; k < 4; ++k) v[k] -= 2 * vn * n[k];
      ++impacts;
      // A grazing touch emits less energy than a head-on collision while a
      // minimum response keeps shallow contacts legible in the HDR scene.
      flash = std::max(flash, std::clamp(std::abs(vn), .24, 1.0));
    }
    if (hyper) {
      double norm = std::sqrt(-dot(p, p));
      for (auto& x : p) x /= norm;
      const double tangent = dot(p, v);
      for (int k = 0; k < 4; ++k) v[k] += tangent * p[k];
      norm = std::sqrt(dot(v, v));
      for (auto& x : v) x /= norm;
    }
  }
};
struct World {
  Ball cube{false, true}, flat{}, hyper{true, false};
  double tick = 0;
  void advance(double beat) {
    if (!std::isfinite(beat) || beat < 0) return;
    double target = std::floor(std::min(beat, 1e9) * 120);
    if (target < tick || target - tick > 4096) {
      *this = World();
      tick = target;
      return;
    }
    while (tick < target) {
      cube.step(.45 / 120);
      flat.step(.45 / 120);
      hyper.step(.45 / 120);
      ++tick;
    }
  }
  Ball sample(const Ball& ball, double beat) const {
    Ball result = ball;
    const double clock = std::isfinite(beat) ? std::clamp(beat, 0.0, 1e9) * 120 : 0;
    const double sub = clock - std::floor(clock);
    result.step(std::clamp(sub, 0.0, 1.0) * .45 / 120);
    return result;
  }
};
} // namespace infinity_ball
