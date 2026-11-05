#pragma once
#include <string>
#include <iterator>
#include <string_view>
#include <vector>

namespace musiccat {
struct Track { const char* id; const char* file; const char* title; const char* artist; const char* license; double bpm; bool estimate=true; };
inline constexpr Track tracks[]={
 {"stars","cs127_-_stars_4ch.mod","Stars (4ch.)","cs127","CC BY 4.0",0},
 {"rendezvous","drozerix_-_digital_rendezvous.xm","Digital Rendezvous","Drozerix","Public domain",0},
 {"silicon","drozerix_-_silicon_dancer.mod","Silicon Dancer","Drozerix","Public domain",0},
 {"pulse","wireframe_pulse.wav","Wireframe Pulse","Demo Engine","CC0",128},
 {"edm","fupi_melodic_edm.wav","Melodic EDM","Fupi","CC0",140},
 {"dnb","joth_black_diamond.mp3","Black Diamond","Joth","CC0",143},
 {"jazz","ruskerdax_moil.mp3","Moil","Ruskerdax","CC0",0},
 {"ambient","cynicmusic_synthwave_15k.mp3","Synthwave 15k","The Cynic Project / cynicmusic","CC0",0,false}
};
inline const Track* find(std::string_view file){for(const auto& track:tracks)if(file==track.file)return &track;return nullptr;}
inline std::vector<std::string> playlist(std::string_view id="all") {
  std::vector<std::string> result;
  for(size_t i=0;i<std::size(tracks);++i)
    if(id=="all"||(id=="new"&&i>=4)||id==tracks[i].id)
      result.push_back(std::string("demos/wireframe/assets/music/")+tracks[i].file);
  return result;
}
inline std::string credit(const std::string& path) {
  const auto index=path.find_last_of("/\\");
  const auto* track=find(path.substr(index==std::string::npos?0:index+1));
  return track?std::string(track->title)+" by "+track->artist+" / "+track->license:path;
}
} // namespace musiccat
