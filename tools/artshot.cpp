// Dump art.frag at chosen times to PPM files (visual check of the procedural background).
//   artshot <shaderdir> <outprefix> <t0> [t1 ...]
#include "GLUtil.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
int main(int argc, char** argv) {
  if (argc < 4) return 2;
  glfwInit();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* w = glfwCreateWindow(64, 64, "a", nullptr, nullptr);
  if (!w) return 2;
  glfwMakeContextCurrent(w);
  const std::string dir = argv[1];
  std::string err;
  unsigned p = glutil::linkProgram(glutil::readText(dir + "/post.vert"), glutil::readText(dir + "/art.frag"), err);
  if (!p) { std::printf("link failed: %s\n", err.c_str()); return 1; }
  const int W = 480, H = 270;
  unsigned vao, vbo; glutil::setupFullscreenVao(vao, vbo);
  unsigned fbo, rt; glGenFramebuffers(1, &fbo); glGenTextures(1, &rt);
  glBindTexture(GL_TEXTURE_2D, rt);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt, 0);
  glViewport(0, 0, W, H);
  glUseProgram(p);
  for (int a = 3; a < argc; ++a) {
    float t = float(std::atof(argv[a]));
    glUniform1f(glGetUniformLocation(p, "uTime"), t);
    glUniform1f(glGetUniformLocation(p, "uMusicLevel"), .5f);
    glUniform1f(glGetUniformLocation(p, "uBpm"), 132.f);
    glUniform1f(glGetUniformLocation(p, "uFxAmount"), 1.f);
    glUniform2f(glGetUniformLocation(p, "uResolution"), float(W), float(H));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::vector<float> px(size_t(W) * H * 4);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_FLOAT, px.data());
    char name[512]; std::snprintf(name, sizeof name, "%s_%s.ppm", argv[2], argv[a]);
    FILE* f = std::fopen(name, "wb"); std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int y = H - 1; y >= 0; --y) for (int x = 0; x < W; ++x) for (int c = 0; c < 3; ++c) {
      float v = px[(size_t(y) * W + x) * 4 + c]; v = v / (1.f + v); // soft tonemap for viewing
      unsigned char b = (unsigned char)(std::fmin(std::fmax(v, 0.f), 1.f) * 255.f); std::fputc(b, f); }
    std::fclose(f);
  }
  return 0;
}
