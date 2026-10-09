#pragma once
// Minimal OpenGL 4.1 core loader for Windows.
//
// opengl32.dll only exports OpenGL 1.1, and the Windows SDK has no <GL/glext.h>. Everything newer
// (shaders, buffers, VAOs, framebuffers...) has to be fetched at run time through the context's
// extension mechanism, which GLFW wraps as glfwGetProcAddress. This header declares exactly the
// functions and constants the engine uses; GLWin32.cpp defines the pointers and fills them in.
// Other platforms do not include this file.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#include <cstddef>

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

// Constants newer than GL 1.1. Plain macros, each guarded, so ones the SDK already defines are left alone.
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD 0x8006
#endif
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_NONE
#define GL_NONE 0
#endif


#define DE_GL_FUNCTIONS(X) \
  X(void,    glActiveTexture,           (GLenum texture)) \
  X(void,    glAttachShader,            (GLuint program, GLuint shader)) \
  X(void,    glBindBuffer,              (GLenum target, GLuint buffer)) \
  X(void,    glBindFramebuffer,         (GLenum target, GLuint framebuffer)) \
  X(void,    glBindRenderbuffer,        (GLenum target, GLuint renderbuffer)) \
  X(void,    glBindVertexArray,         (GLuint array)) \
  X(void,    glBlendEquation,           (GLenum mode)) \
  X(void,    glBufferData,              (GLenum target, GLsizeiptr size, const void* data, GLenum usage)) \
  X(void,    glBufferSubData,           (GLenum target, GLintptr offset, GLsizeiptr size, const void* data)) \
  X(GLenum,  glCheckFramebufferStatus,  (GLenum target)) \
  X(void,    glCompileShader,           (GLuint shader)) \
  X(GLuint,  glCreateProgram,           (void)) \
  X(GLuint,  glCreateShader,            (GLenum type)) \
  X(void,    glDeleteBuffers,           (GLsizei n, const GLuint* buffers)) \
  X(void,    glDeleteFramebuffers,      (GLsizei n, const GLuint* framebuffers)) \
  X(void,    glDeleteProgram,           (GLuint program)) \
  X(void,    glDeleteRenderbuffers,     (GLsizei n, const GLuint* renderbuffers)) \
  X(void,    glDeleteShader,            (GLuint shader)) \
  X(void,    glDeleteVertexArrays,      (GLsizei n, const GLuint* arrays)) \
  X(void,    glEnableVertexAttribArray, (GLuint index)) \
  X(void,    glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)) \
  X(void,    glFramebufferTexture2D,    (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
  X(void,    glGenBuffers,              (GLsizei n, GLuint* buffers)) \
  X(void,    glGenFramebuffers,         (GLsizei n, GLuint* framebuffers)) \
  X(void,    glGenRenderbuffers,        (GLsizei n, GLuint* renderbuffers)) \
  X(void,    glGenVertexArrays,         (GLsizei n, GLuint* arrays)) \
  X(void,    glGenerateMipmap,          (GLenum target)) \
  X(void,    glGetProgramInfoLog,       (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
  X(void,    glGetProgramiv,            (GLuint program, GLenum pname, GLint* params)) \
  X(void,    glGetShaderInfoLog,        (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
  X(void,    glGetShaderiv,             (GLuint shader, GLenum pname, GLint* params)) \
  X(GLint,   glGetUniformLocation,      (GLuint program, const GLchar* name)) \
  X(void,    glLinkProgram,             (GLuint program)) \
  X(void,    glRenderbufferStorage,     (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
  X(void,    glShaderSource,            (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)) \
  X(void,    glUniform1f,               (GLint location, GLfloat v0)) \
  X(void,    glUniform1i,               (GLint location, GLint v0)) \
  X(void,    glUniform2f,               (GLint location, GLfloat v0, GLfloat v1)) \
  X(void,    glUniform3f,               (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)) \
  X(void,    glUniformMatrix4fv,        (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
  X(void,    glUseProgram,              (GLuint program)) \
  X(void,    glVertexAttribPointer,     (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer))

#define DE_GL_DECLARE(ret, name, args) extern "C++" { extern ret (APIENTRY* name) args; }
DE_GL_FUNCTIONS(DE_GL_DECLARE)
#undef DE_GL_DECLARE

namespace de {
// Fills every pointer above through `getProc` (pass glfwGetProcAddress). Needs a current context.
// Returns false and writes the first missing function name to `missing` (if given).
bool loadGL(void* (*getProc)(const char*), const char** missing = nullptr);
}
