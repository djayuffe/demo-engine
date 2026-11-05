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
  float moving=step(.5,dot(ant.zw,ant.zw));
  float arc=sin(3.14159265*uPhase);
  float cadence=clamp(uPhase+arc*.22*sin(float(id+1)*2.17),0.,1.);
  float wobble=arc*.65*sin(float(id+1)*4.31+uPhase*3.14)*moving;
  vec2 position=(ant.xy+ant.zw*cadence)*8.+vec2(-ant.w,ant.z)*wobble+offset;
  vec2 corner=vec2((gl_VertexID==1||gl_VertexID==2||gl_VertexID==4)?1.:0.,
                   (gl_VertexID==2||gl_VertexID==4||gl_VertexID==5)?1.:0.);
  vec2 pixel=floor(position)-3.+corner*7.;
  gl_Position=vec4(pixel/32.-1.,0.,1.);
  vPosition=position;vDirection=ant.zw;
  vMetadata=vec2(uFlash[id/4][id%4]*(1.-cadence),float(id)/15.);
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
  float moving=step(.5,dot(vDirection,vDirection));
  float thorax=exp(-(along*along*.85+across*across*2.4)*1.15);
  float head=exp(-((along-1.05)*(along-1.05)*1.7+across*across*3.0));
  float abdomen=exp(-((along+1.0)*(along+1.0)*.75+across*across*1.75));
  float legs=exp(-abs(abs(across)-1.15)*4.2-abs(along)*.7)*.16;
  float detailed=.52*thorax+.32*head+.48*abdomen+legs;
  float resting=exp(-dot(r,r)*1.2);
  float variation=.88+.12*sin((vMetadata.y*15.+1.)*1.73);
  float body=mix(resting,detailed,moving)*variation;
  color=body*vec4(1.,vMetadata,1.);
}
)GLSL";
} // namespace infinity_ants
