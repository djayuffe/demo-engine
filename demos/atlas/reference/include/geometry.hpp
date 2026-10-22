#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
namespace dmt {
constexpr double pi=3.14159265358979323846;
struct V2 {double x{},y{}; V2 operator+(V2 b)const{return{x+b.x,y+b.y};} V2 operator-(V2 b)const{return{x-b.x,y-b.y};} V2 operator*(double s)const{return{x*s,y*s};} V2 operator/(double s)const{return{x/s,y/s};}};
inline double dot(V2 a,V2 b){return a.x*b.x+a.y*b.y;} inline double norm2(V2 a){return dot(a,a);} inline double norm(V2 a){return std::sqrt(norm2(a));}
inline V2 rotate(V2 p,double a){return {std::cos(a)*p.x-std::sin(a)*p.y,std::sin(a)*p.x+std::cos(a)*p.y};}
inline V2 mobius(V2 a,V2 b){if(norm2(a)>=1||norm2(b)>=1)throw std::domain_error("outside disk");double ab=dot(a,b),aa=norm2(a),bb=norm2(b);double den=1+2*ab+aa*bb;return ((a*(1+2*ab+bb))+(b*(1-aa)))/den;}
inline double diskDistance(V2 a,V2 b){if(norm2(a)>=1||norm2(b)>=1)throw std::domain_error("outside disk");return std::acosh(std::max(1.,1+2*norm2(a-b)/((1-norm2(a))*(1-norm2(b)))));}
struct Geodesic {bool diameter{};V2 center{};double radius{};V2 normal{};};
inline Geodesic geodesic(V2 a,V2 b){if(norm2(a)>=1||norm2(b)>=1)throw std::domain_error("outside disk");double cross=a.x*b.y-a.y*b.x;if(std::abs(cross)<1e-12){V2 n={-a.y,a.x};if(norm(n)<1e-12)n={-b.y,b.x};if(norm(n)<1e-12)throw std::domain_error("coincident origin");return {true,{},0,n/norm(n)};}double A=(norm2(a)+1)/2,B=(norm2(b)+1)/2;V2 c={(A*b.y-a.y*B)/cross,(a.x*B-A*b.x)/cross};return {false,c,std::sqrt(std::max(0.,norm2(c)-1)),{}};}
inline V2 reflect(V2 p,Geodesic g){if(g.diameter)return p-g.normal*(2*dot(p,g.normal));V2 q=p-g.center;return g.center+q*(g.radius*g.radius/norm2(q));}
inline double polygonDiskRadius(int p,int q){if(p<3||q<3||(p-2)*(q-2)<=4)throw std::domain_error("not hyperbolic");double R=std::acosh((1/std::tan(pi/p))*(1/std::tan(pi/q)));return std::tanh(R/2);}
using V4=std::array<double,4>;
inline double lorentz(V4 a,V4 b){return -a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3];}
inline V4 h3Point(double x,double y,double z){return {std::sqrt(1+x*x+y*y+z*z),x,y,z};}
inline double h3Distance(V4 a,V4 b){return std::acosh(std::max(1.,-lorentz(a,b)));}
inline V4 boostX(V4 a,double rapidity){double c=std::cosh(rapidity),s=std::sinh(rapidity);return {c*a[0]+s*a[1],s*a[0]+c*a[1],a[2],a[3]};}
inline V4 rotatePlane(V4 p,int i,int j,double theta){if(i<0||j>3||i>=j)throw std::domain_error("plane");double c=std::cos(theta),s=std::sin(theta),x=p[i],y=p[j];p[i]=c*x-s*y;p[j]=s*x+c*y;return p;}
inline std::array<V4,16> tesseract(){std::array<V4,16> out{};for(int i=0;i<16;i++)for(int k=0;k<4;k++)out[i][k]=(i&(1<<k))?1.:-1.;return out;}
inline std::vector<std::array<int,2>> tesseractEdges(){std::vector<std::array<int,2>> e;for(int i=0;i<16;i++)for(int k=0;k<4;k++){int j=i^(1<<k);if(i<j)e.push_back({i,j});}return e;}
// Clip an edge to the visible side w <= cameraW-nearPlane before perspective projection.
inline bool clip4DEdge(V4 &a,V4 &b,double cameraW=4,double nearPlane=0.05){double limit=cameraW-nearPlane;bool ia=a[3]<=limit,ib=b[3]<=limit;if(!ia&&!ib)return false;if(ia!=ib){double t=(limit-a[3])/(b[3]-a[3]);V4 p;for(int k=0;k<4;k++)p[k]=a[k]+t*(b[k]-a[k]);if(!ia)a=p;else b=p;}return true;}
inline std::array<double,3> project4D(V4 p,double cameraW=4){double d=cameraW-p[3];if(d<=0)throw std::domain_error("behind 4D eye");return {p[0]*cameraW/d,p[1]*cameraW/d,p[2]*cameraW/d};}
// Deterministic hash for hierarchical refinement; child ID is stable across camera motion.
inline uint64_t mix64(uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
inline uint64_t childSeed(uint64_t parent,uint32_t child){return mix64(parent^(uint64_t(child)*0x9e3779b97f4a7c15ULL));}
}
