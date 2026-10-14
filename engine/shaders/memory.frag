#version 410 core
// Persistent reflection memory (spec effect #25, screen-space form): the previous state is advected
// along a slow swirling flow, diffused, hue-aged, decayed and re-energised by the current frame:
//   R' = decay * (R(u') + beta * laplacian(R)) + (1 - decay) * L
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uPrev;
uniform sampler2D uScene;
uniform float uTime;
uniform float uDt;       // seconds since the previous update
uniform float uAspect;
uniform vec2 uTexel;     // 1 / memory texture size
void main(){
  vec2 p=(uv-.5)*vec2(uAspect,1.);
  vec2 flow=vec2(sin(uv.y*7.+uTime*.31),cos(uv.x*6.+uTime*.27))*.018+vec2(-p.y,p.x)*.05;   // advection, UV units per second
  vec2 u0=clamp(uv-flow*uDt,0.,1.);
  vec3 c=texture(uPrev,u0).rgb;
  vec3 lap=texture(uPrev,u0+vec2(uTexel.x,0)).rgb+texture(uPrev,u0-vec2(uTexel.x,0)).rgb
          +texture(uPrev,u0+vec2(0,uTexel.y)).rgb+texture(uPrev,u0-vec2(0,uTexel.y)).rgb-4.*c;
  c+=lap*.12*clamp(uDt*60.,0.,2.);                                    // surface diffusion
  c=mix(c,c.gbr,.5*(1.-exp(-uDt*.9)));                                  // nonlinear ageing: colours cycle as they get old
  float decay=exp(-uDt/1.6);
  vec3 L=texture(uScene,uv).rgb;
  c=c*decay+L*(1.-decay)*.6;
  FragColor=vec4(min(c,vec3(8.)),1.);
}
