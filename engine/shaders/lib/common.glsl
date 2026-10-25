// Shared GLSL helpers (included by art.frag and post.frag via `#include "lib/common.glsl"`).
// Pure functions only: no uniforms, so any shader may include it.
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float hash21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
float fbm(vec2 p){float v=0.,a=.5;for(int i=0;i<4;i++){v+=a*noise(p);p=mat2(.8,-.6,.6,.8)*p*2.03+vec2(3.1,1.7);a*=.5;}return v;}
vec3 pal(float t){return .48+.52*cos(6.28318*(vec3(.02,.28,.56)+t));}
// ACES filmic approximation (Narkowicz 2015).
vec3 aces(vec3 x){
  // Negative grain and extreme half-float energy must not turn black into light or overflow.
  x=(any(isnan(x))||any(isinf(x)))?vec3(0.):clamp(x,0.,65504.);
  const float a=2.51,b=.03,c=2.43,d=.59,e=.14;
  return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.,1.);
}
vec2 rot2(vec2 p,float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c)*p;}
