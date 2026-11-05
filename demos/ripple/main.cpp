// Ripple demo: the example from the README, kept as a real, compiled demo so the docs cannot rot.
#include "Engine.hpp"

struct MyDemo : de::Demo {
  const std::shared_ptr<const Mesh3> base = std::make_shared<const Mesh3>(geo::torus(96, 32, 1.f, .35f));
  de::Config config() const override {
    de::Config c; c.title = "My Demo"; c.version = "1.0"; c.binaryName = "ripple_demo";
    c.fxAmount = 0.4f;                                         // dial the post look down so the torus stays readable
    return c;
  }
  bool update(const de::FrameInfo& f, const de::Input& in, de::Frame& out) override {
    out.retainedMesh = base;
    out.revision = 1;                                      // upload the static mesh once
    out.gpuWave[0] = .2f;                                  // vertex shader animates the ripple
    out.scale = 1.f + .1f * f.sync.pulse;                      // pulse on the beat
    out.ticker = "MY DEMO  -  a torus that ripples";
    return !in.pressed(de::Key::Q);                            // Q ends the demo
  }
};
int main(int argc, char** argv) { MyDemo d; return de::run(d, argc, argv); }
