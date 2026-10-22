#pragma once
#include "Optimize.hpp"
namespace infinity_matrix {
// Native surface detail: deterministic procedural walkers, evaluated in the
// same intrinsic coordinates as the original geodesic grid and glyph rain.
inline std::string expand(std::string source) {
  if (source.find("vec3 faceFX(") == std::string::npos) return source;
  infinity_opt::replace(source, "  // Glyph rain falls down the X = const geodesics.", R"GLSL(
  float seed=h21(vec2(lane+31.0,addr));
  float ants=0.0;
  if(fp<0.012){
  float antPos=fract((q.x-dir*uFlow*0.11)/0.48+seed)-0.5;
  float antD=abs(antPos)*0.48;
  float antWidth=max(0.0025,fp);
  float beads=exp(-pow(antD/antWidth,2.0))
    +0.65*exp(-pow((antD-0.008)/antWidth,2.0));
  float antDetail=1.0-smoothstep(0.003,0.012,fp);
  float gait=0.7+0.3*sin(uFlow*22.847947+seed*6.2831853);
  ants=lineCov(gd.y,0.0017,fp)*beads*antDetail*gait;
  }
  float beatAccent=exp(-fract(uFlow/0.55)*9.0);
  // Glyph rain falls down the X = const geodesics.
)GLSL");
  infinity_opt::replace(
      source, "return TRON * grid * 0.8 + TRON * cyc * 2.6 + code * rain * 1.3 + vec3(0.85, 1.0, 0.9) * lead * 1.8;",
      "return "
      "TRON*grid*0.68+TRON*cyc*(2.3+0.35*beatAccent)+code*rain*1.3+vec3(0.85,1.0,0.9)*lead*1.8+mix(CODE,TRON,seed)*"
      "ants*(1.4+0.8*beatAccent);");
  return source;
}
} // namespace infinity_matrix
