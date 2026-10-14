// Proof tool for shader refactors: renders an OLD shader file and the NEW (include-expanded) one with the
// same uniforms and textures, and reports the largest per-channel difference. Not part of the default build.
//   g++ -std=c++20 -Iengine/src -Iengine/include tools/shaderdiff.cpp engine/src/GLUtil.cpp -lglfw -lGL -o shaderdiff
//   xvfb-run -a ./shaderdiff old/art.frag engine/shaders/art.frag engine/shaders/post.vert
#include "GLUtil.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main(int argc, char** argv) {
  if (argc < 4) return 2;
  glfwInit();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* w = glfwCreateWindow(64, 64, "d", nullptr, nullptr);
  if (!w) { std::puts("no context"); return 2; }
  glfwMakeContextCurrent(w);
  const std::string vs = glutil::readText(argv[3]);
  std::string err;
  unsigned pa = glutil::linkProgram(vs, slurp(argv[1]), err), pb = glutil::linkProgram(vs, glutil::readText(argv[2]), err);
  if (!pa || !pb) { std::printf("link failed: %s\n", err.c_str()); return 1; }
  const int W = 320, H = 200;
  unsigned vao, vbo; glutil::setupFullscreenVao(vao, vbo);
  // two deterministic test textures (scene-like and art-like)
  unsigned tex[2];
  glGenTextures(2, tex);
  for (int k = 0; k < 2; ++k) {
    std::vector<float> px(size_t(64) * 64 * 4);
    for (int i = 0; i < 64 * 64; ++i) {
      unsigned h = unsigned(i) * 2654435761u + unsigned(k) * 97u;
      px[size_t(i) * 4] = float((h >> 8) & 255) / 120.f; px[size_t(i) * 4 + 1] = float((h >> 16) & 255) / 255.f;
      px[size_t(i) * 4 + 2] = float((h >> 4) & 255) / 90.f; px[size_t(i) * 4 + 3] = 1.f;
    }
    glBindTexture(GL_TEXTURE_2D, tex[k]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 64, 64, 0, GL_RGBA, GL_FLOAT, px.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  unsigned fbo, rt; glGenFramebuffers(1, &fbo); glGenTextures(1, &rt);
  glBindTexture(GL_TEXTURE_2D, rt);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt, 0);
  glViewport(0, 0, W, H);
  struct U { float time, music, bpm, hasLogo, logoVis, fx, gold, bypass; };
  const U cases[] = {{1.3f, 0.f, 132, 0, 0, 1, 0, 0}, {17.9f, .8f, 100, 0, 0, 1, 0, 0}, {42.2f, .4f, 132, 1, .6f, 1, 0, 0},
                     {63.7f, .6f, 140, 0, 0, .5f, 1, 0}, {5.5f, .9f, 132, 0, 0, 1, 0, 1}, {120.1f, .2f, 90, 0, 0, 0, 0, 0}};
  float worst = 0;
  for (const U& u : cases) {
    std::vector<float> img[2];
    for (int v = 0; v < 2; ++v) {
      unsigned p = v ? pb : pa;
      glUseProgram(p);
      auto f1 = [&](const char* n, float x) { int l = glGetUniformLocation(p, n); if (l >= 0) glUniform1f(l, x); };
      f1("uTime", u.time); f1("uMusicLevel", u.music); f1("uBpm", u.bpm); f1("uHasLogo", u.hasLogo);
      f1("uLogoVis", u.logoVis); f1("uFxAmount", u.fx); f1("uGold", u.gold); f1("uBypass", u.bypass); f1("uArtOn", 1.f);
      int r = glGetUniformLocation(p, "uResolution"); if (r >= 0) glUniform2f(r, float(W), float(H));
      for (int k = 0; k < 2; ++k) {
        glActiveTexture(GL_TEXTURE0 + k); glBindTexture(GL_TEXTURE_2D, tex[k]);
      }
      int a = glGetUniformLocation(p, "uScene"); if (a >= 0) glUniform1i(a, 0);
      a = glGetUniformLocation(p, "uArt"); if (a >= 0) glUniform1i(a, 1);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      img[v].resize(size_t(W) * H * 4);
      glReadPixels(0, 0, W, H, GL_RGBA, GL_FLOAT, img[v].data());
    }
    float m = 0;
    for (size_t i = 0; i < img[0].size(); ++i) m = std::fmax(m, std::fabs(img[0][i] - img[1][i]));
    std::printf("t=%.1f music=%.1f logo=%.1f fx=%.1f gold=%.0f bypass=%.0f: max |diff| = %g\n", u.time, u.music, u.hasLogo, u.fx, u.gold, u.bypass, m);
    worst = std::fmax(worst, m);
  }
  std::printf("%s (worst %g)\n", worst == 0 ? "IDENTICAL" : "DIFFERENT", worst);
  return worst == 0 ? 0 : 1;
}
