#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace infinity_sync {
inline constexpr int sceneCount = 12;
inline constexpr double beatsPerScene = 32.0;
struct Cue {
  int scene = 0;
  float beat = 0, bass = 0, mid = 0, treble = 0, energy = 0, fade = 1;
  double flow = 0;
};
inline Cue sample(double beat, float pulse, bool active, float bass, float mid, float treble, int manual = -1) {
  beat = std::isfinite(beat) ? std::max(0.0, beat) : 0.0;
  auto unit = [](float x) { return std::isfinite(x) ? std::clamp(x, 0.f, 1.f) : 0.f; };
  Cue c;
  c.scene = manual >= 0 ? manual % sceneCount : int(std::fmod(std::floor(beat / beatsPerScene), double(sceneCount)));
  // Phase-locked attack/decay follows the same continuous beat clock as packets.
  const float clockPulse = std::exp(-float(beat - std::floor(beat)) * 9.f);
  c.beat = std::max(clockPulse, .25f * unit(pulse));
  c.bass = active ? unit(bass) : c.beat;
  c.mid = active ? unit(mid) : .35f;
  c.treble = active ? unit(treble) : .2f;
  c.energy = std::clamp(.15f + .55f * c.bass + .25f * c.mid + .2f * c.beat, 0.f, 1.5f);
  c.flow = beat * .55;
  if (manual < 0) {
    const double phase = std::fmod(beat, beatsPerScene);
    float edge = float(std::min(phase, beatsPerScene - phase));
    edge = std::clamp(edge / .75f, 0.f, 1.f);
    c.fade = edge * edge * (3.f - 2.f * edge);
  }
  return c;
}
inline void pulses(double beat, float bass, float* out) {
  beat = std::isfinite(beat) ? std::max(0.0, beat) : 0.0;
  for (int i = 0; i < 6; ++i) {
    const double index = std::floor(beat) - i;
    const double age = beat - index;
    const auto seed = uint32_t(std::fmod(std::max(0.0, index), 65536.0));
    out[4 * i] = float(age * 1.55);
    out[4 * i + 1] = index < 0 ? 0.f : (.65f + .8f * std::clamp(bass, 0.f, 1.f)) * std::exp(-float(age) * .16f);
    out[4 * i + 2] = float((seed * 2654435761u) & 65535u) / 65535.f;
    out[4 * i + 3] = .24f;
  }
}
} // namespace infinity_sync
