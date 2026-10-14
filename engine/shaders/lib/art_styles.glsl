// Selectable background styles (--background-style / key V). Each returns linear HDR colour for
// the half-resolution art pass. REQUIRES lib/common.glsl and `uniform float uTime;`.

// ---- 1: SYNTHWAVE -- striped sun, neon perspective grid racing toward you, glowing ridgeline ----
float swRidge(float x,float t){
  float h=0.,a=.5,f=1.3;
  for(int i=0;i<4;i++){h+=a*(1.-abs(2.*noise(vec2(x*f+t,float(i)*7.3))-1.));a*=.5;f*=2.1;}
  return h;
}
vec3 styleSynthwave(vec2 p,float ml,float beat){
  float hor=-.12+uPointer.y*.18;             // horizon tilts with the pointer
  vec2 pg=p;                                 // ground plane keeps the nearest parallax
  pg.x-=uPointer.x*.55;
  p.x-=uPointer.x*.12;
  vec3 col=vec3(0);
  // sky: indigo to hot magenta at the horizon
  float sk=clamp((p.y-hor)/1.1,0.,1.);
  col=mix(vec3(.55,.06,.42),vec3(.015,.01,.09),pow(sk,.55));
  // stars
  vec2 sp=(p-uPointer*.03)*vec2(95.,60.);          // stars: farthest plane
  float st=step(.992,hash(floor(sp)))*smoothstep(.5,.0,length(fract(sp)-.5))*(.5+.5*sin(uTime*3.+hash(floor(sp))*40.));
  col+=vec3(.9,.8,1.)*st*sk*1.4;
  // sun with horizontal slits that widen toward the bottom
  vec2 sc=vec2(-uPointer.x*.06,hor+.34);          // sun: far
  float R=.40*(1.+.05*beat+.04*ml);
  float d=length(p-sc);
  float sy=(p.y-(hor))/(R*1.7);                    // 0 at horizon -> ~1 at sun top
  float slit=smoothstep(.0,.06,abs(fract(sy*7.-uTime*.15)-.5)*2.-mix(.75,.05,clamp(sy*1.15,0.,1.)));
  float disc=smoothstep(R,R-.012,d)*(p.y>hor?1.:0.)*(sy>.55?1.:slit);
  vec3 sunC=mix(vec3(1.4,.12,.55),vec3(1.7,1.1,.15),clamp((p.y-sc.y+R)/(2.*R),0.,1.));
  col+=sunC*disc;
  col+=vec3(1.,.2,.6)*exp(-max(d-R,0.)*5.5)*(.55+.5*ml)*(p.y>hor?1.:.4);
  // mountains (two ridge layers, neon rim light)
  for(int k=0;k<2;k++){
    float fk=float(k);
    float rh=hor+(.05+.11*swRidge((p.x+uPointer.x*(.20+.50*fk))*(1.1+fk*.7)+fk*3.,fk*.9))*(1.-.35*fk);   // two ridges, two depths
    float edge=exp(-abs(p.y-rh)*(70.+fk*30.))*(.9+.8*beat);
    if(p.y<rh&&p.y>hor) col=mix(col,vec3(.01,0,.04),.96);
    col+=mix(vec3(.1,.9,1.2),vec3(1.2,.2,.9),fk)*edge*.8;
  }
  // grid floor: perspective lines streaming toward the camera, speed lifts with the music
  if(p.y<hor){
    float h=hor-p.y;
    float z=.42/h;
    float sp2=uTime*(1.1+1.6*ml);
    vec2 g=vec2((pg.x)*z*.9,z+sp2);
    vec2 gl=abs(fract(g)-.5);
    float w=.02+.12*h;
    float line=smoothstep(w,.0,min(gl.x,gl.y)*.5+0.0)+smoothstep(w*2.,0.,min(gl.x,gl.y))*.25;
    float fade=smoothstep(.0,.35,h);
    vec3 gc=mix(vec3(1.1,.15,.8),vec3(.1,.85,1.2),clamp(h*2.2,0.,1.));
    col=vec3(.02,0,.06)+vec3(.55,.05,.35)*exp(-h*6.)*.6;  // dark ground + horizon glow
    col+=gc*line*fade*(.9+1.2*beat);
    col+=vec3(1.,.3,.7)*exp(-h*30.)*.7;                   // bright horizon line
  }
  return col;
}

// ---- 2: NEBULA -- parallax volumetric gas, self-shadowed, drifting star-fields, shooting stars ----
float nebDensity(vec2 q,float t){
  vec2 w=vec2(fbm(q+vec2(t*.05,0.)),fbm(q+vec2(5.2,1.3)-t*.04));
  return fbm(q+2.2*w);
}
vec3 styleNebula(vec2 p,float ml,float beat){
  vec3 col=vec3(.004,.006,.02);
  float t=uTime;
  vec2 L=normalize(vec2(-.6,.8));
  for(int i=0;i<3;i++){
    float fi=float(i);
    float sc=1.0+fi*.9;
    vec2 q=rot2(p-uPointer*(.04+.22*fi*fi*.5+.10*fi),fi*.7+t*.01)*sc*.9+vec2(fi*13.,t*(.012+.01*fi));   // each gas layer parallaxes differently
    float d=nebDensity(q,t+fi*20.);
    float d2=nebDensity(q+L*.12,t+fi*20.);
    float lit=clamp((d-d2)*5.5+.35,0.,1.6);               // cheap directional light
    float body=smoothstep(.28,.85,d);
    vec3 hue=mix(pal(.62+fi*.17+t*.006),pal(.05+fi*.13-d*.4),smoothstep(.3,.8,d));
    col+=hue*body*(.07+.26*lit+.12*ml)*(1.-.22*fi);
    col*=1.-.35*smoothstep(.55,.9,nebDensity(q*1.9+7.,t))*(i==1?1.:0.);  // dust lanes
  }
  // two parallax star layers + bright flared stars
  for(int i=0;i<2;i++){
    float fi=float(i);
    vec2 sp=(p-uPointer*(.02+.34*fi)+vec2(t*(.012+.02*fi),0.))*(55.+fi*70.);
    vec2 id=floor(sp);float h=hash(id+fi*31.);
    float tw=.6+.4*sin(t*(2.+h*5.)+h*60.);
    float s=step(.985-fi*.004,h)*smoothstep(.45,.0,length(fract(sp)-.5))*tw;
    col+=mix(vec3(.7,.8,1.),vec3(1.,.8,.6),hash(id+3.))*s*(i==0?1.6:.9);
  }
  // flared stars: nearest plane
  vec2 bs=(p-uPointer*.55)*7.+vec2(t*.03,0.);vec2 bid=floor(bs);vec2 bf=fract(bs)-.5;
  float bh=hash(bid+90.);
  if(bh>.9){
    vec2 o=(vec2(hash(bid+1.),hash(bid+2.))-.5)*.5;vec2 r=abs(bf-o);
    float fl=exp(-length(r)*38.)+.5*exp(-r.x*70.-r.y*4.)+.5*exp(-r.y*70.-r.x*4.);
    col+=vec3(1.,.9,.8)*fl*(1.+1.5*beat)*.8;
  }
  // shooting star every few seconds
  float ct=floor(t*.33);float ft=fract(t*.33);
  vec2 a0=(vec2(hash(vec2(ct,1.)),hash(vec2(ct,2.)))*2.-1.)*vec2(1.5,.8);
  vec2 dir=normalize(vec2(1.,-.4-hash(vec2(ct,3.))*.5));
  vec2 hp=a0+dir*ft*2.2;vec2 rel=p-hp;
  float along=-dot(rel,dir),perp=abs(dot(rel,vec2(-dir.y,dir.x)));
  col+=vec3(.9,.95,1.2)*step(0.,along)*exp(-along*5.)*exp(-perp*110.)*smoothstep(.0,.1,ft)*smoothstep(1.,.7,ft)*2.;
  return col*(.85+.5*ml);
}

// ---- 3: MANDALA -- folded kaleidoscope of iridescent neon crystal, orbit-trap shaded ----
vec3 styleMandala(vec2 p,float ml,float beat){
  float t=uTime;
  p-=uPointer*.55;                            // the mandala follows the pointer
  float r0=length(p);
  vec2 q=rot2(p,t*.05+.15*sin(t*.2)+uPointer.x*.6);
  float seg=6.2831853/10.;
  float a=atan(q.y,q.x);a=abs(mod(a,seg)-seg*.5);
  q=vec2(cos(a),sin(a))*r0;
  q*=1.35+.08*sin(t*.3)+.05*beat;
  float trapP=1e3,trapL=1e3,trapC=1e3;
  vec3 acc=vec3(0);
  vec2 off=vec2(.62+.12*sin(t*.17),.31+.08*cos(t*.13));
  for(int i=0;i<7;i++){
    q=abs(q)-off;
    q=rot2(q,.55+.25*sin(t*.11)+float(i)*.06);
    q*=1.32;
    trapP=min(trapP,length(q));
    trapL=min(trapL,abs(q.x*.8+q.y*.6));
    trapC=min(trapC,abs(length(q)-.7));
    float k=float(i)/7.;
    acc+=pal(k*.7+t*.04+r0*.12)*exp(-trapL*(9.+k*3.))*.16;
  }
  vec3 col=vec3(.004,.002,.012);
  col+=pal(trapP*.6+t*.03+r0*.1)*exp(-trapP*2.2)*.20*(.7+ml);
  col+=pal(trapC*.9+.4+t*.05)*exp(-trapC*14.)*.55*(.6+ml+.7*beat);
  col+=acc*(.8+1.4*ml);
  // expanding beat rings
  float ring=pow(.5+.5*sin(length(p+uPointer*.30)*16.-t*(2.+3.*ml)),26.)*exp(-r0*1.1);   // ring plane sits in front of the crystal
  col+=pal(t*.06+r0)*ring*(.4+.9*beat);
  return col*smoothstep(2.6,.4,r0)+vec3(.0,.0,.01);
}


// ---- 4: BLACK HOLE -- a Schwarzschild ray-marcher: lensed galaxy, Doppler-beamed accretion disk,
//         photon ring and polar jets. Rays are bent with a = -1.5 h^2 r^-5 pos (rs = 1). ----
vec3 bhGalaxy(vec3 d,float t){
  // sky seen in direction d: a milky-way band with dust lanes, a distant spiral and deep starfield
  float lat=d.y,lon=atan(d.z,d.x);
  float band=exp(-pow(lat*3.2+.25*sin(lon*2.+t*.01),2.));
  vec2 q=vec2(lon*2.2,lat*5.);
  float gas=fbm(q*1.6+vec2(t*.004,0.));
  float dust=smoothstep(.45,.8,fbm(q*3.1+9.))*.8;
  vec3 c=mix(vec3(.9,.45,.3),vec3(.35,.5,1.),gas)*band*gas*(1.-dust)*.55;
  c+=vec3(1.,.8,.55)*exp(-pow(length(vec2(lat*2.2,sin(lon*.5)*.6)),2.)*4.)*band*.5; // bulge
  // stars on a sphere: cell grid in (lon,lat); finer grid = fainter, more numerous
  for(int i=0;i<3;i++){
    float k=90.+float(i)*110.;
    vec2 sp=vec2(lon*k*.5,lat*k*.5+float(i)*17.);vec2 id=floor(sp);
    float h=hash(id+float(i)*5.3);
    float m=step(.975,h)*smoothstep(.5,.0,length(fract(sp)-.5)*1.4);
    c+=mix(vec3(.6,.75,1.),vec3(1.,.8,.6),hash(id+1.7))*m*(i==0?1.8:.9);
  }
  return c;
}
vec3 styleBlackHole(vec2 p,float ml,float beat){
  float t=uTime;
  // camera: slow orbit, ~10 degrees above the disk plane, drifts closer on the beat
  float R=13.-1.2*ml-.5*beat-2.5*uPulse;
  float az=t*.045+uPointer.x*1.1,el=clamp(.17+.07*sin(t*.11)+uPointer.y*.75,-.2,1.2);   // pointer flies the camera
  vec3 ro=R*vec3(sin(az)*cos(el),sin(el),-cos(az)*cos(el));
  vec3 fw=normalize(-ro),rt=normalize(cross(vec3(0,1,0),fw)),up=cross(fw,rt);
  vec3 rd=normalize(fw*1.7+rt*p.x+up*p.y);
  vec3 pos=ro,vel=rd;
  vec3 col=vec3(0.);float trans=1.;
  float ring=0.,jet=0.;
  bool dead=false;
  for(int i=0;i<64;i++){
    float r2=dot(pos,pos),r=sqrt(r2);
    if(r<1.02){dead=true;break;}
    if(r>40.&&dot(pos,vel)>0.)break;
    float dt=clamp(.06*r,.05,1.1);
    vec3 hv=cross(pos,vel);float h2=dot(hv,hv);
    vec3 acc=-1.5*h2*pos/(r2*r2*r);
    vec3 np=pos+vel*dt;vel=normalize(vel+acc*dt);
    // accretion disk (y = 0 plane, 2.6 < r < 9): translucent, Doppler-beamed, turbulent
    if(pos.y*np.y<0.){
      float f=pos.y/(pos.y-np.y);vec3 hp=mix(pos,np,f);
      float rr=length(hp.xz);
      if(rr>2.6&&rr<9.5){
        float om=pow(rr,-1.5)*9.;
        float ang=atan(hp.z,hp.x)-t*om*.35;
        float turb=fbm(vec2(rr*2.4,ang*3.+rr*.6))*.7+fbm(vec2(rr*7.,ang*9.-t*.2))*.5;
        float lanes=.5+.5*sin(rr*9.+turb*9.+sin(ang*2.)*2.);
        lanes=mix(.45,1.,lanes*lanes);
        float dens=smoothstep(2.6,3.4,rr)*smoothstep(9.5,6.,rr)*(.35+turb)*lanes;
        vec3 tang=normalize(vec3(-hp.z,0.,hp.x));
        float dop=clamp(1.+.75*dot(tang,-vel),.15,2.2);          // approaching side blazes
        float temp=pow(clamp(2.8/rr,0.,1.),.75)*dop;
        vec3 bb=mix(vec3(1.,.28,.04),vec3(1.,.82,.55),clamp(temp,0.,1.));
        bb=mix(bb,vec3(.7,.85,1.4),smoothstep(.9,1.6,temp)*.6);
        float a=clamp(dens*.85,0.,1.);
        col+=trans*a*bb*pow(dop,2.4)*(.55+.9*ml+.5*beat)*(.5+1.8*exp(-(rr-2.6)*1.1));
        trans*=1.-a*.82;
      }
    }
    // photon-ring glow hugging r ~ 1.5 and faint polar jets along the spin axis
    ring+=trans*dt*.012/(.05+abs(r-1.55)*abs(r-1.55)*6.);
    float ax=dot(pos.xz,pos.xz);
    jet+=trans*dt*exp(-ax*.9)*smoothstep(1.5,3.,abs(pos.y))*exp(-abs(pos.y)*.18)*.05;
    pos=np;
  }
  if(!dead) col+=trans*bhGalaxy(normalize(vel),t)*(.8+.4*ml);
  col+=vec3(1.,.72,.42)*ring*(.3+.5*ml);
  col+=vec3(.45,.6,1.2)*jet*(1.+1.5*beat);
  return col;
}

vec3 styleLayer(int style,vec2 p,float ml,float beat){
  if(style==1) return styleSynthwave(p,ml,beat);
  if(style==2) return styleNebula(p,ml,beat);
  if(style==4) return styleBlackHole(p,ml,beat);
  return styleMandala(p,ml,beat);
}
