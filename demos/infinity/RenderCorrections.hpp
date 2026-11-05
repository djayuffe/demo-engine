#pragma once
#include <string>
#include <stdexcept>
#include <utility>

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
  for (const auto& change :
       {std::pair<std::string, std::string>{"col = 1.0 - exp(-col * 1.35);", "col = hueShoulder(col,1.35);"},
        {"c = 1.0 - exp(-c * 1.1);", "c = hueShoulder(c,1.1);"}}) {
    size_t pos = 0;
    while ((pos = source.find(change.first, pos)) != std::string::npos) {
      source.replace(pos, change.first.size(), change.second);
      pos += change.second.size();
    }
  }
  const auto line = source.find('\n');
  if (line == std::string::npos) throw std::runtime_error("Shader version line missing");
  source.insert(line + 1, R"GLSL(
vec3 hueShoulder(vec3 c,float exposure){
  c=max(c,vec3(0.0));
  float peak=max(c.r,max(c.g,c.b));
  return c*((1.0-exp(-peak*exposure))/max(peak,1e-6));
}
float orderedRamp(float a,float b,float x){
  if(a==b)return step(a,x);
  return a<b?smoothstep(a,b,x):1.0-smoothstep(b,a,x);
}
)GLSL");
  return source;
}
} // namespace infinity_render
