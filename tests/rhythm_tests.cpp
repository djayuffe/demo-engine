#include "AudioRhythm.hpp"
#include "BeatResponse.hpp"
#include "Timeline.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 try {
  for(int rate:{22050,44100,48000})for(double bpm:{90.,120.,143.}) {
    const int channels=rate==44100?2:1;
    std::vector<float> audio(size_t(rate*10*channels));
    for(int i=0;i<rate*10;++i) {
      const double time=double(i)/rate-.073;
      const double age=time>=0?std::fmod(time,60/bpm):10;
      const float signal=float(.5*std::exp(-age*45)*std::sin(6.283185307*90*time));
      for(int channel=0;channel<channels;++channel)audio[size_t(i*channels+channel)]=signal*(channel?-1.f:1.f);
    }
    const auto tempo=audio_rhythm::estimate(audio,channels,rate);
    std::cout<<"tempo "<<bpm<<" @ "<<rate<<": "<<tempo.bpm<<" confidence "<<tempo.confidence<<'\n';
    require(std::abs(tempo.bpm-bpm)<=1 && tempo.confidence>.35f,"tempo analysis misses periodic percussion");
  }
  std::vector<float> steady(22050*5);
  for(size_t i=0;i<steady.size();++i)steady[i]=.1f*std::sin(float(i)*6.2831853f*100/22050);
  require(audio_rhythm::estimate(steady,1,22050).bpm==0,"steady tone invents a tempo");
  std::fill(steady.begin(),steady.end(),0);
  require(audio_rhythm::estimate(steady,1,22050).bpm==0,"silence invents a tempo");
  std::fill(steady.begin(),steady.end(),std::numeric_limits<float>::quiet_NaN());
  require(audio_rhythm::estimate(steady,1,22050).bpm==0,"nonfinite samples invent a tempo");
  for(double step:{512./48000,512./44100,.005}) {
    audio_rhythm::Onset detector;float previous=0;int attacks=0;
    for(double time=0;time<1.99;time+=step) {
      const double age=std::fmod(time,.5);
      const float level=age<.075?.25f:0.f;
      const float envelope=detector.process(level,level*.8f,level*.25f,0,step);
      if(envelope>previous+.05f)++attacks;
      previous=envelope;
    }
    require(attacks==4,"onset count depends on device buffer duration");
    detector.reset();
    for(int i=0;i<200;++i)previous=detector.process(.2f,.1f,.05f,.02f,.01);
    require(previous<1e-5f,"sustained sound holds onset envelope open");
  }
  rhythm::Settings settings;
  const auto between=rhythm::sample(.5,120,true,.7f,settings);
  const auto live=rhythm::withAudio(between,true,1,1,settings);
  require(live.pulse>.9f && live.flash>.6f,"off-grid attack does not reach visual pulse");
  settings.source=rhythm::Source::Audio;
  const auto silent=rhythm::withAudio(rhythm::sample(0,120,true,1,settings),true,0,1,settings);
  require(silent.pulse==0 && silent.flash==0,"audio-only source invents clock flashes");
  settings.source=rhythm::Source::Clock;
  require(rhythm::withAudio(between,true,1,1,settings).pulse==between.pulse,"clock override accepts live onset");
  settings.source=rhythm::Source::Audio;
  require(rhythm::withAudio(between,false,0,0,settings).pulse==between.pulse,"silent playback loses deterministic fallback");
  Timeline clock(120);const auto before=clock.sample(1.234);clock.retempo(1.234,143);
  require(std::abs(clock.sample(1.234).beat-before.beat)<1e-9,"tempo update jumps animation clock");
  std::cout<<"Rhythm DSP and integration tests passed\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
