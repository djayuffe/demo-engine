#pragma once
#include <cstdint>
struct SyncState { double seconds=0, beat=0, bar=0; float beatPhase=0, barPhase=0, pulse=0; uint64_t beatIndex=0, barIndex=0; };
class Timeline {
public:
 explicit Timeline(double bpm=132.0):bpm_(bpm){}
 SyncState sample(double seconds) const;
 void setBpm(double b){bpm_=b>1?b:1;}
 // Change tempo at `seconds` without a jump: the beat count carries on from where it is, so a new
 // track at a different BPM does not skip scenes or restart the bar.
 void retempo(double seconds,double bpm);
 double bpm()const{return bpm_;}
private: double bpm_; double originSeconds_=0, originBeat_=0;
};
