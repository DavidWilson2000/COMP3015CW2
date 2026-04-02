#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform float time;
uniform float waveStrength;

out vec3 FragPos;
out vec3 Normal;
out float waveHeight;
out vec4 FragPosLightSpace;

void main()
{
    vec3 pos = aPos;

    float waveA = sin(pos.x * 0.22 + time * 1.2) * 0.18 * waveStrength;
    float waveB = cos(pos.z * 0.24 + time * 1.5) * 0.14 * waveStrength;
    float waveC = sin((pos.x + pos.z) * 0.16 + time * 0.85) * 0.10 * waveStrength;
    pos.y += waveA + waveB + waveC;

    vec3 tangentX = vec3(1.0, 0.22 * cos(pos.x * 0.22 + time * 1.2) * 0.18 * waveStrength +
                               0.16 * cos((pos.x + pos.z) * 0.16 + time * 0.85) * 0.10 * waveStrength, 0.0);
    vec3 tangentZ = vec3(0.0, -0.24 * sin(pos.z * 0.24 + time * 1.5) * 0.14 * waveStrength +
                               0.16 * cos((pos.x + pos.z) * 0.16 + time * 0.85) * 0.10 * waveStrength, 1.0);
    vec3 localNormal = normalize(cross(tangentZ, tangentX));

    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = worldPos.xyz;
    Normal = normalize(mat3(transpose(inverse(model))) * localNormal);
    waveHeight = pos.y;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
