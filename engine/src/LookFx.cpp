#include "LookFx.hpp"
#include "GLUtil.hpp"
#include <algorithm>

namespace {
void target(unsigned& tex, unsigned& fbo, int w, int h) {
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
}  // namespace

bool LookFx::init(const std::string& dir, std::string& err) {
  const std::string vs = glutil::readText(dir + "/post.vert"), fs = glutil::readText(dir + "/lookfx.frag"),
                    cs = glutil::readText(dir + "/fxcopy.frag");
  if (vs.empty() || fs.empty() || cs.empty()) {
    err = "cannot read look-effect shaders from " + dir;
    return false;
  }
  prog_ = glutil::linkProgram(vs, fs, err);
  copy_ = prog_ ? glutil::linkProgram(vs, cs, err) : 0;
  if (!copy_) return false;
  uScene_ = glGetUniformLocation(prog_, "uScene");
  uPrev_ = glGetUniformLocation(prog_, "uPrev");
  uTexel_ = glGetUniformLocation(prog_, "uTexel");
  uTime_ = glGetUniformLocation(prog_, "uTime");
  uAspect_ = glGetUniformLocation(prog_, "uAspect");
  uGrav_ = glGetUniformLocation(prog_, "uGrav");
  uFail_ = glGetUniformLocation(prog_, "uFail");
  uCol_ = glGetUniformLocation(prog_, "uColorless");
  uSpec_ = glGetUniformLocation(prog_, "uSpec");
  uNeg_ = glGetUniformLocation(prog_, "uNegRefl");
  uCopySrc_ = glGetUniformLocation(copy_, "uSrc");
  glutil::setupFullscreenVao(vao_, vbo_);
  return true;
}

bool LookFx::resize(int width, int height, std::string& err) {
  width = std::max(width, 2);
  height = std::max(height, 2);
  if (width == w_ && height == h_ && outFbo_) return true;
  w_ = width;
  h_ = height;
  target(out_, outFbo_, w_, h_);
  bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  target(prev_, prevFbo_, w_, h_);
  ok = ok && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  prevValid_ = false;
  if (!ok) err = "look-effect framebuffer incomplete";
  return ok;
}

unsigned LookFx::apply(unsigned sceneTex, float time, float aspect, const Amounts& a) {
  if (!prog_ || !outFbo_ || !a.any()) {
    prevValid_ = false;  // the next activation must not diff against a stale frame
    return sceneTex;
  }
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glBindVertexArray(vao_);
  glViewport(0, 0, w_, h_);
  if (!prevValid_) {  // first active frame: predict "no change"
    glBindFramebuffer(GL_FRAMEBUFFER, prevFbo_);
    glUseProgram(copy_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glUniform1i(uCopySrc_, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    prevValid_ = true;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, outFbo_);
  glUseProgram(prog_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sceneTex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, prev_);
  glUniform1i(uScene_, 0);
  glUniform1i(uPrev_, 1);
  glUniform2f(uTexel_, 1.f / float(w_), 1.f / float(h_));
  glUniform1f(uTime_, time);
  glUniform1f(uAspect_, aspect);
  glUniform1f(uGrav_, a.gravity);
  glUniform1f(uFail_, a.failure);
  glUniform1f(uCol_, a.colorless);
  glUniform1f(uSpec_, a.specular);
  glUniform1f(uNeg_, a.negReflection);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  // remember this frame's input as the next frame's prediction
  glBindFramebuffer(GL_FRAMEBUFFER, prevFbo_);
  glUseProgram(copy_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sceneTex);
  glUniform1i(uCopySrc_, 0);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glActiveTexture(GL_TEXTURE0);
  return out_;
}

void LookFx::shutdown() {
  if (outFbo_) glDeleteFramebuffers(1, &outFbo_);
  if (prevFbo_) glDeleteFramebuffers(1, &prevFbo_);
  if (out_) glDeleteTextures(1, &out_);
  if (prev_) glDeleteTextures(1, &prev_);
  if (prog_) glDeleteProgram(prog_);
  if (copy_) glDeleteProgram(copy_);
  if (vbo_) glDeleteBuffers(1, &vbo_);
  if (vao_) glDeleteVertexArrays(1, &vao_);
  outFbo_ = prevFbo_ = out_ = prev_ = prog_ = copy_ = vbo_ = vao_ = 0;
  w_ = h_ = 0;
  prevValid_ = false;
}
