#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D materialTex;
uniform sampler2D shadowMap;
uniform int materialType;
uniform int shadowFilterMode;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 viewPos;
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;
uniform float worldDanger;
uniform float sunIntensity;
uniform float ambientStrength;
uniform float shadowLightSize;
uniform float shadowSoftnessScale;

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

float sampleShadowFiltered(vec2 uv, float compareDepth, float bias, vec2 texelSize, mat2 rotation, float radius)
{
    float shadow = 0.0;
    float weightSum = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        vec2 sampleOffset = rotation * kPoisson[i] * radius;
        float sampleDepth = texture(shadowMap, uv + sampleOffset).r;
        float distWeight = 1.0 - saturate(length(kPoisson[i]) * 0.55);
        shadow += ((compareDepth - bias) > sampleDepth ? 1.0 : 0.0) * distWeight;
        weightSum += distWeight;
    }
    return shadow / max(weightSum, 0.0001);
}

float findAverageBlocker(vec2 uv, float compareDepth, float bias, vec2 texelSize, mat2 rotation, float searchRadius)
{
    float blockerDepth = 0.0;
    float blockers = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        vec2 sampleOffset = rotation * kPoisson[i] * searchRadius;
        float sampleDepth = texture(shadowMap, uv + sampleOffset).r;
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

    float ndotl = max(dot(normal, lightDir), 0.0);
    float bias = max(0.0035 * (1.0 - ndotl), 0.0010);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    float randAngle = hash12(FragPos.xz * 1.91 + projCoords.xy * 23.7) * 6.2831853;
    mat2 rotation = mat2(cos(randAngle), -sin(randAngle), sin(randAngle), cos(randAngle));

    float receiverDepth = projCoords.z;
    float pcfRadius = texelSize.x * mix(1.2, 2.6, saturate(receiverDepth));
    pcfRadius *= mix(0.95, 1.35, worldDanger);

    if (shadowFilterMode == 0)
    {
        return sampleShadowFiltered(projCoords.xy, receiverDepth, bias, texelSize, rotation, pcfRadius);
    }

    float searchRadius = texelSize.x * mix(2.5, 5.5, saturate(receiverDepth));
    searchRadius *= mix(0.95, 1.30, worldDanger);
    float avgBlockerDepth = findAverageBlocker(projCoords.xy, receiverDepth, bias, texelSize, rotation, searchRadius);
    if (avgBlockerDepth < 0.0)
        return 0.0;

    float penumbra = max(receiverDepth - avgBlockerDepth, 0.0) / max(avgBlockerDepth, 0.0001);
    float filterRadius = shadowLightSize * penumbra * shadowSoftnessScale;
    filterRadius = clamp(filterRadius, texelSize.x * 1.5, texelSize.x * shadowSoftnessScale);

    return sampleShadowFiltered(projCoords.xy, receiverDepth, bias, texelSize, rotation, filterRadius);
}

void main()
{
    vec3 texColor = texture(materialTex, TexCoord).rgb;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(sunDirection);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    float roughness = 0.55;
    float specStrength = 0.16;
    float ambientBoost = 1.0;
    vec3 materialTint = vec3(1.0);

    if (materialType == 0) // wood
    {
        roughness = 0.72;
        specStrength = 0.08;
        materialTint = vec3(1.08, 1.00, 0.96);
    }
    else if (materialType == 1) // rock
    {
        roughness = 0.82;
        specStrength = 0.06;
        materialTint = vec3(0.95, 0.98, 1.02);
    }
    else if (materialType == 2) // grass
    {
        roughness = 0.88;
        specStrength = 0.04;
        ambientBoost = 1.08;
        materialTint = vec3(0.96, 1.06, 0.96);
    }
    else if (materialType == 3) // boat
    {
        roughness = 0.42;
        specStrength = 0.18;
        materialTint = vec3(1.04, 0.99, 0.98);
    }
    else if (materialType == 4) // buoy
    {
        roughness = 0.36;
        specStrength = 0.22;
        materialTint = vec3(1.05, 1.03, 1.00);
    }
    else if (materialType == 5) // emissive/light object
    {
        roughness = 0.18;
        specStrength = 0.34;
        ambientBoost = 1.15;
        materialTint = vec3(1.16, 1.11, 1.02);
    }

    texColor *= materialTint;

    float ndotl = max(dot(norm, lightDir), 0.0);
    float wrappedDiffuse = saturate((dot(norm, lightDir) + 0.25) / 1.25);
    float diff = mix(ndotl, wrappedDiffuse, 0.28);

    float specPower = mix(8.0, 96.0, 1.0 - roughness);
    float spec = pow(max(dot(norm, halfDir), 0.0), specPower) * specStrength;

    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 5.0);
    float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 2.25) * mix(0.05, 0.16, worldDanger);

    float upness = norm.y * 0.5 + 0.5;
    vec3 hemiSky = mix(fogColor, sunColor, 0.38) * ((ambientStrength + worldDanger * 0.08) * ambientBoost);
    vec3 hemiGround = vec3(0.10, 0.12, 0.14) * (1.0 - worldDanger * 0.15);
    vec3 hemiAmbient = mix(hemiGround, hemiSky, upness);

    float shadow = calculateShadow(FragPosLightSpace, norm, lightDir);

    vec3 diffuseLight = texColor * diff * sunColor * sunIntensity;
    vec3 specColor = mix(vec3(1.0), texColor, 0.12) * spec * sunColor * sunIntensity;
    vec3 lit = hemiAmbient * texColor + (1.0 - shadow) * (diffuseLight + specColor);
    lit += texColor * rim;
    lit += sunColor * fresnel * 0.03;

    if (materialType == 5)
    {
        lit += texColor * (0.18 + 0.10 * fresnel);
    }

    float dist = length(viewPos - FragPos);
    float fogFactor = clamp((fogFar - dist) / max(fogFar - fogNear, 0.001), 0.0, 1.0);
    fogFactor *= fogFactor;
    vec3 finalColor = mix(fogColor, lit, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
