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
uniform sampler2D uArt;   // the art pass (half resolution), see art.frag
uniform float uArtOn;
// Selected background style (0 = classic). Styled backgrounds keep their own palette: the hue drift,
// solarize and posterize grade is mostly held back so a pink synthwave sun stays pink.
uniform float uBgStyle;
uniform vec2 uPointer;   // -1..1, y up
uniform float uPulse;    // click shockwave 1 -> 0
uniform float uDepth;    // independent depth-mote layer on/off
uniform sampler2D uBloom;

#include "lib/common.glsl"
#include "lib/depth_layers.glsl"

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
  float calm=max(hasLogo,clamp(uGold,0.,1.));   // warps and colour grading off: logo card or golden scene

  // --- Screen-space UV warps (applied to the scene sample) -------------------
  // These displace the sampling UV. They look great on a wireframe-on-black
  // but destroy a photographic logo, so every warp is multiplied by
  // (1.0-hasLogo) — a no-op in logo mode.
  vec2 suv=uv;
   // 1) Gravitational lensing: true 1/r^2 central magnification.
   // Clamp the displaced UV to [0,1] — near the center the magnification can
   // push it well outside the buffer, which would sample clamped black borders.
   { vec2 cp=suv-.5; float rr=max(length(cp),.035);
     float lens=(.35+.65*beat)*pow(1.0-calm,3.0);
     suv=clamp(.5+cp*(1.0+lens*.035/(rr*rr)),0.0,1.0); }
  // 2) Refractive shockwave ring: a Gaussian ring that displaces the image.
  { float shock=fract(uTime*.145);
    float r=length(suv-.5);
    float ring=exp(-pow((r-shock*.72)/.025,2.0));
    suv=.5+(suv-.5)*(1.0-ring*.09*(.5+mlev)*(1.0-calm)); }
  // 3) Barrel/lens breathing (pulse-driven).
  { vec2 bp=suv-.5; float r2=dot(bp,bp);
    suv=.5+bp*(1.0+(.02+.05*beat)*r2*(1.0-calm)); }


  // --- Wild shader events: one warp at a time, picked every 6 s with a smooth envelope ----
  float wslot=floor(uTime/6.0);
  int wid=int(hash(vec2(wslot,7.3))*7.0);
  float wenv=pow(sin(3.14159265*fract(uTime/6.0)),.7)*(1.0-calm);
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
  vec2 ca=(suv-.5)*(.002+.006*beat)*(1.0-calm);
  vec3 scene=vec3(texture(uScene,suv+ca).r,texture(uScene,suv).g,texture(uScene,suv-ca).b);

  // bloom: multi-scale pyramid built by bloom.cpp (the glow spreads across the screen, wire-width to full-frame)
  vec3 bloom=texture(uBloom,suv).rgb*.5;


  // Eye candy: anamorphic horizontal streak + radial light rays (bright pixels only).
  vec3 streak=vec3(0),rays=vec3(0);
  for(int i=1;i<=8;i++){
    float fi=float(i);float wgt=exp(-fi*.33);
    float tap=mix(7.,3.,uGold);   // tighter taps for the golden glints, so they read as soft rays, not combs
    vec3 a=texture(uScene,suv+vec2(px.x*fi*tap,0)).rgb,b=texture(uScene,suv-vec2(px.x*fi*tap,0)).rgb;
    streak+=(max(a-.55,0.)+max(b-.55,0.))*wgt;
    vec2 dir=(vec2(.5)-suv)*(fi*.018);
    rays+=max(texture(uScene,suv+dir).rgb-.6,0.)*(1.-fi/9.);
    if(uGold>.5){   // vertical arm of the glint: with the horizontal streak, a four-point star
      vec3 u=texture(uScene,suv+vec2(0,px.y*fi*tap)).rgb,v=texture(uScene,suv-vec2(0,px.y*fi*tap)).rgb;
      streak+=(max(u-.45,0.)+max(v-.45,0.))*wgt*.8;
    }
  }
  streak*=mix(vec3(.55,.8,1.35),vec3(1.45,1.0,.42)*1.6,uGold)*.11;   // cool blue anamorphic tint, or gold glints
  rays*=mix(pal(uTime*.05),vec3(1.3,.9,.4),uGold)*.05*(.5+mlev+beat);   // rainbow rays, or gold in the golden scenes
  // Procedural background + art layer: art.frag renders it at half resolution into uArt, and the
  // renderer skips that pass while a logo card covers the screen (uArtOn = 0).
  vec3 bg=uArtOn>.5?texture(uArt,uv).rgb:vec3(.0015,.003,.013);

  // In logo mode the scene already contains the logo backdrop, so skip the
  // procedural background (it would wash out the picture) and use a lower
  // scene gain (the logo is 0-1, not a bright wireframe on black). hasLogo is
  // already computed above (before the warps).
  vec3 hdr=(1.0-hasLogo)*bg*(1.0-.75*uGold) + scene*mix(1.35,1.0,hasLogo) + bloom*(.32+uMusicLevel*.70)*mix(1.0,.30,hasLogo);   // little glow over a logo card: it washes out
  hdr+=(streak+rays)*(1.0-.85*hasLogo);
  if(uDepth>.5&&uFxAmount>.001) hdr+=depthMotes(p,uPointer*vec2(uResolution.x/max(uResolution.y,1.),1.),uPulse,uTime,mlev,beat)*(1.0-.8*hasLogo)*uFxAmount;
  hdr+=(1.0-hasLogo)*(1.0-uGold)*pal(length(p)*.08+uTime*.02)*pow(max(scene.r,max(scene.g,scene.b)),2.2)*(.45+uMusicLevel);

  // --- Uber-compositor signature FX ------------------------------------------
  // 5) SDF nested-triangle energy sculpture (screen space, pulsing).
  { vec2 sp=suv-.5; float aa=atan(sp.y,sp.x);
    float tri=cos(floor(.5+aa/2.0943951)*2.0943951-aa)*length(sp);
    float sdf=exp(-abs(tri-.20-.025*sin(uTime*2.))*120.0);
    hdr+=vec3(1.,.015,.002)*sdf*(.10+.4*beat)*clamp(uFxAmount,0.,1.)*(1.0-uGold); }
  // 6) Holographic spectral interference (subtle RGB sin stripes).
  { float holo=.5+.5*sin(uTime*.3);
    hdr+=clamp(uFxAmount,0.,1.)*(1.0-uGold)*holo*.04*vec3(sin(uv.x*90.+uTime*4.)*.5+.5,sin(uv.y*90.+uTime*4.+2.1)*.5+.5,sin((uv.x+uv.y)*90.+uTime*4.+4.2)*.5+.5); }
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
  { float k=(1.0-calm)*(uBgStyle>.5?.12:1.0);
    float ha=uTime*.12*k;float ch=cos(ha),sh=sin(ha);
    mat3 hue=mat3(.299+.701*ch+.168*sh,.299-.299*ch-.328*sh,.299-.3*ch+1.25*sh,
                  .587-.587*ch+.33*sh,.587+.413*ch+.035*sh,.587-.588*ch-1.05*sh,
                  .114-.114*ch-.497*sh,.114-.114*ch+.292*sh,.114+.886*ch-.203*sh);
    hdr=mix(hdr,hue*hdr,k);
    float sol=beat*(wid==3?0.:1.)*k*.18*(.4+mlev);
    // Only bright pixels solarize: abs(0-.5)*2 = 1 would lift the black background to grey on every beat.
    hdr=mix(hdr,abs(hdr-.5)*2.,sol*smoothstep(.15,.6,dot(hdr,vec3(.2126,.7152,.0722))));
    float pst=smoothstep(.55,.95,sin(uTime*.31))*k*.5;
    hdr=mix(hdr,floor(hdr*5.+.5)/5.,pst); }
  hdr=mix(hdr,hdr*vec3(1.10,1.0,.80),.55*uGold);   // warm grade for the golden scenes
  hdr=(hdr-.5)*1.075+.5;
  hdr=pow(max(hdr,0.),vec3(mix(.86,1.0,hasLogo)));
  // Final touch: triangular-PDF dither (+-1 LSB) so the slow dark gradients of the background styles
  // (nebula gas, synthwave sky) quantise to 8 bits without visible banding.
  vec2 dq=gl_FragCoord.xy+fract(uTime*7.31)*64.;
  hdr+=(hash(dq)+hash(dq+37.7)-1.0)/255.0;
  FragColor=vec4(hdr,1);
}
