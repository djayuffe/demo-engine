#include "ShowpieceGeometry.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
namespace geo {
namespace {
constexpr float kPi=3.14159265358979f;
struct Builder{
  Mesh3 m;std::set<std::pair<uint32_t,uint32_t>> seen;
  uint32_t add(V3 p){m.v.push_back(p);return uint32_t(m.v.size()-1);}
  void edge(uint32_t a,uint32_t b){if(a==b)return;auto p=std::minmax(a,b);if(seen.insert(p).second)m.e.push_back({a,b});}
};
V3 norm(V3 a){float l=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);return l>1e-9f?V3{a.x/l,a.y/l,a.z/l}:V3{0,1,0};}
V3 cross(V3 a,V3 b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
V3 sub(V3 a,V3 b){return{a.x-b.x,a.y-b.y,a.z-b.z};}
V3 add3(V3 a,V3 b){return{a.x+b.x,a.y+b.y,a.z+b.z};}
V3 mul(V3 a,float s){return{a.x*s,a.y*s,a.z*s};}
V3 rotY(V3 p,float a){float c=std::cos(a),s=std::sin(a);return{c*p.x+s*p.z,p.y,-s*p.x+c*p.z};}
V3 rotX(V3 p,float a){float c=std::cos(a),s=std::sin(a);return{p.x,c*p.y-s*p.z,s*p.y+c*p.z};}
V3 rotZ(V3 p,float a){float c=std::cos(a),s=std::sin(a);return{c*p.x-s*p.y,s*p.x+c*p.y,p.z};}
struct Ico{std::vector<V3> v;std::vector<std::array<uint32_t,3>> f;};
Ico icosahedron(){
  const float t=(1.f+std::sqrt(5.f))*.5f;Ico I;
  const V3 p[12]={{-1,t,0},{1,t,0},{-1,-t,0},{1,-t,0},{0,-1,t},{0,1,t},{0,-1,-t},{0,1,-t},{t,0,-1},{t,0,1},{-t,0,-1},{-t,0,1}};
  for(auto q:p)I.v.push_back(norm(q));
  I.f={{0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},{1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},{3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}};
  return I;
}
}
Mesh3 geodesicDome(unsigned sub,float r,float bump){
  Ico I=icosahedron();
  for(unsigned s=0;s<sub;++s){
    std::map<std::pair<uint32_t,uint32_t>,uint32_t> mid;
    auto midpoint=[&](uint32_t a,uint32_t b){auto k=std::minmax(a,b);auto it=mid.find(k);if(it!=mid.end())return it->second;
      I.v.push_back(norm(mul(add3(I.v[a],I.v[b]),.5f)));return mid[k]=uint32_t(I.v.size()-1);};
    std::vector<std::array<uint32_t,3>> nf;
    for(auto&f:I.f){uint32_t a=midpoint(f[0],f[1]),b=midpoint(f[1],f[2]),c=midpoint(f[2],f[0]);
      nf.push_back({f[0],a,c});nf.push_back({f[1],b,a});nf.push_back({f[2],c,b});nf.push_back({a,b,c});}
    I.f=std::move(nf);
  }
  Builder B;
  for(size_t i=0;i<I.v.size();++i){V3 p=I.v[i];float h=1.f+bump*std::sin(p.x*5.f)*std::sin(p.y*4.f)*std::sin(p.z*6.f);B.add(mul(p,r*h));}
  for(auto&f:I.f){B.edge(f[0],f[1]);B.edge(f[1],f[2]);B.edge(f[2],f[0]);}
  return B.m;
}
Mesh3 tubeTrefoil(unsigned rings,unsigned sides,float tubeR){
  Builder B;rings=std::max(rings,16u);sides=std::max(sides,4u);
  auto C=[&](float u){return V3{std::sin(u)+2.f*std::sin(2.f*u),std::cos(u)-2.f*std::cos(2.f*u),-std::sin(3.f*u)};};
  std::vector<std::vector<uint32_t>> idx(rings,std::vector<uint32_t>(sides));
  for(unsigned i=0;i<rings;++i){
    float u=2.f*kPi*float(i)/float(rings),h=1e-3f;
    V3 p=C(u),T=norm(sub(C(u+h),C(u-h))),N0=norm(cross(T,{0,0,1}));V3 Bn=norm(cross(T,N0));
    for(unsigned j=0;j<sides;++j){float a=2.f*kPi*float(j)/float(sides)+u*1.5f;
      V3 off=add3(mul(N0,std::cos(a)*tubeR),mul(Bn,std::sin(a)*tubeR));idx[i][j]=B.add(mul(add3(p,off),.3f));}
  }
  for(unsigned i=0;i<rings;++i){unsigned n=(i+1)%rings;
    for(unsigned j=0;j<sides;++j){B.edge(idx[i][j],idx[i][(j+1)%sides]);B.edge(idx[i][j],idx[n][j]);}}
  return B.m;
}
Mesh3 dnaHelix(unsigned steps,float twist){
  Builder B;steps=std::max(steps,8u);uint32_t pa=0,pb=0;
  for(unsigned i=0;i<steps;++i){
    float y=(float(i)/float(steps-1)-.5f)*3.6f,a=y*3.1f+twist;const float R=.6f;
    uint32_t A=B.add({R*std::cos(a),y,R*std::sin(a)}),Bv=B.add({-R*std::cos(a),y,-R*std::sin(a)});
    if(i){B.edge(pa,A);B.edge(pb,Bv);}
    if(i%2==0){ // base-pair rung with a mid-point node
      uint32_t M=B.add({0,y,0});B.edge(A,M);B.edge(M,Bv);}
    pa=A;pb=Bv;
  }
  return B.m;
}
Mesh3 atomOrbits(unsigned samples,float t){
  Builder B;samples=std::max(samples,24u);
  Mesh3 core=geodesicDome(1,.35f,0.f);
  uint32_t base=uint32_t(B.m.v.size());
  for(auto p:core.v)B.add(p);
  for(auto e:core.e)B.edge(base+e.a,base+e.b);
  const float tilt[3]={0.f,kPi/3.f,-kPi/3.f};
  for(int k=0;k<3;++k){
    uint32_t first=0,prev=0;
    for(unsigned i=0;i<samples;++i){float a=2.f*kPi*float(i)/float(samples);
      V3 p=rotZ(rotX({1.5f*std::cos(a),0,.9f*std::sin(a)},tilt[k]),tilt[k]*.5f+t*.2f);
      uint32_t id=B.add(p);if(i==0)first=id;else B.edge(prev,id);prev=id;}
    B.edge(prev,first);
    // electron: a small octahedron riding the ring
    float ea=t*(1.1f+.4f*float(k))+float(k)*2.1f;
    V3 c=rotZ(rotX({1.5f*std::cos(ea),0,.9f*std::sin(ea)},tilt[k]),tilt[k]*.5f+t*.2f);
    const V3 o[6]={{.12f,0,0},{-.12f,0,0},{0,.12f,0},{0,-.12f,0},{0,0,.12f},{0,0,-.12f}};
    uint32_t ids[6];for(int q=0;q<6;++q)ids[q]=B.add(add3(c,o[q]));
    for(int a2=0;a2<6;++a2)for(int b2=a2+1;b2<6;++b2)if(a2/2!=b2/2)B.edge(ids[a2],ids[b2]);
  }
  return B.m;
}
Mesh3 waveTerrain(unsigned n,float t){
  Builder B;n=std::max(n,8u);std::vector<uint32_t> id(size_t(n)*n);
  for(unsigned j=0;j<n;++j)for(unsigned i=0;i<n;++i){
    float x=(float(i)/float(n-1)-.5f)*3.4f,z=(float(j)/float(n-1)-.5f)*3.4f,r=std::sqrt(x*x+z*z);
    float y=.28f*std::sin(r*4.2f-t*1.6f)/(1.f+r*.6f)+.12f*std::sin(x*3.f+t*.9f)*std::cos(z*2.6f-t*.7f);
    id[size_t(j)*n+i]=B.add(rotX({x*1.15f,y*1.4f-.2f,z*1.15f},.62f));}
  for(unsigned j=0;j<n;++j)for(unsigned i=0;i<n;++i){
    if(i+1<n)B.edge(id[size_t(j)*n+i],id[size_t(j)*n+i+1]);
    if(j+1<n)B.edge(id[size_t(j)*n+i],id[size_t(j+1)*n+i]);}
  return B.m;
}
Mesh3 platonicCompound(float t){
  Builder B;Ico I=icosahedron();const float R=1.5f;
  auto spin=[&](V3 p){return rotY(rotX(p,t*.21f),t*.17f);};
  std::vector<uint32_t> iv;for(auto p:I.v)iv.push_back(B.add(spin(mul(p,R))));
  for(auto&f:I.f){B.edge(iv[f[0]],iv[f[1]]);B.edge(iv[f[1]],iv[f[2]]);B.edge(iv[f[2]],iv[f[0]]);}
  // dual dodecahedron: face centres, joined across shared edges
  std::vector<uint32_t> dv;std::vector<V3> dc;
  for(auto&f:I.f){V3 c=norm(add3(add3(I.v[f[0]],I.v[f[1]]),I.v[f[2]]));dc.push_back(c);dv.push_back(B.add(spin(mul(c,R*.82f))));}
  for(size_t a=0;a<I.f.size();++a){
    for(size_t b=a+1;b<I.f.size();++b){
      int shared=0;
      for(auto x:I.f[a])for(auto y:I.f[b])if(x==y)++shared;
      if(shared==2)B.edge(dv[a],dv[b]);
    }
  }
  // inscribed cube-pair (stella octangula) pulsing in and out
  float s=.62f+.1f*std::sin(t*1.3f);uint32_t cv[8];int k=0;
  for(int x=-1;x<=1;x+=2)for(int y=-1;y<=1;y+=2)for(int z=-1;z<=1;z+=2)cv[k++]=B.add(spin({x*s,y*s,z*s}));
  for(int a=0;a<8;++a)for(int b=a+1;b<8;++b){int d=((a^b)&1)+(((a^b)>>1)&1)+(((a^b)>>2)&1);if(d==1)B.edge(cv[a],cv[b]);}
  return B.m;
}

Mesh3 goldenOrbit(OrbitMap kind,float t,float yaw,unsigned n){
  n=std::clamp(n,1000u,40000u);
  const bool dj=kind==OrbitMap::DeJong;
  // Constants from uber256-lab src/galaxy.asm and src/cliff.asm.
  const float a0=dj?1.4f:-1.4f,b=dj?-2.3f:1.6f,c=dj?2.4f:1.0f,d0=dj?-2.1f:.7f;
  // The originals nudge a and d by 0.0006 (GALAXY) / 0.0004 (CLIFF) per ~70 Hz frame, forever.
  // Same initial speed here, but bounded (a sine around the start) so the show never drifts into noise.
  const float rate=dj?.042f:.028f,amp=.35f;
  Builder B;
  for(int pass=0;pass<2;++pass){             // pass 1 is a sparser, older "ghost" orbit: the fading trail
    const float tt=t-1.6f*float(pass),drift=amp*std::sin(rate*tt/amp),a=a0+drift,d=d0+drift;
    auto F=[&](float x,float y,float& ox,float& oy){
      if(dj){ox=std::sin(a*y)-std::cos(b*x);oy=std::sin(c*x)-std::cos(d*y);}
      else  {ox=std::sin(a*y)+c*std::cos(a*x);oy=std::sin(b*x)+d*std::cos(b*y);}
    };
    // GALAXY starts at (1,1), CLIFF at (0,1), as the x87 stack is primed in the .asm sources.
    float x=dj?1.f:0.f,y=1.f,z=0.f,ux=1.f,uy=0.f;
    const unsigned count=pass?n/3:n,warm=64;
    const float eps=1e-3f,len=.024f;
    for(unsigned k=0;k<count+warm;++k){
      float nx,ny,sx,sy;
      F(x,y,nx,ny);
      F(x+eps*ux,y+eps*uy,sx,sy);           // track a neighbour: its direction converges to the unstable
      float dx=sx-nx,dy=sy-ny,L=std::hypot(dx,dy);   // direction, i.e. along the visible filaments
      if(L>1e-9f){ux=dx/L;uy=dy/L;}
      z=.55f*std::sin(1.7f*x-1.1f*y+z);       // bounded companion coordinate: thickness, no feedback into x,y
      x=nx;y=ny;
      if(k<warm)continue;
      V3 p{x,y,z},h{ux*len*.5f,uy*len*.5f,0.f};
      uint32_t i0=B.add(sub(p,h)),i1=B.add(add3(p,h));B.edge(i0,i1);
      if(pass==0&&k%61==0){                  // a few four-point glints for the glimmer
        V3 q{-uy*len*.8f,ux*len*.8f,0.f},w{0.f,0.f,len*.8f};
        uint32_t j0=B.add(sub(p,q)),j1=B.add(add3(p,q)),j2=B.add(sub(p,w)),j3=B.add(add3(p,w));
        B.edge(j0,j1);B.edge(j2,j3);
      }
    }
  }
  if(yaw!=0.f)for(auto& p:B.m.v)p=rotY(p,yaw);
  return B.m;
}
}
