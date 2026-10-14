// Depth motes: three independent parallax planes of glowing particles drawn IN FRONT of the wires.
// Far planes are small, sharp and slow; the near plane is large, soft bokeh that sweeps past quickly.
// The pointer shifts each plane by a different amount (parallax) and pushes motes away like a lens.
// REQUIRES lib/common.glsl.
vec3 depthMotes(vec2 p,vec2 ptr,float pulse,float t,float ml,float beat){
  vec3 acc=vec3(0.);
  vec2 dm=p-ptr;float dl=length(dm);
  vec2 push=dm/max(dl,.001)*exp(-dl*dl*5.)*(.10+.30*pulse);   // cursor repels, click shockwave
  for(int i=0;i<3;i++){
    float fi=float(i);
    float z=.45+fi*.55;                       // depth: 0.45 (near) .. 1.55 (far)
    float inv=1./z;
    vec2 q=p-push*inv-ptr*(.05+.14*inv);      // per-plane parallax
    q+=vec2(sin(t*.13+fi*2.),-t*.045)*inv*.35+vec2(t*.02*inv,0.);
    float cells=mix(5.,13.,fi/2.)*(1.+.0*ml);
    vec2 g=q*cells;vec2 id=floor(g);vec2 f=fract(g)-.5;
    float h=hash(id+fi*19.7);
    if(h<.55) continue;
    vec2 o=(vec2(hash(id+3.1),hash(id+7.7))-.5)*.55;
    float d=length(f-o);
    float rad=mix(.30,.07,fi/2.)*(.7+.5*hash(id+1.3))*(1.+.5*beat*(1.-fi*.3));
    float soft=mix(.55,.12,fi/2.);            // near = defocused disc with a bokeh rim
    float disc=smoothstep(rad,rad*(1.-soft),d);
    float rim=smoothstep(rad*.25,.0,abs(d-rad*.85))*(1.-fi*.4)*.8;
    float tw=.55+.45*sin(t*(1.5+3.*h)+h*50.);
    vec3 c=mix(vec3(.6,.8,1.),pal(h*3.+t*.03),.55);
    float inten=(disc*mix(.16,.9,fi/2.)+rim*.25)*tw*(.55+.9*ml);
    acc+=c*inten*mix(.55,1.,fi/2.);
  }
  // click shockwave: an expanding ring around the cursor
  float rr=length(p-ptr);
  float ringR=(1.-pulse)*1.3;
  acc+=pal(t*.1+rr)*smoothstep(.05,.0,abs(rr-ringR))*pulse*pulse*.7;
  return acc*.55;
}
