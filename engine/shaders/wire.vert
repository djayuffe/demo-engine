#version 410 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;

out float vDepth;
out vec3 vObject;
out vec2 vNdc;

void main(){
  vec4 p=uMVP*vec4(aPos,1.0);
  gl_Position=p;
  vObject=aPos;
  vNdc=p.xy/max(abs(p.w),1e-5);
  // Guard the depth division: a vertex at/near the clip plane (p.w ~ 0) would
  // otherwise produce inf/nan, which clamps to a wrong depth band. Use the same
  // 1e-5 floor as vNdc so a clipped vertex gets a bounded, clampable depth.
  vDepth=clamp(1.0-abs(p.z/max(abs(p.w),1e-5))*0.16,0.15,1.0);
}
