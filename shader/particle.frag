#version 460 core
in vec4 ParticleColor;
out vec4 FragColor;

void main()
{
    vec2 c = gl_PointCoord * 2.0 - 1.0;
    float d = dot(c, c);
    if (d > 1.0) discard;
    float alpha = (1.0 - d) * ParticleColor.a;
    FragColor = vec4(ParticleColor.rgb, alpha);
}
