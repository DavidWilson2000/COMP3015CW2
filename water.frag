#version 460 core

in vec2 TexCoord;
in float waveHeight;

out vec4 FragColor;

void main()
{
    vec3 deepColor = vec3(0.0, 0.15, 0.35);
    vec3 shallowColor = vec3(0.0, 0.35, 0.55);

    float t = clamp((waveHeight + 0.2) * 2.0, 0.0, 1.0);
    vec3 color = mix(deepColor, shallowColor, t);

    FragColor = vec4(color, 1.0);
}