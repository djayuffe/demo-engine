#pragma once
#include <algorithm>
#include <cmath>

namespace rhythm {
enum class Source { Hybrid, Clock, Audio };
inline const char* name(Source source){return source==Source::Audio?"audio":source==Source::Clock?"clock":"hybrid";}
struct Settings {
  Source source=Source::Hybrid;
  float pulse = 1.f, flash = .65f, decay = 9.f;
  int division = 1;
  double offset = 0; // seconds; positive anticipates the next beat
};
struct Drive {
  double position = 0;
  float pulse = 0, flash = 0, accent = 0, bass = 0, phase = 0;
};
inline float unit(float value) { return std::isfinite(value) ? std::clamp(value, 0.f, 1.f) : 0.f; }
inline Drive sample(double beat, double bpm, bool active, float bass, const Settings& settings) {
  Drive d;
  beat = std::isfinite(beat) ? std::clamp(beat, 0.0, 1e12) : 0;
  bpm = std::isfinite(bpm) ? std::clamp(bpm, 1.0, 1000.0) : 132;
  const double offset = std::isfinite(settings.offset) ? std::clamp(settings.offset, -2.0, 2.0) : 0;
  d.position = std::max(0.0, beat + offset * bpm / 60.0);
  const int division = settings.division == 2 || settings.division == 4 ? settings.division : 1;
  const double steps = d.position * division;
  d.phase = float(steps - std::floor(steps));
  const float decay = std::isfinite(settings.decay) ? std::clamp(settings.decay, 2.f, 30.f) : 9.f;
  const float hit = std::exp(-decay * d.phase);
  const int slot = int(std::fmod(std::floor(steps), double(4 * division)));
  d.accent = slot == 0 ? 1.f : slot % division ? .55f : slot == 2 * division ? .85f : .72f;
  d.bass = active ? unit(bass) : hit;
  const float music = active ? .75f + .25f * d.bass : 1.f;
  const float pulse = std::isfinite(settings.pulse) ? std::clamp(settings.pulse, 0.f, 2.f) : 0;
  const float flash = std::isfinite(settings.flash) ? std::clamp(settings.flash, 0.f, 2.f) : 0;
  d.pulse = hit * music * pulse;
  d.flash = std::exp(-decay * 1.7f * d.phase) * d.accent * music * flash;
  return d;
}
inline Drive withAudio(Drive d,bool active,float onset,float confidence,const Settings& settings) {
  if(!active||settings.source==Source::Clock)return d;
  const float decay=std::isfinite(settings.decay)?std::clamp(settings.decay,2.f,30.f):9.f;
  const float attack=std::pow(unit(onset),decay/9.f);
  const float pulse=std::isfinite(settings.pulse)?std::clamp(settings.pulse,0.f,2.f):0;
  const float flash=std::isfinite(settings.flash)?std::clamp(settings.flash,0.f,2.f):0;
  const float clock=settings.source==Source::Audio?0.f:.3f+.4f*unit(confidence);
  d.pulse=std::max(d.pulse*clock,attack*pulse);
  d.flash=std::max(d.flash*clock,std::pow(attack,1.7f)*flash);
  return d;
}

} // namespace rhythm
