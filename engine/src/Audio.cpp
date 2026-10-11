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

const std::filesystem::path& AudioPlayer::trackPath(int i) const {
  static const std::filesystem::path none;
  return i >= 0 && i < int(tracks_.size()) ? tracks_[size_t(i)].path : none;
}

double AudioPlayer::nativeBpm(int i) const {
  return i >= 0 && i < int(tracks_.size()) ? tracks_[size_t(i)].bpm : 0.0;
}

bool AudioPlayer::open(const std::vector<std::filesystem::path>& playlist) {
  close();
#if defined(IW_HAS_AUDIO)
  struct PendingWav { size_t track; SDL_AudioSpec spec; Uint8* buf; Uint32 len; };
  std::vector<PendingWav> wavs;
  for (const auto& path : playlist) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { std::cerr << "Music not found: " << path << '\n'; continue; }
    std::vector<char> data((std::istreambuf_iterator<char>(file)), {});
    Track t; t.path = path;
    // Tracker modules go to libopenmpt; a .wav (which libopenmpt cannot read) goes to SDL. The ext
    // check keeps libopenmpt from logging a spurious error for an obvious .wav.
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return char(std::tolower(ch)); });
    if (ext != ".wav")
      t.mod = openmpt_module_create_from_memory2(data.data(), data.size(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (t.mod) {
      const int sp = int(openmpt_module_get_current_speed(t.mod));
      const double tp = openmpt_module_get_current_tempo2(t.mod);
      if (sp > 0 && tp > 0) {
        double b = tp * 6.0 / sp;
        while (b >= 180.0) b *= .5;   // 8 rows per beat at a fast speed reads as double tempo
        while (b < 70.0) b *= 2.0;
        t.bpm = b;
      }
      tracks_.push_back(std::move(t));
      continue;
    }
    SDL_AudioSpec spec{}; Uint8* buf = nullptr; Uint32 len = 0;
    SDL_RWops* rw = SDL_RWFromConstMem(data.data(), int(data.size()));
    if (!rw || !SDL_LoadWAV_RW(rw, 1, &spec, &buf, &len)) { std::cerr << "Not a tracker module or WAV file: " << path << '\n'; continue; }
    wavs.push_back({tracks_.size(), spec, buf, len});
    tracks_.push_back(std::move(t));
  }
  const auto freeWavs = [&]() { for (auto& w : wavs) if (w.buf) { SDL_FreeWAV(w.buf); w.buf = nullptr; } };
  if (tracks_.empty()) return false;
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
    freeWavs(); close();
    return false;
  }
  if (got.format != AUDIO_F32SYS || got.channels != 2) {
    std::cerr << "Unsupported audio format returned by SDL\n";
    freeWavs(); close();
    return false;
  }
  rate_ = got.freq;
  // WAVs are converted now that the device rate is known.
  std::vector<bool> bad(tracks_.size(), false);
  for (auto& w : wavs) {
    SDL_AudioCVT cvt;
    Track& t = tracks_[w.track];
    if (SDL_BuildAudioCVT(&cvt, w.spec.format, w.spec.channels, w.spec.freq, AUDIO_F32SYS, 2, rate_) < 0) {
      std::cerr << "SDL_BuildAudioCVT: " << SDL_GetError() << '\n'; bad[w.track] = true; continue;
    }
    std::vector<Uint8> work(size_t(w.len) * size_t(std::max(1, cvt.len_mult)));
    std::copy(w.buf, w.buf + w.len, work.begin());
    cvt.len = int(w.len);
    cvt.buf = work.data();
    if (cvt.needed && SDL_ConvertAudio(&cvt) < 0) { std::cerr << "SDL_ConvertAudio: " << SDL_GetError() << '\n'; bad[w.track] = true; continue; }
    const size_t bytes = cvt.needed ? size_t(cvt.len_cvt) : size_t(w.len);
    t.pcm.assign(reinterpret_cast<const float*>(work.data()), reinterpret_cast<const float*>(work.data()) + bytes / sizeof(float));
    t.pcm.resize(t.pcm.size() & ~size_t(1));   // whole stereo frames only
    if (t.pcm.empty()) { std::cerr << "WAV file has no samples: " << t.path << '\n'; bad[w.track] = true; }
  }
  freeWavs();
  for (size_t i = tracks_.size(); i-- > 0;) if (bad[i]) tracks_.erase(tracks_.begin() + std::ptrdiff_t(i));
  if (tracks_.empty()) { close(); return false; }
  current_.store(0);
  SDL_PauseAudioDevice(device_, 0);
  return true;
#else
  (void)playlist;
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
  for (auto& t : tracks_) if (t.mod) openmpt_module_destroy(t.mod);
  current_.store(0);
  playedFrames_ = 0;
  seconds_.store(0.0);
  level_.store(0.0f);
  bass_.store(0.0f); mid_.store(0.0f); treble_.store(0.0f);
  lpB_=0.f; lpM_=0.f; lpT_=0.f;
#endif
  tracks_.clear();
}

MusicState AudioPlayer::state() const {
  MusicState s{};
#if defined(IW_HAS_AUDIO)
  s.active = device_ != 0 && !tracks_.empty();
  s.seconds = seconds_.load();
  s.level = level_.load();
  s.bass = bass_.load();
  s.mid = mid_.load();
  s.treble = treble_.load();
  s.track = s.active ? current_.load() : -1;
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

void AudioPlayer::rewind(Track& t) {
  if (t.mod) openmpt_module_set_position_seconds(t.mod, 0.0);
  t.pos = 0;
}

void AudioPlayer::render(float* out, int frames) {
  if (tracks_.empty()) {
    std::fill(out, out + frames * 2, 0.0f);
    return;
  }
  // Fill the whole block, moving to the next track (and wrapping to the first) whenever the current
  // one ends, so there is no gap between tracks. A one-track playlist simply loops that track.
  int done = 0, switches = 0;
  int cur = current_.load();
  while (done < frames) {
    Track& t = tracks_[size_t(cur)];
    const int want = frames - done;
    int got = 0;
    if (t.mod) {
      got = int(openmpt_module_read_interleaved_float_stereo(t.mod, rate_, size_t(want), out + 2 * done));
    } else {
      const size_t total = t.pcm.size() / 2;
      got = int(std::min<size_t>(size_t(want), total - std::min(t.pos, total)));
      std::copy(t.pcm.begin() + std::ptrdiff_t(2 * t.pos), t.pcm.begin() + std::ptrdiff_t(2 * (t.pos + size_t(got))), out + 2 * done);
      t.pos += size_t(got);
    }
    done += got;
    if (got < want) {
      // A track that produces nothing even after a rewind would spin forever: give up on this block.
      if (++switches > 2 * int(tracks_.size()) + 1) { std::fill(out + 2 * done, out + 2 * frames, 0.0f); break; }
      cur = (cur + 1) % int(tracks_.size());
      rewind(tracks_[size_t(cur)]);
    }
  }
  current_.store(cur);
  analyze(out, frames * 2);
  playedFrames_ += uint64_t(frames);
  seconds_.store(double(playedFrames_) / double(rate_));
  if (openmpt_module* m = tracks_[size_t(cur)].mod) {
    order_.store(int(openmpt_module_get_current_order(m)));
    pattern_.store(int(openmpt_module_get_current_pattern(m)));
    row_.store(int(openmpt_module_get_current_row(m)));
    tempo_.store(int(std::lround(openmpt_module_get_current_tempo2(m))));
    speed_.store(int(openmpt_module_get_current_speed(m)));
  }
}

void AudioPlayer::analyze(const float* out, int valid) {
  double sum = 0.0;
  float bSum=0,mSum=0,tSum=0;
  // Zero-phase-ish 3-band split via simple one-pole IIR (stable, no aliasing
  // guard needed for audio-rate signals). Bass <350 Hz, mid 350-3000, treble
  // >3000. Coefficients precomputed for 48 kHz (close enough for 44.1 too).
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
