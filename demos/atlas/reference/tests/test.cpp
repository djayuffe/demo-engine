#include "geometry.hpp"
#include <cassert>
#include <iostream>
using namespace dmt;
int main(){auto close=[](double a,double b,double e=1e-9){assert(std::abs(a-b)<e);};close(diskDistance({0,0},{0,0}),0);auto x=mobius({.2,.1},{-.1,.2});assert(norm2(x)<1);auto g=geodesic({.2,.1},{-.2,.1});auto r=reflect({.2,.1},g);close(r.x,.2);close(r.y,.1);close(polygonDiskRadius(7,3),std::tanh(std::acosh(1/std::tan(pi/7)/std::tan(pi/3))/2));auto p=h3Point(.2,.3,.4);close(lorentz(p,p),-1);auto b=boostX(p,1.2);close(lorentz(b,b),-1);close(h3Distance(p,p),0,1e-7);auto t=tesseract();assert(t.size()==16);assert(tesseractEdges().size()==32);auto v=rotatePlane(t[0],0,3,pi/2);close(v[0]*v[0]+v[3]*v[3],2);V4 a={0,0,0,3},c={1,0,0,5};bool clipped=clip4DEdge(a,c);assert(clipped);close(c[3],3.95);auto projected=project4D(c);assert(std::isfinite(projected[0]));assert(childSeed(42,1)==childSeed(42,1));std::cout<<"All geometry invariants passed\n";}
