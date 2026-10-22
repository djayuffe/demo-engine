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
uniform float uBeat;
uniform float uPulse;
uniform float uBass;
uniform mat3 uCam;
uniform vec3 uEye;
uniform vec4 uTurn[3];
vec3 projectRay(vec2 r,float along,float ray,float sheet){
  float phi=ray*6.2831853+sheet*1.8;
  vec4 q=vec4((r.x/uAspect-0.5)*2.0*uAspect,(r.y-0.5)*2.0,
    0.32*sin(along*6.2831853+phi)+(sheet-1.0)*0.38,
    0.30*cos(along*6.2831853-phi)+(sheet-1.0)*0.24);
  // Traveling muscular waves: bounded and anchored at the path endpoints.
  float body=sin(3.14159265*along); body*=body;
  float wave=along*9.42477796-uBeat*1.57079633+sheet*1.8;
  float coil=body*(0.065+0.045*uBass+0.035*uPulse);
  q.xy+=coil*vec2(sin(wave),cos(wave*0.75+phi*0.18));
  q.z+=body*(0.10*sin(wave+phi*0.25)+0.055*sin(wave*2.0));
  q.w+=coil*cos(wave*0.5);
  vec4 turn=uTurn[int(sheet)];
  q.xw=mat2(turn.x,-turn.y,turn.y,turn.x)*q.xw;
  q.yz=mat2(turn.z,-turn.w,turn.w,turn.z)*q.yz;
  vec3 p=transpose(uCam)*(q.xyz*(2.8/max(1.2,3.0-q.w)));
  p-=uEye;
  float depth=3.2/max(1.6,3.8-p.z);
  return vec3(vec2(0.5)+p.xy*depth/vec2(2.0*uAspect,2.0),depth);
})GLSL");
  infinity_opt::replace(source, "int row = gl_InstanceID;", R"GLSL(int row=gl_InstanceID%(uRays*uNW);
  float sheet=float(gl_InstanceID/(uRays*uNW));)GLSL");
  infinity_opt::replace(source, "vec2 p = toPx(s.xy);\n  vec2 t = normalize(vec2(s.z / uAspect, s.w) * uRes);",
                        R"GLSL(float ray=float(row%uRays)/float(uRays-1);
  vec3 projected=projectRay(s.xy,float(i)/511.0,ray,sheet);
  int lo=max(i-1,0),hi=min(i+1,511);
  vec2 before=texelFetch(uState,ivec2(lo,row),0).xy;
  vec2 after=texelFetch(uState,ivec2(hi,row),0).xy;
  vec2 delta=(projectRay(after,float(hi)/511.0,ray,sheet).xy
    -projectRay(before,float(lo)/511.0,ray,sheet).xy)*uRes;
  vec2 t=length(delta)>1e-5?normalize(delta):vec2(1.0,0.0);
  vec2 p=projected.xy*uRes;
  vDepth=clamp(projected.z,0.4,1.2);
  vSheet=sheet;)GLSL");
  infinity_opt::replace(source, "side * uWidth;", "side * uWidth * clamp(vDepth,0.55,1.0);");
  // Use real clip W so path position, depth and light packets interpolate
  // perspectively across each segment rather than sliding over screen space.
  infinity_opt::replace(source, "gl_Position = vec4(px / uRes * 2.0 - 1.0, 0.0, 1.0);",
                        R"GLSL(float clipW=3.2/projected.z;
  float clipZ=(20.0+0.1)/(20.0-0.1)*clipW-2.0*20.0*0.1/(20.0-0.1);
  gl_Position=vec4((px/uRes*2.0-1.0)*clipW,clipZ,clipW);)GLSL");
  return source;
}
inline std::string fragment(std::string source) {
  infinity_opt::replace(source, "in float vLambda;",
                        "in float vLambda;\nin float vDepth;\nin float vSheet;\nuniform float uBeat;\nuniform float "
                        "uPulse;\nuniform float uCore;");
  infinity_opt::replace(source, "float prof = exp(-vSide * vSide * uSharp);",
                        R"GLSL(float edge=max(fwidth(vSide),0.025);
  float prof=exp(-vSide*vSide*uSharp)
    *(1.0-smoothstep(1.0-edge,1.0,abs(vSide)));)GLSL");
  infinity_opt::replace(source, "float phase = fract(uTime * 0.085 + vRay * 0.37);",
                        "float phase = fract(uBeat * 0.25 + vSheet * 0.25);");
  infinity_opt::replace(source, "float dt = vT - phase;", "float dt = mod(vT-phase+0.5,1.0)-0.5;");
  infinity_opt::replace(source, "float a = prof * uIntensity * fade * (0.22 + pulse);",
                        R"GLSL(c=mix(c,c.brg,0.12*vSheet);
  // Hot central filaments retain spectral shoulders instead of washing out.
  float hot=exp(-vSide*vSide*12.0)*smoothstep(1.0,4.5,pulse)*uCore;
  c=mix(c,vec3(max(c.r,max(c.g,c.b))),0.18*hot);
  float footprint=fwidth(vT)*94.24778;
  float detail=1.0-smoothstep(0.5,3.14159265,footprint);
  float tendon=0.88+0.12*detail*sin(vT*94.24778-uBeat*3.14159265+vSheet);
  float a=prof*uIntensity*fade*(0.14*tendon+min(pulse,5.0))
    *vDepth/(1.0+vSheet*0.6)*(0.78+0.85*uPulse);)GLSL");
  return source;
}
} // namespace infinity_dimension
