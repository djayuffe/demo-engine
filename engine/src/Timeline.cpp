#include "Timeline.hpp"
#include <algorithm>
#include <cmath>
SyncState Timeline::sample(double s)const{
 SyncState o; o.seconds=std::max(0.0,s); o.beat=std::max(0.0,originBeat_+(o.seconds-originSeconds_)*bpm_/60.0); o.bar=o.beat/4.0;
 o.beatIndex=(uint64_t)std::floor(o.beat); o.barIndex=(uint64_t)std::floor(o.bar);
 o.beatPhase=float(o.beat-std::floor(o.beat)); o.barPhase=float(o.bar-std::floor(o.bar));
 float a=std::exp(-9.f*o.beatPhase), b=.55f*std::exp(-14.f*std::fmod(float(o.beat),.5f));
 o.pulse=std::clamp(a+b,0.f,1.f); return o;
}
void Timeline::retempo(double s,double bpm){
 originBeat_=sample(s).beat; originSeconds_=std::max(0.0,s); setBpm(bpm);
}
