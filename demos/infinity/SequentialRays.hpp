#pragma once
#include <string>
#include <stdexcept>
namespace infinity_sequential {
inline constexpr const char* vertex = R"GLSL(#version 410 core
flat out int vertexIndex;
void main(){vertexIndex=gl_VertexID;gl_Position=vec4(0.0,0.0,0.0,1.0);}
)GLSL";
inline constexpr const char* fragment = R"GLSL(#version 410 core
flat in vec4 rayState;
out vec4 outState;
void main(){outState=rayState;}
)GLSL";
// Extract equations directly from the preserved imported shader: no second
// hand-maintained integrator. Geometry shaders emit each prefix once.
inline std::string geometry(const std::string& original, bool checkpoints) {
  auto definitions = original.find("uniform float uK;");
  auto definitionsEnd = original.find("uniform vec2 uSource;");
  auto derivative = original.find("float gDs;");
  auto derivativeEnd = original.find("void main()", derivative);
  auto step = original.find("vec4 k1 = deriv(s);");
  const std::string last = "s.zw = normalize(s.zw);";
  auto stepEnd = original.find(last, step);
  if (definitions == std::string::npos || definitionsEnd == std::string::npos || derivative == std::string::npos ||
      derivativeEnd == std::string::npos || step == std::string::npos || stepEnd == std::string::npos)
    throw std::runtime_error("Sequential RK4 shader contract missing");
  return std::string("#version 410 core\nlayout(points) in;\nlayout(points,max_vertices=") +
         (checkpoints ? "16" : "32") + ") out;\nflat in int vertexIndex[];\nflat out vec4 rayState;\n" +
         original.substr(definitions, definitionsEnd - definitions) +
         "uniform vec2 uSource;\nuniform float uSpread,uDs;\nuniform sampler2D uCk;\n" +
         original.substr(derivative, derivativeEnd - derivative) + "vec4 advanceState(vec4 s){\n" +
         original.substr(step, stepEnd + last.size() - step) + "\nreturn s;\n}\n" +
         (checkpoints ? R"GLSL(
void main(){
  int row=vertexIndex[0],ray=row%uRays;
  gDs=dispS(lambdaOf(row/uRays));
  float a=mix(-uSpread,uSpread,float(ray)/float(uRays-1));
  vec4 s=vec4(uSource,cos(a),sin(a));
  for(int col=0;col<16;++col){
    rayState=s;gl_Position=vec4((float(col)+0.5)/16.0*2.0-1.0,
      (float(row)+0.5)/float(uRays*uNW)*2.0-1.0,0.0,1.0);
    gl_PointSize=1.0;EmitVertex();EndPrimitive();
    if(col<15)for(int i=0;i<32;++i)s=advanceState(s);
  }
}
)GLSL"
                      : R"GLSL(
void main(){
  int row=vertexIndex[0]/16,chunk=vertexIndex[0]%16;
  gDs=dispS(lambdaOf(row/uRays));
  vec4 s=texelFetch(uCk,ivec2(chunk,row),0);
  for(int i=0;i<32;++i){
    rayState=s;gl_Position=vec4((float(chunk*32+i)+0.5)/512.0*2.0-1.0,
      (float(row)+0.5)/float(uRays*uNW)*2.0-1.0,0.0,1.0);
    gl_PointSize=1.0;EmitVertex();EndPrimitive();
    if(i<31)s=advanceState(s);
  }
}
)GLSL");
}
} // namespace infinity_sequential
