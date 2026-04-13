#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform int mode;
uniform vec2 texelSize;
uniform float time;
uniform vec2 sunScreenPos;
uniform vec3 sunColor;
uniform float sunVisibility;
uniform float dayFactor;

float luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 applyEdge(vec2 uv)
{
    float kernelX[9] = float[](
        -1, 0, 1,
        -2, 0, 2,
        -1, 0, 1
    );
    float kernelY[9] = float[](
        -1, -2, -1,
         0,  0,  0,
         1,  2,  1
    );

    vec2 offsets[9] = vec2[](
        vec2(-texelSize.x,  texelSize.y), vec2(0.0,  texelSize.y), vec2(texelSize.x,  texelSize.y),
        vec2(-texelSize.x,  0.0),         vec2(0.0,  0.0),         vec2(texelSize.x,  0.0),
        vec2(-texelSize.x, -texelSize.y), vec2(0.0, -texelSize.y), vec2(texelSize.x, -texelSize.y)
    );

    float gx = 0.0;
    float gy = 0.0;
    for (int i = 0; i < 9; ++i)
    {
        float l = luminance(texture(sceneTex, uv + offsets[i]).rgb);
        gx += l * kernelX[i];
        gy += l * kernelY[i];
    }
    float edge = clamp(length(vec2(gx, gy)), 0.0, 1.0);
    return vec3(edge);
}

vec3 applyBlur(vec2 uv)
{
    vec3 col = vec3(0.0);
    float weightSum = 0.0;
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float w = 1.0 / (1.0 + float(x * x + y * y));
            col += texture(sceneTex, uv + vec2(x, y) * texelSize).rgb * w;
            weightSum += w;
        }
    }
    return col / max(weightSum, 0.0001);
}

vec3 applyNightVision(vec2 uv)
{
    vec3 color = texture(sceneTex, uv).rgb;
    float gray = luminance(color);
    float scan = 0.92 + 0.08 * sin(uv.y * 900.0 + time * 8.0);
    float noise = fract(sin(dot(uv + time, vec2(12.9898, 78.233))) * 43758.5453);
    vec3 nv = vec3(0.08, 0.95, 0.18) * gray * scan;
    nv += vec3(noise * 0.05);
    return nv;
}

vec3 applyGodRays(vec2 uv)
{
    vec3 scene = texture(sceneTex, uv).rgb;

    vec2 sunUV = sunScreenPos;
    vec2 delta = sunUV - uv;
    float dist = length(delta);

    if (dayFactor <= 0.001 || dist > 1.8)
        return scene;

    vec2 stepUV = delta / 64.0;
    vec2 sampleUV = uv;

    float illuminationDecay = 1.0;
    float shafts = 0.0;
    const float exposure = 0.65;
    const float decay = 0.965;
    const float density = 0.90;
    const float weight = 0.070;

    for (int i = 0; i < 64; ++i)
    {
        sampleUV += stepUV * density;
        vec3 sampleCol = texture(sceneTex, clamp(sampleUV, vec2(0.0), vec2(1.0))).rgb;
        float sampleLum = luminance(sampleCol);
        float bright = smoothstep(0.35, 1.0, sampleLum);
        shafts += bright * illuminationDecay * weight;
        illuminationDecay *= decay;
    }

    float disk = smoothstep(0.20, 0.0, dist) * 0.9;
    float mask = smoothstep(1.3, 0.0, dist);
    float visibility = max(sunVisibility, 0.35 * dayFactor);
    vec3 rays = sunColor * (shafts * exposure + disk) * visibility * mask;
    return scene + rays;
}

void main()
{
    vec3 color = texture(sceneTex, TexCoords).rgb;

    if (mode == 1)
        color = applyEdge(TexCoords);
    else if (mode == 2)
        color = applyBlur(TexCoords);
    else if (mode == 3)
        color = applyNightVision(TexCoords);
    else if (mode == 4)
        color = applyGodRays(TexCoords);

    FragColor = vec4(color, 1.0);
}
