#ifdef _WIN32
#include "GLWin32.hpp"
#define DE_GL_DEFINE(ret, name, args) ret (APIENTRY* name) args = nullptr;
DE_GL_FUNCTIONS(DE_GL_DEFINE)
#undef DE_GL_DEFINE
namespace de {
bool loadGL(void* (*getProc)(const char*), const char** missing){
#define DE_GL_LOAD(ret, name, args) \
  name = reinterpret_cast<decltype(name)>(getProc(#name)); \
  if(!name){ if(missing) *missing = #name; return false; }
  DE_GL_FUNCTIONS(DE_GL_LOAD)
#undef DE_GL_LOAD
  return true;
}
}
#endif
