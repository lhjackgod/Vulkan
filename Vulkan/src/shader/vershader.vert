#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

layout(location = 0) out vec3 vColor;
layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 perspective;
};
void main()
{
    gl_Position = perspective * view * model * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}