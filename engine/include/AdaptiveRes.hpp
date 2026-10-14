#pragma once
// Frame-time governor for the background pass: if the demo cannot hold ~45 fps the procedural
// background (the heavy part) renders at a coarser resolution (divisor 2 -> 3 -> 4 of the HDR size);
// when there is headroom for a long while it tries a finer one again. Pure and unit-tested.
#include <algorithm>

namespace adaptive {
struct Governor {
  int divisor = 2;                    // art target = HDR size / divisor
  static constexpr int kMin = 2, kMax = 4;
  float slowFor = 0.f, fastFor = 0.f, cooldown = 0.f, ema = 1.f / 60.f;
  // Feed one frame time in seconds; returns true when the divisor changed.
  bool update(float dt) {
    dt = std::clamp(dt, 0.f, .25f);
    ema += (dt - ema) * .08f;
    cooldown = std::max(0.f, cooldown - dt);
    slowFor = ema > 1.f / 42.f ? slowFor + dt : 0.f;
    fastFor = ema < 1.f / 52.f ? fastFor + dt : 0.f;
    if (slowFor > 1.5f && divisor < kMax && cooldown <= 0.f) {
      ++divisor; slowFor = fastFor = 0.f; cooldown = 3.f; return true;
    }
    if (fastFor > 20.f && divisor > kMin && cooldown <= 0.f) {   // vsync-capped frames look "fast" at 60 Hz: retry slowly
      --divisor; slowFor = fastFor = 0.f; cooldown = 3.f; return true;
    }
    return false;
  }
};
}  // namespace adaptive
