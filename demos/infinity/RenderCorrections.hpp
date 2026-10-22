#pragma once
#include <string>
#include <stdexcept>

namespace infinity_render {
// GLSL smoothstep requires edge0 < edge1. Imported decreasing ramps used
// reversed bounds; preserve their intended fade with a defined complement.
inline std::string orderedRamps(std::string source) {
  const std::string token = "smoothstep(";
  size_t at = 0;
  while ((at = source.find(token, at)) != std::string::npos) {
    source.replace(at, token.size(), "orderedRamp(");
    at += 12;
  }
  const auto line = source.find('\n');
  if (line == std::string::npos) throw std::runtime_error("Shader version line missing");
  source.insert(line + 1, R"GLSL(
float orderedRamp(float a,float b,float x){
  if(a==b)return step(a,x);
  return a<b?smoothstep(a,b,x):1.0-smoothstep(b,a,x);
}
)GLSL");
  return source;
}
} // namespace infinity_render
