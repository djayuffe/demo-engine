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

V3 modelPoint(V3 p, float t, V3 center) {
  p = cubeRotation(p, t);
  return {p.x + center.x, p.y + center.y, p.z + center.z};
}

// A faceted, four-lobed reactor heart. Latitude/longitude loops make it read
// as a solid object from every camera angle, while the lobes visibly contract
// and release on the beat instead of merely scaling the whole scene.
void pulseCore(Builder& b, float t, float envelope, V3 center) {
  constexpr unsigned rings = 9, slices = 24;
  std::array<std::array<uint32_t, slices>, rings> ids{};
  for (unsigned y = 0; y < rings; ++y) {
    const float v = float(y) / float(rings - 1);
    const float latitude = (v - .5f) * pi;
    for (unsigned x = 0; x < slices; ++x) {
      const float longitude = 2.f * pi * float(x) / float(slices);
      const float ribs = .045f * std::cos(latitude * 6.f - t * 2.1f);
      const float lobes = .075f * std::cos(longitude * 4.f + t * 1.7f) * std::pow(std::cos(latitude), 2.f);
      const float radius = .34f * (1.f + .16f * envelope) + ribs + lobes;
      V3 p{radius * std::cos(latitude) * std::cos(longitude),
           radius * 1.12f * std::sin(latitude),
           radius * std::cos(latitude) * std::sin(longitude)};
      // The core counter-rotates inside the chassis, giving the model an
      // independently articulated centre rather than nested copies of a cube.
      p = rotateY(rotateZ(p, -t * .67f), t * .91f);
      ids[y][x] = b.add(modelPoint(p, t, center));
    }
  }
  for (unsigned y = 0; y < rings; ++y)
    for (unsigned x = 0; x < slices; ++x) {
      b.edge(ids[y][x], ids[y][(x + 1) % slices]);
      if (y + 1 < rings) b.edge(ids[y][x], ids[y + 1][x]);
    }
}

// Six radial bus bars bind the core to the chassis. Repeating coils and a
// larger terminal ring provide depth cues and give the sculpture a machine-like
// silhouette instead of reading as unrelated concentric effects.
void conduits(Builder& b, float t, float envelope, V3 center) {
  constexpr unsigned coilSamples = 18;
  const std::array<V3, 6> axes{{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
  for (size_t face = 0; face < axes.size(); ++face) {
    const V3 n = axes[face];
    const V3 u = std::abs(n.x) > .5f ? V3{0, 1, 0} : V3{1, 0, 0};
    const V3 v = std::abs(n.z) > .5f ? V3{0, 1, 0} : V3{0, 0, 1};
    const float inner = .40f, outer = 1.04f;
    b.segment(modelPoint({n.x * inner, n.y * inner, n.z * inner}, t, center),
              modelPoint({n.x * outer, n.y * outer, n.z * outer}, t, center));
    uint32_t previousFirst = 0;
    for (unsigned coil = 0; coil < 5; ++coil) {
      const float along = inner + (outer - inner) * float(coil) / 4.f;
      const float coilRadius = (.075f + .018f * envelope) * (coil == 4 ? 1.42f : 1.f);
      uint32_t first = 0, previous = 0;
      for (unsigned i = 0; i < coilSamples; ++i) {
        const float a = 2.f * pi * float(i) / float(coilSamples) + t * (face % 2 ? -.8f : .8f);
        V3 p{n.x * along + coilRadius * (u.x * std::cos(a) + v.x * std::sin(a)),
             n.y * along + coilRadius * (u.y * std::cos(a) + v.y * std::sin(a)),
             n.z * along + coilRadius * (u.z * std::cos(a) + v.z * std::sin(a))};
        const uint32_t id = b.add(modelPoint(p, t, center));
        if (i == 0) first = id;
        else b.edge(previous, id);
        if (coil > 0 && i % 3 == 0) b.edge(previousFirst + i, id);
        previous = id;
      }
      b.edge(previous, first);
      previousFirst = first;
    }
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

  // The PulseGrid Reactor is one coherent model: chassis, heart and power
  // buses all share the same transform, while the heart articulates within it.
  latticeCube(b, 6, 1.13f * (1.f + envelope * .055f), .20f, t, cubeCenter);
  pulseCore(b, t, envelope, cubeCenter);
  conduits(b, t, envelope, cubeCenter);

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
