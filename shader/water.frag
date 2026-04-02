#version 460 core

in vec3 FragPos;
in vec3 Normal;
in float waveHeight;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 zoneTint;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float worldDanger;
uniform sampler2D shadowMap;

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float bias = max(0.0015 * (1.0 - dot(normal, lightDir)), 0.0004);
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float shadow = calculateShadow(FragPosLightSpace, norm, lightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 48.0);

    vec3 deepColor = mix(vec3(0.02, 0.10, 0.18), zoneTint * 0.45, 0.55);
    vec3 shallowColor = mix(vec3(0.06, 0.26, 0.36), zoneTint * 0.95, 0.65);
    float waveMix = clamp((waveHeight + 0.28) * 1.45, 0.0, 1.0);
    vec3 base = mix(deepColor, shallowColor, waveMix);

    float foam = smoothstep(0.20, 0.34, waveHeight + fresnel * 0.16);
    vec3 foamColor = mix(vec3(0.80, 0.90, 0.96), vec3(0.66, 0.82, 0.90), worldDanger);

    vec3 ambient = base * 0.34;
    vec3 diffuse = base * diff * (1.0 - shadow * 0.60);
    vec3 highlights = vec3(0.90, 0.96, 1.0) * spec * (1.0 - shadow * 0.45);
    vec3 color = ambient + diffuse + highlights + foamColor * foam * 0.30 + foamColor * fresnel * 0.18;

    float distanceToCamera = length(viewPos - FragPos);
    float fogAmount = 1.0 - exp(-pow(distanceToCamera * (fogDensity * 1.15), 1.25));
    fogAmount = clamp(fogAmount, 0.0, 1.0);
    color = mix(color, fogColor, fogAmount);

    FragColor = vec4(color, 0.98);
}
