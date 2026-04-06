#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D materialTex;
uniform sampler2D shadowMap;
uniform int materialType;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;
uniform float worldDanger;

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;

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

void main()
{
    vec3 texColor = texture(materialTex, TexCoord).rgb;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    float diff = max(dot(norm, lightDir), 0.0);
    float specPower = 16.0;
    float specStrength = 0.18;

    if (materialType == 0) { specPower = 12.0; specStrength = 0.12; } // wood
    else if (materialType == 1) { specPower = 20.0; specStrength = 0.08; } // rock
    else if (materialType == 2) { specPower = 10.0; specStrength = 0.05; } // grass
    else if (materialType == 3) { specPower = 32.0; specStrength = 0.25; } // boat paint
    else if (materialType == 4) { specPower = 22.0; specStrength = 0.18; } // buoy
    else if (materialType == 5) { specPower = 48.0; specStrength = 0.45; } // light

    float spec = pow(max(dot(norm, halfDir), 0.0), specPower) * specStrength;
    float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 2.5) * 0.12;

    float shadow = calculateShadow(FragPosLightSpace, norm, lightDir);
    vec3 ambient = texColor * mix(0.28, 0.18, worldDanger);
    vec3 lit = ambient + (1.0 - shadow) * (texColor * diff + vec3(spec)) + texColor * rim;

    float dist = length(viewPos - FragPos);
    float fogFactor = clamp((fogFar - dist) / max(fogFar - fogNear, 0.001), 0.0, 1.0);
    vec3 finalColor = mix(fogColor, lit, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
