#pragma once
#include "BackgroundStyle.hpp"
#include "Geometry.hpp"
#include "Effects.hpp"
#include "BeatResponse.hpp"
#include "SpecialFx.hpp"
#include "Bloom.hpp"
#include "ArtPass.hpp"
#include "LookFx.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
struct GLFWwindow;
class Renderer {
public:
  struct UploadStats { size_t vertices=0, indices=0, bytes=0; };
  const UploadStats& uploadStats() const { return uploadStats_; }
  bool init(GLFWwindow* w, const std::string& shaderDir = "shaders");
  bool upload(const Mesh3& m);
  bool draw(float time, int width, int height, float aspect, int sceneIndex = 0, float scale = 1.f,
            float lineWidth = 1.f, float musicLevel = 0.f);
  // Fullscreen beat-synced text marquee (scene name + provenance).
  bool drawScroller(float time, float beatPhase, int width, int height, const char* text, float musicLevel = 0.f);
  // Static, responsive control/status card, rendered with the resident GPU font.
  bool drawOverlay(int width, int height, const std::string& text, float opacity = 1.f);
  // Logo background: load the UBER_*_1920x1080.jpg cards from a directory.
  bool loadLogos(const std::string& dir);
  // Runtime switches: the logo cards and the procedural background can each be turned off, leaving the
  // wireframe (and the screen-space look) over black.
  void setShowLogos(bool on) { showLogos_ = on; }
  bool showLogos() const { return showLogos_; }
  void setShowBackground(bool on) { showBackground_ = on; }
  bool showBackground() const { return showBackground_; }
  void setBackgroundStyle(int s) { bgStyle_ = bgstyle::wrap(s); }  // 0 classic, 1 synthwave, 2 nebula, 3 mandala, 4 blackhole
  int backgroundStyle() const { return bgStyle_; }
  // Pointer in -1..1 (y up) and a 0..1 click pulse: the backgrounds react to it, the depth layer is pushed by it.
  void setPointer(float x, float y, float pulse) { ptrX_ = x; ptrY_ = y; ptrPulse_ = pulse; }   // effects plane (wires, motes)
  void setBgPointer(float x, float y) { bgPtrX_ = x; bgPtrY_ = y; }                             // background: its own gain
  void setArtDivisor(int d) { art_.setDivisor(d, error_); }   // background resolution = HDR size / d
  bgstyle::Objects& objects() { return objects_; }   // per-style object switches (F1..F12)
  void setShowGloss(bool on) { showGloss_ = on; }
  bool showGloss() const { return showGloss_; }
  void setCut(float in, float out) { cutIn_ = std::clamp(in, 0.f, 1.f); cutOut_ = std::clamp(out, 0.f, 1.f); }
  void setShowDepth(bool on) { showDepth_ = on; }
  bool showDepth() const { return showDepth_; }
  int logoCount() const { return (int)logos_.size(); }
  // Draw a full-screen logo backdrop behind the wireframe (crossfades on
  // scene change, subtle Ken Burns). logoComposite=true renders into the HDR
  // capture buffer (over the wireframe, normal alpha blend) instead of the
  // screen — used in logo mode so the post/FX pass sees the logo too.
  bool drawBackground(float time, int width, int height, int sceneIndex, float musicLevel = 0.f,
                      bool logoComposite = false);
  // Draw the traveling objects moving back and forth through the wireframe.
  bool drawTravelers(float time, int width, int height, float aspect, float scale, float musicLevel = 0.f);
  // Logo mode: composite the fullscreen logo over the captured wireframe frame
  // and run the post/FX pass. Called after drawTravelers in the logo path.
  // bpm syncs the synthetic beat pulse in the post shader to the real tempo.
  bool finishLogoFrame(float time, int width, int height, int sceneIndex, float musicLevel = 0.f, float bpm = 132.f);
  // Re-create all GL objects after a context loss (GPU reset / driver crash).
  // The caller has already re-makethe-context-current; this just rebuilds
  // programs, VAOs, buffers, textures, and the HDR FBO. Logo textures are
  // re-loaded by the caller (loadLogos) since they need the image data.
  bool reinit(const std::string& shaderDir);
  // Save the current default-framebuffer contents (the finished frame, after
  // the post pass + scroller) as an 8-bit PNG via glReadPixels + stb_image_write.
  // Used for headless/automated visual verification. Returns false (with
  // error_ set) on GL or write failure.
  bool screenshot(const std::string& path, int width, int height);
  void shutdown();
  // Render the HDR target (3D wireframe + travelers + post/FX) at a fraction of
  // the screen resolution to ease GPU load on weak hardware. The post pass then
  // upscales to the full-screen default framebuffer. 1.0 = native (default).
  void setRenderScale(float s) { renderScale_ = std::clamp(s, 0.25f, 1.0f); }
  float renderScale() const { return renderScale_; }
  // Pixel budget for the HDR target (0 = unlimited): the scale is capped so the target never exceeds it.
  void setMaxHdrPixels(double px) { maxHdrPixels_ = px; }
  // When true, the screen-space FX post pass is skipped entirely: the captured
  // HDR frame is blitted straight to the screen. Faster, useful for debugging.
  void setPostEnabled(bool on) { postEnabled_ = on; }
  bool postEnabled() const { return postEnabled_; }
  // 0..1 strength of the signature post look (art background, wild warps, lensing...). Default 1.
  void setFxAmount(float a) { fxAmount_ = std::clamp(a, 0.f, 1.f); }
  // Wire look: 0 = the default electric palette, 1 = golden glimmer (no depth test, so orbit clouds
  // accumulate light additively; gold twinkle in wire.frag; gold cross glints in post.frag).
  void setWireStyle(int s) { wireStyle_ = std::clamp(s, 0, 1); }
  void setBeatDrive(const rhythm::Drive& beat) { beat_ = beat; }
  void setGpuWave(const float wave[4]) { std::copy(wave, wave + 4, gpuWave_); }
  void setGpuEffect(const GpuDeformation& effect) { gpuEffect_ = effect; }
  // Model-space transform applied to the wireframe and travelers before the camera: a row-major 3x3
  // (identity by default). The noncommutative-camera effect drives it with products of shears.
  void setModelTransform(const float m3[9]) { std::copy(m3, m3 + 9, modelShear_); }
  // 0..1 strength of the temporal-refraction sphere (a glass sphere showing the scene from other moments).
  void setTemporalRefraction(float a) { temporalAmt_ = std::clamp(a, 0.f, 1.f); }
  // 0..1 strength of the persistent memory layer (advected, diffused, hue-ageing ghosts of past frames).
  void setMemory(float a) { memoryAmt_ = std::clamp(a, 0.f, 1.f); }
  // Five 0..1 look strengths: chromatic gravity, prediction failure, colorless matter, specular mutation, negative reflection.
  void setLook(const float a[5]) {
    look_ = {std::clamp(a[0], 0.f, 1.f), std::clamp(a[1], 0.f, 1.f), std::clamp(a[2], 0.f, 1.f), std::clamp(a[3], 0.f, 1.f),
             std::clamp(a[4], 0.f, 1.f)};
  }
  const std::string& error() const { return error_; }

private:
  bool drawText(float time, float phase, int width, int height, const char* text, float music,
                bool panel, float opacity);
  int uScPanel_ = -1;
  UploadStats uploadStats_;
  rhythm::Drive beat_;
  int uPostBeatDrive_=-1, uTravelerBeat_=-1;
  void bindWireProgram();
  void drawWireMesh();
  unsigned tessProgram_ = 0, activeWireProgram_ = 0;
  float setWireWidth(float lineWidth, float music) const;   // returns the effective width in pixels
  void cameraMvp(float t, float aspect, float scale, float musicLevel, float* M) const;
  bool createGl(const std::string& shaderDir);
  bool initPost(const std::string& shaderDir);
  bool resizeHdr(int width, int height);
  bool initScroller(const std::string& shaderDir);
  bool initBackground(const std::string& shaderDir);
  bool initTravelers(const std::string& shaderDir);
  unsigned vao_ = 0, vbo_ = 0, ebo_ = 0, program_ = 0, postProgram_ = 0, postVao_ = 0, postVbo_ = 0, hdrFbo_ = 0,
           hdrTex_ = 0, depthRbo_ = 0, scrollerProgram_ = 0, scrollerVao_ = 0, scrollerVbo_ = 0, bgProgram_ = 0,
           bgVao_ = 0, bgVbo_ = 0, bgTexA_ = 0, bgTexB_ = 0, travelerProgram_ = 0, travelerVao_ = 0, travelerVbo_ = 0,
           travelerEbo_ = 0;
  int edgeCount_ = 0, hdrW_ = 0, hdrH_ = 0;
  size_t vboCapacity_ = 0, eboCapacity_ = 0;
  std::vector<Edge> uploadedEdges_;
  std::vector<V3> uploadedVertices_;
  GpuDeformation gpuEffect_;
  int uGpuEffect_=-1, uGpuCenter_=-1, uGpuSeed_=-1;
  void submitGpuEffect();
  float gpuWave_[4] = {0, 3, 1, 0};
  int uGpuWave_ = -1;
  int uMVP_ = -1, uTime_ = -1, uColor_ = -1, uGain_ = -1, uMusicLevel_ = -1, uPostScene_ = -1, uPostTime_ = -1,
      uPostResolution_ = -1, uPostMusic_ = -1, uPostBpm_ = -1, uPostBypass_ = -1, uPostHasLogo_ = -1,
      uPostLogoVis_ = -1, uPostFx_ = -1;
  int uScTime_ = -1, uScPhase_ = -1, uScRes_ = -1, uScMusic_ = -1, uScOffset_ = -1, uScTextWidth_ = -1, uScFont_ = -1,
      uScCodes_ = -1, uScCount_ = -1, uScOpacity_ = -1;
  unsigned scrollerFontTex_ = 0, scrollerCodesTex_ = 0;
  int scrollerCodesTexW_ = 0;
  int travelerEdgeCount_ = 0;
  int uBgMix_ = -1, uBgZoom_ = -1, uBgAspect_ = -1, uBgTexAspect_ = -1, uBgOp_ = -1, uBgTime_ = -1, uBgMusic_ = -1,
      uBgTexA_ = -1, uBgTexB_ = -1;
  int uTrMVP_ = -1, uTrTime_ = -1, uTrColor_ = -1, uTrSpeed_ = -1;
  bool hasLogos_ = false;
  bool showLogos_ = true, showBackground_ = true;
  int bgStyle_ = 0;               // requested style
  bgstyle::Fade styleFade_;       // style actually on screen (lags the request by a fade-through-black)
  float lastArtT_ = -1.f;
  bool showDepth_ = true, showGloss_ = true;
  float cutIn_ = 0.f, cutOut_ = 0.f;
  bgstyle::Objects objects_;
  float ptrX_ = 0, ptrY_ = 0, ptrPulse_ = 0, bgPtrX_ = 0, bgPtrY_ = 0;
  bool logosActive() const { return hasLogos_ && showLogos_; }
  bool logoCapturePending_ = false;
  float renderScale_ = 1.f;
  double maxHdrPixels_ = 0;
  bool postEnabled_ = true;
  float fxAmount_ = 1.f;
  int wireStyle_ = 0;
  int uStyle_ = -1, uPostGold_ = -1, uPostBgStyle_ = -1, uPostPtr_ = -1, uPostPulse_ = -1, uPostDepth_ = -1, uPostGloss_ = -1, uPostCut_ = -1;
  // Art pass (procedural background) at half resolution, see shaders/art.frag.
  ArtPass art_;
  int uPostArt_ = -1, uPostArtOn_ = -1;
  SpecialFx fx_;
  Bloom bloom_;
  bool wideWires_ = false;
  int uViewport_ = -1, uWidthPx_ = -1;
  LookFx lookFx_;
  LookFx::Amounts look_;
  bool lookOk_ = false;
  int uPostBloom_ = -1;
  bool bloomOk_ = false;
  float modelShear_[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  float temporalAmt_ = 0.f, memoryAmt_ = 0.f;
  std::string error_;
  struct Logo {
    unsigned tex = 0;
    int w = 0, h = 0;
  };
  std::vector<Logo> logos_;
  int bgCurrent_ = -1;       // logo index currently shown in slot A
  int bgTarget_ = -1;        // logo index fading in (slot B)
  float bgMix_ = 0.f;        // 0=A, 1=B
  float logoVis_ = 1.f;      // 0..1 how visible the logo card is right now (fade in / hold / fade out / black)
  double bgLastTime_ = -1.0; // last drawBackground time (for frame-rate-independent fade)
};
