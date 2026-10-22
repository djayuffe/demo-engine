#pragma once
#include "geometry.hpp"
namespace dmt::slice {
using P3 = std::array<double, 3>;
inline double dot4(V4 a, V4 b) {
  double s = 0;
  for (int i = 0; i < 4; ++i) s += a[i] * b[i];
  return s;
}
inline double dot3(P3 a, P3 b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
inline P3 cross(P3 a, P3 b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
inline V4 rotated(V4 v, double phase) {
  int k = 0;
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j) v = rotatePlane(v, i, j, phase * (.23 + .137 * k++));
  return v;
}
struct Section {
  std::array<V4, 8> planes{};
  std::vector<P3> vertices;
  bool empty = false;
};
// Restrict all eight cubic-cell halfspaces to an orthonormal slice basis.
// This also handles a plane coincident with a cubic cell without duplicate faces.
inline Section section(V4 normal, double offset, double phase, bool topology = true) {
  double n = std::sqrt(dot4(normal, normal));
  if (!std::isfinite(n) || n < 1e-12 || !std::isfinite(offset)) throw std::domain_error("invalid slice plane");
  for (auto& x : normal) x /= n;
  offset /= n;
  std::array<V4, 3> basis{};
  int count = 0;
  for (int a = 0; a < 4 && count < 3; ++a) {
    V4 b{};
    b[a] = 1;
    double d = dot4(b, normal);
    for (int j = 0; j < 4; ++j) b[j] -= d * normal[j];
    for (int k = 0; k < count; ++k) {
      d = dot4(b, basis[k]);
      for (int j = 0; j < 4; ++j) b[j] -= d * basis[k][j];
    }
    d = std::sqrt(dot4(b, b));
    if (d < 1e-10) continue;
    for (auto& x : b) x /= d;
    basis[count++] = b;
  }
  Section out;
  for (int axis = 0; axis < 4; ++axis)
    for (int side = 0; side < 2; ++side) {
      V4 v{};
      v[axis] = side ? 1 : -1;
      v = rotated(v, phase);
      V4 p{dot4(v, basis[0]), dot4(v, basis[1]), dot4(v, basis[2]), 1 - offset * dot4(v, normal)};
      out.planes[2 * axis + side] = p;
      if (p[0] * p[0] + p[1] * p[1] + p[2] * p[2] < 1e-20 && p[3] < -1e-10) out.empty = true;
    }
  if (out.empty || (!topology && std::abs(offset) < 1)) return out;
  for (int i = 0; i < 8; ++i)
    for (int j = i + 1; j < 8; ++j)
      for (int k = j + 1; k < 8; ++k) {
        auto a = out.planes[i], b = out.planes[j], c = out.planes[k];
        P3 aa{a[0], a[1], a[2]}, bb{b[0], b[1], b[2]}, cc{c[0], c[1], c[2]};
        auto bc = cross(bb, cc), ca = cross(cc, aa), ab = cross(aa, bb);
        double det = dot3(aa, bc);
        if (std::abs(det) < 1e-10) continue;
        P3 v{};
        for (int t = 0; t < 3; ++t) v[t] = (a[3] * bc[t] + b[3] * ca[t] + c[3] * ab[t]) / det;
        bool valid = true;
        for (auto p : out.planes)
          if (p[0] * v[0] + p[1] * v[1] + p[2] * v[2] > p[3] + 1e-8) valid = false;
        for (auto p : out.vertices) {
          P3 d{p[0] - v[0], p[1] - v[1], p[2] - v[2]};
          if (dot3(d, d) < 1e-16) valid = false;
        }
        if (valid) out.vertices.push_back(v);
      }
  double volume = 0;
  if (out.vertices.size() >= 4) {
    const auto origin = out.vertices[0];
    for (size_t a = 1; a < out.vertices.size(); ++a)
      for (size_t b = a + 1; b < out.vertices.size(); ++b)
        for (size_t c = b + 1; c < out.vertices.size(); ++c) {
          P3 x{}, y{}, z{};
          for (int j = 0; j < 3; ++j) {
            x[j] = out.vertices[a][j] - origin[j];
            y[j] = out.vertices[b][j] - origin[j];
            z[j] = out.vertices[c][j] - origin[j];
          }
          volume = std::max(volume, std::abs(dot3(x, cross(y, z))));
        }
  }
  out.empty = volume < 1e-12;
  return out;
}
} // namespace dmt::slice
