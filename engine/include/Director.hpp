#pragma once
// The "director" behind --background-style auto: listens to the music energy and decides WHAT world to
// show and WHEN to cut. Pure, deterministic and unit-tested (no GL, no clock of its own).
//  * Two energy followers (fast ~0.4 s, slow ~8 s) detect a DROP (energy jumps well above its recent
//    level) and a BREAKDOWN (energy collapses) the way a DJ hears them.
//  * Cuts only land on a bar line, after a minimum hold; a drop or breakdown shortens the hold, a
//    maximum hold forces a change.
//  * The next style is chosen by how well it suits the current energy (calm -> nebula/black hole,
//    peak -> hyperspace/synthwave), discounted by how recently it was shown.
//  * The depth/parallax gain breathes with the energy.
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace director {
constexpr int kStyles = 6;
// affinity[style][class]: class 0 = calm, 1 = driving, 2 = peak
inline float affinity(int style, int cls) {
  static const float a[kStyles][3] = {{.60f, .50f, .30f},   // classic
                                      {.30f, .80f, .70f},   // synthwave
                                      {1.0f, .50f, .10f},   // nebula
                                      {.50f, .90f, .60f},   // mandala
                                      {.80f, .70f, .50f},   // blackhole
                                      {.02f, .50f, 1.0f}};  // hyperspace
  return a[((style % kStyles) + kStyles) % kStyles][std::clamp(cls, 0, 2)];
}
inline int energyClass(float slowEnergy) { return slowEnergy < .28f ? 0 : slowEnergy < .58f ? 1 : 2; }

struct Decision {
  int switchTo = -1;       // style to cut to now, or -1
  bool drop = false;       // a drop was detected this frame (fire a shockwave)
  bool breakdown = false;
  float gain = 1.6f;       // suggested parallax gain
};

struct Director {
  float minHold = 24.f, maxHold = 60.f;
  float fast = 0.f, slow = 0.f;
  double lastT = -1, enteredAt = 0, lastDrop = -100;
  uint64_t lastBar = 0, seed = 12345;
  double shownAt[kStyles] = {};
  int style = 1;
  float gain = 1.6f;
  bool pendingCut = false;

  void start(int first, double t) {
    style = first;
    enteredAt = t;
    for (double& s : shownAt) s = t - 90.0;
    shownAt[((first % kStyles) + kStyles) % kStyles] = t;
  }
  int pick(float slowEnergy, double t) {
    const int cls = energyClass(slowEnergy);
    int best = style;
    float bestScore = -1.f;
    for (int s = 0; s < kStyles; s++) {
      if (s == style) continue;
      const float since = float(std::clamp((t - shownAt[s]) / 120.0, 0.0, 1.0));
      seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;   // deterministic jitter breaks ties
      const float jit = float((seed >> 40) & 0xFFFF) / 65535.f * .12f;
      const float score = affinity(s, cls) * (.2f + .8f * since) + jit;
      if (score > bestScore) { bestScore = score; best = s; }
    }
    return best;
  }
  Decision update(double t, float level, uint64_t barIndex) {
    Decision d;
    const float dt = lastT < 0 ? 0.f : float(std::clamp(t - lastT, 0.0, .25));
    lastT = t;
    level = std::clamp(level, 0.f, 1.f);
    fast += (level - fast) * (1.f - std::exp(-dt / .4f));
    slow += (level - slow) * (1.f - std::exp(-dt / 8.f));
    const double held = t - enteredAt;
    if (fast > slow + .26f && slow < .75f && t - lastDrop > 8.0) { d.drop = true; lastDrop = t; }
    if (fast < slow * .45f && slow > .3f && held > 12.0 && !pendingCut) d.breakdown = true;
    if (!pendingCut && held >= minHold && (held >= maxHold || d.drop || d.breakdown)) pendingCut = true;
    if (!pendingCut && d.drop && held >= 8.0) pendingCut = true;   // a drop is worth an early cut
    if (pendingCut && barIndex != lastBar) {                       // land the cut on the next bar line
      const int next = pick(slow, t);
      style = next;
      shownAt[next] = t;
      enteredAt = t;
      pendingCut = false;
      d.switchTo = next;
    }
    lastBar = barIndex;
    const float target = 1.0f + 1.2f * std::clamp(slow * 1.4f + fast * .5f, 0.f, 1.f);   // 1.0 .. 2.2
    gain += (target - gain) * (1.f - std::exp(-dt / 2.5f));
    d.gain = gain;
    return d;
  }
};
}  // namespace director
