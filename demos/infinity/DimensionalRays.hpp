#pragma once
#include "Optimize.hpp"

namespace infinity_dimension {
// Lift each integrated path to R4, rotate in XW and YZ, then project to R3
// and the camera. This is a visual embedding, not a new optical solver.
inline std::string vertex(std::string source) {
  infinity_opt::replace(source, "out float vLambda;", R"GLSL(out float vLambda;
out float vDepth;
out float vSheet;
uniform float uFlow;
uniform mat3 uCam;
vec3 projectRay(vec2 r,float along,float ray,float sheet){
  float phi=ray*6.2831853+sheet*1.8;
  vec4 q=vec4((r.x/uAspect-0.5)*2.0*uAspect,(r.y-0.5)*2.0,
    0.32*sin(along*6.2831853+phi)+(sheet-1.0)*0.38,
    0.30*cos(along*6.2831853-phi)+(sheet-1.0)*0.24);
  float a=uFlow*0.11+sheet*0.34,b=uFlow*0.07+0.25;
  q.xw=mat2(cos(a),-sin(a),sin(a),cos(a))*q.xw;
  q.yz=mat2(cos(b),-sin(b),sin(b),cos(b))*q.yz;
  vec3 p=uCam*(q.xyz*(2.8/max(1.2,3.0-q.w)));
  float depth=3.2/max(1.6,3.8-p.z);
  return vec3(vec2(0.5)+p.xy*depth/vec2(2.0*uAspect,2.0),depth);
})GLSL");
  infinity_opt::replace(source, "int row = gl_InstanceID;", R"GLSL(int row=gl_InstanceID%(uRays*uNW);
  float sheet=float(gl_InstanceID/(uRays*uNW));)GLSL");
  infinity_opt::replace(source, "vec2 p = toPx(s.xy);\n  vec2 t = normalize(vec2(s.z / uAspect, s.w) * uRes);",
                        R"GLSL(float ray=float(row%uRays)/float(uRays-1);
  vec3 projected=projectRay(s.xy,float(i)/511.0,ray,sheet);
  int j=i<511?i+1:i-1;
  vec2 next=texelFetch(uState,ivec2(j,row),0).xy;
  vec2 delta=(projectRay(next,float(j)/511.0,ray,sheet).xy-projected.xy)*uRes;
  if(i==511)delta=-delta;
  vec2 t=length(delta)>1e-5?normalize(delta):vec2(1.0,0.0);
  vec2 p=projected.xy*uRes;
  vDepth=clamp(projected.z,0.4,1.2);
  vSheet=sheet;)GLSL");
  infinity_opt::replace(source, "side * uWidth;", "side * uWidth * clamp(vDepth,0.55,1.0);");
  return source;
}
inline std::string fragment(std::string source) {
  infinity_opt::replace(
      source, "in float vLambda;",
      "in float vLambda;\nin float vDepth;\nin float vSheet;\nuniform float uBeat;\nuniform float uPulse;");
  infinity_opt::replace(source, "float phase = fract(uTime * 0.085 + vRay * 0.37);",
                        "float phase = fract(uBeat * 0.25 + vSheet * 0.25);");
  infinity_opt::replace(source, "float dt = vT - phase;", "float dt = mod(vT-phase+0.5,1.0)-0.5;");
  infinity_opt::replace(source, "float a = prof * uIntensity * fade * (0.22 + pulse);",
                        R"GLSL(c=mix(c,c.brg,0.12*vSheet);
  float a=prof*uIntensity*fade*(0.10+min(pulse,4.5))
    *vDepth/(1.0+vSheet*0.6)*(0.80+0.65*uPulse);)GLSL");
  return source;
}
} // namespace infinity_dimension
