#include "advanced.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <numbers>
using namespace dmt;using namespace dmt::advanced;
struct Pixel{unsigned char r,g,b;};
static Pixel rgb(double r,double g,double b){auto cv=[](double x){return (unsigned char)std::lround(std::clamp(x,0.,1.)*255);};return {cv(r),cv(g),cv(b)};}
static double hash(double x,double y){uint64_t h=mix64((uint64_t)std::llround((x+1000)*1024)^mix64((uint64_t)std::llround((y+1000)*1024)));return double(h>>11)*(1./9007199254740992.);}
static Pixel render(double x,double y,double time,int mode){
 double r=std::hypot(x,y),a=std::atan2(y,x);
 // Coordinate-chart aperture: screen is Euclidean; inside is an H3 raytraced scene.
 double aperture=.48+.055*std::sin(time*.7+7*a);
 double feather=smoothstep(aperture-.018,aperture+.018,r);
 double ornament=std::sin(14*a+time+3*std::log(r+.01))*std::cos(24*r-time*.4);
 Pixel exterior=rgb(.12+.24*(ornament+.8),.06+.16*(1+ornament),.25+.24*(1-ornament));
 if(mode==1)feather=0;
 if(mode==2)feather=1;
 if(feather>=1)return exterior;
 // Camera at H3 origin, geodesic rays and exact sphere intersection.
 Vec3 dir=normalized({x*.92,y*.92,1.25});V4 o={1,0,0,0},d=tangentAtOrigin(dir);
 double nearest=std::numeric_limits<double>::infinity();int material=-1;V4 center{};
 for(int i=0;i<56;i++){
  double phi=i*2.399963229728653;double depth=1.7+(i%8)*.44;
  double radius=.16+.035*(i%4);
  Vec3 p={.92*std::cos(phi+time*.09),.92*std::sin(phi+time*.09),depth};
  Sphere s{h3Point(p.x,p.y,p.z),radius};double t=intersectSphere(o,d,s);
  if(t<nearest){nearest=t;material=i;center=s.center;}
 }
 Pixel interior;
 if(material<0){double lattice=std::pow(std::abs(std::sin(15*a+8*std::log(r+.02)-time)),18.);interior=rgb(.025+.14*lattice,.07+.3*lattice,.13+.52*lattice);}
 else {
  V4 hit=geodesicRay(o,d,nearest);Vec3 hp=poincareBall(hit),cp=poincareBall(center);
  Vec3 n=normalized(hp-cp);double diffuse=std::max(.08,dot3(n,normalized({-.5,.8,-.7})));
  double bands=.5+.5*std::sin(22*std::atan2(n.y,n.x)+time+14*n.z);
  interior=rgb((.16+.58*bands)*diffuse,(.3+.5*(1-bands))*diffuse,(.65+.3*bands)*diffuse);
 }
 double f=feather;return rgb(((1-f)*interior.r+f*exterior.r)/255.,((1-f)*interior.g+f*exterior.g)/255.,((1-f)*interior.b+f*exterior.b)/255.);
}
int main(int argc,char**argv){try{
 std::string path=argc>1?argv[1]:"advanced.ppm";int size=argc>2?std::stoi(argv[2]):640;double time=argc>3?std::stod(argv[3]):0;int mode=argc>4?std::stoi(argv[4]):0;
 if(size<64||size>2048||!std::isfinite(time)||mode<0||mode>2)throw std::runtime_error("invalid arguments: path size[64..2048] time mode[0..2]");
 std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("output open failed");out<<"P6\n"<<size<<" "<<size<<"\n255\n";
 for(int j=0;j<size;j++)for(int i=0;i<size;i++){double x=(2.*i+1-size)/size,y=(size-2.*j-1)/size;Pixel p=render(x,y,time,mode);out.write(reinterpret_cast<const char*>(&p),3);}
 if(!out)throw std::runtime_error("output write failed");std::cout<<"wrote "<<path<<"\n";
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
