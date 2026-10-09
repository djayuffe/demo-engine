#include "Audio.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

AudioPlayer::~AudioPlayer(){ close(); }

bool AudioPlayer::open(const std::filesystem::path& path) {
  close();
  path_ = path;
#if defined(IW_HAS_AUDIO)
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::cerr << "Music not found: " << path << '\n';
    return false;
  }
  std::vector<char> data((std::istreambuf_iterator<char>(file)), {});
  module_ = openmpt_module_create_from_memory2(
      data.data(), data.size(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  if (!module_) {
    std::cerr << "libopenmpt rejected: " << path << '\n';
    return false;
  }
  SDL_AudioSpec want{}, got{};
  want.freq = rate_;
  want.format = AUDIO_F32SYS;
  want.channels = 2;
  want.samples = 512;
  want.callback = &AudioPlayer::callback;
  want.userdata = this;
  device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &got, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (!device_) {
    std::cerr << "SDL_OpenAudioDevice: " << SDL_GetError() << '\n';
    close();
    return false;
  }
  if (got.format != AUDIO_F32SYS || got.channels != 2) {
    std::cerr << "Unsupported audio format returned by SDL\n";
    close();
    return false;
  }
  rate_ = got.freq;
  SDL_PauseAudioDevice(device_, 0);
  return true;
#else
  std::cerr << "Audio support was not compiled in; using BPM-only timing.\n";
  return false;
#endif
}

void AudioPlayer::close() {
#if defined(IW_HAS_AUDIO)
  if (device_) {
    SDL_CloseAudioDevice(device_);
    device_ = 0;
  }
  if (module_) {
    openmpt_module_destroy(module_);
    module_ = nullptr;
  }
  seconds_.store(0.0);
  level_.store(0.0f);
  bass_.store(0.0f); mid_.store(0.0f); treble_.store(0.0f);
  lpB_=0.f; lpM_=0.f; lpT_=0.f;
#endif
}

MusicState AudioPlayer::state() const {
  MusicState s{};
#if defined(IW_HAS_AUDIO)
  s.active = module_ != nullptr;
  s.seconds = seconds_.load();
  s.level = level_.load();
  s.bass = bass_.load();
  s.mid = mid_.load();
  s.treble = treble_.load();
  s.order = order_.load();
  s.pattern = pattern_.load();
  s.row = row_.load();
  s.tempo = tempo_.load();
  s.speed = speed_.load();
#endif
  return s;
}

#if defined(IW_HAS_AUDIO)
void AudioPlayer::callback(void* userdata, Uint8* stream, int len) {
  auto* self = static_cast<AudioPlayer*>(userdata);
  self->render(reinterpret_cast<float*>(stream), len / int(sizeof(float) * 2));
}

void AudioPlayer::render(float* out, int frames) {
  if (!module_) {
    std::fill(out, out + frames * 2, 0.0f);
    return;
  }
  size_t got = openmpt_module_read_interleaved_float_stereo(
      module_, rate_, static_cast<size_t>(frames), out);
  // Loop the track: when the module ends it keeps returning 0 frames forever,
  // which would freeze the visual at the last frame. Detect that and rewind to
  // 0 so the music (and the beat-synced visuals) keep going.
  if (got == 0) {
    openmpt_module_set_position_seconds(module_, 0.0);
    got = openmpt_module_read_interleaved_float_stereo(
        module_, rate_, static_cast<size_t>(frames), out);
  }
  if (got < static_cast<size_t>(frames)) {
    std::fill(out + got * 2, out + frames * 2, 0.0f);
  }
  double sum = 0.0;
  float bSum=0,mSum=0,tSum=0;
  // Zero-phase-ish 3-band split via simple one-pole IIR (stable, no aliasing
  // guard needed for audio-rate signals). Bass <350 Hz, mid 350-3000, treble
  // >3000. Coefficients precomputed for 48 kHz (close enough for 44.1 too).
  // Metrics are computed over the VALID samples only (got*2) — not the
  // zero-filled tail — so the silence at the wrap point doesn't drag the RMS
  // and band levels down.
  const float aB=.021f, aM=.14f, aT=.5f;  // 1-pole lowpass alphas
  // lpB_/lpM_/lpT_ are persistent member state (not per-call locals) so the
  // one-pole filters stay warm across the ~37 render() calls/second.
  const int valid = static_cast<int>(got * 2);
  for (int i = 0; i < valid; ++i) {
    float s=out[i];
    sum += double(s) * double(s);
    lpB_ += aB*(s-lpB_); lpM_ += aM*(s-lpM_); lpT_ += aT*(s-lpT_);
    float lo=lpB_, band=lpM_-lpB_, hi=lpT_-lpM_;
    bSum+=std::fabs(lo); mSum+=std::fabs(band); tSum+=std::fabs(hi);
  }
  const int N=std::max(1,valid);
  const float rms = static_cast<float>(std::sqrt(sum / N));
  level_.store(std::clamp(rms * 4.0f, 0.0f, 1.0f));
  bass_.store(std::clamp(bSum/float(N)*6.0f,0.f,1.f));
  mid_.store(std::clamp(mSum/float(N)*6.0f,0.f,1.f));
  treble_.store(std::clamp(tSum/float(N)*6.0f,0.f,1.f));
  seconds_.store(openmpt_module_get_position_seconds(module_));
  order_.store(int(openmpt_module_get_current_order(module_)));
  pattern_.store(int(openmpt_module_get_current_pattern(module_)));
  row_.store(int(openmpt_module_get_current_row(module_)));
  tempo_.store(int(std::lround(openmpt_module_get_current_tempo2(module_))));
  speed_.store(int(openmpt_module_get_current_speed(module_)));
}
#endif
