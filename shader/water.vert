#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform float time;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;
out float waveHeight;

void main()
{
    vec3 pos = aPos;
    pos.y += sin(pos.x * 0.30 + time * 1.35) * 0.16;
    pos.y += cos(pos.z * 0.22 + time * 1.10) * 0.12;
    pos.y += sin((pos.x + pos.z) * 0.12 + time * 0.80) * 0.10;

    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = worldPos.xyz;
    Normal = aNormal;
    TexCoord = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    waveHeight = pos.y;

    gl_Position = projection * view * worldPos;
}
