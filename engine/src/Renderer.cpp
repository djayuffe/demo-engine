#include "Renderer.hpp"
#include "TextScroller.hpp"
#include "Image.hpp"
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
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
// stb_image_write is third-party and trips -Wall -Wextra (missing field
// initializers, etc.). Silence those for this TU only so CI's -Werror doesn't
// fail over a vendored header. Our own code below keeps full warnings.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"   /* sprintf in the vendored header (Apple clang) */
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
namespace {std::string readText(const std::string&p){std::ifstream f(p);if(!f)return {};std::ostringstream s;s<<f.rdbuf();return s.str();}
bool shader(GLuint&out,GLenum type,const std::string&s,std::string&err){out=glCreateShader(type);const char*p=s.c_str();glShaderSource(out,1,&p,nullptr);glCompileShader(out);GLint ok=0;glGetShaderiv(out,GL_COMPILE_STATUS,&ok);if(!ok){GLint n=0;glGetShaderiv(out,GL_INFO_LOG_LENGTH,&n);std::string log(std::max(1,n),'\0');glGetShaderInfoLog(out,n,nullptr,log.data());err=log;glDeleteShader(out);out=0;return false;}return true;}
GLuint linkProgram(const std::string&vs,const std::string&fs,std::string&err){GLuint v=0,f=0;if(!shader(v,GL_VERTEX_SHADER,vs,err)||!shader(f,GL_FRAGMENT_SHADER,fs,err)){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){GLint n=0;glGetProgramiv(p,GL_INFO_LOG_LENGTH,&n);err.resize(std::max(1,n));glGetProgramInfoLog(p,n,nullptr,err.data());glDeleteProgram(p);return 0;}return p;}
void perspective(float*m,float fovy,float aspect,float zn,float zf){float f=1/std::tan(fovy*.5f);for(int i=0;i<16;i++)m[i]=0;m[0]=f/aspect;m[5]=f;m[10]=(zf+zn)/(zn-zf);m[11]=-1;m[14]=(2*zf*zn)/(zn-zf);}
void mul(float*o,const float*a,const float*b){float r[16]{};for(int c=0;c<4;c++)for(int rr=0;rr<4;rr++)for(int k=0;k<4;k++)r[c*4+rr]+=a[k*4+rr]*b[c*4+k];std::copy(r,r+16,o);}
// Set up a VAO for the "no-VBO fullscreen triangle" idiom (the vertex shader
// generates positions from gl_VertexID). glDrawArrays on a VAO with NO enabled
// vertex attribute is GL_INVALID_OPERATION, so a 0-byte ARRAY_BUFFER + a
// dummy vertex attribute pointer are required even though the shader ignores
// them. This is the fix for post/scroller/bg, which previously drew on empty
// VAOs (invalid GL, nothing rendered).
void setupFullscreenVao(unsigned& vao,unsigned& vbo){glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,0,nullptr,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,nullptr);glBindVertexArray(0);}}
// Create all GL objects (programs, VAOs, buffers, textures, HDR FBO). Called by
// init() and reinit() (after a context loss). Must run with a current context.
bool Renderer::createGl(const std::string&dir){
  // Reset mesh/HDR/scroll state so a reinit (after a context loss) doesn't
  // carry stale capacities or a stale capture flag from the dead context.
  // First init these are already 0/false.
  edgeCount_=0;vboCapacity_=0;eboCapacity_=0;
  hdrW_=hdrH_=0;logoCapturePending_=false;hasLogos_=false;bgLastTime_=-1.0;
  std::string vs=readText(dir+"/wire.vert"),fs=readText(dir+"/wire.frag");
  if(vs.empty()||fs.empty()){error_="cannot read shaders from "+dir;return false;}
  program_=linkProgram(vs,fs,error_);
  if(!program_)return false;
  uMVP_=glGetUniformLocation(program_,"uMVP");uTime_=glGetUniformLocation(program_,"uTime");
  uColor_=glGetUniformLocation(program_,"uColor");uGain_=glGetUniformLocation(program_,"uGain");uMusicLevel_=glGetUniformLocation(program_,"uMusicLevel");
  glGenVertexArrays(1,&vao_);glGenBuffers(1,&vbo_);glGenBuffers(1,&ebo_);
  if(!initPost(dir))return false;
  // Non-fatal extras: scroller, logo background, traveling objects. A missing
  // shader degrades (no marquee / no background / no travelers) rather than
  // failing the app. drawScroller/drawBackground/drawTravelers early-return
  // when their program is 0.
  if(!initScroller(dir))error_="scroller shader unavailable (no marquee): "+error_;
  initBackground(dir);
  initTravelers(dir);
  glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);return true;}
bool Renderer::init(GLFWwindow*,const std::string&dir){
  // Reset all GL object ids so createGl generates fresh ones. On a first init
  // they're already 0; on a reinit after shutdown() they may be stale.
  shutdown();
  return createGl(dir);
}
// Re-create all GL objects after a context loss. The caller has already
// re-makethe-context-current. Logo textures are re-loaded by the caller
// (loadLogos) since they need the image data on disk.
bool Renderer::reinit(const std::string&dir){
  // Delete only the GL objects that survived the context loss (they're invalid
  // handles now, but deleting them is the correct cleanup before re-creating).
  // We do NOT touch the logo image data (logos_) — only their GL textures.
  if(hdrFbo_)glDeleteFramebuffers(1,&hdrFbo_);
  if(hdrTex_)glDeleteTextures(1,&hdrTex_);
  if(depthRbo_)glDeleteRenderbuffers(1,&depthRbo_);
  hdrFbo_=hdrTex_=depthRbo_=0;hdrW_=hdrH_=0;
  for(auto&L:logos_)if(L.tex)glDeleteTextures(1,&L.tex),L.tex=0;
  if(scrollerFontTex_)glDeleteTextures(1,&scrollerFontTex_),scrollerFontTex_=0;
   if(scrollerCodesTex_)glDeleteTextures(1,&scrollerCodesTex_),scrollerCodesTex_=0,scrollerCodesTexW_=0;
   bgTexA_=bgTexB_=0;bgCurrent_=bgTarget_=-1;bgMix_=0.f;hasLogos_=false;
  return createGl(dir);
}
bool Renderer::initBackground(const std::string&dir){std::string vs=readText(dir+"/bg.vert"),fs=readText(dir+"/bg.frag");if(vs.empty()||fs.empty())return false;bgProgram_=linkProgram(vs,fs,error_);if(!bgProgram_)return false;uBgMix_=glGetUniformLocation(bgProgram_,"uMix");uBgZoom_=glGetUniformLocation(bgProgram_,"uZoom");uBgAspect_=glGetUniformLocation(bgProgram_,"uAspect");uBgTexAspect_=glGetUniformLocation(bgProgram_,"uTexAspect");uBgOp_=glGetUniformLocation(bgProgram_,"uOpacity");uBgTime_=glGetUniformLocation(bgProgram_,"uTime");uBgMusic_=glGetUniformLocation(bgProgram_,"uMusicLevel");uBgTexA_=glGetUniformLocation(bgProgram_,"uTexA");uBgTexB_=glGetUniformLocation(bgProgram_,"uTexB");setupFullscreenVao(bgVao_,bgVbo_);return true;}
bool Renderer::initTravelers(const std::string&dir){std::string vs=readText(dir+"/traveler.vert"),fs=readText(dir+"/traveler.frag");if(vs.empty()||fs.empty())return false;travelerProgram_=linkProgram(vs,fs,error_);if(!travelerProgram_)return false;uTrMVP_=glGetUniformLocation(travelerProgram_,"uMVP");uTrTime_=glGetUniformLocation(travelerProgram_,"uTime");uTrColor_=glGetUniformLocation(travelerProgram_,"uColor");uTrSpeed_=glGetUniformLocation(travelerProgram_,"uSpeed");
  // Build 12 travelers, each a unit octahedron (6 verts, 12 edges), with a
  // per-vertex path attribute.
  const int TR=12;
  std::vector<float> data;
  data.reserve(TR*6*2*2); // TR travelers * 6 verts * (pos3? no: pos3 + path1) -> we interleave as location0=pos3, location1=path (stride 16B)
  // unit octahedron vertices
  float ov[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
  int oe[12][2]={{0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{2,5},{3,4},{3,5}};
  std::vector<uint32_t> idx;
  for(int t=0;t<TR;t++){
    float path=(float)t/(float)(TR-1);
    for(int vi=0;vi<6;vi++){
      data.push_back(ov[vi][0]);data.push_back(ov[vi][1]);data.push_back(ov[vi][2]);data.push_back(path);
    }
  }
  for(int t=0;t<TR;t++){
    for(int ei=0;ei<12;ei++){
      idx.push_back((uint32_t)(t*6+oe[ei][0]));
      idx.push_back((uint32_t)(t*6+oe[ei][1]));
    }
  }
  glGenVertexArrays(1,&travelerVao_);
  glGenBuffers(1,&travelerVbo_);
  glGenBuffers(1,&travelerEbo_);
  glBindVertexArray(travelerVao_);
  glBindBuffer(GL_ARRAY_BUFFER,travelerVbo_);
  glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(float),data.data(),GL_STATIC_DRAW);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,16,nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,16,(void*)(12));
  glEnableVertexAttribArray(1);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,travelerEbo_);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(uint32_t),idx.data(),GL_STATIC_DRAW);
   travelerEdgeCount_=(int)idx.size();
   glBindVertexArray(0);
   return true;}
bool Renderer::initScroller(const std::string&dir){std::string vs=readText(dir+"/scroller.vert"),fs=readText(dir+"/scroller.frag");if(vs.empty()||fs.empty()){error_="cannot read scroller shaders";return false;}scrollerProgram_=linkProgram(vs,fs,error_);if(!scrollerProgram_)return false;uScTime_=glGetUniformLocation(scrollerProgram_,"uTime");uScPhase_=glGetUniformLocation(scrollerProgram_,"uBeatPhase");uScRes_=glGetUniformLocation(scrollerProgram_,"uResolution");uScMusic_=glGetUniformLocation(scrollerProgram_,"uMusicLevel");uScOffset_=glGetUniformLocation(scrollerProgram_,"uScrollOffset");uScTextWidth_=glGetUniformLocation(scrollerProgram_,"uTextWidth");uScFont_=glGetUniformLocation(scrollerProgram_,"uFont");uScCodes_=glGetUniformLocation(scrollerProgram_,"uCodes");uScCount_=glGetUniformLocation(scrollerProgram_,"uCount");uScOpacity_=glGetUniformLocation(scrollerProgram_,"uOpacity");setupFullscreenVao(scrollerVao_,scrollerVbo_);glGenTextures(1,&scrollerFontTex_);auto data=textfont::pack();
    // 1x665 GL_R8 GL_TEXTURE_2D (NOT 1D — Apple's Metal-based OpenGL driver
    // does not support 1D textures; glTexImage1D is a silent no-op there).
     glBindTexture(GL_TEXTURE_2D,scrollerFontTex_);
     glPixelStorei(GL_UNPACK_ALIGNMENT,1);  // row=665 bytes, not a multiple of 4
     glTexImage2D(GL_TEXTURE_2D,0,GL_R8,(GLsizei)data.size(),1,0,GL_RED,GL_UNSIGNED_BYTE,data.data());
     glPixelStorei(GL_UNPACK_ALIGNMENT,4);
     glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D,0);
    return true;}
bool Renderer::initPost(const std::string&dir){std::string vs=readText(dir+"/post.vert"),fs=readText(dir+"/post.frag");if(vs.empty()||fs.empty()){error_="cannot read post shaders from "+dir;return false;}postProgram_=linkProgram(vs,fs,error_);if(!postProgram_)return false;uPostScene_=glGetUniformLocation(postProgram_,"uScene");uPostTime_=glGetUniformLocation(postProgram_,"uTime");uPostResolution_=glGetUniformLocation(postProgram_,"uResolution");uPostMusic_=glGetUniformLocation(postProgram_,"uMusicLevel");uPostBpm_=glGetUniformLocation(postProgram_,"uBpm");uPostBypass_=glGetUniformLocation(postProgram_,"uBypass");uPostHasLogo_=glGetUniformLocation(postProgram_,"uHasLogo");uPostLogoVis_=glGetUniformLocation(postProgram_,"uLogoVis");uPostFx_=glGetUniformLocation(postProgram_,"uFxAmount");setupFullscreenVao(postVao_,postVbo_);glUseProgram(postProgram_);if(uPostScene_>=0)glUniform1i(uPostScene_,0);return true;}
bool Renderer::resizeHdr(int width,int height){
  // Apply the render scale: the HDR target (and everything rendered into it —
  // the 3D wireframe, travelers, and the post/FX pass) runs at a fraction of
  // screen resolution on weak GPUs. The post pass upscales to the full-screen
  // default framebuffer. 1.0 = native.
  width=std::max(1,(int)std::lround(width*renderScale_));
  height=std::max(1,(int)std::lround(height*renderScale_));
  if(width==hdrW_&&height==hdrH_&&hdrFbo_)return true;
  hdrW_=width;hdrH_=height;
  if(!hdrFbo_)glGenFramebuffers(1,&hdrFbo_);
  if(!hdrTex_)glGenTextures(1,&hdrTex_);
  if(!depthRbo_)glGenRenderbuffers(1,&depthRbo_);
  glBindTexture(GL_TEXTURE_2D,hdrTex_);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,width,height,0,GL_RGBA,GL_FLOAT,nullptr);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glBindRenderbuffer(GL_RENDERBUFFER,depthRbo_);
  glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
  glBindFramebuffer(GL_FRAMEBUFFER,hdrFbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,hdrTex_,0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthRbo_);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){error_="HDR framebuffer incomplete";glBindFramebuffer(GL_FRAMEBUFFER,0);return false;}
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  return true;
}
// Additive lines pile up: the 600-cell projection draws ~1200 edges through the same pixels and
// burns out to white, while a sparse scene needs the full strength. Scale each line by the square
// root of the density (about constant total energy) and never brighten above 1.
static float wireGainFor(int indexCount){
  float lines=float(std::max(1,indexCount/2));
  return std::clamp(0.80f*std::sqrt(500.f/lines),0.05f,1.0f);   // dense recipe scenes reach tens of thousands of lines
}
bool Renderer::upload(const Mesh3&m){std::string why;if(!geo::validate(m,&why)){error_=why;return false;}std::vector<uint32_t>ix;ix.reserve(m.e.size()*2);for(auto e:m.e){ix.push_back(e.a);ix.push_back(e.b);}edgeCount_=(int)ix.size();glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,vbo_);size_t vb=m.v.size()*sizeof(V3);if(vb>vboCapacity_){vboCapacity_=std::max(vb,vboCapacity_*2+4096);glBufferData(GL_ARRAY_BUFFER,vboCapacity_,nullptr,GL_DYNAMIC_DRAW);}if(vb)glBufferSubData(GL_ARRAY_BUFFER,0,vb,m.v.data());glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(V3),nullptr);glEnableVertexAttribArray(0);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo_);size_t eb=ix.size()*sizeof(uint32_t);if(eb>eboCapacity_){eboCapacity_=std::max(eb,eboCapacity_*2+4096);glBufferData(GL_ELEMENT_ARRAY_BUFFER,eboCapacity_,nullptr,GL_DYNAMIC_DRAW);}if(eb)glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,eb,ix.data());return true;}
bool Renderer::draw(float t,int width,int height,float aspect,int sceneIndex,float scale,float lineWidth,float musicLevel){if(!resizeHdr(width,height))return false;float ml=std::clamp(musicLevel,0.f,1.f);
 float fly=std::sin(t*.19f),zoom=std::sin(t*.23f+1.7f),breath=1.f+.075f*std::sin(t*.83f)+.12f*ml;
 float P[16],V[16]={1,0,0,0,0,1,0,0,0,0,1,0,.22f*fly,.14f*std::cos(t*.13f),-4.25f+.52f*zoom-.28f*ml,1};
 float Ry[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},Rx[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},R[16],X[16],M[16];
 float cy=std::cos(t*.17f),sy=std::sin(t*.17f),cx=std::cos(.27f*std::sin(t*.11f)),sx=std::sin(.27f*std::sin(t*.11f));
 Ry[0]=cy*scale*breath;Ry[2]=-sy*scale*breath;Ry[8]=sy*scale*breath;Ry[10]=cy*scale*breath;Ry[5]=scale*breath;
 Rx[5]=cx;Rx[6]=sx;Rx[9]=-sx;Rx[10]=cx;
  perspective(P,(52.f+3.f*std::sin(t*.07f)-2.f*ml)*3.14159265f/180.f,std::max(.05f,aspect),.05f,100.f);mul(R,Ry,Rx);mul(X,V,R);mul(M,P,X);
    if(hasLogos_){
    // Logo section: the logo is the BACKDROP. Draw it FIRST into the HDR FBO
    // (opaque, no depth), then the wireframe + travelers render ADDITIVELY on
    // top, so the 3D content glows over the picture. The post/FX pass runs later
    // (finishLogoFrame) over the combined buffer.
     glBindFramebuffer(GL_FRAMEBUFFER,hdrFbo_);
     // The viewport must match the HDR target size (which is the screen scaled
     // by renderScale_ for --quality), NOT the full screen.
     glViewport(0,0,hdrW_,hdrH_);
     glClearColor(0,0,0,1);
     glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
     // 1) Logo backdrop (full-screen, opaque, no depth).
     drawBackground(t,hdrW_,hdrH_,sceneIndex,ml,true);
    // 2) Wireframe additive over the logo.
    glEnable(GL_DEPTH_TEST);
     glUseProgram(program_);
     glUniformMatrix4fv(uMVP_,1,GL_FALSE,M);
     if(uTime_>=0)glUniform1f(uTime_,t);
     if(uMusicLevel_>=0)glUniform1f(uMusicLevel_,ml);
    glUniform3f(uColor_,1.2f+.75f*ml,1.55f+.35f*ml,2.1f+1.1f*ml);if(uGain_>=0)glUniform1f(uGain_,wireGainFor(edgeCount_)*(1.f-.22f*logoVis_));
    glLineWidth(std::max(1.f,lineWidth+ml*.75f));
    glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);glBlendEquation(GL_FUNC_ADD);
    glBindVertexArray(vao_);
    glDrawElements(GL_LINES,edgeCount_,GL_UNSIGNED_INT,nullptr);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    logoCapturePending_=true;
    return true;
  }
  // No logo: capture the wireframe into the HDR buffer (on a black clear). The
  // post/FX pass is deferred to finishLogoFrame() so the travelers (drawn next)
  // are captured too and get the same post-processing as in logo mode.
   glBindFramebuffer(GL_FRAMEBUFFER,hdrFbo_);glViewport(0,0,hdrW_,hdrH_);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glUseProgram(program_);glUniformMatrix4fv(uMVP_,1,GL_FALSE,M);if(uTime_>=0)glUniform1f(uTime_,t);if(uMusicLevel_>=0)glUniform1f(uMusicLevel_,ml);glUniform3f(uColor_,1.2f+.75f*ml,1.55f+.35f*ml,2.1f+1.1f*ml);if(uGain_>=0)glUniform1f(uGain_,wireGainFor(edgeCount_));glLineWidth(std::max(1.f,lineWidth+ml*.75f));glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);glBlendEquation(GL_FUNC_ADD);glBindVertexArray(vao_);glDrawElements(GL_LINES,edgeCount_,GL_UNSIGNED_INT,nullptr);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
   logoCapturePending_=true;
   return true;}
bool Renderer::loadLogos(const std::string&dir){
  auto files=findLogos(dir);
  if(files.empty())return false;
  // Idempotent: clear any pre-existing entries first. On a reinit (after a
  // context loss) the old logo GL textures are already deleted by reinit() and
  // their .tex set to 0 — but the vector would otherwise still hold those stale
  // zero-tex entries, and a second loadLogos would APPEND to them, doubling the
  // cycle length and leaving ~half the cards blank (tex==0 -> texture 0).
  for(auto&L:logos_)if(L.tex)glDeleteTextures(1,&L.tex);
  logos_.clear();
  bgTexA_=bgTexB_=0;bgCurrent_=bgTarget_=-1;bgMix_=0.f;
  for(auto& f:files){
    Image img;
    if(!Image::loadFromFile(f,img)) continue;
    Logo L;
    glGenTextures(1,&L.tex);
    glBindTexture(GL_TEXTURE_2D,L.tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,img.w,img.h,0,GL_RGBA,GL_UNSIGNED_BYTE,img.rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
     // Check for a GL error after the upload: a corrupt/oversized image can
     // leave the texture incomplete (glTexImage2D returns without filling it),
     // which would render as a black card. Detect it and skip the entry.
     if(glGetError()!=GL_NONE){
       glDeleteTextures(1,&L.tex);L.tex=0;continue;
     }
     L.w=img.w;L.h=img.h;
     logos_.push_back(L);
   }
  glBindTexture(GL_TEXTURE_2D,0);
  if(logos_.empty())return false;
  hasLogos_=true;
  bgCurrent_=0;bgTarget_=0;bgMix_=0.f;
  bgTexA_=logos_[0].tex;bgTexB_=logos_[0].tex;
  return true;
}
// Fullscreen logo: cover-fit to the screen, crossfade on scene change,
// subtle Ken Burns. Drawn DIRECTLY to the default framebuffer so the picture
// is guaranteed visible (no post-shader FBO blend to break).
bool Renderer::drawBackground(float time,int width,int height,int sceneIndex,float musicLevel,bool logoComposite){
  if(!bgProgram_||logos_.empty())return false;
  (void)sceneIndex;
  // Logo show, driven by time (not by scene changes): each card fades in, holds for a few seconds,
  // fades out to black, then a short black beat shows only the wireframe, then the next card.
  // Eased fades; the cycle never depends on the frame rate.
  static constexpr double kFadeIn=1.0,kHold=3.5,kFadeOut=1.0,kBlack=9.0;   // a long black beat between cards
  static constexpr double kCycle=kFadeIn+kHold+kFadeOut+kBlack;
  const int n=(int)logos_.size();
  const double cyc=std::max(0.0,(double)time)/kCycle;
  const int idx=int(std::floor(cyc))%n;
  const double ph=(cyc-std::floor(cyc))*kCycle;
  auto ease=[](double x){x=std::clamp(x,0.0,1.0);return float(x*x*(3.0-2.0*x));};
  float vis;
  if(ph<kFadeIn)vis=ease(ph/kFadeIn);
  else if(ph<kFadeIn+kHold)vis=1.f;
  else if(ph<kFadeIn+kHold+kFadeOut)vis=1.f-ease((ph-kFadeIn-kHold)/kFadeOut);
  else vis=0.f;
  logoVis_=vis;
  bgCurrent_=bgTarget_=idx;bgMix_=0.f;
  bgTexA_=bgTexB_=logos_[idx].tex;
  bgLastTime_=time;
  if(!bgTexA_) bgTexA_=logos_[0].tex;
  if(!bgTexB_) bgTexB_=logos_[0].tex;
  // Ken Burns: slow zoom in/out.
  float zoom=1.05f+0.04f*std::sin(time*0.06f);
  glUseProgram(bgProgram_);
  if(uBgMix_>=0)glUniform1f(uBgMix_,bgMix_);
  if(uBgZoom_>=0)glUniform1f(uBgZoom_,zoom);
   if(uBgAspect_>=0)glUniform1f(uBgAspect_,float(width)/std::max(1.f,float(height)));
   // Use the actual logo dimensions (all cards are 1920x1080 today, but don't
   // hard-code it — a non-standard card would be distorted).
   const auto& cur=logos_[bgCurrent_>=0?bgCurrent_:0];
   if(uBgTexAspect_>=0)glUniform2f(uBgTexAspect_,(float)std::max(1,cur.w),(float)std::max(1,cur.h));
  if(uBgOp_>=0)glUniform1f(uBgOp_,logoVis_);   // the card's brightness: fades to black
  if(uBgTime_>=0)glUniform1f(uBgTime_,time);
  if(uBgMusic_>=0)glUniform1f(uBgMusic_,std::clamp(musicLevel,0.f,1.f));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,bgTexA_);
  if(uBgTexA_>=0)glUniform1i(uBgTexA_,0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,bgTexB_);
  if(uBgTexB_>=0)glUniform1i(uBgTexB_,1);
  // In logo mode the logo is the BACKDROP: drawn first into the HDR capture
  // (opaque, no blend) so the wireframe + travelers render additively on top.
  // Otherwise it is drawn straight to the screen.
  if(logoComposite){
    glBindFramebuffer(GL_FRAMEBUFFER,hdrFbo_);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER,0);
  }
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glViewport(0,0,width,height);
  glBindVertexArray(bgVao_);
  glDrawArrays(GL_TRIANGLES,0,3);
  glActiveTexture(GL_TEXTURE0);
  return true;
}
bool Renderer::drawTravelers(float time,int width,int height,float aspect,float scale,float musicLevel){
  if(!travelerProgram_)return false;
  (void)width;(void)height;
  // Reuse the same view/projection the wireframe used so travelers sit inside
  // the same 3D space. Reconstruct from the same params as draw().
  float fly=std::sin(time*.19f),zoom=std::sin(time*.23f+1.7f),breath=1.f+.075f*std::sin(time*.83f)+.12f*std::clamp(musicLevel,0.f,1.f);
  float P[16],V[16]={1,0,0,0,0,1,0,0,0,0,1,0,.22f*fly,.14f*std::cos(time*.13f),-4.25f+.52f*zoom-.28f*std::clamp(musicLevel,0.f,1.f),1};
  float Ry[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},Rx[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},R[16],X[16],M[16];
  float cy=std::cos(time*.17f),sy=std::sin(time*.17f),cx=std::cos(.27f*std::sin(time*.11f)),sx=std::sin(.27f*std::sin(time*.11f));
  Ry[0]=cy*scale*breath;Ry[2]=-sy*scale*breath;Ry[8]=sy*scale*breath;Ry[10]=cy*scale*breath;Ry[5]=scale*breath;
  Rx[5]=cx;Rx[6]=sx;Rx[9]=-sx;Rx[10]=cx;
   // FOV must match draw() exactly (incl. the -2.f*ml music term) so the
   // travelers share the wireframe's camera.
   perspective(P,(52.f+3.f*std::sin(time*.07f)-2.f*std::clamp(musicLevel,0.f,1.f))*3.14159265f/180.f,std::max(.05f,aspect),.05f,100.f);
   mul(R,Ry,Rx);mul(X,V,R);mul(M,P,X);
   // In logo mode the travelers are captured into the HDR buffer (additive) so
   // the post/FX pass sees them. In no-logo mode they also capture to the HDR
   // buffer (the post pass runs AFTER travelers, not inside draw()).
    glBindFramebuffer(GL_FRAMEBUFFER,hdrFbo_);
    // The viewport is inherited from draw() (which set it to hdrW_ x hdrH_), but
    // state it EXPLICITLY here so this dependency isn't fragile — if draw() ever
    // changed its viewport, the travelers would silently render at the wrong scale.
    glViewport(0,0,hdrW_,hdrH_);
   glEnable(GL_DEPTH_TEST);
   glUseProgram(travelerProgram_);
    if(uTrMVP_>=0)glUniformMatrix4fv(uTrMVP_,1,GL_FALSE,M);
   if(uTrTime_>=0)glUniform1f(uTrTime_,time);
   if(uTrSpeed_>=0)glUniform1f(uTrSpeed_,0.85f);
   if(uTrColor_>=0)glUniform3f(uTrColor_,1.0f,0.9f,1.2f);
   glLineWidth(1.4f+std::clamp(musicLevel,0.f,1.f)*1.0f);
   // The travelers are captured into the HDR (float) buffer in BOTH logo and
   // no-logo mode, so they must always render ADDITIVELY (ONE,ONE) to glow over
   // the wireframe — regardless of hasLogos_. (The old code only enabled blend
   // in logo mode, so in no-logo mode the travelers inherited whatever blend
   // state leaked in and often rendered opaque-replace, clobbering the mesh.)
   glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);glBlendEquation(GL_FUNC_ADD);
    glBindVertexArray(travelerVao_);
    // Bind the EBO explicitly: drawTravelers does NOT rely on the global
    // GL_ELEMENT_ARRAY_BUFFER state still pointing at travelerEbo_ from init()
    // (any other EBO bound by another path would silently break the draw).
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,travelerEbo_);
    glDrawElements(GL_LINES,(GLsizei)travelerEdgeCount_,GL_UNSIGNED_INT,nullptr);
    glDisable(GL_BLEND);
    return true;
 }
bool Renderer::finishLogoFrame(float time,int width,int height,int sceneIndex,float musicLevel,float bpm){
  (void)sceneIndex;
  if(!logoCapturePending_)return false;
  logoCapturePending_=false;
  // In both logo and no-logo mode the HDR buffer now holds the wireframe +
  // travelers (+ logo backdrop in logo mode). Run the post/FX pass over it.
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glDisable(GL_DEPTH_TEST);
  glViewport(0,0,width,height);
  glUseProgram(postProgram_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,hdrTex_);
  if(uPostScene_>=0)glUniform1i(uPostScene_,0);
  if(uPostTime_>=0)glUniform1f(uPostTime_,time);
   // The post pass runs on the HDR buffer (hdrW_ x hdrH_), not the screen. Pass
   // the HDR size so the shader's pixel-size (bloom taps) and aspect (p) are
   // measured in the buffer the scene actually lives in — at --quality<1 the
   // screen size would make the bloom taps too coarse.
   if(uPostResolution_>=0)glUniform2f(uPostResolution_,float(std::max(1,hdrW_)),float(std::max(1,hdrH_)));
  if(uPostMusic_>=0)glUniform1f(uPostMusic_,std::clamp(musicLevel,0.f,1.f));
  if(uPostBpm_>=0)glUniform1f(uPostBpm_,bpm);
  if(uPostBypass_>=0)glUniform1f(uPostBypass_,postEnabled_?0.f:1.f);
  if(uPostHasLogo_>=0)glUniform1f(uPostHasLogo_,hasLogos_?1.f:0.f);
  if(uPostLogoVis_>=0)glUniform1f(uPostLogoVis_,logoVis_);
  if(uPostFx_>=0)glUniform1f(uPostFx_,fxAmount_);
  glBindVertexArray(postVao_);
  glDrawArrays(GL_TRIANGLES,0,3);
  return true;
 }
 bool Renderer::screenshot(const std::string& path,int width,int height){
   width=std::max(1,width);height=std::max(1,height);
   std::vector<unsigned char> px((size_t)width*height*3);
   // Read from the default framebuffer (the finished frame: post pass + scroller
   // are already there). glReadPixels returns bottom-up, so we flip vertically
   // while writing the RGB rows for stb.
   glBindFramebuffer(GL_FRAMEBUFFER,0);
   glPixelStorei(GL_PACK_ALIGNMENT,1);
   glReadBuffer(GL_BACK);  // void in core GL; ensures we read the back buffer
   // glGetError is sticky: an error left by an earlier call (for example glLineWidth > 1 is invalid
   // in a macOS core profile) would otherwise be blamed on glReadPixels. Clear it first.
   while(glGetError()!=GL_NONE){}
   glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,px.data());
   if(glGetError()!=GL_NONE){
     error_="glReadPixels failed";return false;
   }
   std::vector<unsigned char> out((size_t)width*height*3);
   for(int y=0;y<height;y++){
     const unsigned char* src=&px[(size_t)(height-1-y)*width*3];
     unsigned char* dst=&out[(size_t)y*width*3];
     std::memcpy(dst,src,(size_t)width*3);
   }
   if(!stbi_write_png(path.c_str(),width,height,3,out.data(),width*3)){
     error_="stbi_write_png failed: "+path;return false;
   }
   return true;
 }
  bool Renderer::drawScroller(float time,float beatPhase,int width,int height,const char*text,float musicLevel){
   if(!scrollerProgram_||!text||!text[0])return false;
   // Measure text in font pixels (scale=6, spacing=1).
   float scale=6.0f;  // must match scroller.frag
   float textWidth=(float)textfont::measure(text,1)*scale;
   float offset=scrollOffset(time,beatPhase,(float)width,(int)textWidth,musicLevel);
   // Build a 1-char code strip: one byte per visible character = its ASCII
   // code. The shader reads this to map a marquee POSITION to the right glyph
   // (the position alone is not the character). Uploaded every frame (the text
   // is at most a few hundred bytes) into a dynamic 1xN GL_R8 texture.
    int n=0; while(text[n])++n;
    std::vector<uint8_t> codes((size_t)n);
    for(int i=0;i<n;++i)codes[i]=(uint8_t)text[i];
    // The code strip is a small 1xN GL_R8 texture (one byte per character).
    // Allocate the storage ONCE at a fixed cap (512 chars — far more than
    // any marquee string) and update it per frame with glTexSubImage2D. The old
    // code did a full glTexImage2D every frame, which re-allocated GPU memory
    // each frame (a needless hitch). glTexSubImage2D updates in place.
    const GLsizei kMaxCodes=512;
    if(n>kMaxCodes)n=kMaxCodes;
    if(!scrollerCodesTex_)glGenTextures(1,&scrollerCodesTex_);
    glBindTexture(GL_TEXTURE_2D,scrollerCodesTex_);
    if(scrollerCodesTexW_<kMaxCodes){
      scrollerCodesTexW_=kMaxCodes;
      glPixelStorei(GL_UNPACK_ALIGNMENT,1);
      // glTexStorage2D is OpenGL 4.2; macOS stops at 4.1, so allocate once with glTexImage2D.
      glTexImage2D(GL_TEXTURE_2D,0,GL_R8,kMaxCodes,1,0,GL_RED,GL_UNSIGNED_BYTE,nullptr);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,n,1,GL_RED,GL_UNSIGNED_BYTE,codes.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);
   glUseProgram(scrollerProgram_);
   if(uScTime_>=0)glUniform1f(uScTime_,time);
   if(uScPhase_>=0)glUniform1f(uScPhase_,beatPhase);
   if(uScRes_>=0)glUniform2f(uScRes_,(float)width,(float)height);
   if(uScMusic_>=0)glUniform1f(uScMusic_,std::clamp(musicLevel,0.f,1.f));
   if(uScOffset_>=0)glUniform1f(uScOffset_,offset);
   if(uScTextWidth_>=0)glUniform1f(uScTextWidth_,textWidth);
   if(uScCount_>=0)glUniform1i(uScCount_,n);
   if(uScOpacity_>=0)glUniform1f(uScOpacity_,0.95f);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D,scrollerFontTex_);
   if(uScFont_>=0)glUniform1i(uScFont_,1);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D,scrollerCodesTex_);
   if(uScCodes_>=0)glUniform1i(uScCodes_,2);
  // The scroller shader outputs PREMULTIPLIED rgb (col*alpha, alpha), so the
  // blend must be (ONE, ONE_MINUS_SRC_ALPHA) — using SRC_ALPHA would dim the
  // text by alpha a second time.
  glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  glViewport(0,0,width,height);
   glBindVertexArray(scrollerVao_);
   glDrawArrays(GL_TRIANGLES,0,3);
   glDisable(GL_BLEND);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D,0);
   return true;
 }
void Renderer::shutdown(){for(auto&L:logos_)if(L.tex)glDeleteTextures(1,&L.tex);logos_.clear();if(scrollerFontTex_)glDeleteTextures(1,&scrollerFontTex_);if(scrollerCodesTex_)glDeleteTextures(1,&scrollerCodesTex_);scrollerCodesTex_=0,scrollerCodesTexW_=0;if(hdrFbo_)glDeleteFramebuffers(1,&hdrFbo_);if(hdrTex_)glDeleteTextures(1,&hdrTex_);if(depthRbo_)glDeleteRenderbuffers(1,&depthRbo_);if(postProgram_)glDeleteProgram(postProgram_);if(postVao_)glDeleteVertexArrays(1,&postVao_);if(postVbo_)glDeleteBuffers(1,&postVbo_);if(scrollerProgram_)glDeleteProgram(scrollerProgram_);if(scrollerVao_)glDeleteVertexArrays(1,&scrollerVao_);if(scrollerVbo_)glDeleteBuffers(1,&scrollerVbo_);if(bgProgram_)glDeleteProgram(bgProgram_);if(bgVao_)glDeleteVertexArrays(1,&bgVao_);if(bgVbo_)glDeleteBuffers(1,&bgVbo_);if(travelerProgram_)glDeleteProgram(travelerProgram_);if(travelerVao_)glDeleteVertexArrays(1,&travelerVao_);if(travelerVbo_)glDeleteBuffers(1,&travelerVbo_);if(travelerEbo_)glDeleteBuffers(1,&travelerEbo_);if(program_)glDeleteProgram(program_);if(ebo_)glDeleteBuffers(1,&ebo_);if(vbo_)glDeleteBuffers(1,&vbo_);if(vao_)glDeleteVertexArrays(1,&vao_);program_=postProgram_=scrollerProgram_=bgProgram_=travelerProgram_=vao_=postVao_=postVbo_=scrollerVao_=scrollerVbo_=bgVao_=bgVbo_=travelerVao_=vbo_=ebo_=travelerVbo_=travelerEbo_=scrollerFontTex_=hdrFbo_=hdrTex_=depthRbo_=0;}
