#include "ArtPass.hpp"
#include "GLUtil.hpp"
#include <algorithm>

bool ArtPass::init(const std::string& dir, std::string& err) {
  const std::string vs = glutil::readText(dir + "/post.vert"), fs = glutil::readText(dir + "/art.frag");
  if (vs.empty() || fs.empty()) {
    err = "cannot read art shaders from " + dir;
    return false;
  }
  prog_ = glutil::linkProgram(vs, fs, err);
  if (!prog_) return false;
  uTime_ = glGetUniformLocation(prog_, "uTime");
  uRes_ = glGetUniformLocation(prog_, "uResolution");
  uMusic_ = glGetUniformLocation(prog_, "uMusicLevel");
  uBpm_ = glGetUniformLocation(prog_, "uBpm");
  uHasLogo_ = glGetUniformLocation(prog_, "uHasLogo");
  uLogoVis_ = glGetUniformLocation(prog_, "uLogoVis");
  uGold_ = glGetUniformLocation(prog_, "uGold");
  uFx_ = glGetUniformLocation(prog_, "uFxAmount");
  uStyle_ = glGetUniformLocation(prog_, "uStyle");
  uPointer_ = glGetUniformLocation(prog_, "uPointer");
  uPulse_ = glGetUniformLocation(prog_, "uPulse");
  glutil::setupFullscreenVao(vao_, vbo_);
  return true;
}

bool ArtPass::resize(int hdrWidth, int hdrHeight, std::string& err) {
  w_ = std::max(1, hdrWidth / 2);
  h_ = std::max(1, hdrHeight / 2);
  if (!fbo_) glGenFramebuffers(1, &fbo_);
  if (!tex_) glGenTextures(1, &tex_);
  glBindTexture(GL_TEXTURE_2D, tex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w_, h_, 0, GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);
  const bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (!ok) err = "art framebuffer incomplete";
  return ok;
}

void ArtPass::render(const Params& p) {
  if (!prog_ || !fbo_) return;
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glViewport(0, 0, w_, h_);
  glUseProgram(prog_);
  glUniform1f(uTime_, p.time);
  glUniform2f(uRes_, float(w_), float(h_));
  glUniform1f(uMusic_, std::clamp(p.music, 0.f, 1.f));
  glUniform1f(uBpm_, p.bpm);
  glUniform1f(uHasLogo_, p.hasLogos ? 1.f : 0.f);
  glUniform1f(uLogoVis_, p.logoVisibility);
  glUniform1f(uGold_, p.gold ? 1.f : 0.f);
  glUniform1f(uFx_, p.fxAmount);
  glUniform2f(uPointer_, std::clamp(p.pointerX, -1.f, 1.f), std::clamp(p.pointerY, -1.f, 1.f));
  glUniform1f(uPulse_, std::clamp(p.pulse, 0.f, 1.f));
  glUniform1i(uStyle_, std::clamp(p.style, 0, 4));
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ArtPass::shutdown() {
  if (prog_) glDeleteProgram(prog_);
  if (fbo_) glDeleteFramebuffers(1, &fbo_);
  if (tex_) glDeleteTextures(1, &tex_);
  if (vbo_) glDeleteBuffers(1, &vbo_);
  if (vao_) glDeleteVertexArrays(1, &vao_);
  prog_ = fbo_ = tex_ = vbo_ = vao_ = 0;
  w_ = h_ = 0;
}
