#include "advanced.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace dmt;using namespace dmt::advanced;
void require(bool ok){if(!ok)throw std::runtime_error("advanced invariant failed");}
int main(){
 V4 o{1,0,0,0},d=tangentAtOrigin({0,0,1});
 for(double t:{0.,.1,1.,2.,4.}){V4 p=geodesicRay(o,d,t);require(std::abs(lorentz(p,p)+1)<1e-9);require(std::abs(h3Distance(o,p)-t)<1e-8);}
 Sphere s{h3Point(0,0,std::sinh(2.)),.35};double hit=intersectSphere(o,d,s);require(std::abs(hit-1.65)<1e-8);
 require(!std::isfinite(intersectSphere(o,tangentAtOrigin({0,0,-1}),s)));
 V4 shifted=boostX(o,.75),dt=tangentFromOrigin(shifted,{0,0,1});require(std::abs(lorentz(shifted,dt))<1e-10);require(std::abs(lorentz(dt,dt)-1)<1e-10);
 for(double t:{.1,1.,3.})require(std::abs(lorentz(geodesicRay(shifted,dt,t),geodesicRay(shifted,dt,t))+1)<1e-9);
 require(radialLod(100,1)>radialLod(10,1));std::cout<<"advanced invariants pass\n";
}
