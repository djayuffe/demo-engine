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
// Centre a mesh on its bounding box and scale it so the farthest vertex sits at radius r (0 = leave alone).
Mesh3 fit(Mesh3 m,float r){
  if(m.v.empty())return m;
  V3 lo=m.v[0],hi=m.v[0];
  for(auto& p:m.v){lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
  V3 c=mul(add3(lo,hi),.5f);float far=1e-6f;
  for(auto& p:m.v){p=sub(p,c);far=std::max(far,len(p));}
  for(auto& p:m.v)p=mul(p,r/far);
  return m;}
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
  const float dt=.21f,a1=theta(t),a0=theta(t-dt);
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
      p[i]=add3(p[i],vel[i]);
      float pl=len(p[i]);if(pl>2.f)p[i]=mul(p[i],2.f/pl);}}      // soft wall: no agent is ever flung out of frame
  Builder B;std::vector<uint32_t> node(N);
  for(unsigned i=0;i<N;++i){node[i]=B.add(p[i]);
    V3 dir=norm({f[i*4],f[i*4+1],f[i*4+2]},{1,0,0});B.edge(node[i],B.add(add3(p[i],mul(dir,.07f))));}
  for(unsigned i=0;i<N;++i){
    int b1=-1,b2=-1;float d1=1.0f,d2=1.0f;
    for(unsigned j=0;j<N;++j){if(j==i||sm[size_t(i)*N+j]<.5f)continue;float d=len(sub(p[i],p[j]));
      if(d<d1){b2=b1;d2=d1;b1=int(j);d1=d;}else if(d<d2){b2=int(j);d2=d;}}
    if(b1>=0)B.edge(node[i],node[unsigned(b1)]);
    if(b2>=0)B.edge(node[i],node[unsigned(b2)]);
    int best=-1;float bs=.82f;                                       // the most similar agent anywhere: a long "relationship" edge
    for(unsigned j=0;j<N;++j)if(j!=i&&sm[size_t(i)*N+j]>bs&&len(sub(p[i],p[j]))<1.5f){bs=sm[size_t(i)*N+j];best=int(j);}
    if(best>=0&&(i&1)==0)B.edge(node[i],node[unsigned(best)]);}
  return fit(std::move(B.m),1.9f);
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
    unsigned j=((buf[i]>>16)*2654435761u>>7)%W;if((i&1)==0)B.edge(Bi[i],Bi[j]);}
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
  // close the poles: one vertex at the mean radius of each end ring, spoked to every column
  for(int pole=0;pole<2;++pole){
    int row=pole?R-1:0;float mean=0;for(int c=0;c<C;++c)mean+=r[size_t(row)*C+c];mean=mean/float(C)*S;
    uint32_t pv=B.add({0,pole?-mean:mean,0});
    for(int c=0;c<C;c+=3)B.edge(pv,id[size_t(row)*C+c]);}
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

// ---- scenes 67..72: spec families I, E, B, H ------------------------------------------------------

// #81 Hyperbolic city: the hyperboloid model with curvature k breathing from flat to strongly negative.
// Radius r maps to (sinh(kr)/k) round the circle and cosh height, so circumference grows exponentially and
// the sheet must ruffle (the crochet-coral shape). Towers stand on every ring/spoke crossing.
Mesh3 hyperbolicCity(float t){
  Builder B;const int R=22,S=72;
  const float k=.25f+1.35f*(.5f-.5f*std::cos(t*.35f)),rmax=2.6f;
  auto at=[&](int ri,int si,float h)->V3{
    float r=rmax*float(ri)/float(R),a=kTau*float(si)/float(S);
    float rho=std::sinh(k*r)/k,z=(std::cosh(k*r)-1.f)/k;
    // ruffle: excess circumference is absorbed by out-of-plane waves whose count grows with r
    float w=.045f*rho*std::sin(a*(4.f+std::floor(r*3.f))+t*.8f);
    float rr=rho*.34f;
    return{rr*std::cos(a),(z*.34f-.9f)+w*.5f+h,rr*std::sin(a)};};
  std::vector<uint32_t> id(static_cast<size_t>(R+1)*S);
  for(int ri=0;ri<=R;++ri)for(int si=0;si<S;++si)id[size_t(ri)*S+si]=B.add(at(ri,si,0));
  for(int ri=0;ri<=R;++ri)for(int si=0;si<S;++si){
    if(ri<R)B.edge(id[size_t(ri)*S+si],id[size_t(ri+1)*S+si]);
    B.edge(id[size_t(ri)*S+si],id[size_t(ri)*S+(si+1)%S]);}
  for(int ri=4;ri<=R;ri+=2)for(int si=0;si<S;si+=3){
    uint32_t h=hash32(uint32_t(ri*977+si*131));
    if(h&3)continue;
    float ht=.05f+.22f*u01(h>>3)*(.5f+float(ri)/float(R));
    uint32_t top=B.add(at(ri,si,ht));B.edge(id[size_t(ri)*S+si],top);
    uint32_t top2=B.add(at(ri,(si+1)%S,ht));B.edge(id[size_t(ri)*S+(si+1)%S],top2);B.edge(top,top2);}
  return fit(std::move(B.m),1.9f);
}

// #82 Spherical universe: a lattice of geodesics on S^3 (hyper-latitude, polar, azimuth) seen by an
// observer who drifts through it; stereographic projection puts the far hemisphere inside the near one.
Mesh3 sphericalUniverse(float t){
  Builder B;const int NC=8,NT=10,NP=18;
  const float a1=t*.23f,a2=t*.17f;
  auto pt=[&](int ci,int ti,int pi)->V3{
    float chi=kPi*(float(ci)+.5f)/float(NC),th=kPi*(float(ti)+.5f)/float(NT),ph=kTau*float(pi)/float(NP);
    float x=std::sin(chi)*std::sin(th)*std::cos(ph),y=std::sin(chi)*std::sin(th)*std::sin(ph),
          z=std::sin(chi)*std::cos(th),w=std::cos(chi);
    float c=std::cos(a1),s=std::sin(a1);float x2=c*x+s*w,w2=-s*x+c*w;           // 4D rotation xw
    c=std::cos(a2);s=std::sin(a2);float y2=c*y+s*w2,w3=-s*y+c*w2;                 // 4D rotation yw
    float d=std::max(.12f,1.02f-w3);                                              // stereographic from w
    return{x2/d*1.3f,y2/d*1.3f,z/d*1.3f};};
  std::vector<uint32_t> id(static_cast<size_t>(NC)*NT*NP);
  auto I=[&](int c,int th,int p){return size_t((c*NT+th)*NP+p%NP);};
  for(int c=0;c<NC;++c)for(int th=0;th<NT;++th)for(int p=0;p<NP;++p)id[I(c,th,p)]=B.add(pt(c,th,p));
  for(int c=0;c<NC;++c)for(int th=0;th<NT;++th)for(int p=0;p<NP;++p){
    B.edge(id[I(c,th,p)],id[I(c,th,p+1)]);
    if(th+1<NT)B.edge(id[I(c,th,p)],id[I(c,th+1,p)]);
    if(c+1<NC&&(p&1)==0)B.edge(id[I(c,th,p)],id[I(c+1,th,p)]);}
  for(auto& v:B.m.v){float l=len(v);if(l>3.2f)v=mul(v,3.2f/l);}               // keep the far pole on screen
  return fit(std::move(B.m),1.9f);
}

// #87/#88 Geodesic lightning: light rays in a metric whose conformal factor n(x) = exp(phi) is a set of
// moving gaussian lenses (the metric mutates). Rays obey d/ds (n dx/ds) = grad n, integrated numerically.
Mesh3 geodesicLightning(float t){
  Builder B;const int rays=120,steps=110;
  struct Lens{V3 c;float k,s;};
  Lens L[4];
  for(int i=0;i<4;++i){float a=t*(.21f+.07f*float(i))+float(i)*1.9f;
    L[i]={{1.15f*std::cos(a),.7f*std::sin(a*1.3f+float(i)),1.0f*std::sin(a*.8f)},(i&1?-.9f:1.4f),.55f+.1f*float(i)};}
  auto phi=[&](V3 p){float f=0;for(auto& l:L){V3 d=sub(p,l.c);f+=l.k*std::exp(-dot(d,d)/(l.s*l.s));}return f;};
  for(int r=0;r<rays;++r){
    uint32_t h=hash32(uint32_t(r)*2654435761u+7u);
    float a=kTau*u01(h),b=std::acos(1.f-2.f*u01(hash32(h)));
    V3 p={-2.6f,-.4f+.8f*u01(hash32(h+1))*std::sin(float(r)),0};
    V3 d=norm({std::cos(a)*.35f+1.f,std::sin(b)*.3f*std::sin(a),std::cos(b)*.3f});
    uint32_t prev=B.add(p);
    for(int i=0;i<steps;++i){
      float e=.012f;V3 g={(phi({p.x+e,p.y,p.z})-phi({p.x-e,p.y,p.z}))/(2*e),
                           (phi({p.x,p.y+e,p.z})-phi({p.x,p.y-e,p.z}))/(2*e),
                           (phi({p.x,p.y,p.z+e})-phi({p.x,p.y,p.z-e}))/(2*e)};
      V3 gp=sub(g,mul(d,dot(g,d)));d=norm(add3(d,mul(gp,.045f)));
      p=add3(p,mul(d,.046f));
      float jit=.012f*std::sin(float(i)*1.7f+float(r)+t*9.f);               // lightning crackle
      uint32_t cur=B.add({p.x,p.y+jit,p.z});B.edge(prev,cur);prev=cur;
      if(len(p)>3.0f)break;}}
  return fit(std::move(B.m),1.9f);
}

// #14 Time-dilation cathedral: pointed arches and columns; a local clock field slows time near moving
// "mass" points, so neighbouring bays sway at visibly different phases.
Mesh3 timeDilationCathedral(float t){
  Builder B;const int bays=9,seg=14;
  const float mx=2.2f*std::sin(t*.17f),mz=.4f*std::cos(t*.23f);
  auto clock=[&](float x,float z){float dx=x-mx,dz=z-mz;return 1.f/(1.f+3.5f*std::exp(-(dx*dx+dz*dz)*.6f));};
  float phase[bays];float acc=0;
  for(int i=0;i<bays;++i){float x=-4.f+float(i)*1.0f;acc=t*clock(x,0)*1.6f;phase[i]=acc;}
  std::vector<uint32_t> colL(bays),colR(bays);
  for(int i=0;i<bays;++i){
    float x=-4.f+float(i)*1.0f,ph=phase[i];
    float sway=.12f*std::sin(ph+float(i)*.7f),lift=.1f*std::sin(ph*.7f+1.f);
    uint32_t prevL=0,prevR=0;
    for(int s=0;s<=seg;++s){
      float u=float(s)/float(seg);
      float w=.85f,hgt=.85f;                                                 // half span, spring height
      for(int side=-1;side<=1;side+=2){
        float y=-.9f+u*hgt;
        V3 p={x+sway*u,y+lift*u,float(side)*w};
        uint32_t id=B.add(p);
        uint32_t& pv=(side<0?prevL:prevR);
        if(s>0)B.edge(pv,id);else (side<0?colL:colR)[size_t(i)]=id;
        pv=id;}
      }
    // pointed arch: two circular arcs of radius = span meeting at the apex
    uint32_t apexPrev=0;
    for(int s=0;s<=seg;++s){
      float u=float(s)/float(seg);                                           // 0 left spring .. 1 right spring
      float zz=-.85f+1.7f*u,half=u<.5f?u:1.f-u;
      float ang=half*kPi*.5f*1.f;                                            // 0..pi/4 quarter-arc
      float hy=std::sin(ang*2.f)*.85f*.9f;
      V3 p={x+sway+.0f,-.05f+lift+hy+.0f,zz};
      uint32_t id=B.add(p);
      if(s>0)B.edge(apexPrev,id);
      apexPrev=id;}
    if(i>0){ // longitudinal ribs along the nave tie bays together at the springing and apex
      uint32_t a=B.add({x-1.f+sway,-.9f+.85f,-.85f}),b=B.add({x+sway,-.9f+.85f,-.85f});B.edge(a,b);
      uint32_t c=B.add({x-1.f+sway,-.9f+.85f,.85f}),d=B.add({x+sway,-.9f+.85f,.85f});B.edge(c,d);}
  }
  for(int i=0;i+1<bays;++i){
    float x0=-4.f+float(i),x1=x0+1.f,sw0=.12f*std::sin(phase[i]+float(i)*.7f),sw1=.12f*std::sin(phase[i+1]+float(i+1)*.7f);
    for(int side=-1;side<=1;side+=2)for(int lv=1;lv<=4;++lv){
      float y=-.9f+.85f*float(lv)/4.f,z=float(side)*.85f;
      B.edge(B.add({x0+sw0*float(lv)/4.f,y,z}),B.add({x1+sw1*float(lv)/4.f,y,z}));}
    uint32_t a=B.add({x0+sw0,-.05f,-.85f}),b=B.add({x1+sw1,-.05f,.85f}),c=B.add({x0+sw0,-.05f,.85f}),d=B.add({x1+sw1,-.05f,-.85f});
    uint32_t m1=B.add({(x0+x1)*.5f+.5f*(sw0+sw1),.72f,0});
    B.edge(a,m1);B.edge(m1,b);B.edge(c,m1);B.edge(m1,d);}
  for(auto& p:B.m.v)p=rotX(rotY(p,.7f),.28f);
  return fit(std::move(B.m),2.0f);
}

// #83 Discrete-space crystal: a cubic lattice whose connectivity is dynamic. A moving field removes
// bonds and grows long-range ones; a few spring-relaxation steps then let the geometry follow the graph.
Mesh3 discreteSpaceCrystal(float t){
  Builder B;const int n=8;const float sp=.42f;
  const int N=n*n*n;std::vector<V3> pos(static_cast<size_t>(N)),rest(static_cast<size_t>(N));
  auto I=[&](int x,int y,int z){return (z*n+y)*n+x;};
  for(int z=0;z<n;++z)for(int y=0;y<n;++y)for(int x=0;x<n;++x){
    V3 p={(float(x)-3.5f)*sp,(float(y)-3.5f)*sp,(float(z)-3.5f)*sp};pos[size_t(I(x,y,z))]=rest[size_t(I(x,y,z))]=p;}
  std::vector<std::pair<int,int>> bonds;
  auto field=[&](V3 p){return std::sin(p.x*2.1f+t*.7f)*std::sin(p.y*1.7f-t*.5f)+std::sin(p.z*2.3f+t*.4f)*.8f;};
  for(int z=0;z<n;++z)for(int y=0;y<n;++y)for(int x=0;x<n;++x){
    int a=I(x,y,z);
    int nb[3][3]={{1,0,0},{0,1,0},{0,0,1}};
    for(auto& d:nb){int X=x+d[0],Y=y+d[1],Z=z+d[2];if(X>=n||Y>=n||Z>=n)continue;
      V3 mid=mul(add3(rest[size_t(a)],rest[size_t(I(X,Y,Z))]),.5f);
      if(field(mid)>.78f)continue;                                          // bond cut
      bonds.push_back({a,I(X,Y,Z)});}
    V3 p=rest[size_t(a)];                                                    // wormhole bond across the crystal
    if(field(p)<-.9f){int X=n-1-x,Y=y,Z=n-1-z;if(X!=x||Z!=z)bonds.push_back({a,I(X,Y,Z)});}}
  for(int it=0;it<22;++it){
    std::vector<V3> f(static_cast<size_t>(N),V3{0,0,0});
    for(auto& b:bonds){V3 d=sub(pos[size_t(b.second)],pos[size_t(b.first)]);float l=len(d);if(l<1e-6f)continue;
      V3 F=mul(d,(l-sp)/l*.35f);f[size_t(b.first)]=add3(f[size_t(b.first)],F);f[size_t(b.second)]=sub(f[size_t(b.second)],F);}
    for(int i=0;i<N;++i)pos[size_t(i)]=add3(pos[size_t(i)],add3(mul(f[size_t(i)],.5f),mul(sub(rest[size_t(i)],pos[size_t(i)]),.015f)));}
  std::vector<uint32_t> id(static_cast<size_t>(N));for(int i=0;i<N;++i)id[size_t(i)]=B.add(pos[size_t(i)]);
  for(auto& b:bonds)B.edge(id[size_t(b.first)],id[size_t(b.second)]);
  return fit(std::move(B.m),1.9f);
}

// #74 Conflict geometry: two implicit fields (a torus and a gyroid-warped sphere) disagree about where
// the surface is. The difference field D = f1 - f2 is contoured on stacked slices (marching squares), so
// the wires trace the regions where the two worlds contradict each other.
Mesh3 conflictGeometry(float t){
  Builder B;const int G=64,Z=22;const float half=1.5f;
  auto f1=[&](float x,float y,float z){float q=std::sqrt(x*x+z*z)-1.0f;return std::sqrt(q*q+y*y)-.45f;};
  auto f2=[&](float x,float y,float z){float c=std::cos(t*.4f),s=std::sin(t*.4f);float X=c*x+s*z,Zr=-s*x+c*z;
    return std::sqrt(X*X+y*y+Zr*Zr)-1.05f+.28f*std::sin(3.f*X+t)*std::sin(3.f*y)*std::sin(3.f*Zr);};
  for(int k=0;k<Z;++k){
    float z=-half+2.f*half*(float(k)+.5f)/float(Z);
    std::vector<float> D(static_cast<size_t>(G+1)*(G+1));
    for(int j=0;j<=G;++j)for(int i=0;i<=G;++i){
      float x=-half+2.f*half*float(i)/float(G),y=-half+2.f*half*float(j)/float(G);
      D[size_t(j)*(G+1)+i]=f1(x,y,z)-f2(x,y,z);}
    auto P=[&](int i,int j){return V3{-half+2.f*half*float(i)/float(G),-half+2.f*half*float(j)/float(G),z};};
    for(int j=0;j<G;++j)for(int i=0;i<G;++i){
      float c[4]={D[size_t(j)*(G+1)+i],D[size_t(j)*(G+1)+i+1],D[size_t(j+1)*(G+1)+i+1],D[size_t(j+1)*(G+1)+i]};
      V3 q[4]={P(i,j),P(i+1,j),P(i+1,j+1),P(i,j+1)};
      uint32_t pts[4];int np=0;
      for(int e=0;e<4;++e){int e2=(e+1)&3;if((c[e]<0)!=(c[e2]<0)){float u=c[e]/(c[e]-c[e2]);
        pts[np++]=B.add(add3(q[e],mul(sub(q[e2],q[e]),u)));}}
      if(np==2)B.edge(pts[0],pts[1]);else if(np==4){B.edge(pts[0],pts[1]);B.edge(pts[2],pts[3]);}}
  }
  if(B.m.v.empty()){B.edge(B.add({0,0,0}),B.add({.01f,0,0}));}
  return fit(std::move(B.m),1.9f);
}
}
