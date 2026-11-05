// Hello demo: the smallest useful engine demo. A slowly tumbling cube-in-cube with a ticker.
// Copy this directory to start a new demo (see docs/WRITING_A_DEMO.md).
#include "Engine.hpp"
#include <cmath>
#include <algorithm>
namespace {
class HelloDemo final : public de::Demo {
  const std::shared_ptr<const Mesh3> base_=[] {
    auto mesh=std::make_shared<Mesh3>(geo::cube(1.f));
    const auto inner=geo::cube(.5f);
    const uint32_t base=uint32_t(mesh->v.size());
    for(auto p:inner.v)mesh->v.push_back(p);
    for(auto e:inner.e)mesh->e.push_back({e.a+base,e.b+base});
    return mesh;
  }();
public:
  de::Config config() const override {
    de::Config c;
    c.title="Hello Demo";c.version="1.0";c.binaryName="hello_demo";c.logoShow=false;c.fxAmount=0.f;   // clean wires: no art background or warps
    return c;
  }
  bool update(const de::FrameInfo& f,const de::Input&,de::Frame& out) override {
    const float t=float(f.effectTime);
    out.retainedMesh=base_;
    out.revision=1;
    // The model matrix transforms resident vertices in the shader.
    const float ay=t*.6f,ax=t*.4f,cy=std::cos(ay),sy=std::sin(ay),cx=std::cos(ax),sx=std::sin(ax);
    const float rotation[9]={cy,0,sy, sx*sy,cx,-sx*cy, -cx*sy,sx,cx*cy};
    std::copy(rotation,rotation+9,out.modelTransform);
    out.scale=.7f*(1.f+.08f*f.sync.pulse);   // pulse on the beat
    out.lineWidth=1.f+f.sync.pulse;
    out.ticker="HELLO DEMO ENGINE  -  a cube in a cube  *  edit demos/hello/main.cpp";
    return true;
  }
};
}  // namespace
int main(int argc,char** argv){HelloDemo d;return de::run(d,argc,argv);}
