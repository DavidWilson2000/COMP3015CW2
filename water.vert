#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform float time;

out vec2 TexCoord;
out float waveHeight;

void main()
{
    vec3 pos = aPos;
    pos.y += sin(pos.x * 0.25 + time) * 0.1;
    pos.y += cos(pos.z * 0.25 + time * 1.2) * 0.1;

    waveHeight = pos.y;
    TexCoord = aTexCoord;

    gl_Position = Projection * View * Model * vec4(pos, 1.0);
}