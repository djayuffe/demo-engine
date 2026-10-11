#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <mach-o/dyld.h>   // _NSGetExecutablePath
#elif defined(_WIN32)
#include "GLWin32.hpp"     // windows.h (GetModuleFileNameA) + GL 1.1 + the GL 4.1 function pointers
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include "Engine.hpp"
#include "Renderer.hpp"
#include "WindowSpec.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
namespace de {
namespace fs=std::filesystem;
namespace {
// Set by the SIGINT/SIGTERM handler; the render loop checks it each frame so Ctrl-C / kill
// produces a clean shutdown instead of a hard kill that leaks GL resources.
std::atomic<bool> g_shutdown{false};
void onSignal(int){ g_shutdown.store(true); }

fs::path executablePath(){
  fs::path exe;
#if defined(__APPLE__)
  { char buf[4096]; uint32_t sz=sizeof(buf); if(_NSGetExecutablePath(buf,&sz)==0){ std::error_code ec; exe=fs::canonical(buf,ec); } }
#elif defined(__linux__)
  { std::error_code ec; exe=fs::canonical("/proc/self/exe",ec); }
#elif defined(_WIN32)
  { char buf[4096]; DWORD n=GetModuleFileNameA(nullptr,buf,sizeof buf); if(n>0&&n<sizeof buf){ std::error_code ec; exe=fs::canonical(buf,ec); } }
#endif
  return exe;
}

// Every place an asset named `rel` could live: CWD, the executable dir, then each ancestor of it.
std::vector<fs::path> assetCandidates(const std::string& rel,const fs::path& exeDir){
  std::vector<fs::path> c;
  c.push_back(fs::current_path()/rel);
  if(!exeDir.empty()){
    c.push_back(exeDir/rel);
    auto p=exeDir;
    while(p.has_parent_path()&&p.parent_path()!=p){ p=p.parent_path(); c.push_back(p/rel); }
  }
  return c;
}
}  // namespace

void Input::poll(GLFWwindow* w){
  for(int k=0;k<kKeys;++k){
    prev_[k]=cur_[k];
    // Only query real GLFW key codes; the gaps in the code space would raise GLFW_INVALID_ENUM.
    const bool valid=k==GLFW_KEY_SPACE||k==GLFW_KEY_APOSTROPHE||(k>=GLFW_KEY_COMMA&&k<=GLFW_KEY_9)||k==GLFW_KEY_SEMICOLON||
                     k==GLFW_KEY_EQUAL||(k>=GLFW_KEY_A&&k<=GLFW_KEY_RIGHT_BRACKET)||k==GLFW_KEY_GRAVE_ACCENT||k>=GLFW_KEY_ESCAPE;
    cur_[k]=valid&&glfwGetKey(w,k)==GLFW_PRESS;
  }
}

int run(Demo& demo,int argc,char** argv){
  const Config cfg=demo.config();
  double bpm=cfg.defaultBpm;bool bpmGiven=false;fs::path musicPath,logoOverride;bool noTicker=false,noPost=false;
  double quality=1.0;int winW=0,winH=0;bool fullscreen=false;std::string screenshotPath;int maxFrames=0;
  const auto helpText=[&](){
    std::fprintf(stdout,
      "%s %s\n"
      "Usage: %s [options]\n"
      "  --bpm N         Tempo (default: the module's own tempo, else %g)\n"
      "  --music PATH    Audio module/wav to play\n"
      "  --no-ticker     Start with the text ticker off (toggle with T)\n"
      "  --no-post       Skip the screen-space FX post pass (faster; debugging)\n"
      "  --quality F     Render the HDR target at F of screen resolution (0.25-1.0)\n"
      "  --logos DIR     Directory of *_1920x1080.jpg logo cards\n"
      "  --window WxH    Initial window size (320x200 .. 3840x2160, default 1440x900)\n"
      "  --fullscreen    Full-screen on the primary monitor (overrides --window)\n"
      "  --screenshot P  Save one PNG frame to P (on frame N when --frames N is given)\n"
      "  --frames N      Exit after N frames (0 = run forever)\n"
      "  --version       Print version and exit\n"
      "  -h, --help      Show this help\n"
      "%s"
      "Keys: T toggles the ticker, Esc quits\n",
      cfg.title.c_str(),cfg.version.c_str(),cfg.binaryName.c_str(),cfg.defaultBpm,demo.usage().c_str());
  };
  for(int i=1;i<argc;i++){
    std::string arg(argv[i]);
    if(demo.parseArg(arg,i,argc,argv)) continue;
    if(arg=="--bpm"&&i+1<argc){double v=std::atof(argv[++i]);if(!std::isfinite(v)){std::fprintf(stderr,"--bpm: not a finite number\n");return 2;}bpm=std::max(1.0,v);bpmGiven=true;}
    else if(arg=="--music"&&i+1<argc)musicPath=argv[++i];
    else if(arg=="--no-ticker"||arg=="--no-scroller")noTicker=true;
    else if(arg=="--no-post")noPost=true;
    else if(arg=="--quality"&&i+1<argc){double v=std::atof(argv[++i]);if(!std::isfinite(v)){std::fprintf(stderr,"--quality: not a finite number\n");return 2;}quality=std::clamp(v,0.25,1.0);}
    else if(arg=="--logos"&&i+1<argc)logoOverride=argv[++i];
    else if(arg=="--window"&&i+1<argc){
      std::string werr;int ww,wh;
      if(!parseWindowSpec(argv[++i],ww,wh,werr)){std::fprintf(stderr,"--window: %s\n",werr.c_str());return 2;}
      winW=ww;winH=wh;
    }
    else if(arg=="--fullscreen")fullscreen=true;
    else if(arg=="--version"){std::fprintf(stdout,"%s %s\n",cfg.title.c_str(),cfg.version.c_str());return 0;}
    else if(arg=="--screenshot"&&i+1<argc)screenshotPath=argv[++i];
    else if(arg=="--frames"&&i+1<argc)maxFrames=std::max(0,std::atoi(argv[++i]));
    else if(arg=="--help"||arg=="-h"){helpText();return 0;}
    else {std::fprintf(stderr,"unknown or incomplete option: %s (try --help)\n",arg.c_str());return 2;}
  }
  std::signal(SIGINT,onSignal);
  std::signal(SIGTERM,onSignal);

  const fs::path exe=executablePath();
  const fs::path exeDir=exe.has_parent_path()?exe.parent_path():fs::path();
  if(musicPath.empty()){
    for(auto& m:cfg.musicFiles){ for(auto& c:assetCandidates(m,exeDir)){ std::error_code ec; if(fs::exists(c,ec)){musicPath=c;break;} } if(!musicPath.empty())break; }
  }

  if(!glfwInit()){std::fprintf(stderr,"GLFW initialization failed\n");return 1;}
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
  GLFWmonitor* mon=fullscreen?glfwGetPrimaryMonitor():nullptr;
  const GLFWvidmode* vm=mon?glfwGetVideoMode(mon):nullptr;
  int cw,ch;
  if(fullscreen){cw=vm?vm->width:1920;ch=vm?vm->height:1080;}
  else {cw=winW>0?winW:1440;ch=winH>0?winH:900;}
  const std::string windowTitle=cfg.title+" "+cfg.version;
  GLFWwindow* w=glfwCreateWindow(cw,ch,windowTitle.c_str(),mon,nullptr);
  if(!w){std::fprintf(stderr,"OpenGL 4.1 context creation failed\n");glfwTerminate();return 2;}
  if(fullscreen)glfwSetWindowMonitor(w,mon,0,0,cw,ch,vm?vm->refreshRate:0);
  glfwMakeContextCurrent(w);glfwSwapInterval(fullscreen?0:1);
#ifdef _WIN32
  { const char* missing=nullptr;
    if(!loadGL(reinterpret_cast<void*(*)(const char*)>(glfwGetProcAddress),&missing)){std::fprintf(stderr,"OpenGL loader: missing %s (needs an OpenGL 4.1 driver)\n",missing?missing:"?");glfwDestroyWindow(w);glfwTerminate();return 2;} }
#endif

  AudioPlayer audio;
#if defined(IW_HAS_AUDIO)
  if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) std::fprintf(stderr,"SDL audio init failed: %s\n",SDL_GetError());
#endif
  if(!musicPath.empty()&&audio.open(musicPath)){
    // Lock the visuals to the module's own tempo unless --bpm was given.
    if(!bpmGiven&&audio.nativeBpm()>0)bpm=audio.nativeBpm();
    std::fprintf(stdout,"music: %s (%.0f BPM)\n",musicPath.string().c_str(),bpm);
  }

  std::string shaderDir=cfg.shaderDir.empty()?"shaders":cfg.shaderDir;
  if(!exeDir.empty()&&cfg.shaderDir.empty()){
    auto cand=exeDir/"shaders";
    if(fs::exists(cand/"wire.vert")) shaderDir=cand.string();
  }
  const auto cleanup=[&](Renderer* r){
    if(r) r->shutdown();
    audio.close();
#if defined(IW_HAS_AUDIO)
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
#endif
    glfwDestroyWindow(w);glfwTerminate();
  };
  Renderer r;
  if(!r.init(w,shaderDir)){std::fprintf(stderr,"Renderer init failed: %s\n",r.error().c_str());cleanup(nullptr);return 3;}
  r.setRenderScale(float(quality));
  r.setPostEnabled(!noPost);
  r.setFxAmount(cfg.fxAmount);
  if(cfg.logoShow){
    std::vector<fs::path> cands;
    if(!logoOverride.empty()) cands.push_back(logoOverride);
    for(auto& d:cfg.logoDirs) for(auto& c:assetCandidates(d,exeDir)) cands.push_back(c);
    bool loaded=false;
    for(auto& c:cands){ if(r.loadLogos(c.string())){ loaded=true; std::fprintf(stdout,"logos: %d cards from %s\n",r.logoCount(),c.string().c_str()); break; } }
    if(!loaded&&!cands.empty()){ std::fprintf(stderr,"note: no logo cards found; tried:\n"); for(auto& c:cands)std::fprintf(stderr,"  %s\n",c.string().c_str()); std::fprintf(stderr,"  -> running without logo background\n"); }
  }

  RunInfo ri;ri.bpm=bpm;ri.executableDir=exeDir.string();ri.musicPath=audio.state().active?musicPath.string():std::string();
  if(!demo.init(ri)){std::fprintf(stderr,"demo init failed\n");cleanup(&r);return 4;}

  Timeline timeline(bpm);Input input;Frame frame;
  uint64_t lastRevision=0;bool haveUpload=false;float lastRadius=.1f;
  bool tickerOn=!noTicker,shotTaken=false;int frameCount=0;double lastNow=glfwGetTime();int exitCode=0;
  while(!glfwWindowShouldClose(w)&&!g_shutdown.load()){
    // Minimised windows have no visible framebuffer; skip the frame and wait for a restore.
    if(glfwGetWindowAttrib(w,GLFW_ICONIFIED)){glfwWaitEventsTimeout(.05);continue;}
    const double now=glfwGetTime();
    const MusicState music=audio.state();
    FrameInfo fi;fi.time=music.active?music.seconds:now;fi.dt=now-lastNow;lastNow=now;fi.frame=frameCount;fi.bpm=bpm;fi.music=music;
    fi.sync=timeline.sample(fi.time);fi.sync.pulse=std::max(fi.sync.pulse,music.level);
    int W,H;glfwGetFramebufferSize(w,&W,&H);
    if(W<=0||H<=0){glfwWaitEventsTimeout(.05);continue;}
    fi.width=W;fi.height=H;
    input.poll(w);
    if(input.pressed(Key::T))tickerOn=!tickerOn;

    frame=Frame{};
    if(!demo.update(fi,input,frame))break;

    // Mesh upload: skipped only when the demo promises it is unchanged (revision != 0).
    if(!haveUpload||frame.revision==0||frame.revision!=lastRevision){
      if(!r.upload(frame.mesh)){std::fprintf(stderr,"Mesh rejected: %s\n",r.error().c_str());exitCode=5;break;}
      lastRevision=frame.revision;haveUpload=true;lastRadius=std::max(.1f,geo::stats(frame.mesh).radius);
    }
    const float scale=std::min(1.48f,1.92f/lastRadius)*frame.scale;
    r.setWireStyle(frame.wireStyle);
    // 1) wireframe (+ logo backdrop) into the HDR target, 2) travellers, 3) post pass, 4) ticker.
    if(!r.draw(float(fi.time),W,H,float(W)/float(H),frame.sceneIndex,scale,frame.lineWidth,music.level)){std::fprintf(stderr,"Renderer draw failed: %s\n",r.error().c_str());exitCode=6;break;}
    if(frame.travelers) r.drawTravelers(float(fi.time),W,H,float(W)/float(H),scale,music.level);
    r.finishLogoFrame(float(fi.time),W,H,frame.sceneIndex,music.level,float(bpm));
    if(tickerOn&&!frame.ticker.empty()) r.drawScroller(float(fi.time),fi.sync.beatPhase,W,H,frame.ticker.c_str(),music.level);

    // --screenshot: taken after the ticker so it is included. With --frames N on the Nth frame.
    if(!screenshotPath.empty()&&!shotTaken&&(maxFrames<=0||frameCount+1>=maxFrames)){
      if(r.screenshot(screenshotPath,W,H)){std::fprintf(stdout,"screenshot: %s (%dx%d) at show time %.2fs\n",screenshotPath.c_str(),W,H,fi.time);shotTaken=true;}
      else std::fprintf(stderr,"screenshot failed: %s\n",r.error().c_str());
    }
    ++frameCount;
    if(maxFrames>0&&frameCount>=maxFrames){std::fprintf(stdout,"--frames %d reached; exiting\n",maxFrames);break;}
    glfwSwapBuffers(w);glfwPollEvents();
    if(input.down(Key::Escape)||glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS)glfwSetWindowShouldClose(w,1);
  }
  cleanup(&r);
  return exitCode;
}
}  // namespace de
