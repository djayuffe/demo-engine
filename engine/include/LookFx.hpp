#pragma once
#include <string>
// Spec "look" effects that need only the HDR scene and the previous frame (OpenGL 4.1, one pass):
// chromatic gravity (#27), prediction failure storm (#33), colorless chromatic matter (#23),
// specular mutation (#30) and negative reflection (#24). apply() returns the input untouched, at no cost,
// when every strength is zero.
class LookFx {
public:
  struct Amounts {
    float gravity = 0, failure = 0, colorless = 0, specular = 0, negReflection = 0;
    bool any() const { return gravity > 0 || failure > 0 || colorless > 0 || specular > 0 || negReflection > 0; }
  };
  bool init(const std::string& shaderDir, std::string& err);
  bool resize(int width, int height, std::string& err);
  unsigned apply(unsigned sceneTex, float time, float aspect, const Amounts& a);
  void shutdown();
private:
  unsigned prog_ = 0, copy_ = 0, vao_ = 0, vbo_ = 0, out_ = 0, outFbo_ = 0, prev_ = 0, prevFbo_ = 0;
  int w_ = 0, h_ = 0;
  bool prevValid_ = false;
  int uScene_ = -1, uPrev_ = -1, uTexel_ = -1, uTime_ = -1, uAspect_ = -1, uGrav_ = -1, uFail_ = -1, uCol_ = -1,
      uSpec_ = -1, uNeg_ = -1, uCopySrc_ = -1;
};
