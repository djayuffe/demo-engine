#include "geometry.hpp"
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <string>
#include <numbers>
using namespace dmt;
struct RGB{unsigned char r=5,g=7,b=18;};
struct Image{
 int w,h;std::vector<RGB> px;Image(int W,int H):w(W),h(H),px(W*H){}
 void point(int x,int y,RGB c){if(x>=0&&x<w&&y>=0&&y<h)px[y*w+x]=c;}
 void line(double ax,double ay,double bx,double by,RGB c){int n=std::max(1,(int)std::ceil(std::hypot(bx-ax,by-ay)*1.5));n=std::min(n,10000);for(int i=0;i<=n;i++){double t=double(i)/n;point((int)std::lround(ax+(bx-ax)*t),(int)std::lround(ay+(by-ay)*t),c);}}
 void save(std::string name){std::ofstream o(name,std::ios::binary);if(!o)throw std::runtime_error("cannot write output");o<<"P6\n"<<w<<" "<<h<<"\n255\n";for(auto c:px){char p[3]={(char)c.r,(char)c.g,(char)c.b};o.write(p,3);}}
};
// Reflection-based hyperbolic tiling, with quantized centroid deduplication.
// Numerical hashing is a visualization heuristic, not a canonical Coxeter group representation.
void tiling(Image& im,int ox,int oy,int sz,int p=7,int q=3){
 double r=polygonDiskRadius(p,q);std::vector<V2> base;
 for(int i=0;i<p;i++)base.push_back({r*cos(2*pi*i/p),r*sin(2*pi*i/p)});
 struct Cell{std::vector<V2> v;int depth;};
 std::vector<Cell> queue{{base,0}};std::set<std::pair<int,int>> seen;seen.insert({0,0});
 auto pos=[&](V2 v){return std::pair<double,double>{ox+sz*(0.5+v.x*0.47),oy+sz*(0.5-v.y*0.47)};};
 for(size_t k=0;k<queue.size()&&k<1700;k++){
  auto cell=queue[k]; RGB col{(unsigned char)(80+cell.depth*11%170),(unsigned char)(180-cell.depth*8%100),(unsigned char)(225-cell.depth*13%140)};
  for(int j=0;j<p;j++){
   V2 a=cell.v[j],b=cell.v[(j+1)%p];
   // Geodesic arc sampled using the orthogonal circle, choosing its short inside-disk arc.
   auto g=geodesic(a,b);V2 prev=a;
   for(int s=1;s<=18;s++){
    double t=double(s)/18;V2 v;
    if(g.diameter)v=a*(1-t)+b*t;
    else {double aa=atan2(a.y-g.center.y,a.x-g.center.x),bb=atan2(b.y-g.center.y,b.x-g.center.x);double d=remainder(bb-aa,2*pi);v={g.center.x+g.radius*cos(aa+t*d),g.center.y+g.radius*sin(aa+t*d)};}
    auto [x1,y1]=pos(prev);auto [x2,y2]=pos(v);im.line(x1,y1,x2,y2,col);prev=v;
   }
   if(cell.depth>=6||queue.size()>1550)continue;
   std::vector<V2> next;next.reserve(p);bool valid=true;V2 center{};
   for(V2 v:cell.v){V2 nv=reflect(v,g);if(!std::isfinite(nv.x)||!std::isfinite(nv.y)||norm2(nv)>=0.999999){valid=false;break;}next.push_back(nv);center=center+nv;}
   if(!valid)continue;
   center=center/double(p);
   auto key=std::pair<int,int>{(int)std::lround(center.x*100000),(int)std::lround(center.y*100000)};
   if(seen.insert(key).second)queue.push_back({std::move(next),cell.depth+1});
  }
 }
}
// Actual 4D hyperplane section: each cubic cell yields a planar polygon in the 3D slice.
// Four-dimensional cube has 8 cubic boundary cells, not just 32 edges.
using P3=std::array<double,3>;
P3 add(P3 a,P3 b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
P3 sub(P3 a,P3 b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
P3 mul(P3 a,double s){return {a[0]*s,a[1]*s,a[2]*s};}
double dot3(P3 a,P3 b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
P3 cross3(P3 a,P3 b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
P3 unit(P3 a){return mul(a,1/std::sqrt(dot3(a,a)));}
std::vector<std::vector<P3>> sliceTesseract(V4 normal,double offset,double phase){
 auto verts=tesseract();for(auto& v:verts){v=rotatePlane(v,0,3,phase);v=rotatePlane(v,1,2,phase*.71);v=rotatePlane(v,1,3,phase*.39);}
 double nl=0;for(double x:normal)nl+=x*x;nl=std::sqrt(nl);for(double& x:normal)x/=nl;offset/=nl;
 // 3D orthonormal basis of 4D hyperplane via Gram-Schmidt.
 std::array<V4,3> basis{};int count=0;
 for(int axis=0;axis<4&&count<3;axis++){
  V4 b{};b[axis]=1;double d=0;for(int j=0;j<4;j++)d+=b[j]*normal[j];for(int j=0;j<4;j++)b[j]-=d*normal[j];
  for(int k=0;k<count;k++){d=0;for(int j=0;j<4;j++)d+=b[j]*basis[k][j];for(int j=0;j<4;j++)b[j]-=d*basis[k][j];}
  d=0;for(double x:b)d+=x*x;if(d<1e-10)continue;for(double& x:b)x/=sqrt(d);basis[count++]=b;
 }
 std::vector<std::vector<P3>> faces;
 for(int fixedAxis=0;fixedAxis<4;fixedAxis++)for(int side=0;side<2;side++){
  std::vector<int> ids;for(int i=0;i<16;i++)if(((i>>fixedAxis)&1)==side)ids.push_back(i);
  std::vector<P3> pts;
  auto insert=[&](V4 x){P3 p{};for(int k=0;k<3;k++)for(int j=0;j<4;j++)p[k]+=(x[j]-offset*normal[j])*basis[k][j];for(auto q:pts)if(dot3(sub(p,q),sub(p,q))<1e-12)return;pts.push_back(p);};
  for(int a:ids)for(int b:ids)if(a<b&&std::popcount((unsigned)(a^b))==1){
   double da=-offset,db=-offset;for(int j=0;j<4;j++){da+=normal[j]*verts[a][j];db+=normal[j]*verts[b][j];}
   if(std::abs(da)<1e-10)insert(verts[a]);
   if(std::abs(db)<1e-10)insert(verts[b]);
   if(da*db<0){double t=da/(da-db);V4 v{};for(int j=0;j<4;j++)v[j]=verts[a][j]+t*(verts[b][j]-verts[a][j]);insert(v);}
  }
  if(pts.size()<3)continue;
  P3 c{};for(auto p:pts)c=add(c,p);c=mul(c,1.0/pts.size());P3 u=unit(sub(pts[0],c));P3 n{};
  for(size_t j=1;j+1<pts.size();j++){n=cross3(sub(pts[j],c),sub(pts[j+1],c));if(dot3(n,n)>1e-9)break;}
  if(dot3(n,n)<1e-9)continue;
  n=unit(n);P3 v=cross3(n,u);
  std::sort(pts.begin(),pts.end(),[&](P3 a,P3 b){P3 da=sub(a,c),db=sub(b,c);return atan2(dot3(da,v),dot3(da,u))<atan2(dot3(db,v),dot3(db,u));});faces.push_back(pts);
 }
 return faces;
}
void sliceScene(Image& im,int ox,int oy,int sz,double phase){
 auto faces=sliceTesseract({.31,.48,.25,.77},.16*sin(phase*1.3),phase);
 auto project=[&](P3 a){double z=5.8-a[2];return std::pair<double,double>{ox+sz*(.5+.26*a[0]/z*4),oy+sz*(.5-.26*a[1]/z*4)};};
 for(size_t f=0;f<faces.size();f++)for(size_t j=0;j<faces[f].size();j++){
  auto [x,y]=project(faces[f][j]);auto [xx,yy]=project(faces[f][(j+1)%faces[f].size()]);im.line(x,y,xx,yy,{(unsigned char)(130+f*11),(unsigned char)(105+f*17),(unsigned char)(240-f*10)});
 }
}
// Scale-continuous hierarchical pattern: no randomness based on current frame.
void recursiveScene(Image& im,int ox,int oy,int sz,double phase,uint64_t seed){
 for(int y=0;y<sz;y++)for(int x=0;x<sz;x++){
  double xx=(x-sz*.5)/(sz*.5),yy=(y-sz*.5)/(sz*.5);double r=hypot(xx,yy)+1e-7,a=atan2(yy,xx);
  double sum=0,amp=1;uint64_t h=seed;
  for(int level=0;level<9;level++){
   h=childSeed(h,level);double shift=double(h%100000)/100000.0*2*pi;
   double petals=6+2*(level%5);double freq=pow(1.8,level);
   double v=sin(petals*a+phase*(.25+level*.04)+shift+freq*log(r));
   sum+=amp*v;amp*=.64;
  }
  double aperture=std::exp(-16*r*r);double v=.5+.5*tanh(sum*1.5);
  im.point(ox+x,oy+y,{(unsigned char)(25+210*v),(unsigned char)(18+140*(1-v)+70*aperture),(unsigned char)(60+170*(.5+.5*sin(sum*2+phase)))});
 }
}
void foldedScene(Image& im,int ox,int oy,int sz,double phase){
 for(int y=0;y<sz;y++)for(int x=0;x<sz;x++){
  double u=(x-sz*.5)/(sz*.5),v=(y-sz*.5)/(sz*.5),acc=0;
  for(int k=0;k<7;k++){
   double a=phase*.1+k*.73,c=cos(a),s=sin(a);double xx=c*u-s*v,yy=s*u+c*v;
   u=fabs(xx)-.57;v=fabs(yy)-.49;u*=1.22;v*=1.22;
   acc+=exp(-55*std::min(u*u,v*v))*.15;
  }
  double value=std::min(1.,acc);
  im.point(ox+x,oy+y,{(unsigned char)(12+140*value),(unsigned char)(20+220*value),(unsigned char)(32+185*value)});
 }
}
int main(int argc,char** argv){
 try{
  std::string file=argc>1?argv[1]:"atlas.ppm";double phase=argc>2?std::stod(argv[2]):.7;int tile=argc>3?std::stoi(argv[3]):480;
  if(tile<64||tile>2048)throw std::runtime_error("tile must be 64..2048");
  Image im(tile*2,tile*2);
  tiling(im,0,0,tile);sliceScene(im,tile,0,tile,phase);recursiveScene(im,0,tile,tile,phase,0xD47A51);foldedScene(im,tile,tile,tile,phase);
  im.save(file);std::cout<<"wrote "<<file<<" "<<im.w<<"x"<<im.h<<"\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
