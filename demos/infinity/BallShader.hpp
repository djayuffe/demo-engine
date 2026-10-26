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
uniform vec3 uBallVelocityE;
uniform vec4 uBallVelocityH;
uniform vec3 uBallImpactE;
uniform vec4 uBallImpactH;
uniform vec3 uBallImpactNormalE;
uniform vec4 uBallImpactNormalH;
uniform float uBallSpin,uBallBeat;
vec3 ballPalette(float t){
  return 0.52+0.48*cos(6.2831853*(t+vec3(0.02,0.35,0.68)));
}
vec3 ballRotate(vec3 p,vec3 axis,float angle){
  float c=cos(angle),s=sin(angle);
  return p*c+cross(axis,p)*s+axis*dot(axis,p)*(1.0-c);
}
vec3 ballLight(vec3 n,vec3 ray,vec3 light,vec3 local){
  vec3 view=-ray;
  if(dot(n,view)<0.0)n=-n;
  vec3 spun=ballRotate(local,normalize(vec3(0.37,0.81,0.45)),uBallSpin*1.75);
  float longitude=atan(spun.z,spun.x),latitude=asin(clamp(spun.y,-1.0,1.0));
  float ribbons=0.5+0.5*sin(longitude*8.0+latitude*13.0+sin(latitude*7.0)*1.8);
  float cells=pow(0.5+0.5*cos(longitude*15.0)*cos(latitude*11.0),7.0);
  float seam=pow(1.0-abs(2.0*ribbons-1.0),12.0);
  vec3 base=mix(vec3(0.018,0.20,0.62),vec3(0.72,0.035,0.42),ribbons);
  base=mix(base,ballPalette(0.58+longitude*0.035+latitude*0.055),cells*0.34);
  vec3 halfVector=view+light;
  vec3 h=halfVector*inversesqrt(max(dot(halfVector,halfVector),1e-8));
  float nv=max(dot(n,view),0.001),nl=max(dot(n,light),0.0);
  float nh=max(dot(n,h),0.0),vh=max(dot(view,h),0.0);
  float alpha=mix(0.075,0.18,ribbons),alpha2=alpha*alpha,k=alpha*0.5;
  float den=nh*nh*(alpha2-1.0)+1.0;
  float distribution=alpha2/(3.14159265*den*den);
  float visibility=(nv/(nv*(1.0-k)+k))*(nl/(nl*(1.0-k)+k));
  vec3 fresnel=mix(vec3(0.05),base*0.72,0.58)+vec3(0.34)*pow(1.0-vh,5.0);
  vec3 diffuse=base*(vec3(1.0)-fresnel)/3.14159265;
  vec3 specular=distribution*visibility*fresnel/max(4.0*nv*max(nl,0.001),1e-5);
  float clearcoat=pow(max(dot(reflect(-light,n),view),0.0),96.0);
  float rim=pow(1.0-nv,4.0);
  vec3 subsurface=mix(vec3(0.03,0.16,0.42),vec3(0.55,0.035,0.24),ribbons);
  return (diffuse+specular)*nl*4.2+base*0.07+subsurface*rim*(0.65+0.35*uBallBeat)
    +ballPalette(0.72+latitude*0.08)*seam*(0.10+0.16*uBallBeat)
    +vec3(0.72,0.90,1.20)*clearcoat*0.42+vec3(1.3,0.25,0.035)*uBallImpact*0.34;
}
vec3 ballAura(vec3 n,vec3 ray,float strength,float phase){
  float rim=pow(1.0-abs(dot(n,-ray)),2.5);
  return ballPalette(phase)*rim*strength*(0.55+0.45*uBallBeat);
}
// Inverse boost to the hit point's orthonormal rest frame. All input
// vectors must be tangent at the hit point; this preserves Lorentz angles.
vec3 ballLocal(vec4 tangent,vec4 hit){
  return normalize(tangent.xyz-hit.xyz*(tangent.w/(hit.w+1.0)));
}
float ballFlatAt(vec3 p,vec3 v,vec3 center,float radius){
  vec3 d=p-center;float b=dot(d,v),c=dot(d,d)-radius*radius;
  float disc=b*b-c;if(disc<0.0)return 1e9;
  float root=sqrt(disc),q=-b-(b>=0.0?root:-root);
  float t0=q,t1=abs(q)>1e-8?c/q:1e9;
  float nearT=min(t0,t1),farT=max(t0,t1);
  return nearT>1e-4?nearT:(farT>1e-4?farT:1e9);
}
float ballFlat(vec3 p,vec3 v){return ballFlatAt(p,v,uBallE.xyz,uBallE.w);}
float ballHyperAt(vec4 p,vec4 v,vec4 center,float radius){
  float a=max(-mdot(p,center),1.0),b=-mdot(v,center),r=cosh(radius);
  if(a>r&&b>=0.0)return 1e9;
  float disc=r*r-(a-b)*(a+b);
  if(disc<0.0)return 1e9;
  float root=sqrt(disc);
  // Conjugate form avoids subtracting nearly equal roots. y=exp(distance).
  float nearY=(a-b)/(r+root);
  float farY=(r+root)/max(a+b,1e-8);
  float y=nearY>1.0001?nearY:farY;
  return y>1.0001?log(y):1e9;
}
float ballHyper(vec4 p,vec4 v){return ballHyperAt(p,v,uBallH,uBallE.w);}
float ballTrailFlat(vec3 p,vec3 v,out vec3 center,out float energy){
  float nearest=1e9;center=uBallE.xyz;energy=0.0;
  for(int i=1;i<=2;i++){
    float age=float(i)*0.055;
    vec3 candidate=uBallE.xyz-uBallVelocityE*age;
    float t=ballFlatAt(p,v,candidate,uBallE.w*(1.0-float(i)*0.17));
    if(t<nearest){nearest=t;center=candidate;energy=(1.0-float(i)*0.22)*(1.0-uBallImpact*0.72);}
  }
  return nearest;
}
float ballTrailHyper(vec4 p,vec4 v,out vec4 center,out float radius,out float energy){
  float nearest=1e9;center=uBallH;radius=uBallE.w;energy=0.0;
  for(int i=1;i<=2;i++){
    float age=float(i)*0.055,ch=cosh(age),sh=sinh(age);
    vec4 candidate=ch*uBallH-sh*uBallVelocityH;
    float candidateRadius=uBallE.w*(1.0-float(i)*0.17);
    float t=ballHyperAt(p,v,candidate,candidateRadius);
    if(t<nearest){nearest=t;center=candidate;radius=candidateRadius;
      energy=(1.0-float(i)*0.22)*(1.0-uBallImpact*0.72);}
  }
  return nearest;
}
)GLSL");
  // Each ray segment tests the sphere before reaching its next mirror.
  infinity_opt::replace(source, "    if (bi < 0) break;\n    float ch = cosh(best), sh = sinh(best);", R"GLSL(
    float ballT=ballHyper(p,v);
    // Secondary transparent layers disappear into the mirror fog long before
    // the solid sphere does. Bound their cost to the four nearest rooms.
    if(b<4){
      float auraT=ballHyperAt(p,v,uBallH,uBallE.w*1.48);
      if(auraT<min(best,ballT)){
        vec4 ah=cosh(auraT)*p+sinh(auraT)*v;
        vec4 an=(cosh(uBallE.w*1.48)*ah-uBallH)/sinh(uBallE.w*1.48);
        col+=att*ballAura(ballLocal(an,ah),ballLocal(sinh(auraT)*p+cosh(auraT)*v,ah),0.12,0.58);
      }
      if(uBallImpact>0.008){
        float waveRadius=uBallE.w+0.035+(1.0-uBallImpact)*0.52;
        float waveT=ballHyperAt(p,v,uBallImpactH,waveRadius);
        if(waveT<min(best,ballT)){
          vec4 wh=cosh(waveT)*p+sinh(waveT)*v;
          vec4 wn=(cosh(waveRadius)*wh-uBallImpactH)/sinh(waveRadius);
          float wallRing=exp(-9.0*pow(mdot(wn,uBallImpactNormalH),2.0));
          col+=att*ballAura(ballLocal(wn,wh),ballLocal(sinh(waveT)*p+cosh(waveT)*v,wh),
                            uBallImpact*uBallImpact*0.55*wallRing,0.03);
        }
      }
      if(uBallImpact<0.92){
        vec4 trailCenterH;float trailRadiusH,trailEnergyH;
        float trailT=ballTrailHyper(p,v,trailCenterH,trailRadiusH,trailEnergyH);
        if(trailT<min(best,ballT)){
          vec4 th=cosh(trailT)*p+sinh(trailT)*v;
          vec4 tn=(cosh(trailRadiusH)*th-trailCenterH)/sinh(trailRadiusH);
          col+=att*ballAura(ballLocal(tn,th),ballLocal(sinh(trailT)*p+cosh(trailT)*v,th),
                            trailEnergyH*0.20,0.72);
        }
      }
    }
    if(ballT<best){
      vec4 hp=cosh(ballT)*p+sinh(ballT)*v;
      vec4 hitRay=sinh(ballT)*p+cosh(ballT)*v;
      vec4 normalH=(cosh(uBallE.w)*hp-uBallH)/sinh(uBallE.w);
      vec4 lightH=vec4(normalize(vec3(-0.4,0.7,0.5)),0.0);
      lightH+=mdot(lightH,hp)*hp;
      vec3 localNormal=ballLocal(normalH,hp);
      col+=att*ballLight(localNormal,ballLocal(hitRay,hp),ballLocal(lightH,hp),localNormal);break;
    }
    if (bi < 0) break;
    float ch = cosh(best), sh = sinh(best);)GLSL");
  infinity_opt::replace(source, "    if (bi < 0) break;\n    p += v * best;", R"GLSL(
    float ballT=ballFlat(p,v);
    if(b<4){
      float auraT=ballFlatAt(p,v,uBallE.xyz,uBallE.w*1.48);
      if(auraT<min(best,ballT))col+=att*ballAura(normalize(p+auraT*v-uBallE.xyz),v,0.12,0.58);
      if(uBallImpact>0.008){
        float waveRadius=uBallE.w+0.035+(1.0-uBallImpact)*0.52;
        float waveT=ballFlatAt(p,v,uBallImpactE,waveRadius);
        if(waveT<min(best,ballT)){
          vec3 waveNormal=normalize(p+waveT*v-uBallImpactE);
          float wallRing=exp(-9.0*pow(dot(waveNormal,uBallImpactNormalE),2.0));
          col+=att*ballAura(waveNormal,v,uBallImpact*uBallImpact*0.55*wallRing,0.03);
        }
      }
      if(uBallImpact<0.92){
        vec3 trailCenterE;float trailEnergyE;
        float trailT=ballTrailFlat(p,v,trailCenterE,trailEnergyE);
        if(trailT<min(best,ballT))col+=att*ballAura(normalize(p+trailT*v-trailCenterE),v,trailEnergyE*0.20,0.72);
      }
    }
    if(ballT<best){vec3 localNormal=normalize(p+ballT*v-uBallE.xyz);
      col+=att*ballLight(localNormal,v,normalize(vec3(-0.4,0.7,0.5)),localNormal);break;}
    if (bi < 0) break;
    p += v * best;)GLSL");
  infinity_opt::replace(source,
                        "    vec4 p = boost(vec4(0.0, 0.0, 0.0, 1.0), uBoost.xyz, uBoost.w);\n    vec4 v = "
                        "boost(vec4(rd, 0.0), uBoost.xyz, uBoost.w);",
                        R"GLSL(
    // Translate the orthonormal camera frame onto the hyperboloid before
    // the scene boost. Camera-origin parallax must affect p as well as rd.
    vec3 cameraK=uRo*uT;
    float k2=dot(cameraK,cameraK);
    vec4 originH=vec4(cameraK,1.0)/sqrt(max(1.0-k2,1e-6));
    vec4 tangentH=vec4(rd+originH.xyz*(dot(originH.xyz,rd)/(originH.w+1.0)),dot(originH.xyz,rd));
    vec4 p=boost(originH,uBoost.xyz,uBoost.w);
    vec4 v=boost(tangentH,uBoost.xyz,uBoost.w);)GLSL");
  return source;
}
} // namespace infinity_ball
