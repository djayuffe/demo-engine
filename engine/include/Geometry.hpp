#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
struct V3 { float x=0,y=0,z=0; };
struct V4 { float x=0,y=0,z=0,w=0; };
struct Edge { uint32_t a=0,b=0; };
using Face = std::vector<uint32_t>;
struct Mesh3 { std::vector<V3> v; std::vector<Edge> e; };
struct Polytope4 { std::vector<V4> v; std::vector<Edge> e; std::vector<Face> faces; };
struct MeshStats { size_t vertices=0, edges=0; bool finite=true, indicesValid=true; float radius=0; };
namespace geo {
Mesh3 cube(float s=1.f);
Mesh3 uvSphere(unsigned rings=18,unsigned sectors=36,float r=1.f);
Mesh3 torus(unsigned majorN=48,unsigned minorN=16,float R=1.f,float r=.35f);
Mesh3 boySurface(unsigned uN=72,unsigned vN=36,float scale=.75f);
Mesh3 hopfFibres(unsigned fibres=32,unsigned samples=96,float projectionW=2.35f);
Mesh3 superformula(unsigned uN=96,unsigned vN=48,float m1=7.f,float m2=3.f,float n1=.35f,float n2=1.7f,float n3=1.7f);
Polytope4 tesseract(float s=1.f);
Polytope4 cell16(float s=1.f);
Polytope4 cell24(float s=1.f);
Polytope4 cell600();
Mesh3 project4D(const Polytope4&, float angleXY,float angleZW,float cameraW=3.2f,float angleXW=0.f,float angleYZ=0.f);
Mesh3 sliceFaces4D(const Polytope4&, V4 normal,float h,float eps=1e-5f);
Mesh3 gyroidIsosurface(unsigned n=24,float iso=0.f);
MeshStats stats(const Mesh3&);
bool validate(const Mesh3&,std::string* why=nullptr);
bool validate(const Polytope4&,std::string* why=nullptr);
}
