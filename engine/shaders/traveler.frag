#version 410 core
in float vGlow;
in float vPath;
out vec4 FragColor;
uniform vec3 uColor;
void main(){
  // hue shifts per traveler + glows when near center
  float h=vPath;
  vec3 tint=vec3(0.5)+0.5*cos(6.2831853*(vec3(0.0,0.33,0.67)+h));
  vec3 col=mix(uColor,tint,0.5)*(0.8+0.7*vGlow);
  FragColor=vec4(col,1.0);
}
