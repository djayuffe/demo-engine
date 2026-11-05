#include "Bloom.hpp"
#include "GLUtil.hpp"
#include <algorithm>

bool Bloom::init(const std::string& dir, std::string& err) {
  const std::string vs = glutil::readText(dir + "/post.vert"), dfs = glutil::readText(dir + "/bloom_down.frag"),
                    ufs = glutil::readText(dir + "/bloom_up.frag");
  if (vs.empty() || dfs.empty() || ufs.empty()) {
    err = "cannot read bloom shaders from " + dir;
    return false;
  }
  down_ = glutil::linkProgram(vs, dfs, err);
  up_ = down_ ? glutil::linkProgram(vs, ufs, err) : 0;
  if (!up_) return false;
  dSrc_ = glGetUniformLocation(down_, "uSrc");
  dFirst_ = glGetUniformLocation(down_, "uFirst");
  uSrc_ = glGetUniformLocation(up_, "uSrc");
  uTexel_ = glGetUniformLocation(up_, "uTexel");
  uWeight_ = glGetUniformLocation(up_, "uWeight");
  glutil::setupFullscreenVao(vao_, vbo_);
  return true;
}

bool Bloom::resize(int width, int height, std::string& err) {
  bool ok = true;
  int w = std::max(1, width / 2), h = std::max(1, height / 2);
  levels_ = 0;
  for (int i = 0; i < kLevels; ++i) {
    if (!tex_[i]) glGenTextures(1, &tex_[i]);
    if (!fbo_[i]) glGenFramebuffers(1, &fbo_[i]);
    w_[i] = w;
    h_[i] = h;
    glBindTexture(GL_TEXTURE_2D, tex_[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_[i], 0);
    ok = ok && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    ++levels_;
    if (w == 1 && h == 1) break;
    w = std::max(1, w / 2);
    h = std::max(1, h / 2);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (!ok) err = "bloom framebuffer incomplete";
  return ok;
}

unsigned Bloom::apply(unsigned sceneTex) {
  if (!down_ || !tex_[0] || !levels_) return 0;
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glBindVertexArray(vao_);
  glActiveTexture(GL_TEXTURE0);
  glUseProgram(down_);
  glUniform1i(dSrc_, 0);
  unsigned src = sceneTex;
  for (int i = 0; i < levels_; ++i) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
    glViewport(0, 0, w_[i], h_[i]);
    glBindTexture(GL_TEXTURE_2D, src);
    glUniform1f(dFirst_, i == 0 ? 1.f : 0.f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    src = tex_[i];
  }
  glUseProgram(up_);
  glUniform1i(uSrc_, 0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  for (int i = levels_ - 1; i > 0; --i) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i - 1]);
    glViewport(0, 0, w_[i - 1], h_[i - 1]);
    glBindTexture(GL_TEXTURE_2D, tex_[i]);
    glUniform2f(uTexel_, 1.f / float(w_[i]), 1.f / float(h_[i]));
    glUniform1f(uWeight_, .5f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
  glDisable(GL_BLEND);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return tex_[0];
}

void Bloom::shutdown() {
  for (int i = 0; i < kLevels; ++i) {
    if (fbo_[i]) glDeleteFramebuffers(1, &fbo_[i]);
    if (tex_[i]) glDeleteTextures(1, &tex_[i]);
    fbo_[i] = tex_[i] = 0;
  }
  if (down_) glDeleteProgram(down_);
  if (up_) glDeleteProgram(up_);
  if (vbo_) glDeleteBuffers(1, &vbo_);
  if (vao_) glDeleteVertexArrays(1, &vao_);
  down_ = up_ = vbo_ = vao_ = 0;
  levels_ = 0;
}
