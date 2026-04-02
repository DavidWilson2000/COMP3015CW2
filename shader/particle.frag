#version 460 core

in vec4 ParticleColor;
out vec4 FragColor;

void main()
{
    vec2 centered = gl_PointCoord * 2.0 - 1.0;
    float d = dot(centered, centered);
    if (d > 1.0)
        discard;

    float alpha = (1.0 - smoothstep(0.2, 1.0, d)) * ParticleColor.a;
    FragColor = vec4(ParticleColor.rgb, alpha);
}
