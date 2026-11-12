#include "PulseGridGeometry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>

namespace geo {
namespace {
constexpr float pi = 3.14159265358979f;

struct Builder {
  Mesh3 mesh;
  std::set<std::pair<uint32_t, uint32_t>> edges;
  uint32_t add(V3 p) {
    mesh.v.push_back(p);
    return uint32_t(mesh.v.size() - 1);
  }
  void edge(uint32_t a, uint32_t b) {
    if (a == b) return;
    const auto key = std::minmax(a, b);
    if (edges.insert(key).second) mesh.e.push_back({a, b});
  }
  void segment(V3 a, V3 b) { edge(add(a), add(b)); }
};

V3 rotateX(V3 p, float a) {
  const float c = std::cos(a), s = std::sin(a);
  return {p.x, c * p.y - s * p.z, s * p.y + c * p.z};
}
V3 rotateY(V3 p, float a) {
  const float c = std::cos(a), s = std::sin(a);
  return {c * p.x + s * p.z, p.y, -s * p.x + c * p.z};
}
V3 rotateZ(V3 p, float a) {
  const float c = std::cos(a), s = std::sin(a);
  return {c * p.x - s * p.y, s * p.x + c * p.y, p.z};
}
V3 cubeRotation(V3 p, float t) {
  p = rotateZ(p, .14f * std::sin(t * .31f));
  p = rotateX(p, .38f + t * .27f);
  return rotateY(p, t * .43f);
}
V3 rounded(V3 p, float half, float bevel) {
  const float core = std::max(half - bevel, .01f);
  V3 q{std::clamp(p.x, -core, core), std::clamp(p.y, -core, core), std::clamp(p.z, -core, core)};
  V3 d{p.x - q.x, p.y - q.y, p.z - q.z};
  const float length = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
  if (length > 1e-7f) {
    q.x += d.x * bevel / length;
    q.y += d.y * bevel / length;
    q.z += d.z * bevel / length;
  }
  return q;
}

void latticeCube(Builder& b, unsigned divisions, float half, float bevel, float t, V3 center = {}) {
  divisions = std::max(divisions, 2u);
  std::map<std::array<int, 3>, uint32_t> ids;
  auto boundary = [=](int x, int y, int z) {
    return x == 0 || y == 0 || z == 0 || x == int(divisions) || y == int(divisions) || z == int(divisions);
  };
  for (unsigned z = 0; z <= divisions; ++z)
    for (unsigned y = 0; y <= divisions; ++y)
      for (unsigned x = 0; x <= divisions; ++x) {
        if (!boundary(int(x), int(y), int(z))) continue;
        const auto coordinate = [=](unsigned i) { return (float(i) / float(divisions) * 2.f - 1.f) * half; };
        V3 p = cubeRotation(rounded({coordinate(x), coordinate(y), coordinate(z)}, half, bevel), t);
        p.x += center.x;
        p.y += center.y;
        p.z += center.z;
        ids[{int(x), int(y), int(z)}] = b.add(p);
      }
  const std::array<std::array<int, 3>, 3> steps{{{{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}}};
  for (const auto& [key, id] : ids)
    for (const auto& step : steps) {
      const std::array<int, 3> next{{key[0] + step[0], key[1] + step[1], key[2] + step[2]}};
      if (auto it = ids.find(next); it != ids.end()) b.edge(id, it->second);
    }
}

void ring(Builder& b, float radius, float y, float tiltX, float tiltZ, float phase, unsigned samples = 144) {
  uint32_t first = 0, previous = 0;
  for (unsigned i = 0; i < samples; ++i) {
    const float a = 2.f * pi * float(i) / float(samples) + phase;
    V3 p{radius * std::cos(a), y, radius * std::sin(a)};
    p = rotateZ(rotateX(p, tiltX), tiltZ);
    const uint32_t id = b.add(p);
    if (i == 0) first = id;
    else b.edge(previous, id);
    previous = id;
  }
  b.edge(previous, first);
}
} // namespace

Mesh3 pulseGrid(float seconds) {
  Builder b;
  const float t = std::floor(std::max(0.f, seconds) * 25.f) / 25.f;
  const float beat = t * 138.f / 60.f;
  const float envelope = std::pow(1.f - std::fmod(beat, 1.f), 3.f);
  const V3 cubeCenter{0.f, .20f, 0.f};

  latticeCube(b, 11, 1.13f * (1.f + envelope * .10f), .17f, t, cubeCenter);
  for (int shell = 0; shell < 3; ++shell) {
    const float phase = .5f + .5f * std::sin(beat * 2.f * pi - float(shell) * 1.7f);
    const float size = .42f + float(shell) * .19f + phase * .055f;
    latticeCube(b, 3, size, .07f, t + float(shell) * .08f, cubeCenter);
  }

  for (int i = 0; i < 3; ++i) {
    const float radius = 1.52f + float(i) * .19f + envelope * .035f;
    ring(b, radius, .20f, .18f + float(i) * .47f, -.42f + float(i) * .39f,
         t * (.23f + float(i) * .035f) + float(i) * 2.1f);
  }

  // A world floor replaces the browser version's flat screen treatment.
  constexpr int grid = 9;
  constexpr float extent = 2.75f;
  for (int i = -grid; i <= grid; ++i) {
    const float q = extent * float(i) / float(grid);
    b.segment({-extent, -1.46f, q}, {extent, -1.46f, q});
    b.segment({q, -1.46f, -extent}, {q, -1.46f, extent});
  }
  for (int wave = 0; wave < 3; ++wave) {
    const float travel = std::fmod(beat * .42f + float(wave) / 3.f, 1.f);
    ring(b, .28f + travel * 2.25f, -1.445f, 0.f, 0.f, float(wave) * .7f, 112);
  }

  // Blocky pulse rail: a geometric echo of the source's world-space C64 text.
  constexpr std::array<float, 18> pulse{{0, 0, .18f, .18f, .72f, -.46f, .34f, .08f, .08f,
                                         .52f, -.34f, .22f, 0, 0, .38f, -.20f, 0, 0}};
  for (size_t i = 0; i + 1 < pulse.size(); ++i) {
    const float x0 = (float(i) / float(pulse.size() - 1) * 2.f - 1.f) * 1.55f;
    const float x1 = (float(i + 1) / float(pulse.size() - 1) * 2.f - 1.f) * 1.55f;
    b.segment({x0, -1.17f + pulse[i] * .42f, 1.42f}, {x1, -1.17f + pulse[i + 1] * .42f, 1.42f});
  }
  return b.mesh;
}
} // namespace geo
