#version 460 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;
uniform float time;
uniform float worldDanger;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float dayFactor;

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
        p = p * 2.04 + vec2(13.1, 8.7);
        amp *= 0.5;
    }
    return value;
}

void main()
{
    vec3 dir = normalize(TexCoords);
    vec3 base = texture(skybox, dir).rgb;

    float horizon = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 zenithNight = vec3(0.03, 0.05, 0.10);
    vec3 horizonNight = vec3(0.08, 0.10, 0.16);
    vec3 zenithDay = mix(vec3(0.18, 0.24, 0.34), vec3(0.52, 0.70, 0.92), 1.0 - worldDanger * 0.75);
    vec3 horizonDay = mix(vec3(0.72, 0.66, 0.58), vec3(0.84, 0.89, 0.96), 1.0 - worldDanger * 0.55);

    vec3 zenithTint = mix(zenithNight, zenithDay, dayFactor);
    vec3 horizonTint = mix(horizonNight, horizonDay, dayFactor);
    vec3 gradient = mix(horizonTint, zenithTint, smoothstep(0.0, 1.0, horizon));

    vec3 color = mix(base * mix(0.25, 1.0, dayFactor), gradient, 0.60);

    float sunAmount = max(dot(dir, normalize(sunDirection)), 0.0);
    float sunGlow = pow(sunAmount, 24.0) * mix(0.35, 1.0, dayFactor) * (1.0 - worldDanger * 0.55);
    float sunCore = pow(sunAmount, 340.0) * mix(0.20, 1.0, dayFactor) * (1.0 - worldDanger * 0.85);
    color += sunColor * sunGlow;
    color += mix(vec3(1.0), sunColor, 0.35) * sunCore;

    if (dir.y > -0.12)
    {
        vec2 cloudUV0 = dir.xz / max(abs(dir.y) + 0.34, 0.24);
        vec2 cloudUV1 = dir.xz / max(abs(dir.y) + 0.48, 0.24);
        cloudUV0 = cloudUV0 * 1.18 + vec2(time * 0.010, time * 0.004);
        cloudUV1 = cloudUV1 * 2.05 + vec2(-time * 0.018, time * 0.009);

        float cloud0 = fbm(cloudUV0);
        float cloud1 = fbm(cloudUV1);
        float cloud = mix(cloud0, cloud1, 0.35);
        float thresholdLo = 0.53 - worldDanger * 0.08;
        float thresholdHi = 0.76 - worldDanger * 0.02;
        cloud = smoothstep(thresholdLo, thresholdHi, cloud);
        float cloudMask = cloud * smoothstep(-0.04, 0.58, dir.y);

        vec3 cloudBright = mix(vec3(0.20, 0.24, 0.30), vec3(0.90, 0.91, 0.94), dayFactor);
        vec3 cloudDark = mix(vec3(0.06, 0.08, 0.12), vec3(0.48, 0.54, 0.62), dayFactor);
        cloudBright = mix(cloudBright, vec3(0.80, 0.82, 0.87), worldDanger * 0.45);
        cloudDark = mix(cloudDark, vec3(0.24, 0.28, 0.34), worldDanger * 0.55);
        vec3 cloudColor = mix(cloudDark, cloudBright, horizon);
        color = mix(color, cloudColor, cloudMask * mix(0.28, 0.52, dayFactor));
    }

    float nightMask = 1.0 - dayFactor;
    if (nightMask > 0.01 && dir.y > 0.0)
    {
        vec2 starUV = normalize(dir).xz * 220.0 + vec2(0.0, time * 0.002);
        float starField = hash21(floor(starUV));
        float star = step(0.9925, starField) * smoothstep(0.05, 0.45, dir.y);
        color += vec3(0.72, 0.80, 0.95) * star * nightMask * 0.9;
    }

    float stormBand = smoothstep(0.15, -0.25, dir.y) * worldDanger;
    color = mix(color, color * vec3(0.82, 0.88, 0.95), stormBand * 0.18);

    FragColor = vec4(color, 1.0);
}
