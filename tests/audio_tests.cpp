#include "Audio.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
#if defined(IW_HAS_AUDIO)
static void waitFor(const AudioPlayer& audio, int track) {
  for (int i=0;i<200;++i) {
    const auto state=audio.state();
    if(state.active && state.track==track && state.seconds>.03) return;
    SDL_Delay(5);
  }
  throw std::runtime_error("Audio callback did not advance");
}
static void tone(const std::filesystem::path& path, float frequency, int channels, bool percussion=false) {
  std::ofstream out(path,std::ios::binary);
  constexpr int rate=44100;
  const int frames=rate*(percussion?6:1);
  auto word=[&](uint32_t value,int bytes){for(int i=0;i<bytes;++i)out.put(char((value>>(i*8))&255));};
  const uint32_t bytes=frames*channels*2;
  out.write("RIFF",4);word(36+bytes,4);out.write("WAVEfmt ",8);word(16,4);word(1,2);
  word(uint32_t(channels),2);word(rate,4);word(rate*channels*2,4);word(channels*2,2);word(16,2);
  out.write("data",4);word(bytes,4);
  for(int i=0;i<frames;++i) for(int c=0;c<channels;++c) {
    const double time=double(i)/rate-.073;
    const float envelope=percussion?(time<0?0.f:float(std::exp(-std::fmod(time,.5)*45))):1.f;
    const float sample=.35f*envelope*std::sin(6.2831853f*frequency*float(i)/rate)*(c==1?-1.f:1.f);
    word(uint16_t(int16_t(sample*32767)),2);
  }
}
#endif
int main(int argc,char** argv) {
  try {
    require(trackerBpm(276,6)==138,"double-time tempo fold");
    require(trackerBpm(std::numeric_limits<double>::infinity(),6)==0 && trackerBpm(125,0)==0,
            "invalid tracker tempo");
    AudioPlayer audio;
    audio.setVolume(3);require(audio.volume()==2,"volume upper clamp");
    audio.setVolume(-1);require(audio.volume()==0,"volume lower clamp");
    audio.setVolume(std::numeric_limits<float>::quiet_NaN());require(audio.volume()==1,"nonfinite volume");
#if defined(IW_HAS_AUDIO)
    require(SDL_InitSubSystem(SDL_INIT_AUDIO)==0,"SDL dummy device initialization");
    const auto root=std::filesystem::temp_directory_path()/
                    ("demo-audio-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    const auto bass=root/"bass.wav",treble=root/"treble.wav";
    tone(bass,100,2);tone(treble,8000,1);
    audio.setVolume(0);audio.setPaused(true);
    require(audio.open(std::vector<std::filesystem::path>{bass,treble}),"WAV playlist open");
    SDL_Delay(30);require(audio.state().seconds==0,"start-paused music clock advanced");
    audio.setPaused(false);waitFor(audio,0);
    const auto low=audio.state();
    require(low.level>0 && low.bass>low.treble,"stereo bass analysis or muted visual energy");
    audio.setPaused(true);SDL_Delay(30);
    const double stopped=audio.state().seconds;SDL_Delay(30);
    require(audio.state().seconds==stopped,"paused clock drift");
    audio.skip(1);SDL_Delay(40);
    require(audio.state().track==1 && audio.state().seconds==stopped,"track selection while paused");
    audio.setPaused(false);waitFor(audio,1);SDL_Delay(40);
    const auto high=audio.state();
    require(high.treble>high.bass,"mono conversion or treble analysis");
    require(high.nativeBpm==0,"PCM track retained tracker tempo");
    audio.close();
    audio.startAt(1);audio.setPaused(true);
    const auto module=std::filesystem::path(IW_AUDIO_TEST_MODULE);
    require(audio.open(std::vector<std::filesystem::path>{bass,module}),"mixed module playlist open");
    require(audio.nativeBpm(1)>0,"module tempo metadata");
    require(audio.state().track==1 && audio.state().paused && audio.state().seconds==0,
            "start track selection while paused");
    audio.setPaused(false);waitFor(audio,1);SDL_Delay(30);
    require(audio.state().nativeBpm>0,"live tracker tempo");
    audio.startAt(0);waitFor(audio,0);SDL_Delay(20);
    require(audio.state().nativeBpm==0,"module-to-PCM tempo reset");
    audio.close();
    const auto percussion=root/"percussion.wav";
    tone(percussion,90,2,true);
    require(audio.open(percussion),"percussion PCM decode");
    require(std::abs(audio.nativeBpm(0)-120)<1 && std::string(audio.tempoSource(0))=="estimated",
            "PCM tempo integration");
    waitFor(audio,0);
    float onset=0;
    for(int i=0;i<40;++i){onset=std::max(onset,audio.state().onset);SDL_Delay(5);}
    require(onset>.5f && audio.state().tempoConfidence>.35f,"real PCM attacks do not reach shared music state");
    audio.close();
    for(int i=1;i<argc;++i) {
      audio.setPaused(false);require(audio.open(std::filesystem::path(argv[i])),"extra codec failed to decode");
      waitFor(audio,0);
      require(std::isfinite(audio.state().level) && std::isfinite(audio.state().onset),"codec output nonfinite");
      std::cout<<std::filesystem::path(argv[i]).filename().string()<<": "<<audio.nativeBpm(0)
               <<" BPM ("<<audio.tempoSource(0)<<"), onset "<<audio.state().onset<<'\n';
      audio.close();
    }
    std::filesystem::remove_all(root);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
#else
    (void)argc;(void)argv;
    std::cout<<"Audio playback tests skipped: audio dependencies unavailable\n";
#endif
    std::cout<<"Audio controls and playback tests passed\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
