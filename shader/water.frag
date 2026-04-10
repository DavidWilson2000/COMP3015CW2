#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;
in float waveHeight;
in vec2 WorldXZ;

out vec4 FragColor;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 viewPos;
uniform vec3 zoneTint;
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;
uniform float worldDanger;
uniform float time;
uniform float sunIntensity;
uniform float ambientStrength;
uniform int shadowFilterMode;
uniform float shadowLightSize;
uniform float shadowSoftnessScale;
uniform sampler2D shadowMap;

const vec2 kPoisson[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870), vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845), vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554), vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507), vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367), vec2( 0.14383161, -0.14100790)
);

float hash12(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float saturate(float v)
{
    return clamp(v, 0.0, 1.0);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; ++i)
    {
        value += noise(p) * amp;
        p = p * 2.03 + vec2(9.7, 4.1);
        amp *= 0.5;
    }
    return value;
}

float sampleShadowFiltered(vec2 uv, float compareDepth, float bias, vec2 texelSize, mat2 rotation, float radius)
{
    float shadow = 0.0;
    float weightSum = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        vec2 offset = rotation * kPoisson[i] * radius;
        float pcfDepth = texture(shadowMap, uv + offset).r;
        float weight = 1.0 - saturate(length(kPoisson[i]) * 0.58);
        shadow += ((compareDepth - bias) > pcfDepth ? 1.0 : 0.0) * weight;
        weightSum += weight;
    }
    return shadow / max(weightSum, 0.0001);
}

float findAverageBlocker(vec2 uv, float compareDepth, float bias, vec2 texelSize, mat2 rotation, float searchRadius)
{
    float blockerDepth = 0.0;
    float blockers = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        vec2 offset = rotation * kPoisson[i] * searchRadius;
        float sampleDepth = texture(shadowMap, uv + offset).r;
        if ((compareDepth - bias) > sampleDepth)
        {
            blockerDepth += sampleDepth;
            blockers += 1.0;
        }
    }

    if (blockers < 0.5)
        return -1.0;

    return blockerDepth / blockers;
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / max(fragPosLightSpace.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float bias = max(0.0025 * (1.0 - max(dot(normal, lightDir), 0.0)), 0.0012);

    float randAngle = hash12(WorldXZ * 0.17 + projCoords.xy * 31.0) * 6.2831853;
    mat2 rotation = mat2(cos(randAngle), -sin(randAngle), sin(randAngle), cos(randAngle));

    float receiverDepth = projCoords.z;
    float pcfRadius = texelSize.x * mix(1.0, 2.0, saturate(receiverDepth));
    pcfRadius *= mix(0.95, 1.25, worldDanger);

    if (shadowFilterMode == 0)
    {
        return sampleShadowFiltered(projCoords.xy, receiverDepth, bias, texelSize, rotation, pcfRadius);
    }

    float searchRadius = texelSize.x * mix(2.2, 4.6, saturate(receiverDepth));
    searchRadius *= mix(0.95, 1.25, worldDanger);
    float avgBlockerDepth = findAverageBlocker(projCoords.xy, receiverDepth, bias, texelSize, rotation, searchRadius);
    if (avgBlockerDepth < 0.0)
        return 0.0;

    float penumbra = max(receiverDepth - avgBlockerDepth, 0.0) / max(avgBlockerDepth, 0.0001);
    float filterRadius = shadowLightSize * penumbra * shadowSoftnessScale;
    filterRadius = clamp(filterRadius, texelSize.x * 1.25, texelSize.x * shadowSoftnessScale);
    return sampleShadowFiltered(projCoords.xy, receiverDepth, bias, texelSize, rotation, filterRadius);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(sunDirection);
    vec3 halfDir = normalize(viewDir + lightDir);

    vec3 baseDeep = vec3(0.02, 0.13, 0.24);
    vec3 baseShallow = vec3(0.10, 0.44, 0.60);
    float heightMix = clamp((waveHeight + 0.26) * 1.45, 0.0, 1.0);
    vec3 waterColor = mix(baseDeep, baseShallow, heightMix);
    waterColor = mix(waterColor, zoneTint, 0.34 + worldDanger * 0.18);

    vec2 flowUV0 = WorldXZ * 0.065 + vec2(time * 0.05, -time * 0.03);
    vec2 flowUV1 = WorldXZ * 0.13 + vec2(-time * 0.08, time * 0.04);
    float micro = fbm(flowUV0);
    float foamNoise = fbm(flowUV1 + micro * 0.7);

    float crest = smoothstep(0.10, 0.22, waveHeight + micro * 0.06);
    float foam = smoothstep(0.58, 0.80, foamNoise + crest * 0.22);
    foam *= 0.24 + worldDanger * 0.16;

    float fresnel = pow(1.0 - max(dot(norm, viewDir), 0.0), 4.0);
    float ndotl = max(dot(norm, lightDir), 0.0);
    float sparkle = pow(max(dot(norm, halfDir), 0.0), 96.0) * (0.22 + micro * 0.10);
    float backScatter = pow(1.0 - max(dot(-viewDir, lightDir), 0.0), 3.0) * 0.10;

    float shadow = calculateShadow(FragPosLightSpace, norm, lightDir);

    vec3 sunlightTint = mix(vec3(0.80, 0.88, 0.94), sunColor, 0.65);
    vec3 ambientTint = mix(vec3(0.04, 0.10, 0.16), sunColor * vec3(0.32, 0.28, 0.24), ambientStrength);
    vec3 subsurface = vec3(0.08, 0.20, 0.24) * backScatter * (0.55 + worldDanger * 0.25);

    vec3 color = waterColor * (0.72 + ambientStrength * 0.42);
    color = mix(color, color * 0.72, shadow);
    color += waterColor * ndotl * 0.26 * sunlightTint * sunIntensity;
    color += vec3(0.18, 0.28, 0.34) * fresnel;
    color += sunlightTint * sparkle * (0.85 + sunIntensity * 0.35);
    color += vec3(0.72, 0.82, 0.88) * foam;
    color += subsurface;
    color += ambientTint * waterColor * 0.34;

    float dist = length(viewPos - FragPos);
    float fogFactor = clamp((fogFar - dist) / max(fogFar - fogNear, 0.001), 0.0, 1.0);
    fogFactor *= fogFactor;
    vec3 finalColor = mix(fogColor, color, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
