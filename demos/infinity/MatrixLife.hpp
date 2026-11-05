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
  float colonyFlicker=0.72+0.28*sin(uFlow*(2.1+seed*3.7)+addr*0.017+seed*19.0);
  float antSpark=pow(clamp(ants,0.0,1.0),2.2)*(0.35+0.65*colonyFlicker);
  float dust=h21(floor(q/G*3.0)+floor(uFlow*(0.7+seed))*vec2(7.0,13.0));
  float firefly=step(0.985,dust)*(0.35+0.65*beatAccent)*(1.0-smoothstep(0.002,0.012,fp));
  // Glyph rain falls down the X = const geodesics.
)GLSL");
  infinity_opt::replace(
      source, "return TRON * grid * 0.8 + TRON * cyc * 2.6 + code * rain * 1.3 + vec3(0.85, 1.0, 0.9) * lead * 1.8;",
      "return "
      "TRON*grid*(0.58+0.16*colonyFlicker)+TRON*cyc*(2.2+0.48*beatAccent)+code*rain*1.3+"
      "vec3(0.85,1.0,0.9)*lead*1.8+mix(CODE,TRON,seed)*ants*(1.15+0.72*colonyFlicker+0.8*beatAccent)+"
      "palette(seed+uFlow*0.006)*antSpark*0.75+vec3(1.0,0.20,0.035)*contact*(0.75+0.65*colonyFlicker)+"
      "palette(seed+addr*0.001)*firefly*0.55;");
  infinity_opt::replace(source, "  float cov = lineCov(e, w, fp);", R"GLSL(
  float roomSeed=h21(vec2(addr,total+17.0));
  float wirePulse=0.80+0.20*sin(uFlow*(0.65+roomSeed*0.85)+addr*0.013);
  float liveWidth=w*(0.90+0.13*sin(uFlow*0.42+total*0.75+roomSeed*11.0));
  float cov=lineCov(e,liveWidth,fp);
  float chase=cov*pow(0.5+0.5*sin(total*8.0-uFlow*(2.4+roomSeed)+addr*0.021),18.0)*0.42;
)GLSL");
  infinity_opt::replace(source, "cov * uGlow * flash\n           + 0.11",
                        "cov*uGlow*flash*wirePulse+chase\n           + 0.11");
  return source;
}
} // namespace infinity_matrix
