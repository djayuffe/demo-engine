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
out float gAlong;   // 0..1 along the segment (end caps clamp to the ends)
flat out float gEdge;   // per-segment id, for hashing
void main(){
  vec4 a=gl_in[0].gl_Position,b=gl_in[1].gl_Position;
  // Clip in homogeneous space before dividing by w. A line crossing the
  // near plane must retain its visible part without producing a screen-sized quad.
  float lo=0.,hi=1.;
  vec2 da=vec2(a.z+a.w,a.w-1e-4),db=vec2(b.z+b.w,b.w-1e-4);
  for(int plane=0;plane<2;++plane){
    if(da[plane]<0.&&db[plane]<0.) return;
    if((da[plane]<0.)!=(db[plane]<0.)){
      float t=da[plane]/(da[plane]-db[plane]);
      if(da[plane]<0.) lo=max(lo,t); else hi=min(hi,t);
    }
  }
  if(lo>hi) return;
  vec4 originalA=a,originalB=b;
  a=mix(originalA,originalB,lo);b=mix(originalA,originalB,hi);
  vec2 na=a.xy/a.w,nb=b.xy/b.w;
  vec2 d=(nb-na)*uViewport;
  float l=length(d);
  vec2 dir=l>1e-4?d/l:vec2(1.,0.);
  vec2 n=vec2(-dir.y,dir.x);
  float half_=.5*uWidthPx+.75;                 // +0.75 px of antialiasing skirt
  vec2 off=n*half_*2./uViewport;                         // pixels -> NDC
  vec2 ext=dir*half_*2./uViewport;                       // round-ish caps: extend ends by the half width
  for(int i=0;i<4;++i){
    int k=i&1;                                            // 0,1,0,1 : end of the segment
    float side=(i<2)?-1.:1.;
    vec4 p=(k==0)?a:b;
    float endSign=(k==0)?-1.:1.;
    vec2 o=off*side+ext*endSign;
    gl_Position=vec4(p.xy+o*p.w,p.z,p.w);
    float t=(k==0)?lo:hi;
    gAlong=t;gEdge=float(gl_PrimitiveIDIn);
    gDepth=mix(vDepth[0],vDepth[1],t);
    gObject=mix(vObject[0],vObject[1],t);
    gNdc=(k==0)?na:nb;gSide=side;
    EmitVertex();
  }
  EndPrimitive();
}
