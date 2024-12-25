#version 450 core

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vTexCoord;
layout(binding = 1) uniform sampler2D texSampler;
void main()
{
    vec3 TexColor = texture(texSampler, vTexCoord * 2.0).rgb;
    outColor = vec4(vColor * TexColor, 1.0);
}