// The stb_image implementation is third-party and trips -Wall -Wextra (missing
// field initializers, unused params, sign compares, …). Silence those for this
// TU only so CI's -Werror (IW_WARNINGS_AS_ERRORS) doesn't fail over a vendored
// header. Our own code below still compiles with full warnings.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "Image.hpp"
#include <algorithm>
#include <filesystem>
namespace fs=std::filesystem;
bool Image::loadFromFile(const std::string& path,Image& out){
  fs::path p(path);
  std::error_code ec;
  if(!fs::exists(p,ec)||ec) return false;
  int ch=0;
  uint8_t* px=stbi_load(p.string().c_str(),&out.w,&out.h,&ch,4);
  if(!px) return false;
  // Guard: stbi_load can return a non-null pointer with w or h == 0 (a degenerate
  // but decodable image) — or, on a corrupt header, a huge negative, which would
  // make n = w*h*4 a massive size_t and rgba.assign(px, px+n) read far past the
  // allocation (OOB). Require positive dims before computing the byte count.
  if(out.w<=0||out.h<=0){ stbi_image_free(px); return false; }
  size_t n=(size_t)out.w*out.h*4;
  out.rgba.assign(px,px+n);
  stbi_image_free(px);
  return !out.empty();
}
std::vector<std::string> findLogos(const std::string& dir){
  std::vector<std::string> names;
  fs::path d(dir);
  std::error_code ec;
  if(!fs::is_directory(d,ec)||ec) return names;
  for(auto& e:fs::directory_iterator(d,ec)){
    std::error_code ec2;
    if(!e.is_regular_file(ec2)||ec2) continue;
    std::string fn=e.path().filename().string();
    // Match UBER_<n>_<...>_1920x1080.jpg (the 20 scene cards).
    if(fn.rfind("UBER_",0)==0&&fn.find("_1920x1080.jpg")!=std::string::npos){
      names.push_back(e.path().string());
    }
  }
  std::sort(names.begin(),names.end());
  return names;
}
