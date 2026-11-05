#pragma once
#include "advanced.hpp"
#include <numeric>
namespace dmt::atlas {
using advanced::Sphere;
struct Node {
  Sphere bound;
  int escape = 0, leaf = -1;
};
inline Sphere enclose(Sphere a, Sphere b) {
  double d = h3Distance(a.center, b.center);
  if (a.radius >= d + b.radius) return a;
  if (b.radius >= d + a.radius) return b;
  double r = (d + a.radius + b.radius) * .5, t = (r - a.radius) / d;
  V4 p{};
  double den = std::sinh(d);
  for (int i = 0; i < 4; ++i) p[i] = (std::sinh((1 - t) * d) * a.center[i] + std::sinh(t * d) * b.center[i]) / den;
  return {p, r + 1e-10};
}
struct Atlas {
  std::array<Sphere, 56> spheres{};
  std::vector<Node> nodes;
  Atlas() {
    for (int i = 0; i < 56; ++i) {
      double phi = i * 2.399963229728653;
      spheres[i] = {h3Point(.92 * std::cos(phi), .92 * std::sin(phi), 1.7 + (i % 8) * .44), .16 + .035 * (i % 4)};
    }
    std::vector<int> ids(56);
    std::iota(ids.begin(), ids.end(), 0);
    build(ids, 0, 56);
  }
  int build(std::vector<int>& ids, int lo, int hi) {
    int index = int(nodes.size());
    nodes.push_back({spheres[ids[lo]], 0, -1});
    if (hi - lo == 1)
      nodes[index].leaf = ids[lo];
    else {
      int axis = 1;
      double best = -1;
      for (int a = 1; a < 4; ++a) {
        double mn = 1e9, mx = -1e9;
        for (int k = lo; k < hi; ++k) {
          auto p = spheres[ids[k]].center;
          double x = p[a] / (p[0] + 1);
          mn = std::min(mn, x);
          mx = std::max(mx, x);
        }
        if (mx - mn > best) {
          best = mx - mn;
          axis = a;
        }
      }
      int mid = (lo + hi) / 2;
      std::nth_element(ids.begin() + lo, ids.begin() + mid, ids.begin() + hi, [&](int a, int b) {
        return spheres[a].center[axis] / (spheres[a].center[0] + 1) <
               spheres[b].center[axis] / (spheres[b].center[0] + 1);
      });
      int left = build(ids, lo, mid), right = build(ids, mid, hi);
      nodes[index].bound = enclose(nodes[left].bound, nodes[right].bound);
    }
    nodes[index].escape = int(nodes.size());
    return index;
  }
  double intersect(V4 o, V4 d, bool accelerated = true) const {
    double best = std::numeric_limits<double>::infinity();
    if (!accelerated) {
      for (auto s : spheres) best = std::min(best, advanced::intersectSphere(o, d, s));
      return best;
    }
    int i = 0;
    while (i < int(nodes.size())) {
      auto n = nodes[size_t(i)];
      double entry = h3Distance(o, n.bound.center) <= n.bound.radius ? 0 : advanced::intersectSphere(o, d, n.bound);
      if (entry > best || !std::isfinite(entry)) {
        i = n.escape;
        continue;
      }
      if (n.leaf >= 0) best = std::min(best, advanced::intersectSphere(o, d, spheres[size_t(n.leaf)]));
      ++i;
    }
    return best;
  }
};
} // namespace dmt::atlas
