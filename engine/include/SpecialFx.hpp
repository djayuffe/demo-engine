#pragma once
#include <string>
// Pre-post compositing passes for two effects from the experimental-effects spec, built for OpenGL 4.1
// (no compute, no SSBOs, no texture arrays, so they also run on macOS and need nothing new from the
// Windows loader):
//   * temporal refraction: a glass sphere that shows the scene from a different moment at each pixel,
//     fed by a ring of 16 quarter-resolution HDR frames packed in a 4x4 atlas;
//   * persistent memory: a ping-pong feedback layer that advects, diffuses, hue-ages and decays what
//     the scene drew, so wires leave a slowly evolving ghost.
// apply() returns the texture the post pass should read: the scene itself (zero cost) when both
// effects are off.
class SpecialFx {
public:
  bool init(const std::string& shaderDir, std::string& err);
  bool resize(int width, int height, std::string& err);   // HDR target size
  unsigned apply(unsigned sceneTex, float time, float aspect, float temporal, float memory);
  void shutdown();
private:
  void capture(int tile, unsigned sceneTex);
  void clearMemory();
  static constexpr int kTiles = 16, kCols = 4;
  static constexpr float kInterval = .25f;
  unsigned copyProg_ = 0, memProg_ = 0, specProg_ = 0, vao_ = 0, vbo_ = 0;
  unsigned outTex_ = 0, outFbo_ = 0, histTex_ = 0, histFbo_ = 0, memTex_[2] = {0, 0}, memFbo_[2] = {0, 0};
  int w_ = 0, h_ = 0, memW_ = 0, memH_ = 0;
  int memIdx_ = 0, head_ = 0, filled_ = 1;
  bool histWarm_ = false, memDirty_ = false;
  float lastTime_ = 0.f, nextCapture_ = 0.f;
  // uniform locations
  int uCopySrc_ = -1;
  int uMemPrev_ = -1, uMemScene_ = -1, uMemTime_ = -1, uMemDt_ = -1, uMemAspect_ = -1, uMemTexel_ = -1;
  int uScene_ = -1, uHist_ = -1, uMem_ = -1, uAspect_ = -1, uTime_ = -1, uTemporal_ = -1, uMemory_ = -1,
      uHead_ = -1, uFilled_ = -1, uInterval_ = -1, uTexel_ = -1;
};
