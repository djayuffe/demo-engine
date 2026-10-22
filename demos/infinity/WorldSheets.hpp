#pragma once
#include <array>
#include <cmath>
#include <string>
#include "Optimize.hpp"
namespace infinity_worlds {
// A regular {7,3} polygon's side circles are orthogonal to the unit disk.
inline std::array<float, 2> heptagon() {
  const double ch = std::cos(3.141592653589793 / 3) / std::sin(3.141592653589793 / 7);
  const double sh = std::sqrt(ch * ch - 1);
  return {float(ch / sh), float(1 / sh)};
}
inline constexpr const char* fragment = R"GLSL(#version 410 core
in vec2 vUv;
out vec4 outColor;
uniform vec2 uRes,uPolygon;
uniform mat3 uCam;
uniform vec3 uEye;
uniform float uBeat,uPulse,uBass;
uniform int uMode;
const float PI=3.14159265359;
vec3 palette(float t){return 0.5+0.5*cos(6.2831853*(t+vec3(0.0,0.33,0.67)));}
mat2 rotation(float a){return mat2(cos(a),-sin(a),sin(a),cos(a));}
float stroke(float distance,float width,float footprint){
  return 1.0-smoothstep(width,width+max(footprint,0.0005),abs(distance));
}
vec2 complexProduct(vec2 a,vec2 b){return vec2(a.x*b.x-a.y*b.y,a.x*b.y+a.y*b.x);}
vec2 mobius(vec2 z,vec2 a){
  vec2 numerator=z+a,denominator=vec2(1,0)+complexProduct(vec2(a.x,-a.y),z);
  return complexProduct(numerator,vec2(denominator.x,-denominator.y))/max(dot(denominator,denominator),1e-6);
}
vec3 poincare(vec3 ro,vec3 rd){
  if(abs(rd.z)<1e-6)return vec3(0);
  float t=-ro.z/rd.z;if(t<=0.0)return vec3(0);
  vec2 disk=(ro+rd*t).xy/1.48;
  float r2=dot(disk,disk);if(r2>=0.996)return vec3(0);
  float phase=uBeat*(2.0*PI/32.0);
  vec2 z=mobius(disk,0.23*vec2(cos(phase*0.5),sin(phase*0.5)));
  float address=0.0,depth=0.0;
  for(int i=0;i<36;++i){
    float sector=floor(atan(z.y,z.x)*7.0/(2.0*PI)+0.5);
    z=rotation(sector*(2.0*PI/7.0))*z;
    vec2 d=z-vec2(uPolygon.x,0.0);
    float d2=dot(d,d);
    if(d2>=uPolygon.y*uPolygon.y)break;
    z=vec2(uPolygon.x,0.0)+d*(uPolygon.y*uPolygon.y/max(d2,1e-8));
    address=fract(address*0.6180339+sector*0.137+0.31);depth+=1.0;
  }
  vec2 edgeVector=z-vec2(uPolygon.x,0.0);
  float signedDistance=(dot(edgeVector,edgeVector)-uPolygon.y*uPolygon.y)
    /(uPolygon.y*max(1.0-dot(z,z),1e-5));
  float edge=abs(asinh(signedDistance));
  float fp=max(fwidth(edge),0.001);
  float line=stroke(edge,0.012,fp);
  float halo=exp(-edge*22.0)*0.13;
  float fine=1.0-smoothstep(0.02,0.12,fp);
  float angle=atan(z.y,z.x),radius=length(z);
  float flower=stroke(sin(angle*7.0+phase)*sin(radius*40.0-phase),0.05,fp*8.0)*fine;
  vec3 tint=mix(vec3(0.06,0.7,1.0),palette(address+phase/(8.0*PI)),0.6);
  float ring=stroke(length(disk)-0.985,0.002,2.0/uRes.y);
  float vignette=1.0-smoothstep(0.96,0.998,sqrt(r2));
  return ((tint*(line*1.5+halo)+palette(address+0.2)*flower*0.18)
    *(0.8+0.35*uPulse)*vignette+vec3(0.09,0.25,0.45)*ring);
}
float flowerField(vec3 p){
  float r=max(length(p),1e-5),latitude=acos(clamp(p.z/r,-1.0,1.0));
  float theta=atan(p.y,p.x),phase=uBeat*(2.0*PI/32.0);
  float petal=sin(latitude);petal*=petal;
  float radius=1.04+(0.16+0.025*uBass)*petal*cos(theta*7.0+phase)*cos(latitude*4.0-phase)
    +0.055*sin(latitude*12.0+phase*2.0);
  // Conservative distance estimate for the radial harmonic surface.
  return (r-radius)*0.34;
}
float crystalField(vec3 p){
  vec3 q=p/0.72;float scale=0.72;
  for(int i=0;i<7;++i){
    // Tetrahedral symmetry folds, followed by a contractive inverse map.
    if(q.x+q.y<0.0)q.xy=-q.yx;
    if(q.x+q.z<0.0)q.xz=-q.zx;
    if(q.y+q.z<0.0)q.yz=-q.zy;
    q=q*2.0-vec3(1.0);scale*=0.5;
  }
  return (length(q)-1.0)*scale*0.65;
}
float field(vec3 p){return uMode==0?flowerField(p):crystalField(p);}
vec3 solid(vec3 ro,vec3 rd){
  float b=dot(ro,rd),c=dot(ro,ro)-2.25,disc=b*b-c;
  if(disc<0.0)return vec3(0);
  float root=sqrt(disc),t=max(0.0,-b-root),end=-b+root;
  bool hit=false;float nearest=1.0;
  for(int i=0;i<112;++i){
    float d=field(ro+rd*t);nearest=min(nearest,abs(d));
    if(d<max(0.0007,t/uRes.y*0.6)){hit=true;break;}
    t+=max(d,0.0004);if(t>end)break;
  }
  if(!hit)return vec3(0.02,0.06,0.1)*exp(-nearest*70.0)*0.06;
  vec3 p=ro+rd*t;
  float e=max(0.0006,t/uRes.y*0.4);
  vec3 gradient=vec3(field(p+vec3(e,0,0))-field(p-vec3(e,0,0)),
    field(p+vec3(0,e,0))-field(p-vec3(0,e,0)),field(p+vec3(0,0,e))-field(p-vec3(0,0,e)));
  vec3 n=dot(gradient,gradient)>1e-12?normalize(gradient):-rd;
  vec3 light=normalize(vec3(-0.4,0.75,0.6)),halfVector=light-rd;
  halfVector*=inversesqrt(max(dot(halfVector,halfVector),1e-8));
  float diffuse=max(dot(n,light),0.0),spec=pow(max(dot(n,halfVector),0.0),54.0);
  float phase=uBeat*(2.0*PI/32.0);
  float theta=atan(p.y,p.x),latitude=acos(clamp(p.z/max(length(p),1e-6),-1.0,1.0));
  float lines=uMode==0?sin(theta*14.0+latitude*8.0-phase):sin((p.x+p.y-p.z)*25.0+phase);
  float detail=stroke(lines,0.055,min(0.5,t/uRes.y*22.0));
  vec3 tint=uMode==0?palette(latitude/PI+phase/(8.0*PI)):palette(dot(p,vec3(.22,.31,.17))+phase/(8.0*PI));
  float rim=pow(1.0-max(dot(n,-rd),0.0),3.0);
  return tint*(0.055+diffuse*0.65)+vec3(1.0,0.85,0.6)*spec*2.0
    +tint*detail*(0.55+0.65*uPulse)+vec3(0.1,0.35,0.65)*rim*0.4;
}
// Exact ray intersections with finite hyperbolic-paraboloid patches. Their
// Gaussian curvature is negative but not constant; these are not H2 planes.
vec3 saddles(vec3 ro,vec3 rd){
  vec3 color=vec3(0);float transmission=1.0;
  float distances[10];vec3 shades[10];
  float phase=uBeat*(2.0*PI/32.0);
  for(int sheet=0;sheet<5;++sheet){
    float angle=float(sheet)*PI/5.0+phase*0.25;
    mat2 turn=rotation(angle);
    vec3 o=ro,d=rd;o.xy=turn*o.xy;d.xy=turn*d.xy;
    o.z-=float(sheet-2)*0.25;
    float k=0.43+0.04*sin(phase+float(sheet));
    float a=-k*(d.x*d.x-d.y*d.y),b=d.z-2.0*k*(o.x*d.x-o.y*d.y),c=o.z-k*(o.x*o.x-o.y*o.y);
    float candidates[2];candidates[0]=1e9;candidates[1]=1e9;
    if(abs(a)<1e-7){if(abs(b)>1e-7)candidates[0]=-c/b;}
    else if(b*b-4.0*a*c>=0.0){
      float root=sqrt(b*b-4.0*a*c),q=-0.5*(b+(b>=0.0?root:-root));
      candidates[0]=q/a;candidates[1]=abs(q)>1e-8?c/q:1e9;
    }
    for(int j=0;j<2;++j){
      int index=sheet*2+j;float t=candidates[j];vec3 p=o+d*t;
      bool duplicate=j==1&&abs(candidates[1]-candidates[0])<1e-4;
      bool valid=t>0.0&&t<1e8&&length(p.xy)<1.3&&!duplicate;
      distances[index]=valid?t:1e9;shades[index]=vec3(0);
      if(valid){
        vec3 n=normalize(vec3(-2.0*k*p.x,2.0*k*p.y,1.0));
        float fp=max(t/uRes.y*2.0,0.002);
        float grid=max(stroke(sin(p.x*20.0),0.035,fp*12.0),stroke(sin(p.y*20.0),0.035,fp*12.0));
        float rings=stroke(sin(length(p.xy)*28.0-phase*2.0),0.05,fp*16.0);
        float rim=pow(1.0-abs(dot(n,-d)),2.0);
        shades[index]=palette(float(sheet)*.17+phase/(16.0*PI))*(grid*1.45+rings*.75+rim*.2+0.035);
      }
    }
  }
  // Sort complete surface hits front-to-back before translucent composition.
  for(int pass=0;pass<10;++pass){int front=-1;float t=1e8;
    for(int j=0;j<10;++j)if(distances[j]<t){front=j;t=distances[j];}
    if(front<0)break;
    float alpha=0.38;
    color+=transmission*shades[front]*(0.9+0.35*uPulse)*alpha;
    transmission*=1.0-alpha;distances[front]=1e9;
  }
  return color;
}
void main(){
  vec2 uv=(vUv*2.0-1.0)*vec2(uRes.x/uRes.y,1.0);
  vec3 ro=uCam*vec3(uEye.xy,3.8+uEye.z);
  vec3 rd=normalize(uCam*vec3(uv*0.42,-1.0));
  vec3 color=uMode==1?poincare(ro,rd):(uMode==2?saddles(ro,rd):solid(ro,rd));
  outColor=vec4(max(color,vec3(0)),1.0);
}
)GLSL";
inline std::string specialized(int mode) {
  if (mode < 0 || mode > 3) throw std::runtime_error("Invalid world-sheet mode");
  std::string source = fragment;
  infinity_opt::replace(source, "uniform int uMode;", "const int uMode = " + std::to_string(mode) + ";");
  return source;
}
} // namespace infinity_worlds
