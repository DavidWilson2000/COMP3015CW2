#version 460 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D sceneTex;
uniform int mode;
uniform vec2 texelSize;
uniform float time;

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

void main()
{
    vec3 color = sampleScene(TexCoord);

    if (mode == 1)
        color = edgeDetect(TexCoord);
    else if (mode == 2)
        color = gaussianBlur(TexCoord);
    else if (mode == 3)
        color = nightVision(TexCoord);

    FragColor = vec4(color, 1.0);
}
