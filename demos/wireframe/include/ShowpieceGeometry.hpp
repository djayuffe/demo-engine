#pragma once
#include "Geometry.hpp"
// Showpiece 3D wire objects (scenes 51..56). All meshes are duplicate/self-edge free.
namespace geo {
Mesh3 geodesicDome(unsigned subdivisions=3,float r=1.4f,float bump=0.f);       // icosphere with an optional breathing bump
Mesh3 tubeTrefoil(unsigned rings=220,unsigned sides=14,float tubeR=.26f);       // trefoil knot swept as a wire tube
Mesh3 dnaHelix(unsigned steps=90,float twist=0.f);                              // double helix with base-pair rungs
Mesh3 atomOrbits(unsigned samples=96,float t=0.f);                              // nucleus, 3 tilted electron rings, electrons
Mesh3 waveTerrain(unsigned n=48,float t=0.f);                                   // rippling wire ocean grid
Mesh3 platonicCompound(float t=0.f);                                            // icosahedron + dual dodecahedron + cube
}
