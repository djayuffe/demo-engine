#pragma once
// Pure, GL-free logic behind the selectable backgrounds and pointer depth: style names and parsing,
// the fade-through-black state machine, the parallax-gain cycle and the idle sway. Unit-tested.
#include <algorithm>
#include <cmath>
#include <string>

namespace bgstyle {
constexpr int kCount = 6;
inline const char* name(int s) {
  static const char* const k[kCount] = {"classic", "synthwave", "nebula", "mandala", "blackhole", "hyperspace"};
  return k[((s % kCount) + kCount) % kCount];
}
// "auto" -> -1 (caller turns auto-cycling on); a name or a digit 0..4 -> that style; anything else -> -2.
inline int parse(const std::string& v) {
  if (v == "auto") return -1;
  for (int i = 0; i < kCount; i++)
    if (v == name(i)) return i;
  if (v.size() == 1 && v[0] >= '0' && v[0] < char('0' + kCount)) return v[0] - '0';
  return -2;
}
inline int wrap(int s) { return ((s % kCount) + kCount) % kCount; }

// Fade-through-black between styles: fade out (to 0), swap the shown style, fade back in (to 1).
struct Fade {
  int shown = 0;
  float level = 1.f;
  bool primed = false;
  // Advances by dt seconds towards `requested`; returns the smoothstepped brightness 0..1.
  float step(int requested, float dt, float seconds = .4f) {
    if (!primed) { shown = requested; level = 1.f; primed = true; }
    dt = std::clamp(dt, 0.f, .1f);
    if (shown != requested) {
      level -= dt / seconds;
      if (level <= 0.f) { level = 0.f; shown = requested; }
    } else {
      level = std::min(1.f, level + dt / seconds);
    }
    return level * level * (3.f - 2.f * level);
  }
};

// Parallax gain presets cycled by key G; returns the next preset after `current`.
inline double nextGain(double current) {
  static const double k[] = {0.0, 0.8, 1.6, 2.4, 3.0};
  int at = 0;
  for (int i = 0; i < 5; i++)
    if (std::fabs(k[i] - current) < .05) at = i;
  return k[(at + 1) % 5];
}

// Slow idle sway added to the pointer so depth lives without the mouse; bounded by +-0.30 / +-0.22.
inline void sway(double t, float& sx, float& sy) {
  sx = .22f * std::sin(float(t) * .23f) + .08f * std::sin(float(t) * .61f);
  sy = .16f * std::cos(float(t) * .19f) + .06f * std::cos(float(t) * .53f);
}
}  // namespace bgstyle
