#pragma once
#include "HyperAtlas.hpp"
#include <cstddef>
namespace dmt::atlas {
using GpuVector = std::array<float, 4>;
// std140 arrays of vec4 have a 16-byte stride on every GL 4.1 implementation.
struct alignas(16) GpuGeometry {
  std::array<GpuVector, 112> nodeCenters{}, nodeData{};
  std::array<GpuVector, 56> sphereCenters{}, sphereData{};
};
static_assert(sizeof(GpuGeometry) == 5376);
static_assert(offsetof(GpuGeometry, nodeData) == 1792);
static_assert(offsetof(GpuGeometry, sphereCenters) == 3584);
static_assert(offsetof(GpuGeometry, sphereData) == 4480);
inline GpuGeometry gpuGeometry(const Atlas& atlas) {
  if (atlas.nodes.size() != 111) throw std::domain_error("unsupported Atlas hierarchy layout");
  GpuGeometry data;
  for (size_t i = 0; i < atlas.nodes.size(); ++i) {
    auto n = atlas.nodes[i];
    for (size_t j = 0; j < 4; ++j) data.nodeCenters[i][j] = float(n.bound.center[j]);
    data.nodeData[i] = {float(std::cosh(n.bound.radius + .008)), float(n.escape), float(n.leaf + 1), 0};
  }
  for (size_t i = 0; i < atlas.spheres.size(); ++i) {
    auto s = atlas.spheres[i];
    for (size_t j = 0; j < 4; ++j) data.sphereCenters[i][j] = float(s.center[j]);
    data.sphereData[i] = {float(s.radius), float(std::cosh(s.radius)), float(std::sinh(s.radius)), 0};
  }
  return data;
}
// cosh/sinh addition with bounded delta <= .006625. O(delta^4) cosh and O(delta^5) sinh remainders
// is below float rounding: no per-ray exponential evaluation is necessary.
inline std::array<double, 3> breathingRadius(double radius, double breath) {
  if (!std::isfinite(radius) || radius <= 0 || radius > .3 || !std::isfinite(breath))
    throw std::domain_error("outside cached radius domain");
  double delta = radius * .025 * std::clamp(breath, 0., 1.), c = 1 + delta * delta * .5,
         s = delta * (1 + delta * delta / 6);
  return {radius + delta, std::cosh(radius) * c + std::sinh(radius) * s, std::sinh(radius) * c + std::cosh(radius) * s};
}
} // namespace dmt::atlas
