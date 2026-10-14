#include "GLUtil.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
namespace glutil {
namespace {
bool shader(GLuint& out, GLenum type, const std::string& s, std::string& err) {
  out = glCreateShader(type);
  const char* p = s.c_str();
  glShaderSource(out, 1, &p, nullptr);
  glCompileShader(out);
  GLint ok = 0;
  glGetShaderiv(out, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint n = 0;
    glGetShaderiv(out, GL_INFO_LOG_LENGTH, &n);
    std::string log(std::max(1, n), '\0');
    glGetShaderInfoLog(out, n, nullptr, log.data());
    err = log;
    glDeleteShader(out);
    out = 0;
    return false;
  }
  return true;
}
}  // namespace
std::string readText(const std::string& p) {
  std::ifstream f(p);
  if (!f) return {};
  std::ostringstream s;
  s << f.rdbuf();
  return s.str();
}
unsigned linkProgram(const std::string& vs, const std::string& fs, std::string& err) {
  GLuint v = 0, f = 0;
  if (!shader(v, GL_VERTEX_SHADER, vs, err) || !shader(f, GL_FRAGMENT_SHADER, fs, err)) {
    if (v) glDeleteShader(v);
    if (f) glDeleteShader(f);
    return 0;
  }
  GLuint p = glCreateProgram();
  glAttachShader(p, v);
  glAttachShader(p, f);
  glLinkProgram(p);
  glDeleteShader(v);
  glDeleteShader(f);
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint n = 0;
    glGetProgramiv(p, GL_INFO_LOG_LENGTH, &n);
    err.resize(std::max(1, n));
    glGetProgramInfoLog(p, n, nullptr, err.data());
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

// Set up a VAO for the "no-VBO fullscreen triangle" idiom (the vertex shader
// generates positions from gl_VertexID). glDrawArrays on a VAO with NO enabled
// vertex attribute is GL_INVALID_OPERATION, so a 0-byte ARRAY_BUFFER + a
// dummy vertex attribute pointer are required even though the shader ignores
// them. This is the fix for post/scroller/bg, which previously drew on empty
// VAOs (invalid GL, nothing rendered).
void setupFullscreenVao(unsigned& vao, unsigned& vbo) {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glBindVertexArray(0);
}
}  // namespace glutil
