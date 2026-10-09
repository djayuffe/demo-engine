#pragma once
#include <string>
#include <cstdlib>

// Parse a "--window WxH" spec ("1920x1080"; 'x' or 'X' separator) into a
// (width,height) pair. Returns false (and sets *err) if the string is malformed
// or out of the supported range (320x200 .. 3840x2160). Lives in a header so
// the renderer and the test target share ONE implementation (no drift).
inline bool parseWindowSpec(const char* s,int& w,int& h,std::string& err){
  char* end=nullptr;
  long a=std::strtol(s,&end,10);
  if(!end||!(*end=='x'||*end=='X')){ err=std::string("expected WxH (e.g. 1920x1080), got: ")+(s?s:""); return false; }
  long b=std::strtol(end+1,&end,10);
  if(!end||*end!=0){ err=std::string("trailing junk after WxH: ")+(s?s:""); return false; }
  if(a<320||b<200||a>3840||b>2160){ err=std::string("WxH out of range (320x200 .. 3840x2160): ")+(s?s:""); return false; }
  w=(int)a; h=(int)b; return true;
}
