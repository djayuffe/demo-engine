#pragma once
#include "Geometry.hpp"
#include <cstdint>
#include <string>
namespace geo {
enum class ImplicitKind { Gyroid, SchwarzP, SchwarzD, Neovius, IWP };
Mesh3 implicitSurface(ImplicitKind kind,unsigned n=28,float iso=0.f,float phase=0.f);
Polytope4 cell120Vertices();
Mesh3 quaternionJulia(unsigned n=30,unsigned iterations=9,float threshold=.035f,V4 c={-.2f,.7f,0.f,0.f});
Mesh3 hyperbolicBall(unsigned shells=6,unsigned spokes=18,float curvature=.82f);
Mesh3 cliffordTorus(unsigned u=48,unsigned v=24,float projectionW=2.4f);
Mesh3 lissajousKnot(unsigned samples=1200,int p=3,int q=4,int r=5,float phase=.3f);
Mesh3 strangeAttractor(unsigned samples=18000,float dt=.004f);
Mesh3 discoveredObject(uint64_t seed,unsigned u=72,unsigned v=36);
float noveltyScore(const Mesh3& m);
std::string implicitName(ImplicitKind k);
}
namespace geo {
// v4 exotic geometry expansion (25 distinct families)
Mesh3 mobiusStrip(unsigned u=160,unsigned v=20,float twists=1.f);
Mesh3 kleinBottle(unsigned u=120,unsigned v=48);
Mesh3 enneperSurface(unsigned u=80,unsigned v=36);
Mesh3 helicoidSurface(unsigned u=96,unsigned v=32);
Mesh3 catenoidSurface(unsigned u=96,unsigned v=32);
Mesh3 diniSurface(unsigned u=120,unsigned v=32);
Mesh3 pseudosphere(unsigned u=120,unsigned v=32);
Mesh3 romanSurface(unsigned u=96,unsigned v=48);
Mesh3 crossCap(unsigned u=96,unsigned v=48);
Mesh3 torusKnot(unsigned samples=1800,int p=2,int q=5,float R=0.72f,float r=.28f);
Mesh3 vivianiCurve(unsigned samples=1400);
Mesh3 sphericalSpiral(unsigned samples=1800,float turns=13.f);
Mesh3 hypotrochoidKnot(unsigned samples=1800,float a=5,float b=3,float h=5);
Mesh3 duffingAttractor(unsigned samples=18000,float dt=.01f);
Mesh3 rosslerAttractor(unsigned samples=18000,float dt=.01f);
Mesh3 thomasAttractor(unsigned samples=18000,float dt=.02f);
Mesh3 sierpinskiTetrahedron(unsigned depth=5);
enum class ExoticImplicitKind { Heart, BarthSexticLike, TangleCube, ChmutovLike, CayleyCubic, KummerLike, Goursat, BlobLattice };
Mesh3 exoticImplicit(ExoticImplicitKind kind,unsigned n=26,float phase=0.f);
std::string exoticImplicitName(ExoticImplicitKind k);
unsigned unknownLabCount();
std::string unknownLabName(unsigned id);
Mesh3 unknownLabObject(unsigned id,unsigned quality=48,float phase=0.f);
}
