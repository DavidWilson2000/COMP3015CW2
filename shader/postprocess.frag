#version 460 core
in vec2 TexCoord;
out vec4 FragColor;

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
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec3 sampleScene(vec2 uv)
{
    return texture(sceneTex, uv).rgb;
}

vec3 gaussianBlur(vec2 uv)
{
    vec2 t = texelSize;
    vec3 sum = vec3(0.0);
    sum += sampleScene(uv + t * vec2(-2.0,  0.0)) * 0.06136;
    sum += sampleScene(uv + t * vec2(-1.0, -1.0)) * 0.07650;
    sum += sampleScene(uv + t * vec2(-1.0,  0.0)) * 0.12245;
    sum += sampleScene(uv + t * vec2(-1.0,  1.0)) * 0.07650;
    sum += sampleScene(uv + t * vec2( 0.0, -1.0)) * 0.12245;
    sum += sampleScene(uv)                         * 0.16330;
    sum += sampleScene(uv + t * vec2( 0.0,  1.0)) * 0.12245;
    sum += sampleScene(uv + t * vec2( 1.0, -1.0)) * 0.07650;
    sum += sampleScene(uv + t * vec2( 1.0,  0.0)) * 0.12245;
    sum += sampleScene(uv + t * vec2( 1.0,  1.0)) * 0.07650;
    return sum;
}

vec3 edgeDetect(vec2 uv)
{
    float tl = luminance(sampleScene(uv + texelSize * vec2(-1.0,  1.0)));
    float tc = luminance(sampleScene(uv + texelSize * vec2( 0.0,  1.0)));
    float tr = luminance(sampleScene(uv + texelSize * vec2( 1.0,  1.0)));
    float ml = luminance(sampleScene(uv + texelSize * vec2(-1.0,  0.0)));
    float mc = luminance(sampleScene(uv));
    float mr = luminance(sampleScene(uv + texelSize * vec2( 1.0,  0.0)));
    float bl = luminance(sampleScene(uv + texelSize * vec2(-1.0, -1.0)));
    float bc = luminance(sampleScene(uv + texelSize * vec2( 0.0, -1.0)));
    float br = luminance(sampleScene(uv + texelSize * vec2( 1.0, -1.0)));

    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy = -bl - 2.0 * bc - br + tl + 2.0 * tc + tr;
    float edge = clamp(length(vec2(gx, gy)) * 1.25, 0.0, 1.0);

    vec3 base = sampleScene(uv);
    vec3 ink = mix(vec3(0.08, 0.12, 0.18), vec3(0.95), edge);
    return mix(base * 0.50 + mc * 0.10, ink, edge * 0.88);
}

vec3 nightVision(vec2 uv)
{
    vec3 base = sampleScene(uv);
    float lum = luminance(base);
    float noise = hash(uv * vec2(1280.0, 720.0) + time * 0.5) - 0.5;
    float scan = 0.90 + 0.10 * sin(uv.y * 920.0 + time * 13.0);
    float vignette = smoothstep(0.86, 0.22, length(uv - 0.5));

    float r = texture(sceneTex, uv + vec2(texelSize.x * 0.7, 0.0)).r;
    float g = texture(sceneTex, uv).g;
    float b = texture(sceneTex, uv - vec2(texelSize.x * 0.7, 0.0)).b;
    vec3 shifted = vec3(r, g, b);

    float phosphor = smoothstep(0.10, 0.95, lum + 0.18);
    vec3 green = vec3(0.08, 1.0, 0.24) * (luminance(shifted) * 1.40 + noise * 0.10 + phosphor * 0.08);
    green *= scan * vignette;
    green += vec3(0.01, 0.08, 0.02) * pow(phosphor, 3.0);
    return green;
}

vec3 godRays(vec2 uv)
{
    vec3 base = sampleScene(uv);
    if (sunVisibility <= 0.001 || dayFactor <= 0.001)
        return base;

    vec2 toSun = sunScreenPos - uv;
    float sunDist = length(toSun);
    vec2 dir = toSun / max(sunDist, 0.0001);

    const int NUM_SAMPLES = 36;
    float density = mix(0.72, 0.94, dayFactor);
    float decay = 0.955;
    float weight = 0.23;
    float exposure = mix(0.11, 0.18, dayFactor) * sunVisibility;

    vec2 stepUV = dir * sunDist * density / float(NUM_SAMPLES);
    vec2 sampleUV = uv;
    float illuminationDecay = 1.0;
    vec3 accum = vec3(0.0);

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        sampleUV += stepUV;
        vec3 sampleColor = sampleScene(clamp(sampleUV, vec2(0.001), vec2(0.999)));
        float lum = luminance(sampleColor);
        float brightMask = smoothstep(0.62, 1.05, lum);
        float warmMask = smoothstep(0.55, 0.95, dot(normalize(sampleColor + vec3(0.0001)), normalize(sunColor + vec3(0.0001))));
        float source = mix(brightMask, brightMask * warmMask, 0.45);
        accum += sampleColor * source * illuminationDecay * weight;
        illuminationDecay *= decay;
    }

    float radialCenter = smoothstep(1.15, 0.08, sunDist);
    float radialWide = smoothstep(1.45, 0.18, sunDist);
    vec3 shaftTint = mix(vec3(1.0), sunColor, 0.72);
    vec3 shafts = accum * shaftTint;
    shafts += shaftTint * radialCenter * 0.18 * sunVisibility;

    vec3 color = base + shafts * exposure * radialWide;
    color += shaftTint * pow(max(1.0 - sunDist * 1.35, 0.0), 8.0) * 0.12 * sunVisibility;
    return color;
}

void main()
{
    vec3 color = sampleScene(TexCoord);

    if (mode == 1)
        color = edgeDetect(TexCoord);
    else if (mode == 2)
        color = gaussianBlur(TexCoord);
    else if (mode == 3)
        color = nightVision(TexCoord);
    else if (mode == 4)
        color = godRays(TexCoord);

    FragColor = vec4(color, 1.0);
}
