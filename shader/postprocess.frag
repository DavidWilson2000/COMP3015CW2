#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform sampler2D depthTex;
uniform int mode;
uniform vec2 texelSize;
uniform float time;
uniform vec2 sunScreenPos;
uniform vec3 sunColor;
uniform float sunVisibility;
uniform float dayFactor;
uniform float dofFocusDistance;
uniform float dofFocusRange;
uniform float dofMaxBlurPixels;

const float NEAR_PLANE = 0.1;
const float FAR_PLANE = 500.0;

float saturate(float v)
{
    return clamp(v, 0.0, 1.0);
}

float LineariseDepth(float rawDepth)
{
    float z = rawDepth * 2.0 - 1.0;
    return (2.0 * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

vec3 ApplyBlur(vec2 uv)
{
    vec3 sum = vec3(0.0);
    sum += texture(sceneTex, uv + texelSize * vec2(-1.0, -1.0)).rgb;
    sum += texture(sceneTex, uv + texelSize * vec2( 0.0, -1.0)).rgb;
    sum += texture(sceneTex, uv + texelSize * vec2( 1.0, -1.0)).rgb;
    sum += texture(sceneTex, uv + texelSize * vec2(-1.0,  0.0)).rgb;
    sum += texture(sceneTex, uv).rgb;
    sum += texture(sceneTex, uv + texelSize * vec2( 1.0,  0.0)).rgb;
    sum += texture(sceneTex, uv + texelSize * vec2(-1.0,  1.0)).rgb;
    sum += texture(sceneTex, uv + texelSize * vec2( 0.0,  1.0)).rgb;
    sum += texture(sceneTex, uv + texelSize * vec2( 1.0,  1.0)).rgb;
    return sum / 9.0;
}

vec3 ApplyEdge(vec2 uv)
{
    float lum00 = dot(texture(sceneTex, uv + texelSize * vec2(-1.0, -1.0)).rgb, vec3(0.299, 0.587, 0.114));
    float lum01 = dot(texture(sceneTex, uv + texelSize * vec2( 0.0, -1.0)).rgb, vec3(0.299, 0.587, 0.114));
    float lum02 = dot(texture(sceneTex, uv + texelSize * vec2( 1.0, -1.0)).rgb, vec3(0.299, 0.587, 0.114));

    float lum10 = dot(texture(sceneTex, uv + texelSize * vec2(-1.0,  0.0)).rgb, vec3(0.299, 0.587, 0.114));
    float lum12 = dot(texture(sceneTex, uv + texelSize * vec2( 1.0,  0.0)).rgb, vec3(0.299, 0.587, 0.114));

    float lum20 = dot(texture(sceneTex, uv + texelSize * vec2(-1.0,  1.0)).rgb, vec3(0.299, 0.587, 0.114));
    float lum21 = dot(texture(sceneTex, uv + texelSize * vec2( 0.0,  1.0)).rgb, vec3(0.299, 0.587, 0.114));
    float lum22 = dot(texture(sceneTex, uv + texelSize * vec2( 1.0,  1.0)).rgb, vec3(0.299, 0.587, 0.114));

    float gx = -lum00 - 2.0 * lum10 - lum20 + lum02 + 2.0 * lum12 + lum22;
    float gy = -lum00 - 2.0 * lum01 - lum02 + lum20 + 2.0 * lum21 + lum22;

    float colourEdge = length(vec2(gx, gy));

    float d0 = texture(depthTex, uv + texelSize * vec2(-1.0,  0.0)).r;
    float d1 = texture(depthTex, uv + texelSize * vec2( 1.0,  0.0)).r;
    float d2 = texture(depthTex, uv + texelSize * vec2( 0.0, -1.0)).r;
    float d3 = texture(depthTex, uv + texelSize * vec2( 0.0,  1.0)).r;

    float depthEdge = abs(d1 - d0) + abs(d3 - d2);

    float edge = max(
        smoothstep(0.05, 0.18, colourEdge),
        smoothstep(0.0003, 0.004, depthEdge)
    );

    return vec3(edge);
}

vec3 ApplyNightVision(vec2 uv)
{
    vec3 c = texture(sceneTex, uv).rgb;
    float luminance = dot(c, vec3(0.299, 0.587, 0.114));
    float scanline = 0.88 + 0.12 * sin((uv.y + time * 0.8) * 900.0);
    float vignette = smoothstep(0.85, 0.25, length(uv - 0.5));
    return vec3(0.12, 1.0, 0.18) * luminance * scanline * vignette * 1.8;
}

vec3 ApplyGodRays(vec2 uv)
{
    vec3 base = texture(sceneTex, uv).rgb;

    // Keep a small demo visibility so F11 is visibly different even when the
    // sun is near the edge of the screen. It still becomes stronger when the
    // computed sun visibility is high.
    float visible = max(sunVisibility, 0.28 * dayFactor);

    vec2 rayOrigin = clamp(sunScreenPos, vec2(0.05), vec2(0.95));
    vec2 delta = (uv - rayOrigin) * 0.030;
    vec2 sampleUv = uv;
    vec3 rays = vec3(0.0);
    float weight = 1.0;

    for (int i = 0; i < 64; ++i)
    {
        sampleUv -= delta;
        if (sampleUv.x < 0.0 || sampleUv.x > 1.0 || sampleUv.y < 0.0 || sampleUv.y > 1.0)
            break;

        float depth = texture(depthTex, sampleUv).r;
        float skyAmount = smoothstep(0.970, 1.0, depth);
        vec3 sampleColour = texture(sceneTex, sampleUv).rgb;
        rays += sampleColour * skyAmount * weight;
        weight *= 0.955;
    }

    rays *= 0.030 * visible;
    float vignette = smoothstep(0.95, 0.15, length(uv - rayOrigin));
    return base + rays * sunColor + sunColor * vignette * visible * 0.035;
}

float SampleDepthAO(vec2 uv)
{
    float rawDepth = texture(depthTex, uv).r;
    if (rawDepth >= 0.9999)
        return 1.0;

    float centreDepth = LineariseDepth(rawDepth);

    vec2 dirs[16] = vec2[](
        vec2( 1.0,  0.0), vec2(-1.0,  0.0), vec2( 0.0,  1.0), vec2( 0.0, -1.0),
        vec2( 0.7,  0.7), vec2(-0.7,  0.7), vec2( 0.7, -0.7), vec2(-0.7, -0.7),
        vec2( 1.0,  0.5), vec2(-1.0,  0.5), vec2( 1.0, -0.5), vec2(-1.0, -0.5),
        vec2( 0.5,  1.0), vec2(-0.5,  1.0), vec2( 0.5, -1.0), vec2(-0.5, -1.0)
    );

    float pixelRadius = 10.0;
    float radius = 1.25;
    float bias = 0.05;
    float occlusion = 0.0;

    for (int i = 0; i < 16; ++i)
    {
        float ring = 0.55 + float(i % 4) * 0.25;
        vec2 sampleUv = uv + dirs[i] * texelSize * pixelRadius * ring;
        float sampleRaw = texture(depthTex, sampleUv).r;
        if (sampleRaw >= 0.9999)
            continue;

        float sampleDepth = LineariseDepth(sampleRaw);
        float depthDifference = centreDepth - sampleDepth;
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(depthDifference), 0.001));
        if (depthDifference > bias)
            occlusion += rangeCheck;
    }

    occlusion /= 16.0;
    return clamp(1.0 - occlusion * 1.35, 0.35, 1.0);
}

vec3 ApplySSAO(vec2 uv)
{
    vec3 colour = texture(sceneTex, uv).rgb;
    float ao = SampleDepthAO(uv);
    ao += SampleDepthAO(uv + texelSize * vec2( 1.5, 0.0));
    ao += SampleDepthAO(uv + texelSize * vec2(-1.5, 0.0));
    ao += SampleDepthAO(uv + texelSize * vec2( 0.0, 1.5));
    ao += SampleDepthAO(uv + texelSize * vec2( 0.0,-1.5));
    ao /= 5.0;
    return colour * ao;
}

vec3 DepthOfFieldBlur(vec2 uv, float radius)
{
    vec3 sum = texture(sceneTex, uv).rgb * 4.0;
    float weight = 4.0;

    vec2 offsets[16] = vec2[](
        vec2( 1.0,  0.0), vec2(-1.0,  0.0), vec2( 0.0,  1.0), vec2( 0.0, -1.0),
        vec2( 0.7,  0.7), vec2(-0.7,  0.7), vec2( 0.7, -0.7), vec2(-0.7, -0.7),
        vec2( 1.7,  0.4), vec2(-1.7,  0.4), vec2( 1.7, -0.4), vec2(-1.7, -0.4),
        vec2( 0.4,  1.7), vec2(-0.4,  1.7), vec2( 0.4, -1.7), vec2(-0.4, -1.7)
    );

    for (int i = 0; i < 16; ++i)
    {
        vec2 sampleUv = uv + offsets[i] * texelSize * radius;
        sum += texture(sceneTex, sampleUv).rgb;
        weight += 1.0;
    }

    return sum / weight;
}

vec3 ApplyDepthOfField(vec2 uv)
{
    vec3 sharp = texture(sceneTex, uv).rgb;
    float rawDepth = texture(depthTex, uv).r;
    float depth = LineariseDepth(rawDepth);

    float focusDistance = max(dofFocusDistance, 0.1);
    float focusRange = max(dofFocusRange, 0.1);
    float coc = saturate(abs(depth - focusDistance) / focusRange);
    coc = smoothstep(0.05, 1.0, coc);
    if (rawDepth >= 0.9999)
        coc = max(coc, 0.72);

    float radius = dofMaxBlurPixels * coc;
    vec3 blurred = DepthOfFieldBlur(uv, radius);
    return mix(sharp, blurred, coc * 0.9);
}

void main()
{
    vec2 uv = TexCoords;
    vec3 colour = texture(sceneTex, uv).rgb;

    if (mode == 1)
        colour = ApplyEdge(uv);
    else if (mode == 2)
        colour = ApplyBlur(uv);
    else if (mode == 3)
        colour = ApplyNightVision(uv);
    else if (mode == 4)
        colour = ApplyGodRays(uv);
    else if (mode == 5)
        colour = ApplySSAO(uv);
    else if (mode == 6)
        colour = ApplyDepthOfField(uv);

    FragColor = vec4(colour, 1.0);
}
