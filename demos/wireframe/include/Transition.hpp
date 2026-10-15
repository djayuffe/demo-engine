#pragma once
// Scene-to-scene flow. A scene lasts two bars; instead of a hard cut, the last beat before the cut
// is an EXIT (the form rushes toward the camera, the picture smears into a zoom blur, cinema bars
// close in) and the first beat after it is an ENTRY (a flash on the downbeat, chromatic split, the
// new form grows in from 80%, the bars open). Manual scene changes get the entry too. Pure, tested.
#include <algorithm>
#include <cstdint>

namespace transition {
struct Cut {
  float in = 0.f;    // 1 at the cut, eases to 0 over one beat
  float out = 0.f;   // 0 .. 1 over the last beat before the next cut (auto mode only)
};
inline float smooth(float e0, float e1, float x) {
  const float t = std::clamp((x - e0) / (e1 - e0), 0.f, 1.f);
  return t * t * (3.f - 2.f * t);
}
// barIndex/barPhase: the show clock; beatsPerBar 4; secondsSinceChange: time since the scene last
// changed (manual or auto); beatSeconds: 60/bpm. autoMode = scenes advance by themselves.
inline Cut at(uint64_t barIndex, float barPhase, int barsPerScene, bool autoMode, double secondsSinceChange,
              double beatSeconds) {
  Cut c;
  const float beats = float(barsPerScene * 4);
  const float pos = (float(barIndex % uint64_t(barsPerScene)) + std::clamp(barPhase, 0.f, 1.f)) * 4.f;   // beats into the scene
  if (autoMode) c.out = smooth(beats - 1.f, beats, pos);
  const float since = float(std::max(0.0, secondsSinceChange) / std::max(1e-3, beatSeconds));         // beats since the cut
  const float ein = 1.f - smooth(0.f, 1.f, since);
  c.in = ein * ein;
  return c;
}
}  // namespace transition
