#include "Audio.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
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
  // Tracker modules go to libopenmpt; anything it rejects is tried as a WAV through SDL. The
  // ext check keeps libopenmpt from logging a spurious error for an obvious .wav.
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return char(std::tolower(ch)); });
  if (ext != ".wav")
    module_ = openmpt_module_create_from_memory2(
        data.data(), data.size(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  if (module_) {
    const int sp = int(openmpt_module_get_current_speed(module_));
    const double tp = openmpt_module_get_current_tempo2(module_);
    if (sp > 0 && tp > 0) nativeBpm_ = tp * 6.0 / sp;
  }
  SDL_AudioSpec wavSpec{};
  Uint8* wavBuf = nullptr;
  Uint32 wavLen = 0;
  if (!module_) {
    SDL_RWops* rw = SDL_RWFromConstMem(data.data(), int(data.size()));
    if (!rw || !SDL_LoadWAV_RW(rw, 1, &wavSpec, &wavBuf, &wavLen)) {
      std::cerr << "Not a tracker module or WAV file: " << path << '\n';
      return false;
    }
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
  if (wavBuf) {
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, wavSpec.format, wavSpec.channels, wavSpec.freq, AUDIO_F32SYS, 2, rate_) < 0) {
      std::cerr << "SDL_BuildAudioCVT: " << SDL_GetError() << '\n';
      SDL_FreeWAV(wavBuf);
      close();
      return false;
    }
    std::vector<Uint8> work(size_t(wavLen) * size_t(std::max(1, cvt.len_mult)));
    std::copy(wavBuf, wavBuf + wavLen, work.begin());
    SDL_FreeWAV(wavBuf);
    cvt.len = int(wavLen);
    cvt.buf = work.data();
    if (cvt.needed && SDL_ConvertAudio(&cvt) < 0) {
      std::cerr << "SDL_ConvertAudio: " << SDL_GetError() << '\n';
      close();
      return false;
    }
    const size_t bytes = cvt.needed ? size_t(cvt.len_cvt) : size_t(wavLen);
    pcm_.assign(reinterpret_cast<const float*>(work.data()), reinterpret_cast<const float*>(work.data()) + bytes / sizeof(float));
    pcm_.resize(pcm_.size() & ~size_t(1));   // whole stereo frames only
    pcmPos_ = 0;
    if (pcm_.empty()) {
      std::cerr << "WAV file has no samples: " << path << '\n';
      close();
      return false;
    }
  }
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
  pcm_.clear();
  pcmPos_ = 0;
  playedFrames_ = 0;
  nativeBpm_ = 0.0;
  seconds_.store(0.0);
  level_.store(0.0f);
  bass_.store(0.0f); mid_.store(0.0f); treble_.store(0.0f);
  lpB_=0.f; lpM_=0.f; lpT_=0.f;
#endif
}

MusicState AudioPlayer::state() const {
  MusicState s{};
#if defined(IW_HAS_AUDIO)
  s.active = module_ != nullptr || !pcm_.empty();
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
  if (!pcm_.empty()) {
    // WAV: copy from the decoded buffer, wrapping at the end so the track loops seamlessly.
    const size_t total = pcm_.size() / 2;
    for (int f = 0; f < frames; ++f) {
      out[2 * f] = pcm_[2 * pcmPos_];
      out[2 * f + 1] = pcm_[2 * pcmPos_ + 1];
      if (++pcmPos_ >= total) pcmPos_ = 0;
    }
    analyze(out, frames * 2);
    playedFrames_ += uint64_t(frames);
    seconds_.store(double(playedFrames_) / double(rate_));
    return;
  }
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
  analyze(out, static_cast<int>(got * 2));
  playedFrames_ += uint64_t(frames);
  seconds_.store(double(playedFrames_) / double(rate_));
  order_.store(int(openmpt_module_get_current_order(module_)));
  pattern_.store(int(openmpt_module_get_current_pattern(module_)));
  row_.store(int(openmpt_module_get_current_row(module_)));
  tempo_.store(int(std::lround(openmpt_module_get_current_tempo2(module_))));
  speed_.store(int(openmpt_module_get_current_speed(module_)));
}

void AudioPlayer::analyze(const float* out, int valid) {
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
}
#endif
