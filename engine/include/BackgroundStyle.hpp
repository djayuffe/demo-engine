#pragma once
// Pure, GL-free logic behind the selectable backgrounds and pointer depth: style names and parsing,
// the fade-through-black state machine, the parallax-gain cycle and the idle sway. Unit-tested.
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace bgstyle {
constexpr int kCount = 7;
inline const char* name(int s) {
  static const char* const k[kCount] = {"classic", "synthwave", "nebula", "mandala", "blackhole", "hyperspace", "fractal"};
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

// ---- Per-style objects: every background is built from named objects that can be switched on and
// off one by one (keys F1..F12 for the current style, `--hide a,b` at startup). Bit k of a style's
// mask = object k; the order here MUST match the `e(k)` / `on(k)` bits in lib/art_styles.glsl and
// lib/art_layers.glsl. ----
inline const std::vector<const char*>& elements(int style) {
  static const std::vector<const char*> k[kCount] = {
      {"organism", "ink", "glass", "rose", "julia", "moire", "turing", "contour", "poincare", "kifs", "ribbons", "ambient"},
      {"sun", "mountains", "grid", "stars", "aurora", "reflection", "sky"},
      {"gas", "stars", "flares", "supernova", "comets", "dust"},
      {"crystal", "neon", "folds", "mirror", "rings"},
      {"disk", "photonring", "jets", "galaxy", "stars"},
      {"walls", "streaks", "glow", "flash"},
      {"structure", "orbs", "glow", "fog"}};
  return k[((style % kCount) + kCount) % kCount];
}
constexpr int kAllOn = 0xFFFF;
inline int elementCount(int style) { return int(elements(style).size()); }
struct Objects {
  int mask[kCount] = {kAllOn, kAllOn, kAllOn, kAllOn, kAllOn, kAllOn, kAllOn};
  static_assert(kCount == 7, "extend the mask initialiser with the style count");
  int get(int style) const { return mask[wrap(style)]; }
  // Toggles object k of `style`; false when the style has no such object.
  bool toggle(int style, int k) {
    if (k < 0 || k >= int(elements(style).size())) return false;
    mask[wrap(style)] ^= (1 << k);
    return true;
  }
  bool enabled(int style, int k) const { return (get(style) >> k) & 1; }
  // "sun,grid,stars": switches the named objects off in every style that has them.
  // Returns the names that matched nothing (empty = all fine).
  std::vector<std::string> hide(const std::string& list) {
    std::vector<std::string> unknown;
    size_t i = 0;
    while (i <= list.size()) {
      size_t j = list.find(',', i);
      if (j == std::string::npos) j = list.size();
      const std::string n = list.substr(i, j - i);
      i = j + 1;
      if (n.empty()) continue;
      bool hit = false;
      for (int s = 0; s < kCount; s++)
        for (int k = 0; k < int(elements(s).size()); k++)
          if (n == elements(s)[size_t(k)]) { mask[s] &= ~(1 << k); hit = true; }
      if (!hit) unknown.push_back(n);
    }
    return unknown;
  }
  // "sun on, grid OFF, ..." for the status line
  std::string describe(int style) const {
    std::string out;
    const auto& el = elements(style);
    for (size_t k = 0; k < el.size(); k++) {
      if (k) out += "  ";
      out += "F" + std::to_string(k + 1) + ":" + el[k] + (enabled(style, int(k)) ? "" : "(off)");
    }
    return out;
  }
};

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
