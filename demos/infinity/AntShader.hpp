#pragma once
namespace infinity_ants {
// Nine toroidal images of each seven-pixel sprite. Scissored by the 64x64 FBO,
// this shades the same pixel support as Traffic::raster without a CPU image upload.
constexpr const char* vertex = R"GLSL(#version 410 core
uniform vec4 uAnts[16];
uniform vec4 uFlash[4];
uniform float uPhase;
flat out vec2 vPosition;
flat out vec2 vDirection;
flat out vec2 vMetadata;
void main(){
  int id=gl_InstanceID/9,copy=gl_InstanceID%9;
  vec4 ant=uAnts[id];
  vec2 offset=vec2(copy%3-1,copy/3-1)*64.;
  vec2 position=(ant.xy+ant.zw*uPhase)*8.+offset;
  vec2 corner=vec2((gl_VertexID==1||gl_VertexID==2||gl_VertexID==4)?1.:0.,
                   (gl_VertexID==2||gl_VertexID==4||gl_VertexID==5)?1.:0.);
  vec2 pixel=floor(position)-3.+corner*7.;
  gl_Position=vec4(pixel/32.-1.,0.,1.);
  vPosition=position;vDirection=ant.zw;
  vMetadata=vec2(uFlash[id/4][id%4]*(1.-uPhase),float(id)/15.);
}
)GLSL";
constexpr const char* fragment = R"GLSL(#version 410 core
flat in vec2 vPosition;
flat in vec2 vDirection;
flat in vec2 vMetadata;
out vec4 color;
void main(){
  vec2 r=gl_FragCoord.xy-vPosition;
  float along=dot(r,vDirection),across=r.x*vDirection.y-r.y*vDirection.x;
  float d=dot(vDirection,vDirection)>0.?along*along*.65+across*across*2.:dot(r,r);
  float body=exp(-d*1.2);
  color=body*vec4(1.,vMetadata,1.);
}
)GLSL";
} // namespace infinity_ants
