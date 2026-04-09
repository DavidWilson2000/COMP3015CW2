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
out vec2 WorldXZ;

float waveHeightFn(float x, float z, float t)
{
    float h = 0.0;
    h += sin(x * 0.30 + t * 1.35) * 0.16;
    h += cos(z * 0.22 + t * 1.10) * 0.12;
    h += sin((x + z) * 0.12 + t * 0.80) * 0.10;
    return h;
}

void main()
{
    vec3 pos = aPos;
    pos.y += waveHeightFn(aPos.x, aPos.z, time);

    float dhdx = cos(aPos.x * 0.30 + time * 1.35) * (0.30 * 0.16)
               + cos((aPos.x + aPos.z) * 0.12 + time * 0.80) * (0.12 * 0.10);

    float dhdz = -sin(aPos.z * 0.22 + time * 1.10) * (0.22 * 0.12)
               + cos((aPos.x + aPos.z) * 0.12 + time * 0.80) * (0.12 * 0.10);

    vec3 localNormal = normalize(vec3(-dhdx, 1.0, -dhdz));
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = worldPos.xyz;
    Normal = normalize(normalMatrix * localNormal);
    TexCoord = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    waveHeight = pos.y;
    WorldXZ = worldPos.xz;

    gl_Position = projection * view * worldPos;
}
