#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform sampler2D shadowMap;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float worldDanger;

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float bias = max(0.0025 * (1.0 - dot(normal, lightDir)), 0.0008);
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

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float shadow = calculateShadow(FragPosLightSpace, norm, lightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 24.0);
    float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 2.5);

    float grain = hash21(FragPos.xz * 0.35 + FragPos.y * 0.1);
    vec3 base = objectColor * mix(0.92, 1.08, grain);
    base *= mix(1.0, 0.92, worldDanger * 0.4);

    vec3 ambient = base * (0.28 - worldDanger * 0.06);
    vec3 diffuse = base * diff * (1.0 - shadow * 0.78);
    vec3 specular = vec3(0.75, 0.78, 0.82) * spec * (1.0 - shadow * 0.65);
    vec3 rimLight = base * rim * 0.12;

    vec3 color = ambient + diffuse + specular + rimLight;

    float distanceToCamera = length(viewPos - FragPos);
    float fogAmount = 1.0 - exp(-pow(distanceToCamera * fogDensity, 1.35));
    fogAmount = clamp(fogAmount, 0.0, 1.0);
    color = mix(color, fogColor, fogAmount);

    FragColor = vec4(color, 1.0);
}
