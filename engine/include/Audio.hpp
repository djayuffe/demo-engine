#pragma once
#include <atomic>
#include "AudioRhythm.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>
#include <cstdint>

#if defined(IW_HAS_AUDIO)
#include <SDL.h>
#include <libopenmpt/libopenmpt.h>
#endif

inline double trackerBpm(double tempo, int speed) {
  if (!std::isfinite(tempo) || tempo <= 0 || speed <= 0) return 0;
  double bpm = tempo * 6.0 / speed;
  if (!std::isfinite(bpm) || bpm <= 0) return 0;
  while (bpm >= 180) bpm *= .5;
  while (bpm < 70) bpm *= 2;
  return bpm;
}

struct MusicState {
  bool active=false, paused=false;
  float onset=0, tempoConfidence=0;
  double nativeBpm=0; // current tracker tempo, including mid-track changes; catalog/estimated BPM for PCM
  double seconds=0.0;   // played time since open(): keeps counting across loops and track changes
  float level=0.0f;
  float bass=0.0f;
  float mid=0.0f;
  float treble=0.0f;
  int track=-1;         // index into the playlist of the track that is playing
  int order=0;
  int pattern=0;
  int row=0;
  int tempo=125;
  int speed=6;
};

class AudioPlayer {
public:
  ~AudioPlayer();
  // Plays the tracks in order and loops the list (a one-track list loops that track). Every track is
  // decoded up front, so the audio callback never touches the disk. Tracks that fail to load are
  // reported and skipped; returns false if none loaded or the device could not be opened.
  bool open(const std::vector<std::filesystem::path>& playlist);
  bool open(const std::filesystem::path& path) { return open(std::vector<std::filesystem::path>{path}); }
  void close();
  void setVolume(float volume) { volume_.store(std::isfinite(volume) ? std::clamp(volume, 0.f, 2.f) : 1.f,
                                                std::memory_order_relaxed); }
  float volume() const { return volume_.load(std::memory_order_relaxed); }
  void setPaused(bool paused) { paused_.store(paused, std::memory_order_relaxed); }
  bool paused() const { return paused_.load(std::memory_order_relaxed); }
  static bool extendedFormats();
  MusicState state() const;
  int trackCount() const { return int(tracks_.size()); }
  const std::filesystem::path& trackPath(int i) const;
  // A track's own tempo in BPM, folded into 70..180 (tempo * 6 / speed assumes 4 rows per beat; fast
  // modules use 8, so 276 becomes 138). PCM uses catalog/estimated BPM, or 0 when unreliable.
  double nativeBpm(int i) const;
  const char* tempoSource(int i) const;
  // Jump to another track of the playlist (wraps; negative steps go back). Thread-safe: the audio
  // callback picks the request up at its next block and starts that track from the top.
  void skip(int step = 1) { skip_.fetch_add(step, std::memory_order_relaxed); }
  // Start on track i instead of the first (call before or after open()).
  void startAt(int i) { start_.store(std::max(0, i), std::memory_order_relaxed); }
private:
  std::atomic<int> skip_{0}, start_{-1};
  std::atomic<float> volume_{1.f};
  std::atomic<bool> paused_{false};
  struct Track {
    std::filesystem::path path;
    double bpm = 0.0;
    float confidence=0;
    const char* tempoSource="fallback";
#if defined(IW_HAS_AUDIO)
    openmpt_module* mod = nullptr;
    // Decoded PCM (WAV or optional codecs), converted once to device-rate float stereo.
    std::vector<float> pcm;
    size_t pos = 0;   // in frames (WAV only; modules keep their own position)
#endif
  };
  std::vector<Track> tracks_;
#if defined(IW_HAS_AUDIO)
  static void callback(void* userdata, Uint8* stream, int len);
  void render(float* out,int frames);
  void analyze(const float* out,int validSamples);   // level + bass/mid/treble from the rendered block
  void rewind(Track& t);
  std::atomic<int> current_{0};
  // Frames handed to the device since open(). The music clock is played time, not the position in
  // the track, so the show does not restart with every loop or track change.
  uint64_t playedFrames_{};
  SDL_AudioDeviceID device_{};
  int rate_{48000};
  float bassAlpha_{}, midAlpha_{};
  std::atomic<double> seconds_{}, nativeBpm_{};
  std::atomic<float> level_{}, onset_{}, tempoConfidence_{};
  audio_rhythm::Onset onsetDetector_;
  std::atomic<float> bass_{},mid_{},treble_{};
  std::atomic<int> order_{},pattern_{},row_{},tempo_{125},speed_{6};
  // Persistent one-pole IIR state for the band-split (the audio callback is
  // single-threaded, so these need no sync). Kept across render() calls so the
  // filters aren't re-warmed on every 512-sample chunk.
  float lpB_[2]{},lpM_[2]{};
#endif
};
