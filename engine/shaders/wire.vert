#version 410 core
layout(location=0) in vec3 aPos;
#include "lib/wire_vertex.glsl"
void main(){transformWire(aPos);}
