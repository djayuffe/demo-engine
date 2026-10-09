#version 410 core
layout(location=0) in vec3 aPos;      // unit octahedron
layout(location=1) in float aPath;     // 0..1 lane index (normalized)
uniform mat4 uMVP;
uniform float uTime;
uniform float uSpeed;

out float vGlow;
out float vPath;

void main(){
  // Travel position: bounce along +X, ping-pong through the wireframe.
  float p=aPath;
  // each traveler on its own phase so they don't clump
  float phase=p*6.2831853;
  // bounce in [-1.6, 1.6] on X, gentle drift on Y/Z
  float x=1.6*sin(uTime*uSpeed+phase);
  float y=0.9*sin(uTime*uSpeed*0.61+p*3.7+1.3);
  float z=0.9*cos(uTime*uSpeed*0.43+p*2.9+2.1);
  vec3 center=vec3(x,y,z);
  // orient the octahedron as it travels (tumble)
  float a=uTime*(0.6+p*0.3)+phase;
  float c=cos(a),s=sin(a);
  mat3 rot=mat3(c,-s,0, s,c,0, 0,0,1);
  vec3 pos=center+rot*(aPos*0.12);
  vec4 mvp=uMVP*vec4(pos,1.0);
  gl_Position=mvp;
  // glow strongest when the traveler is near the screen center (z~0 in view)
  vGlow=1.0-abs(center.z)*0.25;
  vPath=p;
}
