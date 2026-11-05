#pragma once
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <string>

namespace infinity_opt {
struct RenderSize {
  int width = 1, height = 1;
  bool operator==(const RenderSize&) const = default;
};

// Preserve the framebuffer aspect ratio while keeping intermediate storage
// inside a strict pixel ceiling. Invalid window sizes still produce a usable
// 1x1 target during minimize/restore transitions.
inline RenderSize fitPixelBudget(int width, int height, int budget, double maxScale = 1.0) {
  width = std::max(1, width);
  height = std::max(1, height);
  budget = std::max(1, budget);
  maxScale = std::isfinite(maxScale) ? std::clamp(maxScale, 1.0, 2.0) : 1.0;
  const double pixels = double(width) * double(height);
  const double scale = std::min(maxScale, std::sqrt(double(budget) / pixels));
  return {std::max(1, int(std::floor(double(width) * scale))),
          std::max(1, int(std::floor(double(height) * scale)))};
}

// Apple's GL 4.1 implementation is Metal-backed and has ample headroom for a
// sharper Retina intermediate. Other drivers retain the established ceiling.
inline int defaultPixelCeiling(bool metalBacked) { return metalBacked ? 2000000 : 400000; }
inline double maximumRenderScale(bool metalBacked) { return metalBacked ? 1.5 : 1.0; }

inline void replace(std::string& source, const std::string& from, const std::string& to) {
  auto at = source.find(from);
  if (at == std::string::npos) throw std::runtime_error("Infinity shader contract missing: " + from);
  source.replace(at, from.size(), to);
}
// The numerical integrator is unchanged. Constant switches let the compiler
// remove unused medium/face/horizon/spectrum/layer branches from its hot loop.
inline std::string specialize(std::string source, int state) {
  if (state < 0 || state > 127) throw std::runtime_error("Invalid optical specialization");
  const char* names[] = {"uMedium", "uHole", "uCurved", "uWhite", "uInside", "uSteps"};
  const int bits[] = {1, 2, 4, 8, 16, 64};
  for (int i = 0; i < 6; ++i)
    replace(source, std::string("uniform int ") + names[i] + ";",
            std::string("const int ") + names[i] + " = " + (state & bits[i] ? "1;" : "0;"));
  replace(source, "uniform float uNout;", std::string("const float uNout = ") + (state & 32 ? "1.33;" : "1.0;"));
  return source;
}
inline int stateForScene(int scene, int overrideState = -1) {
  return overrideState >= 0 ? overrideState
         : scene == 4
             ? 0
             : 5 | (scene == 6 ? 2 : 0) | (scene == 7 ? 64 : 0) | (scene == 8 ? 40 : 0) | (scene == 9 ? 16 : 0);
}
// Pixel-budget governor, independent per scene. Hysteresis and a cooldown
// avoid repeated framebuffer reallocations; exact-budget runs bypass it.
class SceneQuality {
  float pixels_ = 1.f, ema_ = 0.f;
  int last_ = -1, samples_ = 0, next_ = 0;

public:
  float fraction() const { return pixels_; }
  bool observe(double milliseconds, int frame, double target) {
    if (frame <= last_ || !std::isfinite(milliseconds) || milliseconds <= 0 || milliseconds > 2000 ||
        !std::isfinite(target) || target <= 0)
      return false;
    last_ = frame;
    ema_ = samples_ ? ema_ * .8f + float(milliseconds) * .2f : float(milliseconds);
    ++samples_;
    if (samples_ < 8 || frame < next_) return false;
    float before = pixels_;
    if (ema_ > target * 1.15)
      pixels_ = std::max(.25f, pixels_ * .8f);
    else if (ema_ < target * .7)
      pixels_ = std::min(1.f, pixels_ * 1.1f);
    if (before != pixels_) {
      next_ = frame + 30;
      return true;
    }
    return false;
  }
};
struct RaySolveKey {
  float aspect = -1, curvature = -1;
  int rays = 0, wavelengths = 0;
  bool operator==(const RaySolveKey&) const = default;
};
} // namespace infinity_opt
