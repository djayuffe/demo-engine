#pragma once
#include <string>
// The procedural background ("art") pass: art.frag rendered at half resolution into an RGBA16F target
// that post.frag upsamples. Owns its program, framebuffer and texture; the renderer only decides WHEN
// it is needed (it is skipped while a logo card covers the screen).
class ArtPass {
public:
  struct Params {
    float beat = 0;
    float time = 0, music = 0, bpm = 132, logoVisibility = 0, fxAmount = 1;
    bool hasLogos = false, gold = false;
    float fade = 1, pointerX = 0, pointerY = 0, pulse = 0;
    int style = 0;
    int objects = 0xFFFF;  // enabled-object bitmask of that style  // 0 classic, 1 synthwave, 2 nebula, 3 mandala, 4 black hole, 5 hyperspace
  };
  bool init(const std::string& shaderDir, std::string& err);
  bool resize(int hdrWidth, int hdrHeight, std::string& err);  // target is HDR size / divisor (default 2)
  bool setDivisor(int d, std::string& err);                    // re-sizes the target; used by the frame-time governor
  // Draws the pass (using the caller's bound fullscreen-triangle VAO is NOT assumed: it binds its own).
  void render(const Params& p);
  unsigned texture() const { return tex_; }
  void shutdown();
private:
  unsigned prog_ = 0, fbo_ = 0, tex_ = 0, vao_ = 0, vbo_ = 0;
  int w_ = 0, h_ = 0, hdrW_ = 0, hdrH_ = 0, div_ = 2;
  int uBeat_ = -1, uTime_ = -1, uRes_ = -1, uMusic_ = -1, uBpm_ = -1, uHasLogo_ = -1, uLogoVis_ = -1, uGold_ = -1, uFx_ = -1, uStyle_ = -1, uPointer_ = -1, uFade_ = -1, uObjects_ = -1, uPulse_ = -1;
};
