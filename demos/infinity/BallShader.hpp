#pragma once
#include "Optimize.hpp"
namespace infinity_ball {
inline std::string shader(std::string source) {
  if (source.find("vec3 traceHyper(") == std::string::npos) return source;
  const auto pos = source.find("vec3 traceHyper(");
  source.insert(pos, R"GLSL(
uniform vec4 uBallE;
uniform vec4 uBallH;
uniform float uBallImpact;
vec3 ballLight(vec3 n,vec3 view){
  vec3 light=normalize(vec3(-0.4,0.7,0.5));
  float diffuse=max(dot(n,light),0.0);
  float rim=pow(1.0-clamp(abs(dot(n,view)),0.0,1.0),3.0);
  float glint=pow(max(dot(reflect(-light,n),-view),0.0),48.0);
  return vec3(0.04,0.7,1.0)*(0.14+diffuse*1.4)+vec3(0.5,0.9,1.0)*rim*.65
    +vec3(1.0,0.7,0.25)*(glint*3.0+uBallImpact*.65);
}
float ballFlat(vec3 p,vec3 v){
  vec3 d=p-uBallE.xyz;float b=dot(d,v),c=dot(d,d)-uBallE.w*uBallE.w;
  float disc=b*b-c;if(disc<0.0)return 1e9;
  float t=-b-sqrt(disc);return t>1e-4?t:(-b+sqrt(disc)>1e-4?-b+sqrt(disc):1e9);
}
float ballHyper(vec4 p,vec4 v){
  float a=-mdot(p,uBallH),b=-mdot(v,uBallH),r=cosh(0.14);
  if(a>r&&b>=0.0)return 1e9;
  float closest=atanh(clamp(-b/max(a,1e-6),-0.999999,0.999999));
  float minimum=sqrt(max(a*a-b*b,1.0));
  if(minimum>r)return 1e9;
  float delta=acosh(max(r/minimum,1.0));
  float t=closest-delta;return t>1e-4?t:(closest+delta>1e-4?closest+delta:1e9);
}
)GLSL");
  // Each ray segment tests the sphere before reaching its next mirror.
  infinity_opt::replace(source, "    if (bi < 0) break;\n    float ch = cosh(best), sh = sinh(best);", R"GLSL(
    float ballT=ballHyper(p,v);
    if(ballT<best){
      vec4 hp=cosh(ballT)*p+sinh(ballT)*v;
      vec4 normalH=(cosh(0.14)*hp-uBallH)/sinh(0.14);
      col+=att*ballLight(normalize(normalH.xyz),normalize(v.xyz));break;
    }
    if (bi < 0) break;
    float ch = cosh(best), sh = sinh(best);)GLSL");
  infinity_opt::replace(source, "    if (bi < 0) break;\n    p += v * best;", R"GLSL(
    float ballT=ballFlat(p,v);
    if(ballT<best){col+=att*ballLight(normalize(p+ballT*v-uBallE.xyz),v);break;}
    if (bi < 0) break;
    p += v * best;)GLSL");
  return source;
}
} // namespace infinity_ball
