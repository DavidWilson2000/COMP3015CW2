#version 460 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;
uniform float time;
uniform float worldDanger;

float hash21(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 5; ++i)
    {
        value += noise(p) * amp;
        p = p * 2.03 + vec2(11.7, 7.9);
        amp *= 0.5;
    }
    return value;
}

void main()
{
    vec3 dir = normalize(TexCoords);
    vec3 color = texture(skybox, dir).rgb;
    float horizon = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    color = mix(color * 0.75, color * 1.12, horizon);

    if (dir.y > -0.05)
    {
        vec2 cloudUV = dir.xz / max(abs(dir.y) + 0.35, 0.25);
        cloudUV = cloudUV * 1.35 + vec2(time * 0.012, time * 0.006);
        float cloud = fbm(cloudUV);
        cloud = smoothstep(0.54 - worldDanger * 0.06, 0.78, cloud);
        float cloudMask = cloud * smoothstep(-0.02, 0.55, dir.y);
        vec3 cloudColor = mix(vec3(0.70, 0.74, 0.78), vec3(0.88, 0.89, 0.91), horizon);
        color = mix(color, cloudColor, cloudMask * 0.50);
    }

    FragColor = vec4(color, 1.0);
}
