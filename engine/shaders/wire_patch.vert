#version 410 core
layout(location=0) in vec3 aPos;
// Tessellate in object space, then deform each generated vertex in the evaluation stage.
void main(){gl_Position=vec4(aPos,1.0);}
