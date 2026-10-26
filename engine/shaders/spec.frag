#version 410 core
float deSquare(float x){return x*x;}
// Pre-post composite: temporal refraction (spec effect #13) and the persistent memory layer (#25).
// Temporal refraction: inside a glass sphere every pixel is taken from a different moment of the
// recent past, C_out(u,t) = C_scene(u', t - delay(u)), with delay = D * (0.5 + A(1-q^2) sin(k q - w t)).
// The past comes from a ring of 16 quarter-resolution frames packed into a 4x4 atlas.
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uScene;
uniform sampler2D uHist;    // 4x4 atlas of past frames
uniform sampler2D uMem;     // memory layer
uniform float uAspect;
uniform float uTime;
uniform float uTemporal;    // 0..1 strength (also the sphere's visibility)
uniform float uMemory;      // 0..1 strength of the memory layer
uniform int uHead;          // atlas tile holding the newest frame
uniform float uFilled;      // how many tiles hold real history (1..16)
uniform float uInterval;    // seconds between captured frames
uniform vec2 uTexel;        // 1 / atlas size
const float kMaxDelay=3.0;  // seconds of past reachable through the sphere
const float kRadius=.65;    // sphere radius in the [-1,1] screen space

vec3 tile(int i,vec2 u){
  i=((i%16)+16)%16;
  vec2 inset=2.*uTexel;                                    // keep bilinear taps inside the tile
  vec2 l=clamp(u,inset*4.,1.-inset*4.);
  return texture(uHist,(vec2(float(i%4),float(i/4))+l)*.25).rgb;
}
void main(){
  vec3 col=texture(uScene,uv).rgb;
  if(uTemporal>.002){
    vec2 p=(uv*2.-1.)*vec2(uAspect,1.);
    float q=length(p)/kRadius;
    if(q<1.15){
      float dtOff=.5*(1.-min(q*q,1.))*sin(18.*min(q,1.)-uTime*1.7);        // in [-.5,.5]
      vec2 u2=clamp(uv+normalize(p+vec2(1e-5))*dtOff*.035,0.,1.);          // spatial refraction through the surface
      float age=clamp(kMaxDelay*(.5+dtOff)/uInterval,0.,uFilled-1.);         // frames into the past
      float a0=floor(age);
      vec3 h=mix(tile(uHead-int(a0),u2),tile(uHead-int(a0)-1,u2),(a0+1.<uFilled)?fract(age):0.);
      float inside=(1.0-smoothstep(.93,1.,q))*uTemporal;
      col=mix(col,h*1.15,inside);
      col+=vec3(.25,.55,1.)*exp(-deSquare((q-1.)*14.))*uTemporal*.55;        // glass rim so the sphere reads
    }
  }
  if(uMemory>.002) col+=texture(uMem,uv).rgb*vec3(1.15,.9,1.45)*uMemory*.9;
  FragColor=vec4(col,1.);
}
