#pragma once
#include "ShowOptions.hpp"
#include "MusicCatalog.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace musicopt {
struct Options {
  std::vector<std::filesystem::path> files, directories;
  bool explicitSelection=false, shuffle=false, list=false, paused=false;
  uint32_t seed=0;
  float volume=1;
};
inline std::vector<std::string> preset(const std::string& name) {
  return musiccat::playlist(name);
}
inline bool supported(std::filesystem::path path) {
  std::string ext=path.extension().string();
  for(auto& ch:ext) if(ch>='A'&&ch<='Z')ch=char(ch-'A'+'a');
  for(const char* format:{".mod",".xm",".it",".s3m",".mptm",".stm",".mtm",".669",".ult",
                          ".wav",".flac",".ogg",".oga",".mp3",".aif",".aiff",".au"})
    if(ext==format)return true;
  return false;
}
inline showopt::Result parse(int& i,int argc,char** argv,Options& o,std::string& error) {
  const std::string a=argv[i];
  auto next=[&](){if(i+1>=argc){error=a+": missing value";return false;}++i;return true;};
  if(a=="--shuffle"){o.shuffle=true;return showopt::Result::Ok;}
  if(a=="--list-music"){o.list=true;return showopt::Result::Ok;}
  if(a=="--music-paused"){o.paused=true;return showopt::Result::Ok;}
  if(a=="--music"||a=="--music-dir"||a=="--music-preset") {
    if(!next())return showopt::Result::Error;
    if(a=="--music-preset") {
      const auto tracks=preset(argv[i]);
      if(tracks.empty()){error="--music-preset: stars|rendezvous|silicon|pulse|edm|dnb|jazz|ambient|new|all";return showopt::Result::Error;}
      for(const auto& track:tracks)o.files.emplace_back(track);
    } else (a=="--music"?o.files:o.directories).emplace_back(argv[i]);
    o.explicitSelection=true;
    return showopt::Result::Ok;
  }
  if(a=="--volume"||a=="--shuffle-seed") {
    if(!next())return showopt::Result::Error;
    double value=0;
    if(!showopt::number(argv[i],value)){error=a+": expected finite number";return showopt::Result::Error;}
    if(a=="--volume") {
      if(value<0||value>2){error=a+": use 0..2";return showopt::Result::Error;}
      o.volume=float(value);
    } else {
      if(value<0||value>4294967295.0||value!=std::floor(value)){error=a+": use an unsigned 32-bit integer";return showopt::Result::Error;}
      o.seed=uint32_t(value);o.shuffle=true;
    }
    return showopt::Result::Ok;
  }
  return showopt::Result::NotMine;
}
// A fixed Fisher-Yates mapping makes a seed reproducible across standard libraries.
inline void shuffle(std::vector<std::filesystem::path>& files,uint32_t seed) {
  uint32_t state=seed;
  for(size_t i=files.size();i>1;--i) {
    state+=0x9e3779b9u;
    uint32_t h=state;h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;
    std::swap(files[i-1],files[size_t(h)%i]);
  }
}
} // namespace musicopt
