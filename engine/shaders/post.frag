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

void main(){
  // --no-post fast path: sample the captured frame, ACES tone-map it (the buffer
  // is HDR float, so it needs SOME tone map to be visible), output. Skips every
  // FX (no UV warps, no procedural background, no grain, no bloom taps).
  if(uBypass>.5){
    vec3 c=texture(uScene,uv).rgb;
    FragColor=vec4(pow(aces(c*1.25),vec3(.4545)),1.0);
    return;
  }
  vec2 px=1.0/max(uResolution,vec2(1));
  vec2 p=(uv*2.-1.)*vec2(uResolution.x/max(uResolution.y,1.),1.);
  // Synthetic beat pulse, synced to the real tempo (uBpm beats/min). The old
  // hard-coded uTime*3.2 (192 BPM) only matched 192 BPM; at --bpm 132 the FX
  // pulsed ~45% too fast. beatHz = bpm/60, and a 14th-power sine gives the same
  // sharp downbeat spike shape the original had.
  float beatHz=uBpm/60.0;
  float beat=pow(.5+.5*sin(6.2831853*uTime*beatHz),14.0);
  float mlev=uMusicLevel;
  // 1.0 when a logo backdrop is in uScene (logo mode): the logo is a real
  // 0-1 picture, so screen-space UV warps (lensing/shockwave/barrel/CA) would
  // smear it into an unrecognizable smudge. Compute this BEFORE the warps so
  // they can be gated off.
  float hasLogo=max(step(.5,uHasLogo)*clamp(uLogoVis,0.,1.),1.0-clamp(uFxAmount,0.,1.));

  // --- Screen-space UV warps (applied to the scene sample) -------------------
  // These displace the sampling UV. They look great on a wireframe-on-black
  // but destroy a photographic logo, so every warp is multiplied by
  // (1.0-hasLogo) — a no-op in logo mode.
  vec2 suv=uv;
   // 1) Gravitational lensing: true 1/r^2 central magnification.
   // Clamp the displaced UV to [0,1] — near the center the magnification can
   // push it well outside the buffer, which would sample clamped black borders.
   { vec2 cp=suv-.5; float rr=max(length(cp),.035);
     float lens=(.35+.65*beat)*pow(1.0-hasLogo,3.0);
     suv=clamp(.5+cp*(1.0+lens*.035/(rr*rr)),0.0,1.0); }
  // 2) Refractive shockwave ring: a Gaussian ring that displaces the image.
  { float shock=fract(uTime*.145);
    float r=length(suv-.5);
    float ring=exp(-pow((r-shock*.72)/.025,2.0));
    suv=.5+(suv-.5)*(1.0-ring*.09*(.5+mlev)*(1.0-hasLogo)); }
  // 3) Barrel/lens breathing (pulse-driven).
  { vec2 bp=suv-.5; float r2=dot(bp,bp);
    suv=.5+bp*(1.0+(.02+.05*beat)*r2*(1.0-hasLogo)); }


  // --- Wild shader events: one warp at a time, picked every 6 s with a smooth envelope ----
  float wslot=floor(uTime/6.0);
  int wid=int(hash(vec2(wslot,7.3))*7.0);
  float wenv=pow(sin(3.14159265*fract(uTime/6.0)),.7)*(1.0-hasLogo);
  float wamt=wenv*(.55+.45*beat+.4*mlev);
  { vec2 wp=suv-.5;float wr=length(wp);
    if(wid==0){ float a=wamt*2.4*exp(-wr*2.6);float c0=cos(a),s0=sin(a);suv=.5+mat2(c0,-s0,s0,c0)*wp; }            // swirl / twirl
    else if(wid==1){ suv+=normalize(wp+1e-4)*sin(wr*38.-uTime*7.)*.012*wamt; }                                       // radial ripples
    else if(wid==2){ float ka=atan(wp.y,wp.x),seg=6.2831853/6.;ka=abs(mod(ka,seg)-seg*.5);suv=mix(suv,.5+vec2(cos(ka),sin(ka))*wr,min(wamt,1.)); }  // kaleidoscope fold
    else if(wid==3){ float cells=mix(900.,60.,min(wamt,1.));suv=mix(suv,(floor(suv*cells)+.5)/cells,min(wamt,1.)); }   // mosaic crunch
    else if(wid==4){ vec2 bk=floor(suv*vec2(14.,9.));float hb=hash(bk+floor(uTime*9.));suv.x+=(step(.82,hb)*(hb-.9)*.9)*wamt; }  // datamosh block shift
    else if(wid==5){ suv+=vec2(sin(suv.y*18.+uTime*3.),cos(suv.x*16.-uTime*2.6))*.014*wamt; }                       // liquid wobble
    else { suv=mix(suv,.5+vec2(wp.x,abs(wp.y))*vec2(1.,1.),min(wamt,1.)*.8); }                                     // mirror horizon
    suv=clamp(suv,0.0,1.0); }
  // 4) Chromatic aberration (sample R/B at offset UVs, driven by pulse).
  vec2 ca=(suv-.5)*(.002+.006*beat)*(1.0-hasLogo);
  vec3 scene=vec3(texture(uScene,suv+ca).r,texture(uScene,suv).g,texture(uScene,suv-ca).b);

  // bloom (multi-tap)
  vec3 bloom=vec3(0);
  bloom+=texture(uScene,suv+vec2( px.x*1.5,0)).rgb;
  bloom+=texture(uScene,suv+vec2(-px.x*1.5,0)).rgb;
  bloom+=texture(uScene,suv+vec2(0, px.y*1.5)).rgb;
  bloom+=texture(uScene,suv+vec2(0,-px.y*1.5)).rgb;
  bloom+=texture(uScene,suv+vec2( px.x*3.5, px.y*2.5)).rgb;
  bloom+=texture(uScene,suv+vec2(-px.x*3.5,-px.y*2.5)).rgb;
  bloom/=6.;


  // Eye candy: anamorphic horizontal streak + radial light rays (bright pixels only).
  vec3 streak=vec3(0),rays=vec3(0);
  for(int i=1;i<=8;i++){
    float fi=float(i);float wgt=exp(-fi*.33);
    vec3 a=texture(uScene,suv+vec2(px.x*fi*7.,0)).rgb,b=texture(uScene,suv-vec2(px.x*fi*7.,0)).rgb;
    streak+=(max(a-.55,0.)+max(b-.55,0.))*wgt;
    vec2 dir=(vec2(.5)-suv)*(fi*.018);
    rays+=max(texture(uScene,suv+dir).rgb-.6,0.)*(1.-fi/9.);
  }
  streak*=vec3(.55,.8,1.35)*.11;           // cool blue anamorphic tint
  rays*=pal(uTime*.05)*.05*(.5+mlev+beat);
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
  bg+=artLayer(p,mlev,beat)*.52*pow(1.0-hasLogo,3.0);
  bg+=pal(n*.32+uTime*.015)*neb;
  bg+=pal(p.y*.08+uTime*.035+n2*.15)*aurora*(.045+.16*uMusicLevel);
  bg+=vec3(.35,.55,1.2)*(star*.22+grid);
  bg+=vec3(.7,.8,1.0)*dust;
  bg+=pal(uTime*.04+n2*.18+length(p)*.045)*tunnel*(.08+.22*uMusicLevel);
  bg+=pal(uTime*.06+p.x*.025)*(matrixX+matrixY)*(.018+.075*uMusicLevel);
  bg+=pal(uTime*.05+.3)*glimmer;

  // In logo mode the scene already contains the logo backdrop, so skip the
  // procedural background (it would wash out the picture) and use a lower
  // scene gain (the logo is 0-1, not a bright wireframe on black). hasLogo is
  // already computed above (before the warps).
  vec3 hdr=(1.0-hasLogo)*bg + scene*mix(1.35,1.0,hasLogo) + bloom*(.32+uMusicLevel*.70)*mix(1.0,.30,hasLogo);   // little glow over a logo card: it washes out
  hdr+=(streak+rays)*(1.0-.85*hasLogo);
  hdr+=(1.0-hasLogo)*pal(length(p)*.08+uTime*.02)*pow(max(scene.r,max(scene.g,scene.b)),2.2)*(.45+uMusicLevel);

  // --- Uber-compositor signature FX ------------------------------------------
  // 5) SDF nested-triangle energy sculpture (screen space, pulsing).
  { vec2 sp=suv-.5; float aa=atan(sp.y,sp.x);
    float tri=cos(floor(.5+aa/2.0943951)*2.0943951-aa)*length(sp);
    float sdf=exp(-abs(tri-.20-.025*sin(uTime*2.))*120.0);
    hdr+=vec3(1.,.015,.002)*sdf*(.10+.4*beat)*clamp(uFxAmount,0.,1.); }
  // 6) Holographic spectral interference (subtle RGB sin stripes).
  { float holo=.5+.5*sin(uTime*.3);
    hdr+=clamp(uFxAmount,0.,1.)*holo*.04*vec3(sin(uv.x*90.+uTime*4.)*.5+.5,sin(uv.y*90.+uTime*4.+2.1)*.5+.5,sin((uv.x+uv.y)*90.+uTime*4.+4.2)*.5+.5); }
  // 7) Glitch slices (hash-driven horizontal displacement, beat-gated).
  // Gated off in logo mode: re-sampling uScene would re-inject the logo's
  // premultiplied RGB as an uncontrolled horizontal double-exposure.
  { float slice=step(.97,hash21(vec2(floor(uv.y*90.),floor(uTime*12.))));
    hdr+=slice*beat*(1.0-hasLogo)*texture(uScene,vec2(fract(uv.x+.025*sin(uTime*17.)),uv.y)).rgb*.4; }

  // 8) Scanlines / interlace (subtle, always on).
  hdr*=.985+.015*sin(gl_FragCoord.y*3.14159);

  // vignette (kept, slightly stronger)
  float vign=1.-smoothstep(.55,1.85,length(p*vec2(.82,1.)));
  hdr*=max(.30,vign);
  // Pixel-shader soft shadow: a darkened ellipse "cast" below the wireframe,
  // breathing with the scene scale and beat. Centered slightly low. Gated off
  // in logo mode — it would darken the center of the logo picture.
  float shScale=.62+.10*sin(uTime*.41)+.06*uMusicLevel;
  vec2 sc=(p-vec2(0.,-.55))/vec2(shScale,shScale*.5);
  float shadow=exp(-dot(sc,sc))* .38 * (1.0-hasLogo);
  hdr*= (1.0-shadow);
  // 9) Film grain (per-pixel, 60 fps time-quantized).
  hdr+=((hash(gl_FragCoord.xy+floor(uTime*60.))-.5)*.028);
  // 10) ACES tonemap + gentle contrast + gamma.
  // The logo card is already a finished 0..1 picture: expose it lower and skip the mid-tone lift
  // (pow < 1 brightens), or its silver letters burn out to white under the additive wires and bloom.
  hdr=aces(hdr*(.92+.38*mlev)*mix(1.0,.74,hasLogo));

  // --- Wild colour grading (black breaks only): hue drift, solarize on the downbeat, neon posterize ---
  { float k=1.0-hasLogo;
    float ha=uTime*.12*k;float ch=cos(ha),sh=sin(ha);
    mat3 hue=mat3(.299+.701*ch+.168*sh,.299-.299*ch-.328*sh,.299-.3*ch+1.25*sh,
                  .587-.587*ch+.33*sh,.587+.413*ch+.035*sh,.587-.588*ch-1.05*sh,
                  .114-.114*ch-.497*sh,.114-.114*ch+.292*sh,.114+.886*ch-.203*sh);
    hdr=mix(hdr,hue*hdr,k);
    float sol=beat*(wid==3?0.:1.)*k*.18*(.4+mlev);
    hdr=mix(hdr,abs(hdr-.5)*2.,sol);
    float pst=smoothstep(.55,.95,sin(uTime*.31))*k*.5;
    hdr=mix(hdr,floor(hdr*5.+.5)/5.,pst); }
  hdr=(hdr-.5)*1.075+.5;
  hdr=pow(max(hdr,0.),vec3(mix(.86,1.0,hasLogo)));
  FragColor=vec4(hdr,1);
}
