#pragma once
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

namespace audio_rhythm {
struct Tempo { double bpm=0; float confidence=0; };
// Load-time PCM analysis only. Positive energy flux rejects sustained tones;
// normalized multi-period correlation prefers recurring attacks over loudness.
inline Tempo estimate(std::span<const float> samples,int channels,int rate) {
  if(channels<1||channels>8||rate<8000||rate>192000)return {};
  const size_t frames=std::min(samples.size()/size_t(channels),size_t(rate)*90);
  if(frames<size_t(rate)*4)return {};
  const int hop=std::max(1,rate/200);
  const double hz=double(rate)/hop;
  std::vector<float> flux;flux.reserve(frames/size_t(hop));
  float previous=0;
  for(size_t frame=0;frame+size_t(hop)<=frames;frame+=size_t(hop)) {
    double energy=0;
    for(int i=0;i<hop;++i)for(int c=0;c<channels;++c) {
      const float s=samples[(frame+size_t(i))*size_t(channels)+size_t(c)];
      if(std::isfinite(s))energy+=double(s)*s;
    }
    const float level=float(std::sqrt(energy/(hop*channels)));
    flux.push_back(std::max(0.f,level-previous));previous=level;
  }
  const float peak=*std::max_element(flux.begin(),flux.end());
  if(peak<1e-5f)return {};
  double norm=0;int attacks=0;
  for(size_t i=0;i<flux.size();++i) {
    flux[i]=flux[i]<peak*.06f?0.f:flux[i];norm+=double(flux[i])*flux[i];
    if(i&&i+1<flux.size()&&flux[i]>peak*.15f&&flux[i]>flux[i-1]&&flux[i]>=flux[i+1])++attacks;
  }
  if(attacks<6||norm<1e-10)return {};
  auto correlation=[&](double lag) {
    const size_t whole=size_t(lag);const double fraction=lag-double(whole);
    double sum=0,left=0,right=0;
    for(size_t i=whole+1;i<flux.size();++i) {
      const double a=flux[i],b=flux[i-whole]*(1-fraction)+flux[i-whole-1]*fraction;
      sum+=a*b;left+=a*a;right+=b*b;
    }
    return left>0&&right>0?sum/std::sqrt(left*right):0.;
  };
  double best=0,bpm=0;
  for(double candidate=70;candidate<=180;candidate+=.5) {
    const double lag=hz*60/candidate;
    const double score=(correlation(lag)+.5*correlation(lag*2)+.25*correlation(lag*3))/1.75;
    const double ranked=score-.035*std::abs(std::log(candidate/120.));
    if(ranked>best){best=ranked;bpm=candidate;}
  }
  // Half/double-time correlation can be almost identical. Prefer the octave
  // closer to a normal 120 BPM pulse only when its periodic evidence agrees.
  const double doubled=bpm*2;
  if(bpm>0 && doubled<=180 && std::abs(std::log(doubled/120))<std::abs(std::log(bpm/120))) {
    const double lag=hz*60/doubled;
    const double score=(correlation(lag)+.5*correlation(lag*2)+.25*correlation(lag*3))/1.75;
    if(score>=best-.08) {bpm=doubled;best=score;}
  }
  const float confidence=float(std::clamp((best-.15)/.65,0.,1.));
  return confidence>=.35f?Tempo{bpm,confidence}:Tempo{};
}

// Audio-thread DSP: constant storage/work, with adaptive novelty threshold,
// a refractory period and a seconds-based release independent of buffer size.
class Onset {
  float previous_=0,mean_=0,variance_=0,envelope_=0;
  double age_=1;
public:
  void reset(){*this=Onset();}
  float process(float rms,float bass,float mid,float treble,double seconds) {
    const double dt=std::isfinite(seconds)?std::clamp(seconds,0.,.25):0.;
    auto clean=[](float v){return std::isfinite(v)?std::clamp(v,0.f,4.f):0.f;};
    const float level=clean(rms)+clean(bass)*.9f+clean(mid)*.35f+clean(treble)*.15f;
    const float flux=std::max(0.f,level-previous_);previous_=level;
    const float threshold=mean_+1.5f*std::sqrt(std::max(0.f,variance_))+.002f;
    envelope_*=float(std::exp(-dt/.07));age_+=dt;
    if(flux>threshold&&level>.006f&&age_>=.10) {
      envelope_=std::clamp((flux-threshold)/std::max(.004f,threshold*2),.15f,1.f);age_=0;
    }
    const float alpha=float(1-std::exp(-dt/.5));
    const float delta=flux-mean_;
    mean_+=alpha*delta;variance_=(1-alpha)*(variance_+alpha*delta*delta);
    return envelope_;
  }
};
} // namespace audio_rhythm
