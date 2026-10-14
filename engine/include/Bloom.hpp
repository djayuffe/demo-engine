#pragma once
#include <string>
// Multi-scale HDR bloom: a 6-level dual-filter pyramid (Jimenez 2014), OpenGL 4.1 only. Replaces the old
// fixed 6-tap blur, so glow now spreads across the whole screen with a physically plausible falloff.
// A soft brightness knee plus Karis averaging keeps thin bright wires from flickering.
class Bloom {
public:
  bool init(const std::string& shaderDir, std::string& err);
  bool resize(int width, int height, std::string& err);
  // Builds the pyramid from sceneTex; returns the texture holding the finished bloom (half resolution).
  unsigned apply(unsigned sceneTex);
  void shutdown();
  static constexpr int kLevels = 6;
private:
  unsigned down_ = 0, up_ = 0, vao_ = 0, vbo_ = 0, tex_[kLevels] = {}, fbo_[kLevels] = {};
  int w_[kLevels] = {}, h_[kLevels] = {};
  int dSrc_ = -1, dTexel_ = -1, dFirst_ = -1, uSrc_ = -1, uTexel_ = -1, uWeight_ = -1;
};
