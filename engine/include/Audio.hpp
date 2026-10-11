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
  MusicState state() const;
  int trackCount() const { return int(tracks_.size()); }
  const std::filesystem::path& trackPath(int i) const;
  // A track's own tempo in BPM, folded into 70..180 (tempo * 6 / speed assumes 4 rows per beat; fast
  // modules use 8, so 276 becomes 138). 0 for WAV tracks: the caller keeps its --bpm/default.
  double nativeBpm(int i) const;
private:
  struct Track {
    std::filesystem::path path;
    double bpm = 0.0;
#if defined(IW_HAS_AUDIO)
    openmpt_module* mod = nullptr;
    // WAV: libopenmpt only reads tracker modules, so SDL decodes it, converted once to float stereo
    // at the device rate.
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
