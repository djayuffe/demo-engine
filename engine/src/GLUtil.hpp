#pragma once
// Platform GL headers plus the small shader/VAO helpers shared by Renderer and SpecialFx.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#elif defined(_WIN32)
#include "GLWin32.hpp"
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include <string>
namespace glutil {
std::string readText(const std::string& path);   // whole file, or "" if unreadable
// Compile + link a vertex/fragment pair. On failure returns 0 and puts the driver log in `err`.
unsigned linkProgram(const std::string& vs, const std::string& fs, std::string& err);
// A VAO for the "no-VBO fullscreen triangle" idiom: glDrawArrays on a VAO with no enabled attribute is
// GL_INVALID_OPERATION, so a 0-byte buffer plus a dummy attribute pointer are still required.
void setupFullscreenVao(unsigned& vao, unsigned& vbo);
}  // namespace glutil
