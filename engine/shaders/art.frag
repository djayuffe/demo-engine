#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform vec2 uResolution;
uniform float uTime;
uniform float uMusicLevel;
// Tempo in beats per minute. The synthetic beat pulse below is synced to this
// so the FX pulse at, e.g., --bpm 100 actually pulses at 100 BPM (100/60 Hz)
// instead of a hard-coded 3.2 Hz (192 BPM) that only matches 192 BPM.
uniform float uBpm;
// 1.0 when a logo backdrop is in uScene (logo mode): skip the procedural
// background and reduce the scene gain so the logo isn't washed out.
uniform float uHasLogo;
// Logo card visibility 0..1 (drives the time-driven logo show): the art background and
// screen-space warps fade IN as the logo fades OUT, so the black breaks are not empty.
uniform float uLogoVis;
// Demo-wide strength of the signature look (art background, wild warps, lensing, SDF sculpture,
// holographic stripes): 1 = full, 0 = clean wires only. Folded into hasLogo so every gated term obeys it.
uniform float uFxAmount;
// 1 while a golden-glimmer scene is on: gold-tinted streaks plus vertical streaks (cross glints),
// a quieter art background and a warm grade.
uniform float uGold;
// Background style: 0 = classic generative blend, 1 = synthwave, 2 = nebula, 3 = mandala, 4 = black hole.
uniform int uStyle;
uniform vec2 uPointer;   // -1..1, y up: every style reacts to it
uniform float uPulse;    // click impulse 1 -> 0

#include "lib/common.glsl"
#include "lib/art_layers.glsl"
#include "lib/art_styles.glsl"

// ---- Ambient layers: each returns its additive contribution (before the shared weights in main) ----
float starField(vec2 p){return step(.996,hash(floor((p+2.)*vec2(120.,70.)+floor(uTime*.25))));}
// Finer, slower, more numerous drifting motes.
float dustField(vec2 p){
  vec2 dp=floor(p*vec2(46.,26.)+uTime*vec2(.35,.18));
  float dh=hash(dp);
  vec2 df=fract(p*vec2(46.,26.)+uTime*vec2(.35,.18));
  return step(.86,dh)*smoothstep(.16,.0,length(df-.5))*(.3+.5*uMusicLevel);
}
float gridField(vec2 p){return pow(.5+.5*sin((p.x+p.y*.35)*18.+uTime*.55),18.)*(.018+.06*uMusicLevel);}
float matrixField(vec2 p){
  float mx=pow(.5+.5*sin(p.x*32.+sin(p.y*3.2+uTime*.55)*3.0+uTime*.9),28.);
  float my=pow(.5+.5*sin(p.y*24.+cos(p.x*2.5-uTime*.4)*2.5-uTime*.7),26.);
  return mx+my;
}
float auroraField(vec2 p,float n2){return pow(max(0.,sin(p.y*4.5+n2*5.0+uTime*.72)),3.)*smoothstep(-1.15,.45,p.y);}
float tunnelField(vec2 p){
  float tunnel=0.;
  for(int i=0;i<4;i++){
    float z=fract(float(i)*.25+uTime*.055+uMusicLevel*.035);
    tunnel+=tunnelLayer(p,z,uTime*.12+float(i)*1.7)*(1.0-z)*.25;
  }
  return tunnel;
}
float glimmerField(vec2 p){
  return pow(max(0.,sin((p.x*23.-p.y*17.)+uTime*(2.2+uMusicLevel*3.))),28.)*
         smoothstep(.15,1.6,length(p))*(.03+.13*uMusicLevel);
}

// Procedural background + art layer, split out of post.frag. Rendered into a half-resolution
// RGBA16F target (a quarter of the pixels) that post.frag upsamples with bilinear filtering: the
// content is soft, so the lower resolution does not show, and the pass is skipped entirely while
// a logo card covers the screen. uResolution is the art target's size (only its aspect is used).
void main(){
  vec2 p=(uv*2.-1.)*vec2(uResolution.x/max(uResolution.y,1.),1.);
  float beat=pow(.5+.5*sin(6.2831853*uTime*uBpm/60.0),14.0);
  beat=min(1.,beat+uPulse);
  float mlev=uMusicLevel;
  float hasLogo=max(step(.5,uHasLogo)*clamp(uLogoVis,0.,1.),1.0-clamp(uFxAmount,0.,1.));
  if(uStyle>0){
    // Selectable style: a single self-contained scene, still faded out by the logo / fx amount.
    vec3 sc=styleLayer(uStyle,p,mlev,beat)*(1.0-hasLogo)*(1.0-.5*uGold);
    FragColor=vec4(max(sc,vec3(0.)),1.0);
    return;
  }
  float n=fbm(p*1.7+uTime*.035);
  float n2=fbm(p*3.1+vec2(uTime*.05,-uTime*.025));
  float star=starField(p);
  float dust=dustField(p);
  float neb=pow(max(0.,n),3.)*(.10+.25*uMusicLevel);
  float grid=gridField(p);
  float matrix=matrixField(p);
  float aurora=auroraField(p,n2);
  float tunnel=tunnelField(p);
  float glimmer=glimmerField(p);
  vec3 bg=vec3(.0015,.003,.013);
  bg+=artLayer(p,mlev,beat)*.52*pow(1.0-hasLogo,3.0)*(1.0-.65*uGold);
  bg+=pal(n*.32+uTime*.015)*neb;
  bg+=pal(p.y*.08+uTime*.035+n2*.15)*aurora*(.045+.16*uMusicLevel);
  bg+=vec3(.35,.55,1.2)*(star*.22+grid);
  bg+=vec3(.7,.8,1.0)*dust;
  bg+=pal(uTime*.04+n2*.18+length(p)*.045)*tunnel*(.08+.22*uMusicLevel);
  bg+=pal(uTime*.06+p.x*.025)*matrix*(.018+.075*uMusicLevel);
  bg+=pal(uTime*.05+.3)*glimmer;
  FragColor=vec4(bg,1.0);
}
