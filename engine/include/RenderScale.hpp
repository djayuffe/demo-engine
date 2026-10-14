#pragma once
// HDR render-target scale. The user's --quality factor is honoured exactly; when a pixel budget is set
// (the default on large/Retina displays) the scale is also capped so w*h*scale^2 <= maxPixels. A 2880x1800
// Retina framebuffer is 5.2 MP of RGBA16F and several full-resolution passes; ~3.1 MP (about 2480x1250)
// keeps the M-series GPU well inside its frame budget and the half-resolution upsample stays sharp enough.
#include <algorithm>
#include <cmath>
namespace rscale {
constexpr double kDefaultBudget = 3.1e6;
inline double effective(int w, int h, double userScale, double maxPixels) {
  userScale = std::clamp(userScale, .25, 1.0);
  if (maxPixels <= 0 || w <= 0 || h <= 0) return userScale;
  const double px = double(w) * double(h);
  return std::min(userScale, std::max(.25, std::sqrt(maxPixels / px)));
}
}  // namespace rscale
