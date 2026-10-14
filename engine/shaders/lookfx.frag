#version 410 core
// Look effects from the experimental-effects spec that work on wires-on-black HDR, run before the post pass.
//  #27 chromatic gravity : each colour channel is pulled up the gradient of its own brightness, so the
//                          channels fall toward different light and the image separates by colour
//  #33 prediction failure: the previous frame predicts this one; the residual (what was not predicted)
//                          displaces the image and burns hot
//  #23 colorless matter  : still wires lose their colour, hue is created only by motion (angle -> hue)
//  #30 specular mutation : highlights grow prismatic spikes whose orientation drifts with time
//  #24 negative reflection: a floor mirror that shows the colour-inverted scene
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uScene;
uniform sampler2D uPrev;
uniform vec2 uTexel;
uniform float uTime;
uniform float uAspect;
uniform float uGrav, uFail, uColorless, uSpec, uNegRefl;
float lum(vec3 c){return dot(c,vec3(.2126,.7152,.0722));}
vec3 hue(float a){return .5+.5*cos(a+vec3(0.,2.094,4.188));}
vec3 S(vec2 p){return texture(uScene,clamp(p,vec2(0.),vec2(1.))).rgb;}
vec3 P(vec2 p){return texture(uPrev,clamp(p,vec2(0.),vec2(1.))).rgb;}
void main(){
  vec2 q=uv;
  vec3 c;
  // residual between prediction and reality (blurred a little so it is a field, not a speckle)
  vec2 r1=uTexel*3.;
  vec3 res=vec3(0.);
  if(uFail>0.||uColorless>0.){
    vec3 now=S(q)+S(q+vec2(r1.x,0))+S(q-vec2(r1.x,0))+S(q+vec2(0,r1.y))+S(q-vec2(0,r1.y));
    vec3 was=P(q)+P(q+vec2(r1.x,0))+P(q-vec2(r1.x,0))+P(q+vec2(0,r1.y))+P(q-vec2(0,r1.y));
    res=(now-was)*.2;
  }
  float mot=clamp(length(res)*1.5,0.,1.);
  if(uFail>0.){
    vec2 dir=vec2(lum(res)*2.,(res.r-res.b)*1.5);
    q+=dir*.04*uFail*vec2(1./uAspect,1.);
  }
  if(uGrav>0.){
    vec3 acc=vec3(0.);
    vec2 off=uTexel*7.;
    vec2 gR=vec2(S(q+vec2(off.x,0)).r-S(q-vec2(off.x,0)).r,S(q+vec2(0,off.y)).r-S(q-vec2(0,off.y)).r);
    vec2 gG=vec2(S(q+vec2(off.x,0)).g-S(q-vec2(off.x,0)).g,S(q+vec2(0,off.y)).g-S(q-vec2(0,off.y)).g);
    vec2 gB=vec2(S(q+vec2(off.x,0)).b-S(q-vec2(off.x,0)).b,S(q+vec2(0,off.y)).b-S(q-vec2(0,off.y)).b);
    float k=uGrav*.045;
    // sampling AT q - k*grad pulls light toward... the bright side: channel colours fall toward their own light
    c=vec3(S(q+gR*k*3.).r,S(q+gG*k*3.*vec2(-1.,1.)).g,S(q-gB*k*3.).b);
  } else c=S(q);
  if(uFail>0.) c+=vec3(1.,.28,.06)*length(res)*uFail*1.4;
  if(uColorless>0.){
    float l=lum(c);
    float ang=atan(res.g-res.b,res.r-.5*(res.g+res.b))*2.+uTime*.3;
    vec3 col=mix(vec3(l),hue(ang)*l*1.6,clamp(mot*3.5,0.,1.));
    c=mix(c,col,uColorless);
  }
  if(uSpec>0.){
    vec3 spikes=vec3(0.);
    for(int a=0;a<3;++a){
      float ang=3.14159*float(a)/3.+uTime*.35;
      vec2 d=vec2(cos(ang)/uAspect,sin(ang));
      for(int i=1;i<=10;++i){
        float fi=float(i),w=exp(-fi*.28);
        vec2 o=d*fi*.012;
        // prismatic dispersion: red reaches shortest, blue longest
        spikes.r+=max(S(q+o*.8).r-1.1,0.)*w+max(S(q-o*.8).r-1.1,0.)*w;
        spikes.g+=max(S(q+o).g-1.1,0.)*w+max(S(q-o).g-1.1,0.)*w;
        spikes.b+=max(S(q+o*1.25).b-1.1,0.)*w+max(S(q-o*1.25).b-1.1,0.)*w;}}
    c+=spikes*.22*uSpec;
  }
  if(uNegRefl>0.){
    // mirror about y=.27: only below the horizon, fading with distance, colours inverted around luminance
    float h=.27;
    if(uv.y<h){
      vec3 m=S(vec2(uv.x,2.*h-uv.y+.0));
      float l=lum(m);
      vec3 neg=max(vec3(l)*2.-m,0.)*(1.-.5)+vec3(l)*.3;
      c+=neg*exp(-(h-uv.y)*5.)*.7*uNegRefl;
    }
  }
  FragColor=vec4(c,1.);
}
