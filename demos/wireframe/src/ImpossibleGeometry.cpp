#include "ImpossibleGeometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <utility>
#include <vector>
namespace geo {
namespace {
constexpr float kPi=3.14159265358979f,kTau=6.28318530717959f;
struct Builder{
  Mesh3 m;std::set<std::pair<uint32_t,uint32_t>> seen;
  uint32_t add(V3 p){m.v.push_back(p);return uint32_t(m.v.size()-1);}
  void edge(uint32_t a,uint32_t b){if(a==b)return;auto p=std::minmax(a,b);if(seen.insert(p).second)m.e.push_back({a,b});}
};
V3 add3(V3 a,V3 b){return{a.x+b.x,a.y+b.y,a.z+b.z};}
V3 sub(V3 a,V3 b){return{a.x-b.x,a.y-b.y,a.z-b.z};}
V3 mul(V3 a,float s){return{a.x*s,a.y*s,a.z*s};}
float dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V3 cross(V3 a,V3 b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
float len(V3 a){return std::sqrt(dot(a,a));}
V3 norm(V3 a,V3 fb={0,1,0}){float l=len(a);return l>1e-9f?mul(a,1.f/l):fb;}
V3 rotY(V3 p,float a){float c=std::cos(a),s=std::sin(a);return{c*p.x+s*p.z,p.y,-s*p.x+c*p.z};}
V3 rotX(V3 p,float a){float c=std::cos(a),s=std::sin(a);return{p.x,c*p.y-s*p.z,s*p.y+c*p.z};}
uint32_t hash32(uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return x;}
float u01(uint32_t h){return float(h>>8)*(1.f/16777216.f);}
float frac(float x){return x-std::floor(x);}
// A (g+1)^3 vertex lattice that emits the exposed faces of a solid voxel set as wire quads.
struct Lattice{
  int g;float lo,cell;Builder B;std::vector<int> id;
  Lattice(int g_,float half):g(g_),lo(-half),cell(2.f*half/float(g_)),id(size_t(g_+1)*(g_+1)*(g_+1),-1){}
  uint32_t vtx(int i,int j,int k){
    int& r=id[(size_t(k)*(g+1)+j)*(g+1)+i];
    if(r<0)r=int(B.add({lo+cell*float(i),lo+cell*float(j),lo+cell*float(k)}));
    return uint32_t(r);}
  void quad(int i0,int j0,int k0,int i1,int j1,int k1,int i2,int j2,int k2,int i3,int j3,int k3){
    uint32_t a=vtx(i0,j0,k0),b=vtx(i1,j1,k1),c=vtx(i2,j2,k2),d=vtx(i3,j3,k3);
    B.edge(a,b);B.edge(b,c);B.edge(c,d);B.edge(d,a);}
  template<class Solid> void surface(Solid solid){
    auto S=[&](int i,int j,int k){return i>=0&&j>=0&&k>=0&&i<g&&j<g&&k<g&&solid(i,j,k);};
    for(int k=0;k<g;++k)for(int j=0;j<g;++j)for(int i=0;i<g;++i){
      if(!S(i,j,k))continue;
      if(!S(i-1,j,k))quad(i,j,k, i,j+1,k, i,j+1,k+1, i,j,k+1);
      if(!S(i+1,j,k))quad(i+1,j,k, i+1,j+1,k, i+1,j+1,k+1, i+1,j,k+1);
      if(!S(i,j-1,k))quad(i,j,k, i+1,j,k, i+1,j,k+1, i,j,k+1);
      if(!S(i,j+1,k))quad(i,j+1,k, i+1,j+1,k, i+1,j+1,k+1, i,j+1,k+1);
      if(!S(i,j,k-1))quad(i,j,k, i+1,j,k, i+1,j+1,k, i,j+1,k);
      if(!S(i,j,k+1))quad(i,j,k+1, i+1,j,k+1, i+1,j+1,k+1, i,j+1,k+1);}
  }
};
}

Mesh3 noncommutativeLattice(unsigned n,float t){
  n=std::clamp(n,3u,40u);Builder B;
  const float c=float(n-1)*.5f,kappa=.55f+.35f*std::sin(t*.7f),span=1.7f;
  std::vector<uint32_t> up(size_t(n)*n),dn(size_t(n)*n);
  for(unsigned j=0;j<n;++j)for(unsigned i=0;i<n;++i){
    float x=(float(i)-c)/c,y=(float(j)-c)/c;
    // Heisenberg product: X^i Y^j = (i,j, +ij/2); Y^j X^i = (i,j, -ij/2).
    float z=.5f*kappa*x*y*span*2.f;
    up[size_t(j)*n+i]=B.add({span*x,z,span*y});
    dn[size_t(j)*n+i]=B.add({span*x,-z,span*y});}
  for(unsigned j=0;j<n;++j)for(unsigned i=0;i<n;++i){
    size_t k=size_t(j)*n+i;
    if(i+1<n){B.edge(up[k],up[k+1]);B.edge(dn[k],dn[k+1]);}
    if(j+1<n){B.edge(up[k],up[k+n]);B.edge(dn[k],dn[k+n]);}
    B.edge(up[k],dn[k]);}                      // the commutator
  return B.m;
}

Mesh3 holonomySphere(float t){
  Builder B;const float R=1.25f;const unsigned rows=12,cols=36;
  auto P=[&](float th,float ph,float r){return V3{r*std::sin(th)*std::cos(ph),r*std::cos(th),r*std::sin(th)*std::sin(ph)};};
  uint32_t top=B.add({0,R,0}),bot=B.add({0,-R,0});std::vector<uint32_t> g(size_t(rows-1)*cols);
  for(unsigned r=1;r<rows;++r)for(unsigned c=0;c<cols;++c)g[size_t(r-1)*cols+c]=B.add(P(kPi*float(r)/rows,kTau*float(c)/cols,R));
  for(unsigned r=0;r<rows-1;++r)for(unsigned c=0;c<cols;++c){
    uint32_t a=g[size_t(r)*cols+c];B.edge(a,g[size_t(r)*cols+(c+1)%cols]);
    if(r+1<rows-1)B.edge(a,g[size_t(r+1)*cols+c]);
    if(r==0&&c%3==0)B.edge(top,a);
    if(r==rows-2&&c%3==0)B.edge(bot,a);}
  const float head=kTau*frac(t/16.f),colat[5]={.38f,.75f,1.1f,1.4f,1.5707963f};
  const unsigned M=60;
  for(int L=0;L<5;++L){
    const float th=colat[L],hol=-std::cos(th);                // d(alpha)/d(phi) = -cos(theta)
    uint32_t prev=0;bool have=false;
    auto arrow=[&](float ph,float scale){
      float al=.5f+hol*ph;V3 p=P(th,ph,R*1.01f);
      V3 et{std::cos(th)*std::cos(ph),-std::sin(th),std::cos(th)*std::sin(ph)},ef{-std::sin(ph),0.f,std::cos(ph)};
      V3 v=add3(mul(et,std::cos(al)),mul(ef,std::sin(al))),nrm=norm(cross(norm(p),v)),L0=mul(v,.38f*scale);
      uint32_t a=B.add(p),b=B.add(add3(p,L0)),h1=B.add(add3(add3(p,mul(L0,.62f)),mul(nrm,.05f*scale))),h2=B.add(sub(add3(p,mul(L0,.62f)),mul(nrm,.05f*scale)));
      B.edge(a,b);B.edge(b,h1);B.edge(b,h2);};
    for(unsigned s=0;s<=M;++s){
      float ph=kTau*float(s)/M;if(ph>head+1e-6f&&s!=0)break;
      uint32_t v=B.add(P(th,ph,R*1.01f));if(have)B.edge(prev,v);prev=v;have=true;
      if(s%6==0)arrow(ph,1.f);}
    arrow(0.f,1.45f);arrow(head,1.45f);                       // start frame vs transported frame
  }
  return B.m;
}

Mesh3 motionOnlySculpture(float t,unsigned dots){
  dots=std::clamp(dots,200u,12000u);Builder B;
  auto theta=[](float tt){return .9f*tt-1.8f*std::cos(.5f*tt);};  // d/dt = .9+.9 sin(.5t) >= 0: stop-and-go
  auto curve=[](float u){return V3{.38f*(std::sin(u)+2.f*std::sin(2.f*u)),.38f*(std::cos(u)-2.f*std::cos(2.f*u)),-.38f*std::sin(3.f*u)};};
  const float dt=.16f,a1=theta(t),a0=theta(t-dt);
  auto spin=[](V3 p,float a){return rotY(rotX(p,.35f),a);};
  for(unsigned i=0;i<dots;++i){
    uint32_t h=hash32(i*3u+11u);
    if(frac(u01(hash32(h))+t*.9f)>.5f)continue;                // each dash lives half the time
    float u=kTau*u01(h),v=kTau*u01(hash32(h^0x9e3779b9u));
    V3 T=norm(sub(curve(u+.01f),curve(u-.01f))),N=norm(cross(T,{0.f,1.f,.3f}),{1,0,0}),Bn=cross(T,N);
    V3 p=add3(curve(u),mul(add3(mul(N,std::cos(v)),mul(Bn,std::sin(v))),.2f));
    V3 q1=spin(p,a1),q0=spin(p,a0);if(len(sub(q1,q0))<.004f)continue;
    B.edge(B.add(q0),B.add(q1));}
  const unsigned decoys=dots/3;const float box=4.4f;
  for(unsigned i=0;i<decoys;++i){                              // incoherent motion with the same speed statistics
    uint32_t h=hash32(i*7u+90001u);
    V3 p0{u01(h)*box,u01(hash32(h+1))*box,u01(hash32(h+2))*box},d=norm({u01(hash32(h+3))-.5f,u01(hash32(h+4))-.5f,u01(hash32(h+5))-.5f},{1,0,0});
    float sp=.5f+1.1f*u01(hash32(h+6));
    auto at=[&](float tt){V3 p=add3(p0,mul(d,sp*tt));return V3{frac(p.x/box)*box-box*.5f,frac(p.y/box)*box-box*.5f,frac(p.z/box)*box-box*.5f};};
    if(frac(u01(hash32(h+7))+t*.9f)>.5f)continue;
    V3 q1=at(t),q0=at(t-dt);if(len(sub(q1,q0))>.5f||len(sub(q1,q0))<.004f)continue;   // wrap jump or stationary
    B.edge(B.add(q0),B.add(q1));}
  if(B.m.v.empty()){B.edge(B.add({0,0,0}),B.add({.01f,0,0}));}
  return B.m;
}

Mesh3 atomicCollisionField(float t,unsigned grid,unsigned threads){
  grid=std::clamp(grid,8u,48u);threads=std::clamp(threads,1000u,400000u);
  const int G=int(grid);std::vector<uint32_t> field(size_t(G)*G*G,0u);
  const uint32_t frame=uint32_t(std::max(0.f,t)*30.f);
  const float ph=t*.35f;
  for(uint32_t id=0;id<threads;++id){
    uint32_t s=hash32(id+frame*0x9e3779b9u);
    float u=kTau*u01(s),v=kTau*u01(hash32(s^0xa511e9b3u));
    // address function: a (3,5)-rose-modulated torus whose lobes slide with time, then quantised.
    float r=.62f+.22f*std::cos(5.f*u+ph*3.f)*std::cos(3.f*v-ph);
    float x=(1.f+r*std::cos(v))*std::cos(u)*.55f,z=(1.f+r*std::cos(v))*std::sin(u)*.55f,y=r*std::sin(v)*.9f;
    auto q=[&](float f){int i=int((f*.5f/1.15f+.5f)*float(G));return std::clamp(i,0,G-1);};
    ++field[(size_t(q(z))*G+q(y))*G+q(x)];}                    // the CPU twin of atomicAdd(density[dst],1u)
  std::vector<uint32_t> sorted(field);std::sort(sorted.begin(),sorted.end(),std::greater<>());
  const size_t keep=std::min<size_t>(sorted.size()-1,1300);
  const uint32_t thr=std::max(2u,sorted[keep]);
  Lattice L(G,1.5f);
  L.surface([&](int i,int j,int k){return field[(size_t(k)*G+j)*G+i]>=thr;});
  if(L.B.m.v.empty()){L.B.edge(L.B.add({0,0,0}),L.B.add({.01f,0,0}));}
  return L.B.m;
}

Mesh3 relationalGravity(float t,unsigned agents,unsigned steps){
  agents=std::clamp(agents,8u,400u);steps=std::clamp(steps,1u,300u);
  const unsigned N=agents;std::vector<V3> p(N),vel(N,V3{0,0,0});std::vector<float> f(size_t(N)*4);
  for(unsigned i=0;i<N;++i){
    uint32_t h=hash32(i+777u);
    p[i]=mul(norm({u01(hash32(h+1))-.5f,u01(hash32(h+2))-.5f,u01(hash32(h+3))-.5f},{1,0,0}),1.5f*std::cbrt(u01(hash32(h+4))));
    float a=u01(hash32(h+5))*kTau,b=u01(hash32(h+6))*kTau;           // feature point on S3 (Hopf-style coords)
    float eta=u01(hash32(h+7))*kPi*.5f;
    float da=t*(.04f+.12f*u01(hash32(h+8))),db=t*(.03f+.1f*u01(hash32(h+9)));   // agent-dependent drift: sims change
    f[i*4+0]=std::cos(eta)*std::cos(a+da);f[i*4+1]=std::cos(eta)*std::sin(a+da);
    f[i*4+2]=std::sin(eta)*std::cos(b+db);f[i*4+3]=std::sin(eta)*std::sin(b+db);}
  auto sim=[&](unsigned i,unsigned j){return f[i*4]*f[j*4]+f[i*4+1]*f[j*4+1]+f[i*4+2]*f[j*4+2]+f[i*4+3]*f[j*4+3];};
  std::vector<float> sm(size_t(N)*N);for(unsigned i=0;i<N;++i)for(unsigned j=0;j<N;++j)sm[size_t(i)*N+j]=sim(i,j);
  std::vector<V3> acc(N);
  for(unsigned s=0;s<steps;++s){
    std::fill(acc.begin(),acc.end(),V3{0,0,0});
    for(unsigned i=0;i<N;++i)for(unsigned j=i+1;j<N;++j){
      V3 d=sub(p[j],p[i]);float r2=dot(d,d)+.03f,r=std::sqrt(r2);
      float sm_=sm[size_t(i)*N+j];float k=(sm_>0.f?.75f*sm_:.6f*sm_)/r2 - .06f/(r2*r);   // similar pulls hard, dissimilar pushes softly
      V3 fo=mul(d,k/r);acc[i]=add3(acc[i],fo);acc[j]=sub(acc[j],fo);}
    for(unsigned i=0;i<N;++i){
      V3 a=sub(mul(acc[i],1.f/float(N)*14.f),mul(p[i],.1f));
      vel[i]=mul(add3(vel[i],mul(a,.07f)),.86f);
      float sp=len(vel[i]);if(sp>.35f)vel[i]=mul(vel[i],.35f/sp);
      p[i]=add3(p[i],vel[i]);}}
  Builder B;std::vector<uint32_t> node(N);
  for(unsigned i=0;i<N;++i){node[i]=B.add(p[i]);
    V3 dir=norm({f[i*4],f[i*4+1],f[i*4+2]},{1,0,0});B.edge(node[i],B.add(add3(p[i],mul(dir,.07f))));}
  for(unsigned i=0;i<N;++i){
    int b1=-1,b2=-1;float d1=.9f,d2=.9f;
    for(unsigned j=0;j<N;++j){if(j==i||sm[size_t(i)*N+j]<.6f)continue;float d=len(sub(p[i],p[j]));
      if(d<d1){b2=b1;d2=d1;b1=int(j);d1=d;}else if(d<d2){b2=int(j);d2=d;}}
    if(b1>=0)B.edge(node[i],node[unsigned(b1)]);
    if(b2>=0)B.edge(node[i],node[unsigned(b2)]);}
  return B.m;
}

Mesh3 dataCollisionSculpture(float t,unsigned words){
  words=std::clamp(words,16u,4096u);const unsigned W=words;
  std::vector<uint32_t> buf(W);                                  // the whole "scene" is W 32-bit words
  for(unsigned i=0;i<W;++i){
    float x=float(i)*.0173f+t*.21f;
    auto by=[&](float v){return uint32_t(std::clamp(int(127.5f+127.f*v),0,255));};
    buf[i]=by(std::sin(x*3.f))|(by(std::cos(x*2.f+1.f))<<8)|(by(std::sin(x*5.f+2.f)*std::cos(x))<<16)|(by(std::sin(x*.7f))<<24);}
  auto sb=[](uint32_t b){return (float(b)-127.5f)/127.5f;};
  Builder B;std::vector<uint32_t> A(W),Bi(W),C(W);
  for(unsigned i=0;i<W;++i){
    uint32_t w=buf[i];
    V3 a{sb(w&255),sb((w>>8)&255),sb((w>>16)&255)};                    // reading A: bytes 0,1,2 as xyz
    V3 b{sb((w>>8)&255),sb((w>>16)&255),sb(w>>24)};                    // reading B: bytes 1,2,3 as xyz
    float th=kPi*float(w&255)/255.f,ph=kTau*float((w>>8)&255)/255.f,rr=.3f+.7f*float(w>>24)/255.f;
    V3 c{rr*std::sin(th)*std::cos(ph),rr*std::cos(th),rr*std::sin(th)*std::sin(ph)};   // reading C: spherical
    A[i]=B.add(mul(a,1.5f));Bi[i]=B.add(mul(b,1.5f));C[i]=B.add(mul(c,1.5f));
    B.edge(A[i],Bi[i]);B.edge(Bi[i],C[i]);}
  for(unsigned i=0;i<W;++i){                                        // reading D: the same bits as graph links
    B.edge(A[i],A[(i+1)%W]);
    unsigned j=((buf[i]>>16)*2654435761u>>7)%W;B.edge(Bi[i],Bi[j]);}
  return B.m;
}

Mesh3 selfRefutingObject(float t,unsigned iterations){
  iterations=std::clamp(iterations,1u,60u);
  const int R=36,C=72;std::vector<float> r(size_t(R)*C),nr(r.size());
  auto th=[&](int i){return kPi*(float(i)+.5f)/float(R);};
  for(int i=0;i<R;++i)for(int c=0;c<C;++c){float p=kTau*float(c)/C;
    r[size_t(i)*C+c]=1.f+.28f*std::sin(th(i))*std::sin(3.f*p+t*.6f)+.18f*std::cos(th(i)*2.f+t*.35f);}
  const float a=.9f+.45f*std::sin(t*.31f);                          // rotation of the self-reference map
  auto sample=[&](float theta,float phi){
    float fi=std::clamp(theta/kPi*float(R)-.5f,0.f,float(R-1)-1e-4f),fc=frac(phi/kTau)*float(C);
    int i0=int(fi),c0=int(fc)%C,c1=(c0+1)%C;float ti=fi-float(i0),tc=fc-std::floor(fc);
    auto at=[&](int i,int c){return r[size_t(i)*C+c];};
    return (at(i0,c0)*(1-tc)+at(i0,c1)*tc)*(1-ti)+(at(i0+1,c0)*(1-tc)+at(i0+1,c1)*tc)*ti;};
  for(unsigned it=0;it<iterations;++it){
    for(int i=0;i<R;++i)for(int c=0;c<C;++c){
      float rt=sample(kPi-th(i),kTau*float(c)/C+a);                // antipodal-flip + twist: G refers to itself elsewhere
      float F=1.f-.8f*std::tanh(1.8f*(rt-1.f));                    // slope -1.44 at the fixed point r=1: it repels
      nr[size_t(i)*C+c]=std::clamp(.7f*F+.3f*r[size_t(i)*C+c],.3f,1.85f);}
    r.swap(nr);}
  Builder B;std::vector<uint32_t> id(size_t(R)*C);const float S=1.35f;
  for(int i=0;i<R;++i)for(int c=0;c<C;++c){float p=kTau*float(c)/C,rr=r[size_t(i)*C+c]*S;
    id[size_t(i)*C+c]=B.add({rr*std::sin(th(i))*std::cos(p),rr*std::cos(th(i)),rr*std::sin(th(i))*std::sin(p)});}
  for(int i=0;i<R;++i)for(int c=0;c<C;++c){uint32_t v=id[size_t(i)*C+c];
    B.edge(v,id[size_t(i)*C+(c+1)%C]);if(i+1<R)B.edge(v,id[size_t(i+1)*C+c]);}
  return B.m;
}

Mesh3 contradictorySilhouette(float t,unsigned grid){
  grid=std::clamp(grid,8u,56u);const int G=int(grid);Lattice L(G,1.f);
  const float ra=.93f+.05f*std::sin(t*.7f),tower=.72f+.12f*std::sin(t*.5f),rot=t*.8f;
  auto disk=[&](float x,float y){return x*x+y*y<=ra*ra;};                                  // seen along z
  auto cathedral=[&](float z,float y){                                                      // seen along x
    if(y<-.95f)return false;
    if(y<=.05f&&std::fabs(z)<=.82f)return true;                                             // nave
    if(y<=tower&&std::fabs(std::fabs(z)-.52f)<=.17f)return true;                            // twin towers
    if(y>.05f&&std::fabs(z)<=.24f*(1.f-(y-.05f)/1.05f))return true;                         // spire
    return false;};
  auto star=[&](float s,float y){float rho=std::sqrt(s*s+y*y),ang=std::atan2(y,s);          // seen along (1,0,1)/sqrt2
    return rho<=.62f+.34f*std::cos(5.f*ang+rot);};
  L.surface([&](int i,int j,int k){
    float x=L.lo+L.cell*(float(i)+.5f),y=L.lo+L.cell*(float(j)+.5f),z=L.lo+L.cell*(float(k)+.5f);
    return disk(x,y)&&cathedral(z,y)&&star((x-z)*.70710678f,y);});
  Mesh3 m=std::move(L.B.m);for(auto& p:m.v)p=mul(p,1.5f);
  if(m.v.empty()){Builder B;B.edge(B.add({0,0,0}),B.add({.01f,0,0}));return B.m;}
  return m;
}
}
