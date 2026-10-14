#pragma once
// The logo show as a pure function of time: each card fades in, holds, fades to black, then a long black
// beat shows only the wireframe before the next card. Independent of frame rate and of scene changes.
#include <algorithm>
#include <cmath>
namespace logoshow {
constexpr double kFadeIn = 1.0, kHold = 3.5, kFadeOut = 1.0, kBlack = 9.0;
constexpr double kCycle = kFadeIn + kHold + kFadeOut + kBlack;
struct State {
  int card = 0;          // index of the card being shown (or about to be)
  float visibility = 0;  // 0..1 eased brightness of the card
  float zoom = 1.05f;    // Ken Burns scale, a slow breath
};
inline float ease(double x) {
  x = std::clamp(x, 0.0, 1.0);
  return float(x * x * (3.0 - 2.0 * x));
}
inline State at(double time, int cardCount) {
  State s;
  const double cyc = std::max(0.0, time) / kCycle;
  s.card = cardCount > 0 ? int(std::floor(cyc)) % cardCount : 0;
  const double ph = (cyc - std::floor(cyc)) * kCycle;
  if (ph < kFadeIn) s.visibility = ease(ph / kFadeIn);
  else if (ph < kFadeIn + kHold) s.visibility = 1.f;
  else if (ph < kFadeIn + kHold + kFadeOut) s.visibility = 1.f - ease((ph - kFadeIn - kHold) / kFadeOut);
  s.zoom = 1.05f + .04f * std::sin(float(time) * .06f);
  return s;
}
}  // namespace logoshow
