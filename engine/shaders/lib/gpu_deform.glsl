// Final recipe stage, evaluated once per indexed vertex. Parameters follow Effects.cpp.
uniform vec4 uGpuEffect;  // effect id, amount, stage time, bass
uniform vec3 uGpuCenter;
uniform uint uGpuSeed;
uint effectHash(uint x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
float effectRandom(uint x){return float(effectHash(x)&0xffffffu)/float(0xffffffu)*2.-1.;}
vec3 gpuDeform(vec3 p,uint index,int instance){
  int id=int(uGpuEffect.x);float A=uGpuEffect.y,t=uGpuEffect.z,bass=uGpuEffect.w;
  vec3 C=uGpuCenter,x=p-C;
  if((id>=31&&id<=34)||id==52||id==53){
    if(instance==0) return id==52?p-vec3(.08*A,0.,0.):p;
    float k=float(instance),scale=1.,angle=0.;vec3 offset=vec3(0.);
    if(id==31){offset.y=.08*k;scale=1.-.08*k;angle=.13*k+A*.03;}
    if(id==32){angle=t*.3+k*6.28318530718/6.;offset=vec3(1.8*cos(angle),0.,1.8*sin(angle));scale=.35;}
    if(id==33){scale=pow(.55,k);angle=t*.1*k;}
    if(id==34){offset=vec3(-.12,.04,.08)*k;scale=1.-.09*k;angle=-.06*k;}
    if(id==52) offset.x=.16*A;
    if(id==53){float sign_=instance==1?1.:-1.;offset.x=sign_*.04*A;scale=1.+sign_*.01;angle=sign_*.012*A;}
    float co=cos(angle),si=sin(angle);
    return vec3(p.x*co-p.z*si,p.y,p.x*si+p.z*co)*scale+offset;
  }
  if(id==1||id==12){
    float angle=id==1?A*.7*x.y+.2*sin(t):A*1.4/(.35+length(x.xz))+t*.2;
    float co=cos(angle),si=sin(angle);return C+vec3(x.x*co-x.z*si,x.y,x.x*si+x.z*co);
  }
  if(id==2){float angle=A*.22*x.x,co=cos(angle),si=sin(angle);return C+vec3(x.x,x.y*co-x.z*si,x.y*si+x.z*co);}
  if(id==4) return C+x*(1.+A*.18*sin(t*2.));
  if(id==6||id==23){
    uint k=id==6?uGpuSeed+index*3u:uGpuSeed+index+uint(floor(t*3.))*101u;
    vec3 r=vec3(effectRandom(k),effectRandom(k+(id==6?1u:7u)),effectRandom(k+(id==6?2u:19u)));
    return p+r*A*(id==6?.12:.22);
  }
  if(id==13){p.x+=A*.3*sin(p.y*2.7+t);p.z+=A*.2*cos(p.x*2.1-t);return p;}
  if(id==15) return A>=0.?p.yzx:p.zxy;
  if(id==19) return C+x*(1.+A*.2*sin(t*1.7+(dot(x.xz,x.xz)>0.?atan(x.z,x.x):0.)*3.));
  if(id==22) return C+x*max(.02,1.-.35*abs(A));
  if(id==35){float ph=t+float(index%97u)*.025;p.x+=A*.18*sin(ph+p.y);p.z+=A*.18*cos(ph-p.x);return p;}
  if(id==48){p.x+=A*.35*sin(p.y*p.y*2.+t);p.z+=A*.22*sin(p.x*3.-p.y*2.);return p;}
  if(id==54) return C+x*(1.+A*.35*bass);
  if(id==58) return C+x*(1.+A*.3*bass*sin(t*2.));
  if(id==61) return p+A*.12*vec3(sin(p.y+t)-cos(p.z),sin(p.z-t)-cos(p.x),sin(p.x+t)-cos(p.y));
  if(id==62) return p+A*.16*vec3(cos(p.y)-sin(p.z),cos(p.z)-sin(p.x),cos(p.x)-sin(p.y));
  if(id==63){
    p.x+=A*.2*sin(p.y*2.+sin(p.z*3.+t));
    p.y+=A*.2*sin(p.z*2.+sin(p.x*3.-t));
    p.z+=A*.2*sin(p.x*2.+sin(p.y*3.+t));return p;
  }
  return p;
}
