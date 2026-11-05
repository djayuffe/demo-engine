#version 410 core
// Dual-filter downsample (13-tap, Jimenez 2014). uFirst=1 applies a soft brightness knee and a Karis
// average so single fireflies (thin bright wires) do not strobe the whole bloom chain.
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uSrc;
vec2 sourceTexel;   // computed on GPU; no per-frame texture-size driver queries
uniform float uFirst;
vec3 knee(vec3 c){
  float l=max(c.r,max(c.g,c.b));
  float w=clamp((l-.35)/.5,0.,1.);
  return c*(w*w*(3.-2.*w));
}
vec3 fetch(vec2 o){
  vec3 c=texture(uSrc,uv+o*sourceTexel).rgb;
  // Leave headroom for additive pyramid reconstruction in half-float targets.
  c=(any(isnan(c))||any(isinf(c)))?vec3(0.):clamp(c,0.,16376.);
  return uFirst>.5?knee(c):c;
}
float karis(vec3 c){return 1./(1.+dot(c,vec3(.2126,.7152,.0722)));}
void main(){
  sourceTexel=1./vec2(textureSize(uSrc,0));
  vec3 a=fetch(vec2(-2,2)),b=fetch(vec2(0,2)),c=fetch(vec2(2,2));
  vec3 d=fetch(vec2(-2,0)),e=fetch(vec2(0,0)),f=fetch(vec2(2,0));
  vec3 g=fetch(vec2(-2,-2)),h=fetch(vec2(0,-2)),i=fetch(vec2(2,-2));
  vec3 j=fetch(vec2(-1,1)),k=fetch(vec2(1,1)),l=fetch(vec2(-1,-1)),m=fetch(vec2(1,-1));
  vec3 r;
  if(uFirst>.5){
    vec3 g0=(a+b+d+e)*.25,g1=(b+c+e+f)*.25,g2=(d+e+g+h)*.25,g3=(e+f+h+i)*.25,g4=(j+k+l+m)*.25;
    float w0=karis(g0),w1=karis(g1),w2=karis(g2),w3=karis(g3),w4=karis(g4);
    r=(g0*w0*.125+g1*w1*.125+g2*w2*.125+g3*w3*.125+g4*w4*.5)/(.125*(w0+w1+w2+w3)+.5*w4);
  } else {
    r=e*.125+(a+c+g+i)*.03125+(b+d+f+h)*.0625+(j+k+l+m)*.125;
  }
  FragColor=vec4(r,1);
}
