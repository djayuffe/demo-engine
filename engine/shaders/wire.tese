#version 410 core
// A tessellated quad ribbon avoids isoline + geometry-stage software fallbacks
// on Apple drivers. Both deformation and antialiased width stay on the GPU.
layout(quads,equal_spacing,ccw) in;
uniform mat4 uMVP;
uniform vec4 uGpuWave;
uniform float uTime;
uniform vec2 uViewport;
uniform float uWidthPx;
out float gDepth;
out vec3 gObject;
out vec2 gNdc;
out float gSide;
out float gAlong;
flat out float gEdge;
void main(){
  float t=gl_TessCoord.x;
  vec3 a=gl_in[0].gl_Position.xyz,b=gl_in[1].gl_Position.xyz;
  vec3 object=mix(a,b,t),tangent=b-a;
  float phase=object.x*uGpuWave.y+uTime*uGpuWave.z+uGpuWave.w;
  object.y+=uGpuWave.x*sin(phase);
  tangent.y+=uGpuWave.x*uGpuWave.y*cos(phase)*tangent.x;
  vec4 p=uMVP*vec4(object,1.),d=uMVP*vec4(tangent,0.);
  // Derivative of perspective division, scaled to pixels; no finite differences.
  vec2 direction=(d.xy*p.w-p.xy*d.w)*uViewport;
  float len=length(direction);
  vec2 normal=len>1e-6?vec2(-direction.y,direction.x)/len:vec2(0.,1.);
  gSide=gl_TessCoord.y*2.-1.;
  vec2 offset=normal*(.5*uWidthPx+.75)*2./uViewport*gSide;
  gl_Position=vec4(p.xy+offset*p.w,p.z,p.w);
  gObject=object;gAlong=t;gEdge=float(gl_PrimitiveID);
  gNdc=p.xy/max(abs(p.w),1e-5);
  gDepth=clamp(1.-abs(p.z/max(abs(p.w),1e-5))*.16,.15,1.);
}
