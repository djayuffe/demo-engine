#pragma once
// The demo camera as pure functions of time, so it is unit-testable and identical on every run.
// Matrices are column-major 4x4 (OpenGL layout). Model transform G is a row-major 3x3.
#include <algorithm>
#include <cmath>
namespace cam {
constexpr float kPi = 3.14159265358979f;

inline void mul(float* o, const float* a, const float* b) {
  float r[16]{};
  for (int c = 0; c < 4; c++)
    for (int rr = 0; rr < 4; rr++)
      for (int k = 0; k < 4; k++) r[c * 4 + rr] += a[k * 4 + rr] * b[c * 4 + k];
  std::copy(r, r + 16, o);
}
inline void perspective(float* m, float fovy, float aspect, float zn, float zf) {
  const float f = 1 / std::tan(fovy * .5f);
  for (int i = 0; i < 16; i++) m[i] = 0;
  m[0] = f / aspect;
  m[5] = f;
  m[10] = (zf + zn) / (zn - zf);
  m[11] = -1;
  m[14] = (2 * zf * zn) / (zn - zf);
}

// Vertical field of view in radians. The base 52 degrees drifts slowly and tightens with the music; on a
// window narrower than 16:9 it widens so the HORIZONTAL field never drops below what 16:9 would give
// (otherwise a portrait window crops the object).
constexpr float kRefAspect = 16.f / 9.f;
inline float verticalFov(float t, float music, float aspect) {
  const float deg = 52.f + 3.f * std::sin(t * .07f) - 2.f * std::clamp(music, 0.f, 1.f);
  const float v = deg * kPi / 180.f;
  const float a = std::max(.05f, aspect);
  if (a >= kRefAspect) return v;
  const float hHalf = std::atan(std::tan(v * .5f) * kRefAspect);   // horizontal half-angle at 16:9
  return std::min(2.f * std::atan(std::tan(hHalf) / a), 2.6f);      // cap below 150 degrees
}

// Full projection * view * (rotation * model) matrix for the wireframe and travelers.
inline void mvp(float t, float aspect, float scale, float music, const float model3[9], float* M) {
  const float ml = std::clamp(music, 0.f, 1.f);
  const float fly = std::sin(t * .19f), zoom = std::sin(t * .23f + 1.7f),
              breath = 1.f + .075f * std::sin(t * .83f) + .12f * ml;
  float P[16], V[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, .22f * fly, .14f * std::cos(t * .13f), -4.25f + .52f * zoom - .28f * ml, 1};
  float Ry[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, Rx[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        G[16] = {0}, R[16], RG[16], X[16];
  const float yaw = t * .17f, pitch = .27f * std::sin(t * .11f);
  const float cy = std::cos(yaw), sy = std::sin(yaw), cx = std::cos(pitch), sx = std::sin(pitch), s = scale * breath;
  Ry[0] = cy * s;
  Ry[2] = -sy * s;
  Ry[8] = sy * s;
  Ry[10] = cy * s;
  Ry[5] = s;
  Rx[5] = cx;
  Rx[6] = sx;
  Rx[9] = -sx;
  Rx[10] = cx;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) G[c * 4 + r] = model3[r * 3 + c];   // row-major 3x3 -> column-major 4x4
  G[15] = 1.f;
  perspective(P, verticalFov(t, ml, aspect), std::max(.05f, aspect), .05f, 100.f);
  mul(R, Ry, Rx);
  mul(RG, R, G);
  mul(X, V, RG);
  mul(M, P, X);
}
}  // namespace cam
