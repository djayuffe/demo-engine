// Hello demo: the smallest useful engine demo. A slowly tumbling cube-in-cube with a ticker.
// Copy this directory to start a new demo (see docs/WRITING_A_DEMO.md).
#include "Engine.hpp"
#include <cmath>
namespace {
class HelloDemo final : public de::Demo {
public:
  de::Config config() const override {
    de::Config c;
    c.title="Hello Demo";c.version="1.0";c.binaryName="hello_demo";c.logoShow=false;c.fxAmount=0.f;   // clean wires: no art background or warps
    return c;
  }
  bool update(const de::FrameInfo& f,const de::Input&,de::Frame& out) override {
    const float t=float(f.time);
    Mesh3 outer=geo::cube(1.f),inner=geo::cube(.5f);
    // Merge the two cubes into one mesh.
    out.mesh=outer;
    const uint32_t base=uint32_t(out.mesh.v.size());
    for(auto p:inner.v)out.mesh.v.push_back(p);
    for(auto e:inner.e)out.mesh.e.push_back({e.a+base,e.b+base});
    // Tumble every vertex.
    const float ay=t*.6f,ax=t*.4f;
    for(auto& p:out.mesh.v){
      float x=std::cos(ay)*p.x+std::sin(ay)*p.z,z=-std::sin(ay)*p.x+std::cos(ay)*p.z;
      float y=std::cos(ax)*p.y-std::sin(ax)*z;z=std::sin(ax)*p.y+std::cos(ax)*z;
      p={x,y,z};
    }
    out.scale=.7f*(1.f+.08f*f.sync.pulse);   // pulse on the beat
    out.lineWidth=1.f+f.sync.pulse;
    out.ticker="HELLO DEMO ENGINE  -  a cube in a cube  *  edit demos/hello/main.cpp";
    return true;
  }
};
}  // namespace
int main(int argc,char** argv){HelloDemo d;return de::run(d,argc,argv);}
