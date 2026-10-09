#pragma once
#include <cstdint>
struct SyncState { double seconds=0, beat=0, bar=0; float beatPhase=0, barPhase=0, pulse=0; uint64_t beatIndex=0, barIndex=0; };
class Timeline {
public:
 explicit Timeline(double bpm=132.0):bpm_(bpm){}
 SyncState sample(double seconds) const;
 void setBpm(double b){bpm_=b>1?b:1;}
 double bpm()const{return bpm_;}
private: double bpm_;
};
