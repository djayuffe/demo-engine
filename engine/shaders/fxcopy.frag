#version 410 core
// Plain copy: used to capture the HDR scene into one tile of the temporal-history atlas.
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uSrc;
void main(){ FragColor=texture(uSrc,uv); }
