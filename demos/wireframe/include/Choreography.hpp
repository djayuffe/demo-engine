#pragma once
// Time-driven control of the engine's experimental renderer features (spec effects #86, #13, #25).
// Pure functions of time so they can be unit-tested and are identical on every run.
#include <algorithm>
#include <cmath>
namespace choreo {

inline float smooth(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }

// Row-major 3x3 matrices.
struct M3 {
  float m[9];
};
inline M3 identity() { return {{1, 0, 0, 0, 1, 0, 0, 0, 1}}; }
inline M3 operator*(const M3& a, const M3& b) {
  M3 r{};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k) r.m[i * 3 + j] += a.m[i * 3 + k] * b.m[k * 3 + j];
  return r;
}
inline M3 lerp(const M3& a, const M3& b, float u) {
  M3 r{};
  for (int i = 0; i < 9; ++i) r.m[i] = a.m[i] + (b.m[i] - a.m[i]) * u;
  return r;
}
// The two generators of the noncommutative camera (spec #86): A(a)B(b) != B(b)A(a).
inline M3 shearA(float a) { return {{1, a, 0, 0, 1, 0, 0, 0, 1}}; }   // x += a*y
inline M3 shearB(float b) { return {{1, 0, 0, b, 1, 0, 0, 0, 1}}; }   // y += b*x

// 20-second cycle; each new camera move multiplies on the LEFT, so undoing moves in the order they were
// made does not return to the start. The residual is the group commutator.
//   0-4 s  A(a)                    8-12 s  A(-a) B(b) A(a)
//   4-8 s  B(b) A(a)              12-16 s  B(-b) A(-a) B(b) A(a)   (a visible, non-identity residual)
//  16-20 s relax back to the identity before the next, stronger, cycle.
inline M3 noncommutativeCamera(double t) {
  t = std::max(0.0, t);
  const int cycle = int(t / 20.0);
  const float ph = float(std::fmod(t, 20.0)), amp = std::min(1.6f, .55f + .25f * float(cycle)),
              a = .9f * amp, b = .7f * amp;
  const float s = smooth(std::fmod(ph, 4.f) / 4.f);
  const M3 g1 = shearB(b) * shearA(a), g2 = shearA(-a) * g1, g3 = shearB(-b) * shearA(-a) * shearB(b) * shearA(a);
  if (ph < 4.f) return shearA(a * s);
  if (ph < 8.f) return shearB(b * s) * shearA(a);
  if (ph < 12.f) return shearA(-a * s) * g1;
  if (ph < 16.f) return shearB(-b * s) * g2;
  return lerp(g3, identity(), s);
}
// How far the camera's residual is from the identity (0 at the start of every cycle).
inline float shearResidual(const M3& g) {
  const M3 i = identity();
  float r = 0;
  for (int k = 0; k < 9; ++k) r = std::max(r, std::fabs(g.m[k] - i.m[k]));
  return r;
}

// Strength envelope of a sphere/layer that is on for `on` seconds of every `period`, with soft edges.
inline float pulse(double t, double period, double on) {
  const double ph = std::fmod(std::max(0.0, t), period);
  return smooth(float(ph / 1.5)) * smooth(float((on - ph) / 1.5));
}

struct Look {
  M3 shear = identity();
  float temporal = 0.f, memory = 0.f;
  float look[5] = {0, 0, 0, 0, 0};   // gravity, failure, colorless, specular, negative reflection
};
// The per-scene presets (the keys N, T and M force an effect on in any scene).
inline Look forScene(int scene, double t, bool forceShear, bool forceTemporal, bool forceMemory,
                     const bool* forceLook = nullptr) {
  Look l;
  if (scene == 59 || forceShear) l.shear = noncommutativeCamera(t);
  if (scene == 60 || scene == 65) l.temporal = .85f * pulse(t, 12.0, 9.0);
  if (scene == 62 || scene == 63 || scene == 65) l.memory = .8f;
  if (forceTemporal) l.temporal = .85f;
  if (forceMemory) l.memory = .8f;
  // Look presets: (scene -> effect). Each breathes on its own period so it is never on for the whole scene.
  struct P { int scene, fx; double period, on; };
  static const P presets[] = {{61, 2, 14, 10}, {62, 1, 12, 8}, {63, 0, 12, 9}, {64, 3, 10, 8}, {66, 4, 14, 11},
                              {67, 4, 16, 12}, {68, 0, 14, 10}, {69, 3, 12, 9}, {70, 2, 16, 11}, {71, 1, 12, 8},
                              {72, 0, 14, 10}};
  for (const P& p : presets)
    if (p.scene == scene) l.look[p.fx] = .9f * pulse(t, p.period, p.on);
  if (forceLook)
    for (int i = 0; i < 5; ++i)
      if (forceLook[i]) l.look[i] = .9f;
  return l;
}
}  // namespace choreo
