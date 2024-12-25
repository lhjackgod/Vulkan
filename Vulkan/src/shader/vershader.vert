#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vTexCoord;
layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 perspective;
}ubo;
void main()
{
    vTexCoord = aTexCoord;
    gl_Position = ubo.perspective * ubo.view * ubo.model * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}