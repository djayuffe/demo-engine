#include "GLUtil.hpp"
#include <algorithm>
#include <fstream>
#include <set>
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
namespace {
// Raw file contents, or "" if unreadable.
std::string slurp(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return {};
  std::ostringstream s;
  s << f.rdbuf();
  return s.str();
}
std::string dirOf(const std::string& p) {
  const size_t k = p.find_last_of("/\\");
  return k == std::string::npos ? std::string() : p.substr(0, k + 1);
}
// GLSL has no #include: expand `#include "relative/path.glsl"` lines textually, each file once (include-guard
// semantics), to a depth of 8. A missing include leaves a comment so the compile error names the file.
std::string expand(const std::string& path, std::set<std::string>& seen, int depth) {
  std::string src = slurp(path);
  if (src.empty() || depth > 8) return src;
  std::string out;
  size_t pos = 0;
  while (pos < src.size()) {
    size_t eol = src.find('\n', pos);
    if (eol == std::string::npos) eol = src.size();
    const std::string line = src.substr(pos, eol - pos);
    size_t i = line.find_first_not_of(" \t");
    if (i != std::string::npos && line.compare(i, 8, "#include") == 0) {
      const size_t q1 = line.find('"', i), q2 = q1 == std::string::npos ? q1 : line.find('"', q1 + 1);
      if (q2 != std::string::npos) {
        const std::string inc = dirOf(path) + line.substr(q1 + 1, q2 - q1 - 1);
        if (seen.insert(inc).second) {
          const std::string body = expand(inc, seen, depth + 1);
          out += body.empty() ? "// missing include: " + inc : body;
          out += '\n';
        }
        pos = eol + 1;
        continue;
      }
    }
    out += line;
    out += '\n';
    pos = eol + 1;
  }
  return out;
}
}  // namespace
std::string readText(const std::string& p) {
  std::set<std::string> seen;
  return expand(p, seen, 0);
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
