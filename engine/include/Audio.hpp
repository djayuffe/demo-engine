#pragma once
#include <atomic>
#include <filesystem>
#include <vector>
#include <cstdint>

#if defined(IW_HAS_AUDIO)
#include <SDL.h>
#include <libopenmpt/libopenmpt.h>
#endif

struct MusicState {
  bool active=false;
  double seconds=0.0;
  float level=0.0f;
  float bass=0.0f;
  float mid=0.0f;
  float treble=0.0f;
  int order=0;
  int pattern=0;
  int row=0;
  int tempo=125;
  int speed=6;
};

class AudioPlayer {
public:
  ~AudioPlayer();
  bool open(const std::filesystem::path& path);
  void close();
  MusicState state() const;
  const std::filesystem::path& path() const { return path_; }
  // The module's own tempo in BPM (tempo * 6 / speed, i.e. 4 rows per beat) right after open();
  // 0 for WAV tracks or when nothing is open. Lets the visuals lock to any module without --bpm.
  double nativeBpm() const { return nativeBpm_; }
private:
  std::filesystem::path path_;
  double nativeBpm_ = 0.0;
#if defined(IW_HAS_AUDIO)
  static void callback(void* userdata, Uint8* stream, int len);
  void render(float* out,int frames);
  void analyze(const float* out,int validSamples);   // level + bass/mid/treble from the rendered block
  openmpt_module* module_{};
  // Plain PCM (WAV) tracks: libopenmpt only reads tracker modules, so a .wav is decoded by SDL,
  // converted once to float stereo at the device rate and looped from memory.
  std::vector<float> pcm_;
  size_t pcmPos_{};   // in frames
  // Frames handed to the device since open(). The music clock is played time, not the position in
  // the track: it keeps counting when a track loops, so the show does not restart with every loop.
  uint64_t playedFrames_{};
  SDL_AudioDeviceID device_{};
  int rate_{48000};
  std::atomic<double> seconds_{};
  std::atomic<float> level_{};
  std::atomic<float> bass_{},mid_{},treble_{};
  std::atomic<int> order_{},pattern_{},row_{},tempo_{125},speed_{6};
  // Persistent one-pole IIR state for the band-split (the audio callback is
  // single-threaded, so these need no sync). Kept across render() calls so the
  // filters aren't re-warmed on every 512-sample chunk.
  float lpB_{},lpM_{},lpT_{};
#endif
};
