#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uScene;
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
// --no-post: skip the whole FX chain. Just tone-map the captured HDR buffer and
// output it (the 3D + logo are still composited; only the screen-space FX,
// procedural background, and grain are skipped). This is the fast/debug path.
uniform float uBypass;
// Logo card visibility 0..1 (drives the time-driven logo show): the art background and
// screen-space warps fade IN as the logo fades OUT, so the black breaks are not empty.
uniform float uLogoVis;
// Demo-wide strength of the signature look (art background, wild warps, lensing, SDF sculpture,
// holographic stripes): 1 = full, 0 = clean wires only. Folded into hasLogo so every gated term obeys it.
uniform float uFxAmount;
// 1 while a golden-glimmer scene is on: gold-tinted streaks plus vertical streaks (cross glints),
// a quieter art background and a warm grade.
uniform float uGold;

float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float hash21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
float fbm(vec2 p){float v=0.,a=.5;for(int i=0;i<4;i++){v+=a*noise(p);p=mat2(.8,-.6,.6,.8)*p*2.03+vec2(3.1,1.7);a*=.5;}return v;}
vec3 pal(float t){return .48+.52*cos(6.28318*(vec3(.02,.28,.56)+t));}
// ACES filmic approximation (Narkowicz 2015).
vec3 aces(vec3 x){const float a=2.51,b=.03,c=2.43,d=.59,e=.14;return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.,1.);}

float tunnelLayer(vec2 p,float z,float twist){
  vec2 q=p*(1.0+z*.42);
  float a=atan(q.y,q.x)+twist+z*.21;
  float r=length(q);
  float ribs=pow(.5+.5*sin(a*18.0+z*7.0),18.0);
  float rings=pow(.5+.5*sin(r*18.0-z*5.5),22.0);
  return (ribs*.55+rings*.45)*smoothstep(1.95,.18,r);
}


// ---- Arty layers (shown whenever the logo card is not on screen) -----------
vec2 rot2(vec2 p,float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c)*p;}
// Voronoi edge distance: glowing cracked-glass cells.
float voroEdge(vec2 p){
  vec2 g=floor(p),f=fract(p);float d1=8.,d2=8.;
  for(int j=-1;j<=1;j++)for(int i=-1;i<=1;i++){
    vec2 o=vec2(i,j);vec2 h=vec2(hash(g+o),hash(g+o+17.3));
    vec2 r=o+.5+.45*sin(uTime*.35+6.2831*h)-f;float d=dot(r,r);
    if(d<d1){d2=d1;d1=d;}else if(d<d2)d2=d;}
  return sqrt(d2)-sqrt(d1);
}
// Domain-warped ink filaments (ridged fbm of a warped field).
float inkFlow(vec2 p,float t){
  vec2 q=vec2(fbm(p+vec2(0,t*.07)),fbm(p+vec2(5.2,1.3)-t*.05));
  vec2 r=vec2(fbm(p+3.*q+vec2(1.7,9.2)+t*.06),fbm(p+3.*q+vec2(8.3,2.8)-t*.04));
  float v=fbm(p+3.*r);
  return 1.-abs(2.*v-1.);
}

// ---- 3D morphing metaball organism (raymarched, breathing) ------------------
float blobSdf(vec3 q,float t,float br){
  float d=1e3;
  for(int i=0;i<5;i++){
    float fi=float(i);
    vec3 c=vec3(sin(t*.31+fi*1.9)*1.15,cos(t*.27+fi*2.3)*.8,sin(t*.23+fi*1.1)*.9);
    float r=(.42+.12*sin(t*.5+fi*2.))*br;
    float s=length(q-c)-r;
    float k=.55;float h=clamp(.5+.5*(d-s)/k,0.,1.);d=mix(d,s,h)-k*h*(1.-h);   // smooth union: they melt into each other
  }
  // organic surface undulation that breathes with the music
  d+=.05*sin(q.x*5.+t*.9)*sin(q.y*4.7-t*.7)*sin(q.z*5.3+t*.6);
  return d;
}
vec3 blob3D(vec2 p,float ml,float beat){
  float t=uTime;
  float br=1.0+.10*sin(t*.9)+.10*beat+.14*ml;               // breathing
  vec3 ro=vec3(sin(t*.12)*.8,cos(t*.09)*.5,-3.6),rd=normalize(vec3(p*.62,1.5));
  float a=t*.07;rd.xz=mat2(cos(a),-sin(a),sin(a),cos(a))*rd.xz;ro.xz=mat2(cos(a),-sin(a),sin(a),cos(a))*ro.xz;
  float tt=0.,d=0.;vec3 q=ro;float glow=0.;
  for(int i=0;i<26;i++){
    q=ro+rd*tt;d=blobSdf(q,t,br);
    glow+=.018/(.06+abs(d));                                  // volumetric halo
    if(d<.01||tt>7.)break;
    tt+=d*.85;
  }
  vec3 col=vec3(0);
  if(d<.05){
    vec2 e=vec2(.02,0);
    vec3 n=normalize(vec3(blobSdf(q+e.xyy,t,br)-blobSdf(q-e.xyy,t,br),blobSdf(q+e.yxy,t,br)-blobSdf(q-e.yxy,t,br),blobSdf(q+e.yyx,t,br)-blobSdf(q-e.yyx,t,br)));
    vec3 L=normalize(vec3(-.5,.7,-.6));
    float dif=max(dot(n,L),0.),fres=pow(1.-max(dot(n,-rd),0.),3.);
    float spec=pow(max(dot(reflect(-L,n),-rd),0.),24.);
    col=pal(q.y*.25+t*.03+dot(n,vec3(.3))+.2)*(.12+.75*dif)+pal(fres+t*.05)*fres*.95+vec3(1.,.9,.8)*spec*.5;
  }
  col+=pal(t*.04+tt*.1)*glow*.045*(.5+ml);
  return col*smoothstep(2.6,.3,length(p));
}

// Julia-set glow: orbit-trap fractal that slowly morphs its constant.
float juliaTrap(vec2 z,float t){
  vec2 c=.7885*vec2(cos(t*.13),sin(t*.17))*(.92+.08*sin(t*.4));
  float trap=1e3;float it=0.;
  for(int i=0;i<24;i++){
    z=vec2(z.x*z.x-z.y*z.y,2.*z.x*z.y)+c;
    trap=min(trap,abs(length(z)-.9+.25*sin(t+float(i)))*1.0);
    if(dot(z,z)>16.)break;it+=1.;
  }
  return exp(-trap*5.)*(.4+.6*it/24.);
}
// Op-art interference: two drifting ring systems make moire fringes.
float moire(vec2 p,float t){
  vec2 a=p-.55*vec2(cos(t*.21),sin(t*.17)),b=p+.55*vec2(sin(t*.19),cos(t*.23));
  return pow(.5+.5*sin(length(a)*46.+t)*sin(length(b)*46.-t*.8),3.);
}
// Turing-like spots/stripes from three rotated sine fields (cheap reaction-diffusion look).
float turing(vec2 p,float t){
  float v=0.;
  for(int i=0;i<3;i++){float a=float(i)*2.0944+t*.03;v+=cos(dot(p,vec2(cos(a),sin(a)))*9.+sin(t*.2+float(i))*2.);}
  return smoothstep(.4,1.4,v)*smoothstep(3.,1.8,v+.6);
}
vec3 artLayer(vec2 p,float ml,float beat){
  // Breathing, uneven morph: the whole field swells and shears like living tissue.
  float breath=.5+.5*sin(uTime*.55)+.6*beat*.3;
  p+=.07*vec2(sin(p.y*2.3+uTime*.4),cos(p.x*2.0-uTime*.33))*(.6+breath)+.05*ml*vec2(sin(uTime*1.3+p.y*4.),cos(uTime*1.1+p.x*4.));
  p*=1.0+.06*breath;
  vec3 organism=blob3D(p,ml,beat);
  float t=uTime;
  // Three styles cross-fade slowly so the break never looks the same twice.
  float w0=.5+.5*sin(t*.11),w1=.5+.5*sin(t*.11+2.094),w2=.5+.5*sin(t*.11+4.189);
  float ws=w0+w1+w2;w0/=ws;w1/=ws;w2/=ws;
  vec3 c=vec3(0);
  // A: neon ink filaments
  float ink=pow(inkFlow(p*1.15,t),6.0);
  c+=w0*pal(ink*.35+t*.02+p.x*.05)*ink*(.55+.9*ml);
  // B: cracked-glass voronoi with beat-lit seams
  float ve=voroEdge(rot2(p,t*.03)*3.2);
  float seam=exp(-ve*9.0);
  c+=w1*pal(ve*.5+t*.03)*seam*(.35+.9*ml+.8*beat);
  // C: kaleidoscopic rose rings
  vec2 k=p;float a=atan(k.y,k.x),r=length(k);
  float seg=6.2831853/8.;a=abs(mod(a+t*.05,seg)-seg*.5);
  vec2 kp=vec2(cos(a),sin(a))*r;
  float rose=pow(.5+.5*sin(kp.x*14.-t*.9+sin(kp.y*9.+t*.4)*2.),22.)
            +pow(.5+.5*sin(kp.y*11.+t*.7+kp.x*4.),30.);
  c+=w2*pal(r*.2-t*.03)*rose*(.35+.9*ml);
  // D: orbit-trap Julia fractal, E: moire op-art, F: Turing spots; they surface in slow waves.
  float wD=smoothstep(.2,.9,.5+.5*sin(t*.07+1.));
  float wE=smoothstep(.45,.95,.5+.5*sin(t*.09+3.));
  float wF=smoothstep(.45,.95,.5+.5*sin(t*.06+5.));
  c+=wD*pal(t*.03+r*.15)*juliaTrap(p*1.25,t)*(.5+.9*ml);
  c+=wE*pal(t*.05+p.x*.1)*moire(p,t)*(.28+.6*ml);
  c+=wF*pal(t*.04-p.y*.1)*turing(p,t)*(.3+.7*ml+.5*beat);
  // soft iridescent mist so the dark never goes flat
  c+=pal(fbm(p*.9+t*.02)+t*.01)*.035*(.6+ml);
  // Uneven, drifting light pools: some regions glow while others sink into darkness.
  float pools=smoothstep(.15,.85,fbm(p*.7+vec2(uTime*.04,-uTime*.03)));
  return (c*smoothstep(2.4,.2,r)*.9)*(.25+1.5*pools)+organism*(.55+.45*pools);
}

// Procedural background + art layer, split out of post.frag. Rendered into a half-resolution
// RGBA16F target (a quarter of the pixels) that post.frag upsamples with bilinear filtering: the
// content is soft, so the lower resolution does not show, and the pass is skipped entirely while
// a logo card covers the screen. uResolution is the art target's size (only its aspect is used).
void main(){
  vec2 p=(uv*2.-1.)*vec2(uResolution.x/max(uResolution.y,1.),1.);
  float beat=pow(.5+.5*sin(6.2831853*uTime*uBpm/60.0),14.0);
  float mlev=uMusicLevel;
  float hasLogo=max(step(.5,uHasLogo)*clamp(uLogoVis,0.,1.),1.0-clamp(uFxAmount,0.,1.));
  float n=fbm(p*1.7+uTime*.035);
  float n2=fbm(p*3.1+vec2(uTime*.05,-uTime*.025));
  float star=step(.996,hash(floor((p+2.)*vec2(120.,70.)+floor(uTime*.25))));
  // Ambient dust: finer, slower, more numerous drifting motes.
  vec2 dp=floor(p*vec2(46.,26.)+uTime*vec2(.35,.18));
  float dh=hash(dp);
  vec2 df=fract(p*vec2(46.,26.)+uTime*vec2(.35,.18));
  float dust=step(.86,dh)*smoothstep(.16,.0,length(df-.5))*(.3+.5*uMusicLevel);
  float neb=pow(max(0.,n),3.)*(.10+.25*uMusicLevel);
  float grid=pow(.5+.5*sin((p.x+p.y*.35)*18.+uTime*.55),18.)*(.018+.06*uMusicLevel);
  float matrixX=pow(.5+.5*sin(p.x*32.+sin(p.y*3.2+uTime*.55)*3.0+uTime*.9),28.);
  float matrixY=pow(.5+.5*sin(p.y*24.+cos(p.x*2.5-uTime*.4)*2.5-uTime*.7),26.);
  float aurora=pow(max(0.,sin(p.y*4.5+n2*5.0+uTime*.72)),3.)*smoothstep(-1.15,.45,p.y);
  float tunnel=0.;
  for(int i=0;i<4;i++){
    float z=fract(float(i)*.25+uTime*.055+uMusicLevel*.035);
    tunnel+=tunnelLayer(p,z,uTime*.12+float(i)*1.7)*(1.0-z)*.25;
  }
  float glimmer=pow(max(0.,sin((p.x*23.-p.y*17.)+uTime*(2.2+uMusicLevel*3.))),28.)*
                smoothstep(.15,1.6,length(p))*(.03+.13*uMusicLevel);
  vec3 bg=vec3(.0015,.003,.013);
  bg+=artLayer(p,mlev,beat)*.52*pow(1.0-hasLogo,3.0)*(1.0-.65*uGold);
  bg+=pal(n*.32+uTime*.015)*neb;
  bg+=pal(p.y*.08+uTime*.035+n2*.15)*aurora*(.045+.16*uMusicLevel);
  bg+=vec3(.35,.55,1.2)*(star*.22+grid);
  bg+=vec3(.7,.8,1.0)*dust;
  bg+=pal(uTime*.04+n2*.18+length(p)*.045)*tunnel*(.08+.22*uMusicLevel);
  bg+=pal(uTime*.06+p.x*.025)*(matrixX+matrixY)*(.018+.075*uMusicLevel);
  bg+=pal(uTime*.05+.3)*glimmer;
  FragColor=vec4(bg,1.0);
}
