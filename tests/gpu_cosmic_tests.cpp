// Actual GLSL tests; invoked by gpu_smoke.py because CI may lack a display.
#include "GLUtil.hpp"
#include "../demos/cosmic/CosmicShaders.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool ok, const std::string& message) {
  if (!ok) throw std::runtime_error(message);
}
static std::string extract(const char* shader, const char* begin, const char* end) {
  const std::string text(shader);
  const auto a = text.find(begin), b = text.find(end, a);
  require(a != std::string::npos && b != std::string::npos, "Missing cosmic math source");
  return text.substr(a, b - a);
}
int main() {
  GLFWwindow* window = nullptr;
  try {
    require(glfwInit(), "GLFW initialization failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    window = glfwCreateWindow(64, 64, "Cosmic math validation", nullptr, nullptr);
    require(window != nullptr, "GL context unavailable");
    glfwMakeContextCurrent(window);
#ifdef _WIN32
    require(de::loadGL(reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress)), "GL loader failed");
#endif
    const std::string vertex = R"GLSL(#version 410 core
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0.,1.);}
)GLSL";
    const std::string fragment = std::string("#version 410 core\nout vec4 result;\n") +
      extract(cosmic_gpu::mobiusSurfaceVertex, "struct MobiusSample", "vec3 safeNormalize") +
      extract(cosmic_gpu::blackHoleFragment, "float hash12", "vec3 starLayer") + R"GLSL(
vec3 referenceMobius(float u,float v,float time,float bass){
  float twist=sin(u*.5)*2.5;
  float a=u*.5+twist;
  float detail=.018*sin(u*20.+time*.7)+.016*cos(v*15.-time*.31);
  float r=1.5+.5*v*cos(a);
  vec3 p=vec3(r*cos(u),r*sin(u),.5*v*sin(a));
  p.z+=.25*cos(u*.25);
  return p*(.82*(1.+detail+.018*bass));
}
void main(){
  float id=floor(gl_FragCoord.x);
  float u=id/511.*12.56637061436;
  float v=sin(id*1.71), time=id*.017, bass=fract(id*.137)*2.;
  MobiusSample s=sampleMobius(u,v,time,bass);
  const float h=.001;
  vec3 du=(sampleMobius(u+h,v,time,bass).position-sampleMobius(u-h,v,time,bass).position)/(2.*h);
  vec3 dv=(sampleMobius(u,v+h,time,bass).position-sampleMobius(u,v-h,time,bass).position)/(2.*h);
  float eu=length(du-s.du)/max(1.,length(du));
  float ev=length(dv-s.dv)/max(1.,length(dv));
  // Approach the same cell boundary from both sides, including negative cells.
  vec3 q=vec3(mod(id,9.)-4.,mod(floor(id/9.),7.)-3.,mod(floor(id/63.),7.)-3.);
  int axis=int(mod(id,3.));
  q[(axis+1)%3]+=.37;q[(axis+2)%3]+=.61;
  vec3 delta=vec3(0.);delta[axis]=.00001;
  float seam=abs(noise(q-delta)-noise(q+delta));
  if(length(cross(s.du,s.dv))<.01) eu=1.;
  float positionError=length(s.position-referenceMobius(u,v,time,bass));
  result=vec4(eu,ev,seam,positionError);
}
)GLSL";
    std::string error;
    GLuint program = glutil::linkProgram(vertex, fragment, error);
    require(program != 0, error);
    GLuint vao = 0, texture = 0, fbo = 0;
    glGenVertexArrays(1, &vao);glBindVertexArray(vao);
    glGenTextures(1, &texture);glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,512,1,0,GL_RGBA,GL_FLOAT,nullptr);
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Incomplete test framebuffer");
    glViewport(0,0,512,1);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_DITHER);
    glUseProgram(program);glDrawArrays(GL_TRIANGLES,0,3);
    std::vector<float> output(512*4);
    glReadPixels(0,0,512,1,GL_RGBA,GL_FLOAT,output.data());
    require(glGetError()==GL_NO_ERROR,"Cosmic validation GL error");
    float derivativeMax=0,seamMax=0;
    for(int i=0;i<512;++i){
      for(int k=0;k<4;++k) require(std::isfinite(output[size_t(i*4+k)]),"Non-finite cosmic math");
      derivativeMax=std::max({derivativeMax,output[size_t(i*4)],output[size_t(i*4+1)]});
      seamMax=std::max(seamMax,output[size_t(i*4+2)]);
      require(output[size_t(i*4+3)]<.00001f,"Mobius position changed from original equation");
    }
    require(derivativeMax<.004f,"Analytic derivative disagrees with finite difference: "+std::to_string(derivativeMax));
    require(seamMax<.001f,"Discontinuous noise across lattice boundary: "+std::to_string(seamMax));
    glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&texture);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);
    glfwDestroyWindow(window);glfwTerminate();
    std::cout<<"GPU cosmic math: 512 samples passed, derivative error "<<derivativeMax<<", noise seam "<<seamMax<<'\n';
    return 0;
  } catch(const std::exception& error){
    std::cerr<<error.what()<<'\n';
    if(window)glfwDestroyWindow(window);
    glfwTerminate();return 1;
  }
}
