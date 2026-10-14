#pragma once
// Additive-wire exposure. Lines pile up: the 600-cell projection draws ~1200 edges through the same
// pixels and burns out to white, while a sparse scene needs the full strength. The base gain scales
// each line by the square root of the density (about constant total energy) and never brightens
// above 1. Wide wires (the geometry-shader quads, wire.geom) cover `widthPx` pixels instead of 1, so
// in DENSE scenes they deposit that much more light; the width term pulls dense scenes back to the
// energy they had as 1-pixel lines, while sparse scenes keep their bright, wide strokes. Pure, tested.
#include <algorithm>
#include <cmath>

namespace wiregain {
inline float base(int indexCount) {
  const float lines = float(std::max(1, indexCount / 2));
  return std::clamp(0.80f * std::sqrt(500.f / lines), 0.05f, 1.0f);   // dense recipe scenes reach tens of thousands of lines
}
inline float forWidth(int indexCount, float widthPx, bool wide) {
  const float b = base(indexCount);
  if (!wide) return b;
  const float lines = float(std::max(1, indexCount / 2));
  const float dense = std::clamp((lines - 300.f) / 1500.f, 0.f, 1.f);
  const float w = std::max(widthPx, 1.f);
  return b * (1.f + (std::pow(1.f / w, .7f) - 1.f) * dense);
}
}  // namespace wiregain
