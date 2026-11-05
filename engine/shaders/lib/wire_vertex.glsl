#include "gpu_deform.glsl"
uniform mat4 uMVP;
uniform float uTime;
uniform vec4 uGpuWave;

out float vDepth;
out vec3 vObject;
out vec2 vNdc;

void transformWire(vec3 object){
  object=gpuDeform(object,uint(gl_VertexID),gl_InstanceID);
  if(uGpuWave.x!=0.0)
    object.y+=uGpuWave.x*sin(object.x*uGpuWave.y+uTime*uGpuWave.z+uGpuWave.w);
  vec4 p=uMVP*vec4(object,1.0);
  gl_Position=p;
  vObject=object;
  vNdc=p.xy/max(abs(p.w),1e-5);
  // Guard the depth division: a vertex at/near the clip plane (p.w ~ 0) would
  // otherwise produce inf/nan, which clamps to a wrong depth band. Use the same
  // 1e-5 floor as vNdc so a clipped vertex gets a bounded, clampable depth.
  vDepth=clamp(1.0-abs(p.z/max(abs(p.w),1e-5))*0.16,0.15,1.0);
}
