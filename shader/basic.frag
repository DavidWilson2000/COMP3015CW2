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

float hash12(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float bias = max(0.0028 * (1.0 - dot(normal, lightDir)), 0.0010);
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    const vec2 poisson[12] = vec2[](
        vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696,  0.457),
        vec2(-0.203,  0.621), vec2( 0.962, -0.195), vec2( 0.473, -0.480),
        vec2( 0.519,  0.767), vec2( 0.185, -0.893), vec2( 0.507,  0.064),
        vec2( 0.896,  0.412), vec2(-0.322, -0.933), vec2(-0.792, -0.598)
    );

    float randAngle = hash12(FragPos.xz * 2.37) * 6.2831853;
    mat2 rotation = mat2(cos(randAngle), -sin(randAngle), sin(randAngle), cos(randAngle));

    float shadow = 0.0;
    float radius = mix(1.2, 2.2, worldDanger) * 1.1;
    for (int i = 0; i < 12; ++i)
    {
        vec2 offset = rotation * poisson[i] * texelSize * radius;
        float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
        shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
    }
    return shadow / 12.0;
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

    if (materialType == 0) { specPower = 12.0; specStrength = 0.12; }
    else if (materialType == 1) { specPower = 20.0; specStrength = 0.08; }
    else if (materialType == 2) { specPower = 10.0; specStrength = 0.05; }
    else if (materialType == 3) { specPower = 32.0; specStrength = 0.25; }
    else if (materialType == 4) { specPower = 22.0; specStrength = 0.18; }
    else if (materialType == 5) { specPower = 48.0; specStrength = 0.45; }

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
