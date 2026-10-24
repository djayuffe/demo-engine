#include "Audio.hpp"
#include "MusicCatalog.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <limits>
#if defined(IW_HAS_SNDFILE)
#include <sndfile.h>
#endif

AudioPlayer::~AudioPlayer(){ close(); }
bool AudioPlayer::extendedFormats() {
#if defined(IW_HAS_SNDFILE)
  return true;
#else
  return false;
#endif
}

const std::filesystem::path& AudioPlayer::trackPath(int i) const {
  static const std::filesystem::path none;
  return i >= 0 && i < int(tracks_.size()) ? tracks_[size_t(i)].path : none;
}

double AudioPlayer::nativeBpm(int i) const {
  return i >= 0 && i < int(tracks_.size()) ? tracks_[size_t(i)].bpm : 0.0;
}

const char* AudioPlayer::tempoSource(int i) const {
  return i>=0&&i<int(tracks_.size())?tracks_[size_t(i)].tempoSource:"fallback";
}

bool AudioPlayer::open(const std::vector<std::filesystem::path>& playlist) {
  close();
#if defined(IW_HAS_AUDIO)
  struct PendingWav { size_t track; SDL_AudioSpec spec; Uint8* buf; Uint32 len; };
  std::vector<PendingWav> wavs;
  struct PendingDecoded { size_t track; int rate, channels; };
  std::vector<PendingDecoded> decoded;
  for (const auto& path : playlist) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { std::cerr << "Music not found: " << path << '\n'; continue; }
    Track t; t.path = path;
    // Tracker modules go to libopenmpt; a .wav (which libopenmpt cannot read) goes to SDL. The ext
    // check keeps libopenmpt from logging a spurious error for an obvious .wav.
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return char(std::tolower(ch)); });
#if defined(IW_HAS_SNDFILE)
    if (ext == ".flac" || ext == ".ogg" || ext == ".oga" || ext == ".mp3" ||
        ext == ".aif" || ext == ".aiff" || ext == ".au") {
      SF_INFO info{};
#ifdef _WIN32
      SNDFILE* sound = sf_wchar_open(path.c_str(), SFM_READ, &info);
#else
      SNDFILE* sound = sf_open(path.c_str(), SFM_READ, &info);
#endif
      if (!sound) { std::cerr << "Audio decode failed: " << path << ": " << sf_strerror(nullptr) << '\n'; continue; }
      if (info.frames <= 0 || info.channels < 1 || info.channels > 8 || info.samplerate < 8000 ||
          info.samplerate > 192000 || info.frames > std::numeric_limits<int>::max() / (info.channels * int(sizeof(float)))) {
        sf_close(sound); std::cerr << "Unsupported audio dimensions: " << path << '\n'; continue;
      }
      t.pcm.resize(size_t(info.frames) * size_t(info.channels));
      const sf_count_t frames = sf_readf_float(sound, t.pcm.data(), info.frames);
      sf_close(sound);
      if (frames <= 0) { std::cerr << "Audio has no samples: " << path << '\n'; continue; }
      t.pcm.resize(size_t(frames) * size_t(info.channels));
      decoded.push_back({tracks_.size(), info.samplerate, info.channels});
      tracks_.push_back(std::move(t));
      continue;
    }
#endif
    std::vector<char> data((std::istreambuf_iterator<char>(file)), {});
    if (ext != ".wav")
      t.mod = openmpt_module_create_from_memory2(data.data(), data.size(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (t.mod) {
      const int sp = int(openmpt_module_get_current_speed(t.mod));
      const double tp = openmpt_module_get_current_tempo2(t.mod);
      t.bpm = trackerBpm(tp, sp);
      t.confidence = t.bpm>0?1.f:0.f; t.tempoSource="tracker";
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
  for (const auto& source : decoded) {
    auto& t = tracks_[source.track];
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, AUDIO_F32SYS, Uint8(source.channels), source.rate, AUDIO_F32SYS, 2, rate_) < 0) {
      bad[source.track] = true; continue;
    }
    if (!cvt.needed) continue;
    const size_t bytes = t.pcm.size() * sizeof(float);
    if (bytes > size_t(std::numeric_limits<int>::max()) / size_t(std::max(1, cvt.len_mult))) {
      bad[source.track] = true; continue;
    }
    std::vector<Uint8> work(bytes * size_t(std::max(1, cvt.len_mult)));
    std::copy(reinterpret_cast<const Uint8*>(t.pcm.data()), reinterpret_cast<const Uint8*>(t.pcm.data()) + bytes, work.begin());
    cvt.buf = work.data(); cvt.len = int(bytes);
    if (SDL_ConvertAudio(&cvt) < 0) { bad[source.track] = true; continue; }
    const auto* samples = reinterpret_cast<const float*>(work.data());
    t.pcm.assign(samples, samples + size_t(cvt.len_cvt) / sizeof(float));
    t.pcm.resize(t.pcm.size() & ~size_t(1));
    if (t.pcm.empty()) bad[source.track] = true;
  }
  for (size_t i = tracks_.size(); i-- > 0;) if (bad[i]) tracks_.erase(tracks_.begin() + std::ptrdiff_t(i));
  if (tracks_.empty()) { close(); return false; }
  for (auto& track : tracks_) if (!track.mod) {
    const auto* known=musiccat::find(track.path.filename().string());
    if(known && known->bpm>0) {
      track.bpm=known->bpm;track.confidence=1;track.tempoSource="catalog";
    } else if(known && !known->estimate) {
      track.tempoSource="free-time";
    } else {
      const auto tempo=audio_rhythm::estimate(track.pcm,2,rate_);
      track.bpm=tempo.bpm;track.confidence=tempo.confidence;
      track.tempoSource=tempo.bpm>0?"estimated":"fallback";
    }
  }
  const int requested = std::max(0, start_.exchange(-1));
  const int64_t initial = int64_t(requested) + skip_.exchange(0);
  const int64_t count = int64_t(tracks_.size());
  const int selected = int((initial % count + count) % count);
  current_.store(selected);
  nativeBpm_.store(tracks_[size_t(selected)].bpm);
  tempoConfidence_.store(tracks_[size_t(selected)].confidence);
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
  nativeBpm_.store(0.0);
  level_.store(0.0f);
  onset_.store(0);tempoConfidence_.store(0);onsetDetector_.reset();
  bass_.store(0.0f); mid_.store(0.0f); treble_.store(0.0f);
  for (int channel = 0; channel < 2; ++channel) lpB_[channel] = lpM_[channel] = 0;
#endif
  tracks_.clear();
}

MusicState AudioPlayer::state() const {
  MusicState s{};
#if defined(IW_HAS_AUDIO)
  s.active = device_ != 0 && !tracks_.empty();
  s.paused = paused_.load();
  s.nativeBpm = nativeBpm_.load();
  s.onset=onset_.load();s.tempoConfidence=tempoConfidence_.load();
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
  onsetDetector_.reset();onset_.store(0);
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
  const int requested = start_.exchange(-1), step = skip_.exchange(0);
  if (requested >= 0 || step) {
    const int64_t n = int64_t(tracks_.size());
    const int64_t target = int64_t(requested >= 0 ? requested : cur) + step;
    cur = int((target % n + n) % n);
    rewind(tracks_[size_t(cur)]);
    current_.store(cur);
    nativeBpm_.store(tracks_[size_t(cur)].bpm);
    tempoConfidence_.store(tracks_[size_t(cur)].confidence);
    level_.store(0); bass_.store(0); mid_.store(0); treble_.store(0);
    for (int channel = 0; channel < 2; ++channel) lpB_[channel] = lpM_[channel] = 0;
  }
  // Track selection remains responsive while paused, without advancing played time.
  if (paused_.load()) {
    std::fill(out, out + frames * 2, 0.f);
    return;
  }
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
  analyze(out, frames * 2); // analysis precedes volume: muting does not mute visual rhythm
  const float gain = volume_.load();
  for (int i = 0; i < frames * 2; ++i)
    out[i] = std::isfinite(out[i]) ? std::clamp(out[i] * gain, -1.f, 1.f) : 0.f;
  playedFrames_ += uint64_t(frames);
  seconds_.store(double(playedFrames_) / double(rate_));
  if (openmpt_module* m = tracks_[size_t(cur)].mod) {
    order_.store(int(openmpt_module_get_current_order(m)));
    pattern_.store(int(openmpt_module_get_current_pattern(m)));
    row_.store(int(openmpt_module_get_current_row(m)));
    tempo_.store(int(std::lround(openmpt_module_get_current_tempo2(m))));
    const int speed = int(openmpt_module_get_current_speed(m));
    speed_.store(speed);
    nativeBpm_.store(trackerBpm(openmpt_module_get_current_tempo2(m), speed));
    tempoConfidence_.store(1);
  } else {
    nativeBpm_.store(tracks_[size_t(cur)].bpm);
    tempoConfidence_.store(tracks_[size_t(cur)].confidence);
  }
}

void AudioPlayer::analyze(const float* out, int valid) {
  double sum = 0.0;
  float bSum=0,mSum=0,tSum=0;
  // Independent channel filters avoid interleaving L/R samples into one IIR state.
  // Coefficients follow the actual device rate; treble is the >3 kHz residual.
  const float aB=1.f-std::exp(-6.2831853f*350.f/float(rate_));
  const float aM=1.f-std::exp(-6.2831853f*3000.f/float(rate_));
  for (int i = 0; i < valid; ++i) {
    const int channel = i & 1;
    const float s = std::isfinite(out[i]) ? out[i] : 0.f;
    sum += double(s) * double(s);
    lpB_[channel] += aB*(s-lpB_[channel]); lpM_[channel] += aM*(s-lpM_[channel]);
    bSum += std::fabs(lpB_[channel]);
    mSum += std::fabs(lpM_[channel]-lpB_[channel]);
    tSum += std::fabs(s-lpM_[channel]);
  }
  const int N=std::max(1,valid);
  const float rms = static_cast<float>(std::sqrt(sum / N));
  onset_.store(onsetDetector_.process(rms,bSum/float(N),mSum/float(N),tSum/float(N),double(valid)/(2.0*rate_)));
  level_.store(std::clamp(rms * 4.0f, 0.0f, 1.0f));
  bass_.store(std::clamp(bSum/float(N)*6.0f,0.f,1.f));
  mid_.store(std::clamp(mSum/float(N)*6.0f,0.f,1.f));
  treble_.store(std::clamp(tSum/float(N)*6.0f,0.f,1.f));
}
#endif
