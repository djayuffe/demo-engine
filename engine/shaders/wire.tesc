#version 410 core
layout(vertices=2) out;
uniform mat4 uMVP;
uniform vec2 uViewport;
uniform vec4 uGpuWave;
uniform float uTime;
uniform float uTessMax;
vec4 project(vec3 p){
  p.y+=uGpuWave.x*sin(p.x*uGpuWave.y+uTime*uGpuWave.z+uGpuWave.w);
  return uMVP*vec4(p,1.0);
}
void main(){
  gl_out[gl_InvocationID].gl_Position=gl_in[gl_InvocationID].gl_Position;
  if(gl_InvocationID==0){
    vec4 a=project(gl_in[0].gl_Position.xyz),b=project(gl_in[1].gl_Position.xyz);
    float pixels=length((a.xy/max(abs(a.w),1e-4)-b.xy/max(abs(b.w),1e-4))*uViewport*.5);
    float cycles=abs((gl_in[1].gl_Position.x-gl_in[0].gl_Position.x)*uGpuWave.y);
    float detail=uGpuWave.x==0.?1.:ceil(max(pixels/18.,cycles*4.));
    float segments=clamp(detail,1.,max(1.,uTessMax));
    gl_TessLevelOuter[0]=1.;gl_TessLevelOuter[1]=segments;
    gl_TessLevelOuter[2]=1.;gl_TessLevelOuter[3]=segments;
    gl_TessLevelInner[0]=segments;gl_TessLevelInner[1]=1.;
  }
}
