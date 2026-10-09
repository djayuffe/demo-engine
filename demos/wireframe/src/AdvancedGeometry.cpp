#include "AdvancedGeometry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <random>
#include <set>
namespace { constexpr float PI=3.14159265358979323846f;
float val(geo::ImplicitKind k,float x,float y,float z,float p){x+=p;y-=p*.71f;z+=p*.37f;switch(k){case geo::ImplicitKind::Gyroid:return std::sin(x)*std::cos(y)+std::sin(y)*std::cos(z)+std::sin(z)*std::cos(x);case geo::ImplicitKind::SchwarzP:return std::cos(x)+std::cos(y)+std::cos(z);case geo::ImplicitKind::SchwarzD:return std::sin(x)*std::sin(y)*std::sin(z)+std::sin(x)*std::cos(y)*std::cos(z)+std::cos(x)*std::sin(y)*std::cos(z)+std::cos(x)*std::cos(y)*std::sin(z);case geo::ImplicitKind::Neovius:return 3*(std::cos(x)+std::cos(y)+std::cos(z))+4*std::cos(x)*std::cos(y)*std::cos(z);case geo::ImplicitKind::IWP:return 2*(std::cos(x)*std::cos(y)+std::cos(y)*std::cos(z)+std::cos(z)*std::cos(x))-(std::cos(2*x)+std::cos(2*y)+std::cos(2*z));}return 0;}
uint32_t add(Mesh3&m,V3 p,float e=2e-4f){for(uint32_t i=0;i<m.v.size();++i){auto q=m.v[i];float x=p.x-q.x,y=p.y-q.y,z=p.z-q.z;if(x*x+y*y+z*z<e*e)return i;}m.v.push_back(p);return uint32_t(m.v.size()-1);}
void dedup(Mesh3&m){std::set<std::pair<uint32_t,uint32_t>>s;std::vector<Edge>o;for(auto e:m.e){auto p=std::minmax(e.a,e.b);if(p.first!=p.second&&s.insert(p).second)o.push_back({p.first,p.second});}m.e.swap(o);}
V4 qm(V4 a,V4 b){return {a.x*b.x-a.y*b.y-a.z*b.z-a.w*b.w,a.x*b.y+a.y*b.x+a.z*b.w-a.w*b.z,a.x*b.z-a.y*b.w+a.z*b.x+a.w*b.y,a.x*b.w+a.y*b.z-a.z*b.y+a.w*b.x};}
float qn(V4 a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z+a.w*a.w);}
}
namespace geo {
std::string implicitName(ImplicitKind k){switch(k){case ImplicitKind::Gyroid:return"Gyroid";case ImplicitKind::SchwarzP:return"Schwarz P";case ImplicitKind::SchwarzD:return"Schwarz D";case ImplicitKind::Neovius:return"Neovius";case ImplicitKind::IWP:return"I-WP";}return"Implicit";}
Mesh3 implicitSurface(ImplicitKind kind,unsigned n,float iso,float phase){n=std::clamp(n,5u,64u);Mesh3 m;const int T[6][4]={{0,5,1,6},{0,1,2,6},{0,2,3,6},{0,3,7,6},{0,7,4,6},{0,4,5,6}},C[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};for(unsigned z=0;z<n-1;z++)for(unsigned y=0;y<n-1;y++)for(unsigned x=0;x<n-1;x++){V3 P[8];float D[8];for(int c=0;c<8;c++){float X=-PI+2*PI*(x+C[c][0])/(n-1),Y=-PI+2*PI*(y+C[c][1])/(n-1),Z=-PI+2*PI*(z+C[c][2])/(n-1);P[c]={X/PI,Y/PI,Z/PI};D[c]=val(kind,X,Y,Z,phase)-iso;}for(auto&t:T){std::vector<V3>h;for(int a=0;a<4;a++)for(int b=a+1;b<4;b++){int i=t[a],j=t[b];if((D[i]<0)==(D[j]<0))continue;float u=D[i]/(D[i]-D[j]);h.push_back({P[i].x+u*(P[j].x-P[i].x),P[i].y+u*(P[j].y-P[i].y),P[i].z+u*(P[j].z-P[i].z)});}if(h.size()>=3){std::vector<uint32_t>id;for(auto p:h)id.push_back(add(m,p));for(size_t i=0;i<id.size();i++)m.e.push_back({id[i],id[(i+1)%id.size()]});}}}dedup(m);return m;}
Polytope4 cell120Vertices(){
 // Exact combinatorial dual construction: each tetrahedral cell of the canonical
 // 600-cell becomes one 120-cell vertex; cells sharing a triangular face become edges.
 Polytope4 src=cell600(), out; const size_t N=src.v.size();
 std::vector<std::set<uint32_t>> adj(N); for(auto e:src.e){adj[e.a].insert(e.b);adj[e.b].insert(e.a);}
 std::vector<std::array<uint32_t,4>> cells;
 for(uint32_t a=0;a<N;a++) for(uint32_t b:adj[a]) if(a<b) for(uint32_t c:adj[a]) if(b<c&&adj[b].count(c))
   for(uint32_t d:adj[a]) if(c<d&&adj[b].count(d)&&adj[c].count(d)) cells.push_back({a,b,c,d});
 for(auto C:cells){V4 q{};for(auto i:C){q.x+=src.v[i].x;q.y+=src.v[i].y;q.z+=src.v[i].z;q.w+=src.v[i].w;}q.x/=4;q.y/=4;q.z/=4;q.w/=4;out.v.push_back(q);}
 std::map<std::array<uint32_t,3>,uint32_t> owner;
 for(uint32_t ci=0;ci<cells.size();++ci) for(int omit=0;omit<4;omit++){std::array<uint32_t,3> f{};int k=0;for(int j=0;j<4;j++)if(j!=omit)f[k++]=cells[ci][j];std::sort(f.begin(),f.end());auto it=owner.find(f);if(it==owner.end())owner[f]=ci;else out.e.push_back({it->second,ci});}
 return out;
}
Mesh3 quaternionJulia(unsigned n,unsigned it,float th,V4 c){
 n=std::clamp(n,8u,48u); th=std::max(th,.005f); Mesh3 m;
 // Sample escape metric on a 3-D slice (w=0), retain near-boundary lattice
 // points, then connect only axis-neighbour boundary samples. This avoids the
 // false scan-order polyline used by v2.
 const size_t NN=size_t(n)*n*n; std::vector<float> metric(NN); std::vector<uint8_t> boundary(NN,0);
 auto idx=[&](unsigned x,unsigned y,unsigned z){return (size_t(z)*n+y)*n+x;};
 auto eval=[&](float x,float y,float z){V4 q{x,y,z,0}; unsigned k=0; for(;k<it;k++){q=qm(q,q);q={q.x+c.x,q.y+c.y,q.z+c.z,q.w+c.w};if(qn(q)>4.f)break;} return float(k)+std::min(1.f,qn(q)/4.f);};
 for(unsigned z=0;z<n;z++)for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++){
  float X=-1.55f+3.1f*x/(n-1),Y=-1.55f+3.1f*y/(n-1),Z=-1.55f+3.1f*z/(n-1); metric[idx(x,y,z)]=eval(X,Y,Z);
 }
 const int D[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
 for(unsigned z=0;z<n;z++)for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++){
  float v=metric[idx(x,y,z)]; bool edge=false; for(auto&d:D){int X=int(x)+d[0],Y=int(y)+d[1],Z=int(z)+d[2];if(X<0||Y<0||Z<0||X>=int(n)||Y>=int(n)||Z>=int(n))continue;float dv=std::fabs(v-metric[idx(X,Y,Z)]);if(dv>std::max(.35f,th*8.f)){edge=true;break;}}
  boundary[idx(x,y,z)]=edge;
 }
 std::vector<int32_t> map(NN,-1);
 for(unsigned z=0;z<n;z++)for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++)if(boundary[idx(x,y,z)]){map[idx(x,y,z)]=(int32_t)m.v.size();m.v.push_back({-1.55f+3.1f*x/(n-1),-1.55f+3.1f*y/(n-1),-1.55f+3.1f*z/(n-1)});}
 const int P[3][3]={{1,0,0},{0,1,0},{0,0,1}};
 for(unsigned z=0;z<n;z++)for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++){int32_t a=map[idx(x,y,z)];if(a<0)continue;for(auto&d:P){unsigned X=x+d[0],Y=y+d[1],Z=z+d[2];if(X>=n||Y>=n||Z>=n)continue;int32_t b=map[idx(X,Y,Z)];if(b>=0)m.e.push_back({uint32_t(a),uint32_t(b)});}}
 return m;
}
Mesh3 hyperbolicBall(unsigned shells,unsigned spokes,float curv){Mesh3 m;shells=std::max(2u,shells);spokes=std::max(6u,spokes);m.v.push_back({0,0,0});for(unsigned s=1;s<=shells;s++){float r=std::tanh(curv*s/shells*1.55f);for(unsigned a=0;a<spokes;a++){float u=2*PI*a/spokes;for(unsigned b=1;b<spokes/2;b++){float v=PI*b/(spokes/2);m.v.push_back({r*std::sin(v)*std::cos(u),r*std::cos(v),r*std::sin(v)*std::sin(u)});}}}for(uint32_t i=1;i<m.v.size();i++){float best=1e9;uint32_t bi=0;for(uint32_t j=0;j<i;j++){auto a=m.v[i],b=m.v[j];float d=(a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z);if(d<best){best=d;bi=j;}}m.e.push_back({i,bi});}return m;}
Mesh3 cliffordTorus(unsigned U,unsigned V,float pw){Mesh3 m;for(unsigned i=0;i<U;i++)for(unsigned j=0;j<V;j++){float u=2*PI*i/U,v=2*PI*j/V;V4 q{std::cos(u)/std::sqrt(2.f),std::sin(u)/std::sqrt(2.f),std::cos(v)/std::sqrt(2.f),std::sin(v)/std::sqrt(2.f)};float k=pw/(pw-q.w);m.v.push_back({q.x*k,q.y*k,q.z*k});m.e.push_back({i*V+j,i*V+(j+1)%V});m.e.push_back({i*V+j,((i+1)%U)*V+j});}return m;}
Mesh3 lissajousKnot(unsigned N,int p,int q,int r,float ph){Mesh3 m;N=std::max(64u,N);for(unsigned i=0;i<N;i++){float t=2*PI*i/N;m.v.push_back({std::sin(p*t+ph),std::sin(q*t),std::sin(r*t+ph*.37f)});m.e.push_back({i,(i+1)%N});}return m;}
Mesh3 strangeAttractor(unsigned N,float dt){Mesh3 m;float x=.1f,y=0,z=0;for(unsigned i=0;i<N;i++){float dx=10*(y-x),dy=x*(28-z)-y,dz=x*y-(8.f/3)*z;x+=dx*dt;y+=dy*dt;z+=dz*dt;if(i>100){m.v.push_back({x*.045f,y*.045f,(z-25)*.045f});if(m.v.size()>1)m.e.push_back({uint32_t(m.v.size()-2),uint32_t(m.v.size()-1)});}}return m;}
Mesh3 discoveredObject(uint64_t seed,unsigned U,unsigned V){std::mt19937_64 g(seed);std::uniform_real_distribution<float>d(.2f,2.8f);float a=d(g),b=d(g),c=d(g),m=2+int(g()%11),n=2+int(g()%9);Mesh3 out;for(unsigned i=0;i<U;i++)for(unsigned j=0;j<V;j++){float u=2*PI*i/U,v=-PI/2+PI*j/(V-1);float rr=1+.22f*std::sin(m*u+a)+.17f*std::cos(n*v+b)+.11f*std::sin((m+n)*u*v+c);out.v.push_back({rr*std::cos(v)*std::cos(u),rr*std::sin(v),rr*std::cos(v)*std::sin(u)});uint32_t k=i*V+j;if(i+1<U)out.e.push_back({k,(i+1)*V+j});else out.e.push_back({k,j});if(j+1<V)out.e.push_back({k,k+1});}return out;}
float noveltyScore(const Mesh3&m){auto s=stats(m);if(!s.vertices||!s.edges||!s.finite)return 0;float density=float(s.edges)/s.vertices;return std::log1p(float(s.vertices))*.35f+std::min(4.f,density)*.5f+std::min(3.f,s.radius)*.2f;}
}
namespace {
Mesh3 gridSurf(unsigned U,unsigned V,const std::function<V3(float,float)>&f,bool wrapU=false,bool wrapV=false){Mesh3 m;U=std::max(3u,U);V=std::max(3u,V);for(unsigned i=0;i<U;i++)for(unsigned j=0;j<V;j++){float u=float(i)/(U-(wrapU?0:1)),v=float(j)/(V-(wrapV?0:1));m.v.push_back(f(u,v));uint32_t k=i*V+j;if(i+1<U)m.e.push_back({k,k+V});else if(wrapU)m.e.push_back({k,j});if(j+1<V)m.e.push_back({k,k+1});else if(wrapV)m.e.push_back({k,i*V});}return m;}
Mesh3 lineCurve(unsigned N,const std::function<V3(float)>&f,bool closed=true){Mesh3 m;N=std::max(16u,N);for(unsigned i=0;i<N;i++){float t=float(i)/(closed?N:N-1);m.v.push_back(f(t));if(i)m.e.push_back({i-1,i});}if(closed)m.e.push_back({N-1,0});return m;}
float exoticVal(geo::ExoticImplicitKind k,float x,float y,float z,float p){float x2=x*x,y2=y*y,z2=z*z,r2=x2+y2+z2;switch(k){case geo::ExoticImplicitKind::Heart:return std::pow(x2+2.25f*y2+z2-1,3)-x2*z*z*z-.1125f*y2*z*z*z;case geo::ExoticImplicitKind::BarthSexticLike:{float ph=(1+std::sqrt(5.f))/2;float a=(ph*ph*x2-y2)*(ph*ph*y2-z2)*(ph*ph*z2-x2);return 4*a-(1+2*ph)*r2*r2*(r2-1);}case geo::ExoticImplicitKind::TangleCube:return x*x*x*x-5*x2+y*y*y*y-5*y2+z*z*z*z-5*z2+11.8f+.4f*std::sin(p);case geo::ExoticImplicitKind::ChmutovLike:return (x*x*x-3*x)*(y*y*y-3*y)+(y*y*y-3*y)*(z*z*z-3*z)+(z*z*z-3*z)*(x*x*x-3*x)-.15f;case geo::ExoticImplicitKind::CayleyCubic:return x*y*z+x*y+x*z+y*z-.35f;case geo::ExoticImplicitKind::KummerLike:return (r2+.7f)*(r2+.7f)-4*(x2+y2+.35f*z2)-.22f*x*y*z;case geo::ExoticImplicitKind::Goursat:return x*x*x*x+y*y*y*y+z*z*z*z-1.25f*r2+.35f;case geo::ExoticImplicitKind::BlobLattice:return std::cos(3*x)+std::cos(3*y)+std::cos(3*z)+.45f*std::cos(2*x)*std::cos(2*y)*std::cos(2*z)-.15f*std::sin(p);}return 0;}
}
namespace geo {
Mesh3 mobiusStrip(unsigned U,unsigned V,float tw){return gridSurf(U,V,[=](float a,float b){float u=2*PI*a,w=(b-.5f)*.7f,c=std::cos(tw*u*.5f),s=std::sin(tw*u*.5f),R=1+w*c;return V3{R*std::cos(u),w*s,R*std::sin(u)};},true,false);}
Mesh3 kleinBottle(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=2*PI*a,v=2*PI*b;float r=2.1f+.55f*std::cos(u*.5f)*std::sin(v)-.55f*std::sin(u*.5f)*std::sin(2*v);return V3{.45f*r*std::cos(u),.45f*r*std::sin(u),.45f*(.55f*std::sin(u*.5f)*std::sin(v)+.55f*std::cos(u*.5f)*std::sin(2*v))};},true,true);}
Mesh3 enneperSurface(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=(a*2-1)*1.45f,v=(b*2-1)*1.45f;return V3{(u-u*u*u/3+u*v*v)*.45f,(v-v*v*v/3+v*u*u)*.45f,(u*u-v*v)*.45f};});}
Mesh3 helicoidSurface(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=4*PI*a,v=(b*2-1);return V3{v*std::cos(u),u/(4*PI)*1.7f-0.85f,v*std::sin(u)};});}
Mesh3 catenoidSurface(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=2*PI*a,v=(b*2-1)*1.15f,r=.48f*std::cosh(v);return V3{r*std::cos(u),v*.65f,r*std::sin(u)};},true,false);}
Mesh3 diniSurface(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=4*PI*a,v=.15f+b*2.65f;float r=.62f*std::sin(v);return V3{r*std::cos(u),.62f*(std::cos(v)+std::log(std::tan(v*.5f)))+.12f*u,r*std::sin(u)};});}
Mesh3 pseudosphere(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=2*PI*a,v=.12f+b*2.7f;return V3{std::sin(v)*std::cos(u),std::cos(v)+std::log(std::tan(v*.5f)),std::sin(v)*std::sin(u)};},true,false);}
Mesh3 romanSurface(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=PI*a,v=2*PI*b,s=std::sin(u);float d=1.15f+std::cos(u)*std::cos(u);return V3{s*s*std::sin(2*v)/d,s*std::cos(u)*std::cos(v)*2/d,s*std::cos(u)*std::sin(v)*2/d};},false,true);}
Mesh3 crossCap(unsigned U,unsigned V){return gridSurf(U,V,[](float a,float b){float u=PI*a,v=2*PI*b;return V3{std::sin(u)*std::sin(2*v),std::sin(2*u)*std::cos(v),std::cos(2*u)};},false,true);}
Mesh3 torusKnot(unsigned N,int p,int q,float R,float r){return lineCurve(N,[=](float a){float t=2*PI*a,rr=R+r*std::cos(q*t);return V3{rr*std::cos(p*t),r*std::sin(q*t),rr*std::sin(p*t)};});}
Mesh3 vivianiCurve(unsigned N){return lineCurve(N,[](float a){float t=2*PI*a;return V3{.75f*(1+std::cos(t)),.75f*std::sin(t),1.5f*std::sin(t*.5f)};});}
Mesh3 sphericalSpiral(unsigned N,float turns){return lineCurve(N,[=](float a){float th=PI*(a-.5f),ph=2*PI*turns*a,c=std::cos(th);return V3{c*std::cos(ph),std::sin(th),c*std::sin(ph)};},false);}
Mesh3 hypotrochoidKnot(unsigned N,float a,float b,float h){return lineCurve(N,[=](float s){float t=2*PI*s,x=(a-b)*std::cos(t)+h*std::cos((a-b)/b*t),y=(a-b)*std::sin(t)-h*std::sin((a-b)/b*t);return V3{x*.16f,y*.16f,.45f*std::sin(3*t)};});}
Mesh3 duffingAttractor(unsigned N,float dt){Mesh3 m;float x=.1f,v=0,t=0;for(unsigned i=0;i<N;i++){float a=x-x*x*x-.22f*v+.3f*std::cos(1.2f*t);v+=a*dt;x+=v*dt;t+=dt;if(i>500){m.v.push_back({x*.8f,v*.8f,std::sin(t*.12f)});if(m.v.size()>1)m.e.push_back({uint32_t(m.v.size()-2),uint32_t(m.v.size()-1)});}}return m;}
Mesh3 rosslerAttractor(unsigned N,float dt){Mesh3 m;float x=.1f,y=.1f,z=.1f;for(unsigned i=0;i<N;i++){float dx=-y-z,dy=x+.2f*y,dz=.2f+z*(x-5.7f);x+=dx*dt;y+=dy*dt;z+=dz*dt;if(i>500){m.v.push_back({x*.12f,y*.12f,(z-4)*.12f});if(m.v.size()>1)m.e.push_back({uint32_t(m.v.size()-2),uint32_t(m.v.size()-1)});}}return m;}
Mesh3 thomasAttractor(unsigned N,float dt){Mesh3 m;float x=.1f,y=0,z=-.1f;for(unsigned i=0;i<N;i++){float dx=std::sin(y)-.208186f*x,dy=std::sin(z)-.208186f*y,dz=std::sin(x)-.208186f*z;x+=dx*dt;y+=dy*dt;z+=dz*dt;if(i>300){m.v.push_back({x*.35f,y*.35f,z*.35f});if(m.v.size()>1)m.e.push_back({uint32_t(m.v.size()-2),uint32_t(m.v.size()-1)});}}return m;}
Mesh3 sierpinskiTetrahedron(unsigned depth){Mesh3 m;std::array<V3,4>A{{{1,1,1},{-1,-1,1},{-1,1,-1},{1,-1,-1}}};std::function<void(std::array<V3,4>,unsigned)> rec=[&](std::array<V3,4> q,unsigned d){if(!d){uint32_t b=m.v.size();m.v.insert(m.v.end(),q.begin(),q.end());for(uint32_t i=0;i<4;i++)for(uint32_t j=i+1;j<4;j++)m.e.push_back({b+i,b+j});return;}for(int k=0;k<4;k++){std::array<V3,4> n;for(int j=0;j<4;j++)n[j]={(q[j].x+q[k].x)*.5f,(q[j].y+q[k].y)*.5f,(q[j].z+q[k].z)*.5f};rec(n,d-1);}};rec(A,std::min(depth,7u));return m;}
std::string exoticImplicitName(ExoticImplicitKind k){static const char*n[]={"Heart algebraic","Barth sextic-like","Tanglecube","Chmutov-like","Cayley cubic","Kummer-like","Goursat","Blob lattice"};return n[int(k)];}
Mesh3 exoticImplicit(ExoticImplicitKind kind,unsigned n,float phase){n=std::clamp(n,8u,42u);Mesh3 m;const int T[6][4]={{0,5,1,6},{0,1,2,6},{0,2,3,6},{0,3,7,6},{0,7,4,6},{0,4,5,6}},C[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};float scale=(kind==ExoticImplicitKind::TangleCube||kind==ExoticImplicitKind::ChmutovLike)?2.2f:1.55f;for(unsigned z=0;z<n-1;z++)for(unsigned y=0;y<n-1;y++)for(unsigned x=0;x<n-1;x++){V3 P[8];float D[8];for(int c=0;c<8;c++){float X=-scale+2*scale*(x+C[c][0])/(n-1),Y=-scale+2*scale*(y+C[c][1])/(n-1),Z=-scale+2*scale*(z+C[c][2])/(n-1);P[c]={X/scale,Y/scale,Z/scale};D[c]=exoticVal(kind,X,Y,Z,phase);}for(auto&t:T){std::vector<V3>h;for(int a=0;a<4;a++)for(int b=a+1;b<4;b++){int i=t[a],j=t[b];if((D[i]<0)==(D[j]<0))continue;float u=D[i]/(D[i]-D[j]);h.push_back({P[i].x+u*(P[j].x-P[i].x),P[i].y+u*(P[j].y-P[i].y),P[i].z+u*(P[j].z-P[i].z)});}if(h.size()>=3){std::vector<uint32_t>ids;for(auto p:h)ids.push_back(add(m,p));for(size_t i=0;i<ids.size();i++)m.e.push_back({ids[i],ids[(i+1)%ids.size()]});}}}dedup(m);return m;}

namespace {
void labEdge(Mesh3& m,uint32_t a,uint32_t b){if(a!=b)m.e.push_back({a,b});}
Mesh3 labSphere(unsigned n,const std::function<float(float,float)>&radial){Mesh3 m;n=std::clamp(n,16u,96u);unsigned U=n,V=n/2+2;for(unsigned i=0;i<U;i++)for(unsigned j=0;j<V;j++){float ph=2*PI*i/U,th=.001f+(PI-.002f)*j/(V-1),r=radial(th,ph);m.v.push_back({r*std::sin(th)*std::cos(ph),r*std::cos(th),r*std::sin(th)*std::sin(ph)});uint32_t k=i*V+j;if(i+1<U)labEdge(m,k,k+V);else labEdge(m,k,j);if(j+1<V)labEdge(m,k,k+1);}return m;}
Mesh3 labSuperCage(unsigned n,float t,unsigned variant){return labSphere(n,[=](float th,float ph){float a=3.f+(variant%7),b=2.f+((variant*3)%8);float f=std::pow(std::fabs(std::cos(a*ph+t*.13f)),1.4f)+std::pow(std::fabs(std::sin(b*th-t*.09f)),1.6f);return .82f+.34f*f;});}
Mesh3 labKnotBundle(unsigned n,float t,unsigned variant){Mesh3 m;unsigned curves=5+variant%7,samples=std::max(96u,n*3);int p=2+int(variant%5),q=3+int((variant*2)%7);for(unsigned c=0;c<curves;c++){uint32_t b=uint32_t(m.v.size());float ph=2*PI*c/curves;for(unsigned i=0;i<samples;i++){float a=2*PI*i/samples,R=1.05f+.28f*std::cos(q*a+ph),z=.28f*std::sin(q*a+ph),sc=.82f+.035f*c;V3 x{R*std::cos(p*a)*sc,R*std::sin(p*a)*sc,z*sc};x.z+=.13f*std::sin((variant%6+2)*a+t*.2f);m.v.push_back(x);if(i)labEdge(m,b+i-1,b+i);}labEdge(m,b+samples-1,b);}return m;}
Mesh3 labPhyllo(unsigned n,float t,unsigned variant){Mesh3 m;unsigned arms=7+variant%9,samples=std::max(64u,n*2);float ga=2.39996323f;for(unsigned a=0;a<arms;a++){uint32_t b=uint32_t(m.v.size());for(unsigned i=0;i<samples;i++){float u=float(i)/(samples-1),ang=(i+a*.37f)*ga+t*.08f,r=.15f+1.45f*u,z=1.5f*(u-.5f)+.22f*std::sin((3+variant%5)*ang);m.v.push_back({r*std::cos(ang),z,r*std::sin(ang)});if(i)labEdge(m,b+i-1,b+i);}if(a&&samples>12)for(unsigned i=10;i<samples;i+=12)labEdge(m,b+i,b-samples+i);}return m;}
Mesh3 labRuled(unsigned n,float t,unsigned variant){return gridSurf(n,12+variant%9,[=](float u01,float v01){float u=2*PI*u01,w=v01*2.f-1.f,turn=1.f+.25f*(variant%7);float r=.72f+.55f*w*w+.12f*std::cos((3+variant%6)*u+t);return V3{r*std::cos(u)+.38f*w*std::cos(turn*u),.75f*w+.22f*std::sin((2+variant%5)*u+t*.2f),r*std::sin(u)+.38f*w*std::sin(turn*u)};},true,false);}
Mesh3 labStarCage(unsigned n,float t,unsigned variant){Mesh3 m;unsigned rings=6+variant%6,samples=std::max(32u,n),lobes=5+variant%9;for(unsigned r=0;r<rings;r++){uint32_t b=uint32_t(m.v.size());float z=-1.15f+2.3f*r/(rings-1),base=.5f+.55f*std::sin(PI*(r+1.f)/(rings+1.f));for(unsigned i=0;i<samples;i++){float a=2*PI*i/samples,rr=base*(1+.28f*std::cos(lobes*a+t*.17f+r));m.v.push_back({rr*std::cos(a),z,rr*std::sin(a)});if(i)labEdge(m,b+i-1,b+i);}labEdge(m,b+samples-1,b);if(r)for(unsigned i=0;i<samples;i+=std::max(2u,samples/12))labEdge(m,b+i,b-samples+i);}return m;}
}
unsigned unknownLabCount(){return 10;}
std::string unknownLabName(unsigned id){static const char*n[]={"Unknown supercage crown","Unknown knot-bundle reactor","Unknown phyllotaxis vortex","Unknown ruled singularity fan","Unknown prime-lobed lantern","Unknown aperiodic orbit nest","Unknown superformula organism","Unknown knot lattice 13","Unknown golden spiral skeleton","Unknown twisted ruled shell"};return n[id%unknownLabCount()];}
Mesh3 unknownLabObject(unsigned id,unsigned quality,float phase){id%=unknownLabCount();quality=std::clamp(quality,20u,96u);switch(id){case 0:return labSuperCage(quality,phase,0);case 1:return labKnotBundle(quality,phase,7);case 2:return labPhyllo(quality,phase,2);case 3:return labRuled(quality,phase,3);case 4:return labStarCage(quality,phase,4);case 5:return labKnotBundle(quality,phase,17);case 6:return labSuperCage(quality,phase,18);case 7:return labKnotBundle(quality,phase,19);case 8:return labPhyllo(quality,phase,20);default:return labRuled(quality,phase,21);}}
}
