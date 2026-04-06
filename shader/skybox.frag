#version 460 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
    vec3 dir = normalize(TexCoords);
    vec3 color = texture(skybox, dir).rgb;
    float horizon = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    color = mix(color * 0.75, color * 1.12, horizon);
    FragColor = vec4(color, 1.0);
}
