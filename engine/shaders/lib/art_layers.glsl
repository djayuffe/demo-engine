// Art-layer library: the layered, music-reactive procedural look used behind the wireframe.
// REQUIRES: `uniform float uTime;` declared before inclusion, and lib/common.glsl.
float tunnelLayer(vec2 p,float z,float twist){
  vec2 q=p*(1.0+z*.42);
  float a=atan(q.y,q.x)+twist+z*.21;
  float r=length(q);
  float ribs=pow(.5+.5*sin(a*18.0+z*7.0),18.0);
  float rings=pow(.5+.5*sin(r*18.0-z*5.5),22.0);
  return (ribs*.55+rings*.45)*(1.0-smoothstep(.18,1.95,r));
}

// ---- Arty layers (shown whenever the logo card is not on screen) -----------
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
  return col*(1.0-smoothstep(.3,2.6,length(p)));
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
  return smoothstep(.4,1.4,v)*(1.0-smoothstep(1.8,3.,v+.6));
}
// ---- Gallery layers G..J: slow, painterly, each surfaces in its own wave ---------------------------------
// G: topographic contour lines of a drifting height field, with pulses running up the slopes.
float contourFlow(vec2 p,float t){
  float h=fbm(p*1.3+vec2(t*.03,-t*.02))+.35*fbm(p*3.1-t*.05);
  float f=fract(h*9.);
  float line=1.-smoothstep(0.,.07,min(f,1.-f));
  float run=.35+.65*(.5+.5*sin(h*38.-t*1.8));
  return line*line*run;
}
// H: Poincare disk. A Moebius map slides the centre about; hyperbolic rings and spokes tile the disk and
// shrink toward the rim, the way an Escher print does.
float poincare(vec2 p,float t){
  vec2 z=p*.62;
  if(dot(z,z)>=.98) return 0.;
  vec2 c=.38*vec2(cos(t*.1),sin(t*.13));
  vec2 num=z-c,den=vec2(1.-(c.x*z.x+c.y*z.y),c.y*z.x-c.x*z.y);
  z=vec2(num.x*den.x+num.y*den.y,num.y*den.x-num.x*den.y)/max(dot(den,den),1e-4);
  float r=length(z);
  if(r>=.995) return 0.;
  float d=log((1.+r)/(1.-r));                                  // hyperbolic distance from the centre
  float ring=abs(fract(d*1.35-t*.12)-.5)*2.;
  float spokes=abs(fract(atan(z.y,z.x)*(7./6.2831853)+t*.02)-.5)*2.;
  float edge=min(ring,spokes*(.35+.65*smoothstep(0.,2.5,d)));  // spokes thin out toward the centre
  float cell=floor(d*1.35-t*.12)+floor(atan(z.y,z.x)*(7./6.2831853)+t*.02);
  return exp(-edge*14.)*(.5+.5*mod(cell,2.))*(1.0-smoothstep(.82,1.0,r));
}
// I: kaleidoscopic IFS (fold, shift, rotate, scale) - lace-like filaments that breathe.
float kifs(vec2 p,float t){
  vec2 z=p*1.4;float trap=1e3;
  for(int i=0;i<7;i++){
    z=abs(z)-vec2(.55+.12*sin(t*.2),.35);
    z=rot2(z,.45+.1*sin(t*.15));
    z*=1.28;
    trap=min(trap,min(abs(z.x),abs(z.y)));
  }
  return exp(-trap*5.5);
}
// J: auroral ribbons: stacked sine curtains whose brightness travels along them.
float ribbons(vec2 p,float t){
  float s=0.;
  for(int i=0;i<5;i++){
    float fi=float(i);
    float y=p.y*(1.+.1*fi)+.35*sin(p.x*(1.6+.3*fi)+t*(.3+.07*fi)+fi*2.)+.2*sin(p.x*4.1-t*.4+fi);
    s+=exp(-abs(y-(fi-2.)*.28)*14.)*(.5+.5*sin(p.x*3.+t+fi));
  }
  return s*.6;
}

vec3 artLayer(vec2 p,float ml,float beat){
  // Breathing, uneven morph: the whole field swells and shears like living tissue.
  float breath=.5+.5*sin(uTime*.55)+.6*beat*.3;
  p+=.07*vec2(sin(p.y*2.3+uTime*.4),cos(p.x*2.0-uTime*.33))*(.6+breath)+.05*ml*vec2(sin(uTime*1.3+p.y*4.),cos(uTime*1.1+p.x*4.));
  p*=1.0+.06*breath;
  vec3 organism=on(0)?blob3D(p+uPointer*.10,ml,beat):vec3(0.);   // nearest: the organism slides against the pointer
  float t=uTime;
  // Three styles cross-fade slowly so the break never looks the same twice.
  float w0=.5+.5*sin(t*.11),w1=.5+.5*sin(t*.11+2.094),w2=.5+.5*sin(t*.11+4.189);
  float ws=w0+w1+w2;w0/=ws;w1/=ws;w2/=ws;
  vec3 c=vec3(0);
  // A: neon ink filaments
  float ink=on(1)?pow(inkFlow((p-uPointer*.30)*1.15,t),6.0):0.;
  c+=w0*pal(ink*.35+t*.02+p.x*.05)*ink*(.55+.9*ml);
  // B: cracked-glass voronoi with beat-lit seams
  float ve=voroEdge(rot2(p-uPointer*.18,t*.03)*3.2);
  float seam=exp(-ve*9.0);
  c+=e(2)*w1*pal(ve*.5+t*.03)*seam*(.35+.9*ml+.8*beat);
  // C: kaleidoscopic rose rings
  vec2 k=p-uPointer*.12;float a=atan(k.y,k.x),r=length(k);
  float seg=6.2831853/8.;a=abs(mod(a+t*.05,seg)-seg*.5);
  vec2 kp=vec2(cos(a),sin(a))*r;
  float rose=pow(.5+.5*sin(kp.x*14.-t*.9+sin(kp.y*9.+t*.4)*2.),22.)
            +pow(.5+.5*sin(kp.y*11.+t*.7+kp.x*4.),30.);
  c+=e(3)*w2*pal(r*.2-t*.03)*rose*(.35+.9*ml);
  // D: orbit-trap Julia fractal, E: moire op-art, F: Turing spots; they surface in slow waves.
  float wD=smoothstep(.2,.9,.5+.5*sin(t*.07+1.));
  float wE=smoothstep(.45,.95,.5+.5*sin(t*.09+3.));
  float wF=smoothstep(.45,.95,.5+.5*sin(t*.06+5.));
  c+=e(4)*wD*pal(t*.03+r*.15)*juliaTrap((p-uPointer*.22)*1.25,t)*(.5+.9*ml);
  c+=e(5)*wE*pal(t*.05+p.x*.1)*moire(p-uPointer*.40,t)*(.28+.6*ml);
  c+=e(6)*wF*pal(t*.04-p.y*.1)*turing(p-uPointer*.26,t)*(.3+.7*ml+.5*beat);
  // G..J: the gallery layers, each on its own slow wave (skipped outright while their weight is ~0)
  float wG=smoothstep(.45,.95,.5+.5*sin(t*.05+.7));
  float wH=smoothstep(.55,.97,.5+.5*sin(t*.043+2.2));
  float wI=smoothstep(.55,.97,.5+.5*sin(t*.057+4.1));
  float wJ=smoothstep(.45,.95,.5+.5*sin(t*.071+5.9));
  if(on(7)&&wG>.002) c+=wG*pal(t*.02+p.y*.12)*contourFlow(p-uPointer*.34,t)*(.8+1.2*ml);
  if(on(8)&&wH>.002) c+=wH*pal(t*.025+r*.2+.5)*poincare(p-uPointer*.08,t)*(1.0+1.2*ml+.6*beat);
  if(on(9)&&wI>.002) c+=wI*pal(t*.03-r*.1+.2)*kifs(p-uPointer*.16,t)*(.7+1.0*ml);
  if(on(10)&&wJ>.002) c+=wJ*pal(t*.04+p.x*.08+.65)*ribbons(p-uPointer*.48,t)*(.9+1.2*ml);
  // soft iridescent mist so the dark never goes flat
  c+=pal(fbm((p-uPointer*.55)*.9+t*.02)+t*.01)*.035*(.6+ml)*e(11);
  // Uneven, drifting light pools: some regions glow while others sink into darkness.
  float pools=smoothstep(.15,.85,fbm(p*.7+vec2(uTime*.04,-uTime*.03)));
  return (c*(1.0-smoothstep(.2,2.4,r))*.9)*(.25+1.5*pools)+organism*(.55+.45*pools);
}
