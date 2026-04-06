#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;
in float waveHeight;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 zoneTint;
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;
uniform float worldDanger;
uniform float time;
uniform sampler2D shadowMap;

float calculateShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - 0.0015) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main()
{
    vec3 baseDeep = vec3(0.02, 0.16, 0.28);
    vec3 baseShallow = vec3(0.08, 0.42, 0.58);
    float t = clamp((waveHeight + 0.25) * 1.4, 0.0, 1.0);
    vec3 waterColor = mix(baseDeep, baseShallow, t);
    waterColor = mix(waterColor, zoneTint, 0.35 + worldDanger * 0.15);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);
    float fresnel = pow(1.0 - max(dot(normalize(vec3(0.0,1.0,0.0)), viewDir), 0.0), 3.0);
    float sparkle = pow(max(dot(reflect(-lightDir, vec3(0.0,1.0,0.0)), viewDir), 0.0), 36.0) * 0.35;
    float foam = smoothstep(0.12, 0.22, abs(sin(TexCoord.x * 2.2 + time * 0.5) * cos(TexCoord.y * 1.8 + time * 0.45)));

    float shadow = calculateShadow(FragPosLightSpace);
    vec3 color = waterColor * (0.82 - shadow * 0.20);
    color += vec3(0.18, 0.22, 0.28) * fresnel;
    color += vec3(0.85, 0.92, 1.0) * sparkle;
    color += vec3(0.65, 0.75, 0.82) * foam * 0.10;

    float dist = length(viewPos - FragPos);
    float fogFactor = clamp((fogFar - dist) / max(fogFar - fogNear, 0.001), 0.0, 1.0);
    vec3 finalColor = mix(fogColor, color, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
