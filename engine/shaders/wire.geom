#version 410 core
// Wide, antialiased wires. core-profile GL (and macOS in particular) draws every line 1 pixel wide, so each
// line segment is expanded here into a screen-space quad of uWidthPx pixels with a soft edge (gSide: -1..1).
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
uniform vec2 uViewport;   // render target size in pixels
uniform float uWidthPx;
in float vDepth[];
in vec3 vObject[];
in vec2 vNdc[];
out float gDepth;
out vec3 gObject;
out vec2 gNdc;
out float gSide;
void main(){
  vec4 a=gl_in[0].gl_Position,b=gl_in[1].gl_Position;
  // Segments touching or behind the camera plane cannot be expanded safely in screen space: draw them thin.
  const float eps=1e-4;
  bool behind=(a.w<eps)||(b.w<eps);
  vec2 na=a.xy/max(a.w,eps),nb=b.xy/max(b.w,eps);
  vec2 d=(nb-na)*uViewport;
  float l=length(d);
  vec2 dir=l>1e-4?d/l:vec2(1.,0.);
  vec2 n=vec2(-dir.y,dir.x);
  float half_=behind?0.:.5*uWidthPx+.75;                 // +0.75 px of antialiasing skirt
  vec2 off=n*half_*2./uViewport;                         // pixels -> NDC
  vec2 ext=dir*half_*2./uViewport;                       // round-ish caps: extend ends by the half width
  for(int i=0;i<4;++i){
    int k=i&1;                                            // 0,1,0,1 : end of the segment
    float side=(i<2)?-1.:1.;
    vec4 p=(k==0)?a:b;
    float endSign=(k==0)?-1.:1.;
    vec2 o=off*side+ext*endSign;
    gl_Position=vec4(p.xy+o*p.w,p.z,p.w);
    gDepth=vDepth[k];gObject=vObject[k];gNdc=vNdc[k];gSide=behind?0.:side;
    EmitVertex();
  }
  EndPrimitive();
}
