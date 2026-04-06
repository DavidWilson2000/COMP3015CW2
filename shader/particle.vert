#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aSize;

uniform mat4 view;
uniform mat4 projection;

out vec4 ParticleColor;

void main()
{
    ParticleColor = aColor;
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize = aSize;
}
