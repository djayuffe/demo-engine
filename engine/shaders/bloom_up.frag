#version 410 core
// Dual-filter upsample (9-tap tent); blended additively onto the next-larger level.
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uSrc;
uniform vec2 uTexel;   // 1/size of the SOURCE (the smaller level)
uniform float uWeight;
void main(){
  vec3 s=texture(uSrc,uv+uTexel*vec2(-1,-1)).rgb+texture(uSrc,uv+uTexel*vec2(1,-1)).rgb
        +texture(uSrc,uv+uTexel*vec2(-1,1)).rgb+texture(uSrc,uv+uTexel*vec2(1,1)).rgb;
  s+=2.*(texture(uSrc,uv+uTexel*vec2(-1,0)).rgb+texture(uSrc,uv+uTexel*vec2(1,0)).rgb
        +texture(uSrc,uv+uTexel*vec2(0,-1)).rgb+texture(uSrc,uv+uTexel*vec2(0,1)).rgb);
  s+=4.*texture(uSrc,uv).rgb;
  FragColor=vec4(s/16.*uWeight,1);
}
