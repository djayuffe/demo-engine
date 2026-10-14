#pragma once
#include "Geometry.hpp"
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Welds near-coincident vertices while a mesh is being built (marching-tetrahedra output repeats every
// surface vertex several times). add() returns the LOWEST index of an existing vertex closer than eps,
// else appends. That is exactly what a linear scan over the mesh returns, so output is bit-identical
// to the old O(n) scan, but a hashed grid makes each call O(1): the scan made isosurface builds scale
// roughly with n^4 (gyroid n=48 took 3.4 s, n=96 took 54 s).
// Every vertex of `m` must be added through the same VertexWeld.
class VertexWeld {
public:
  VertexWeld(Mesh3& m, float eps) : m_(m), eps_(eps), cell_(double(eps) * 1.01) {}
  uint32_t add(V3 p) {
    const int64_t cx = cellOf(p.x), cy = cellOf(p.y), cz = cellOf(p.z);
    uint32_t best = UINT32_MAX;
    for (int64_t dx = -1; dx <= 1; ++dx)
      for (int64_t dy = -1; dy <= 1; ++dy)
        for (int64_t dz = -1; dz <= 1; ++dz) {
          const auto it = grid_.find(key(cx + dx, cy + dy, cz + dz));
          if (it == grid_.end()) continue;
          for (uint32_t i : it->second) {
            if (i >= best) continue;
            const V3 q = m_.v[i];
            const float x = p.x - q.x, y = p.y - q.y, z = p.z - q.z;
            if (x * x + y * y + z * z < eps_ * eps_) best = i;
          }
        }
    if (best != UINT32_MAX) return best;
    m_.v.push_back(p);
    const uint32_t id = uint32_t(m_.v.size() - 1);
    grid_[key(cx, cy, cz)].push_back(id);
    return id;
  }
private:
  int64_t cellOf(float v) const { return int64_t(std::floor(double(v) / cell_)); }
  // Hash collisions only add candidates; every candidate is distance-checked above.
  static uint64_t key(int64_t x, int64_t y, int64_t z) {
    uint64_t h = uint64_t(x) * 0x9E3779B97F4A7C15ull;
    h ^= (uint64_t(y) + 0x7F4A7C15ull + (h << 6) + (h >> 2)) * 0xC2B2AE3D27D4EB4Full;
    h ^= (uint64_t(z) + 0x165667B1ull + (h << 6) + (h >> 2)) * 0x9E3779B97F4A7C15ull;
    return h;
  }
  Mesh3& m_; float eps_; double cell_;
  std::unordered_map<uint64_t, std::vector<uint32_t>> grid_;
};
