#pragma once
#include "Optimize.hpp"
namespace infinity_matrix {
// Persistent traffic is sampled in intrinsic surface coordinates, so its
// collisions and turns remain attached to the geodesic grid.
inline std::string expand(std::string source) {
  if (source.find("vec3 faceFX(") == std::string::npos) return source;
  source.insert(source.find('\n') + 1, "uniform sampler2D uAntField;\n");
  infinity_opt::replace(source, "  // Glyph rain falls down the X = const geodesics.", R"GLSL(
  float seed=h21(vec2(lane+31.0,addr));
  float ants=0.0,contact=0.0;
  if(fp<0.012){
    vec2 offset=floor(vec2(h21(vec2(addr,7)),h21(vec2(addr,19)))*8.0);
    vec2 antUv=(q/G+offset)/8.0;
    vec3 traffic=textureLod(uAntField,antUv,0.0).rgb;
    float detail=1.0-smoothstep(0.003,0.012,fp);
    ants=traffic.r*detail;
    contact=traffic.g*detail;
    seed=traffic.b/max(traffic.r,1e-5);
  }
  float beatAccent=exp(-fract(uFlow/0.55)*9.0);
  // Glyph rain falls down the X = const geodesics.
)GLSL");
  infinity_opt::replace(
      source, "return TRON * grid * 0.8 + TRON * cyc * 2.6 + code * rain * 1.3 + vec3(0.85, 1.0, 0.9) * lead * 1.8;",
      "return "
      "TRON*grid*0.68+TRON*cyc*(2.3+0.35*beatAccent)+code*rain*1.3+vec3(0.85,1.0,0.9)*lead*1.8+mix(CODE,TRON,seed)*"
      "ants*(1.4+0.8*beatAccent)+vec3(1.0,0.28,0.055)*contact*0.9;");
  return source;
}
} // namespace infinity_matrix
