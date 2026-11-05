#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace infinity_motion {
// Exact exponential response: independent of update subdivision for fixed targets.
class Bands {
  std::array<float, 3> values_{};

public:
  const std::array<float, 3>& update(float bass, float mid, float treble, double dt) {
    const float input[]{bass, mid, treble};
    dt = std::isfinite(dt) ? std::clamp(dt, 0.0, 1.0) : 0.0;
    for (size_t i = 0; i < 3; ++i) {
      float goal = std::isfinite(input[i]) ? std::clamp(input[i], 0.f, 1.f) : 0.f;
      double tau = goal > values_[i] ? .045 : .30;
      values_[i] += float(1 - std::exp(-dt / tau)) * (goal - values_[i]);
    }
    return values_;
  }
};
class Clock {
  double beat_ = -1, time_ = 0;

public:
  void advance(double beat, bool paused, bool reduced) {
    if (!std::isfinite(beat)) return;
    beat = std::clamp(beat, 0.0, 1e9);
    const double rate = .55 * (reduced ? .12 : 1.0);
    if (!paused) {
      if (beat_ >= 0 && beat >= beat_)
        time_ += (beat - beat_) * rate;
      else
        time_ = beat * rate;
    }
    beat_ = beat;
    time_ = std::fmod(time_, 65536.0);
  }
  double time() const { return time_; }
};
inline float breath(double time) {
  if (!std::isfinite(time)) return 0;
  return float(.5 - .5 * std::cos(std::remainder(time, 4.4) * (6.283185307179586 / 4.4)));
}
} // namespace infinity_motion
