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
vec3 ballLight(vec3 n,vec3 ray,vec3 light){
  vec3 view=-ray;
  if(dot(n,view)<0.0)n=-n;
  vec3 halfVector=view+light;
  vec3 h=halfVector*inversesqrt(max(dot(halfVector,halfVector),1e-8));
  float nv=max(dot(n,view),0.001),nl=max(dot(n,light),0.0);
  float nh=max(dot(n,h),0.0),vh=max(dot(view,h),0.0);
  const float alpha=0.09,alpha2=alpha*alpha,k=0.125;
  float den=nh*nh*(alpha2-1.0)+1.0;
  float distribution=alpha2/(3.14159265*den*den);
  float visibility=(nv/(nv*(1.0-k)+k))*(nl/(nl*(1.0-k)+k));
  vec3 fresnel=vec3(0.04)+vec3(0.96)*pow(1.0-vh,5.0);
  vec3 base=vec3(0.025,0.42,0.72);
  vec3 diffuse=base*(vec3(1.0)-fresnel)/3.14159265;
  vec3 specular=distribution*visibility*fresnel/max(4.0*nv*max(nl,0.001),1e-5);
  float rim=pow(1.0-nv,4.0);
  return (diffuse+specular)*nl*4.0+base*0.08+vec3(0.12,0.3,0.5)*rim
    +vec3(1.0,0.35,0.08)*uBallImpact*0.5;
}
// Inverse boost to the hit point's orthonormal rest frame. All input
// vectors must be tangent at the hit point; this preserves Lorentz angles.
vec3 ballLocal(vec4 tangent,vec4 hit){
  return normalize(tangent.xyz-hit.xyz*(tangent.w/(hit.w+1.0)));
}
float ballFlat(vec3 p,vec3 v){
  vec3 d=p-uBallE.xyz;float b=dot(d,v),c=dot(d,d)-uBallE.w*uBallE.w;
  float disc=b*b-c;if(disc<0.0)return 1e9;
  float root=sqrt(disc),q=-b-(b>=0.0?root:-root);
  float t0=q,t1=abs(q)>1e-8?c/q:1e9;
  float nearT=min(t0,t1),farT=max(t0,t1);
  return nearT>1e-4?nearT:(farT>1e-4?farT:1e9);
}
float ballHyper(vec4 p,vec4 v){
  float a=max(-mdot(p,uBallH),1.0),b=-mdot(v,uBallH),r=cosh(uBallE.w);
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
)GLSL");
  // Each ray segment tests the sphere before reaching its next mirror.
  infinity_opt::replace(source, "    if (bi < 0) break;\n    float ch = cosh(best), sh = sinh(best);", R"GLSL(
    float ballT=ballHyper(p,v);
    if(ballT<best){
      vec4 hp=cosh(ballT)*p+sinh(ballT)*v;
      vec4 hitRay=sinh(ballT)*p+cosh(ballT)*v;
      vec4 normalH=(cosh(uBallE.w)*hp-uBallH)/sinh(uBallE.w);
      vec4 lightH=vec4(normalize(vec3(-0.4,0.7,0.5)),0.0);
      lightH+=mdot(lightH,hp)*hp;
      col+=att*ballLight(ballLocal(normalH,hp),ballLocal(hitRay,hp),ballLocal(lightH,hp));break;
    }
    if (bi < 0) break;
    float ch = cosh(best), sh = sinh(best);)GLSL");
  infinity_opt::replace(source, "    if (bi < 0) break;\n    p += v * best;", R"GLSL(
    float ballT=ballFlat(p,v);
    if(ballT<best){col+=att*ballLight(normalize(p+ballT*v-uBallE.xyz),v,normalize(vec3(-0.4,0.7,0.5)));break;}
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
