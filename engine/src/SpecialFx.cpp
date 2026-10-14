#include "SpecialFx.hpp"
#include "GLUtil.hpp"
#include <algorithm>
#include <cmath>
using glutil::linkProgram;
using glutil::readText;

namespace {
void makeTarget(unsigned& tex, unsigned& fbo, int w, int h) {
  if (!tex) glGenTextures(1, &tex);
  if (!fbo) glGenFramebuffers(1, &fbo);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
}
bool complete() { return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE; }
}  // namespace

bool SpecialFx::init(const std::string& dir, std::string& err) {
  const std::string vs = readText(dir + "/post.vert");
  const std::string copyFs = readText(dir + "/fxcopy.frag"), memFs = readText(dir + "/memory.frag"),
                    specFs = readText(dir + "/spec.frag");
  if (vs.empty() || copyFs.empty() || memFs.empty() || specFs.empty()) {
    err = "cannot read special-effect shaders from " + dir;
    return false;
  }
  copyProg_ = linkProgram(vs, copyFs, err);
  memProg_ = copyProg_ ? linkProgram(vs, memFs, err) : 0;
  specProg_ = memProg_ ? linkProgram(vs, specFs, err) : 0;
  if (!specProg_) return false;
  uCopySrc_ = glGetUniformLocation(copyProg_, "uSrc");
  uMemPrev_ = glGetUniformLocation(memProg_, "uPrev");
  uMemScene_ = glGetUniformLocation(memProg_, "uScene");
  uMemTime_ = glGetUniformLocation(memProg_, "uTime");
  uMemDt_ = glGetUniformLocation(memProg_, "uDt");
  uMemAspect_ = glGetUniformLocation(memProg_, "uAspect");
  uMemTexel_ = glGetUniformLocation(memProg_, "uTexel");
  uScene_ = glGetUniformLocation(specProg_, "uScene");
  uHist_ = glGetUniformLocation(specProg_, "uHist");
  uMem_ = glGetUniformLocation(specProg_, "uMem");
  uAspect_ = glGetUniformLocation(specProg_, "uAspect");
  uTime_ = glGetUniformLocation(specProg_, "uTime");
  uTemporal_ = glGetUniformLocation(specProg_, "uTemporal");
  uMemory_ = glGetUniformLocation(specProg_, "uMemory");
  uHead_ = glGetUniformLocation(specProg_, "uHead");
  uFilled_ = glGetUniformLocation(specProg_, "uFilled");
  uInterval_ = glGetUniformLocation(specProg_, "uInterval");
  uTexel_ = glGetUniformLocation(specProg_, "uTexel");
  glutil::setupFullscreenVao(vao_, vbo_);
  return true;
}

bool SpecialFx::resize(int width, int height, std::string& err) {
  width = std::max(width, kCols * 2);
  height = std::max(height, kCols * 2);
  if (width == w_ && height == h_ && outFbo_) return true;
  w_ = width;
  h_ = height;
  memW_ = std::max(2, w_ / 2);
  memH_ = std::max(2, h_ / 2);
  makeTarget(outTex_, outFbo_, w_, h_);
  bool ok = complete();
  makeTarget(histTex_, histFbo_, w_, h_);
  ok = ok && complete();
  for (int i = 0; i < 2; ++i) {
    makeTarget(memTex_[i], memFbo_[i], memW_, memH_);
    ok = ok && complete();
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  histWarm_ = false;
  memDirty_ = false;
  if (!ok) err = "special-effect framebuffer incomplete";
  return ok;
}

void SpecialFx::capture(int tile, unsigned sceneTex) {
  const int tw = w_ / kCols, th = h_ / kCols;
  glBindFramebuffer(GL_FRAMEBUFFER, histFbo_);
  glViewport((tile % kCols) * tw, (tile / kCols) * th, tw, th);
  glUseProgram(copyProg_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sceneTex);
  if (uCopySrc_ >= 0) glUniform1i(uCopySrc_, 0);
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void SpecialFx::clearMemory() {
  for (int i = 0; i < 2; ++i) {
    glBindFramebuffer(GL_FRAMEBUFFER, memFbo_[i]);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  memDirty_ = false;
}

unsigned SpecialFx::apply(unsigned sceneTex, float time, float aspect, float temporal, float memory) {
  temporal = std::clamp(temporal, 0.f, 1.f);
  memory = std::clamp(memory, 0.f, 1.f);
  const bool wantT = temporal > .002f, wantM = memory > .002f;
  float dt = time - lastTime_;
  if (!(dt > 0.f) || dt > .25f) dt = 1.f / 60.f;   // first frame, a time jump or a loop: assume one frame
  lastTime_ = time;
  if (!specProg_ || !outFbo_) return sceneTex;
  if (!wantT && !wantM) {                           // nothing active: free, and forget stale state
    histWarm_ = false;
    if (memDirty_) clearMemory();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return sceneTex;
  }
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  if (wantT) {
    if (!histWarm_) {                               // fill the whole ring with the current frame: no black history
      for (int i = 0; i < kTiles; ++i) capture(i, sceneTex);
      head_ = 0;
      filled_ = 1;
      nextCapture_ = time + kInterval;
      histWarm_ = true;
    } else {
      if (time > nextCapture_ + 1.f) nextCapture_ = time;   // fell far behind: do not replay a backlog
      while (time >= nextCapture_) {
        head_ = (head_ + 1) % kTiles;
        capture(head_, sceneTex);
        filled_ = std::min(filled_ + 1, kTiles);
        nextCapture_ += kInterval;
      }
    }
  }
  if (wantM) {
    const int src = memIdx_, dst = 1 - memIdx_;
    glBindFramebuffer(GL_FRAMEBUFFER, memFbo_[dst]);
    glViewport(0, 0, memW_, memH_);
    glUseProgram(memProg_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, memTex_[src]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    if (uMemPrev_ >= 0) glUniform1i(uMemPrev_, 0);
    if (uMemScene_ >= 0) glUniform1i(uMemScene_, 1);
    if (uMemTime_ >= 0) glUniform1f(uMemTime_, time);
    if (uMemDt_ >= 0) glUniform1f(uMemDt_, dt);
    if (uMemAspect_ >= 0) glUniform1f(uMemAspect_, aspect);
    if (uMemTexel_ >= 0) glUniform2f(uMemTexel_, 1.f / float(memW_), 1.f / float(memH_));
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    memIdx_ = dst;
    memDirty_ = true;
  } else if (memDirty_) {
    clearMemory();
  }
  glBindFramebuffer(GL_FRAMEBUFFER, outFbo_);
  glViewport(0, 0, w_, h_);
  glUseProgram(specProg_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sceneTex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, histTex_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, memTex_[memIdx_]);
  if (uScene_ >= 0) glUniform1i(uScene_, 0);
  if (uHist_ >= 0) glUniform1i(uHist_, 1);
  if (uMem_ >= 0) glUniform1i(uMem_, 2);
  if (uAspect_ >= 0) glUniform1f(uAspect_, aspect);
  if (uTime_ >= 0) glUniform1f(uTime_, time);
  if (uTemporal_ >= 0) glUniform1f(uTemporal_, wantT ? temporal : 0.f);
  if (uMemory_ >= 0) glUniform1f(uMemory_, wantM ? memory : 0.f);
  if (uHead_ >= 0) glUniform1i(uHead_, head_);
  if (uFilled_ >= 0) glUniform1f(uFilled_, float(filled_));
  if (uInterval_ >= 0) glUniform1f(uInterval_, kInterval);
  if (uTexel_ >= 0) glUniform2f(uTexel_, 1.f / float(w_), 1.f / float(h_));
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return outTex_;
}

void SpecialFx::shutdown() {
  if (copyProg_) glDeleteProgram(copyProg_);
  if (memProg_) glDeleteProgram(memProg_);
  if (specProg_) glDeleteProgram(specProg_);
  if (vao_) glDeleteVertexArrays(1, &vao_);
  if (vbo_) glDeleteBuffers(1, &vbo_);
  if (outTex_) glDeleteTextures(1, &outTex_);
  if (outFbo_) glDeleteFramebuffers(1, &outFbo_);
  if (histTex_) glDeleteTextures(1, &histTex_);
  if (histFbo_) glDeleteFramebuffers(1, &histFbo_);
  for (int i = 0; i < 2; ++i) {
    if (memTex_[i]) glDeleteTextures(1, &memTex_[i]);
    if (memFbo_[i]) glDeleteFramebuffers(1, &memFbo_[i]);
  }
  *this = SpecialFx();
}
