#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(location = 0) out vec3 vColor; 
void main()
{
    gl_Position = vec4(position[gl_VertexIndex], 0.0, 1.0);
    vColor = aColor;
}