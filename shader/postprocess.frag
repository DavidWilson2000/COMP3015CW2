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

vec3 gaussianBlur(vec2 uv)
{
    vec2 t = texelSize;
    vec3 sum = texture(sceneTex, uv).rgb * 0.227027;
    sum += texture(sceneTex, uv + vec2(1.0, 0.0) * t).rgb * 0.1945946;
    sum += texture(sceneTex, uv - vec2(1.0, 0.0) * t).rgb * 0.1945946;
    sum += texture(sceneTex, uv + vec2(2.0, 0.0) * t).rgb * 0.1216216;
    sum += texture(sceneTex, uv - vec2(2.0, 0.0) * t).rgb * 0.1216216;
    sum += texture(sceneTex, uv + vec2(0.0, 1.0) * t).rgb * 0.0756757;
    sum += texture(sceneTex, uv - vec2(0.0, 1.0) * t).rgb * 0.0756757;
    return sum;
}

vec3 edgeDetect(vec2 uv)
{
    float tl = luminance(texture(sceneTex, uv + texelSize * vec2(-1.0,  1.0)).rgb);
    float tc = luminance(texture(sceneTex, uv + texelSize * vec2( 0.0,  1.0)).rgb);
    float tr = luminance(texture(sceneTex, uv + texelSize * vec2( 1.0,  1.0)).rgb);
    float ml = luminance(texture(sceneTex, uv + texelSize * vec2(-1.0,  0.0)).rgb);
    float mr = luminance(texture(sceneTex, uv + texelSize * vec2( 1.0,  0.0)).rgb);
    float bl = luminance(texture(sceneTex, uv + texelSize * vec2(-1.0, -1.0)).rgb);
    float bc = luminance(texture(sceneTex, uv + texelSize * vec2( 0.0, -1.0)).rgb);
    float br = luminance(texture(sceneTex, uv + texelSize * vec2( 1.0, -1.0)).rgb);

    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy = -bl - 2.0 * bc - br + tl + 2.0 * tc + tr;
    float edge = clamp(length(vec2(gx, gy)) * 1.4, 0.0, 1.0);

    vec3 base = texture(sceneTex, uv).rgb;
    return mix(base * 0.55, vec3(edge), 0.85);
}

vec3 nightVision(vec2 uv)
{
    vec3 base = texture(sceneTex, uv).rgb;
    float lum = luminance(base);
    float noise = hash(uv * vec2(1280.0, 720.0) + time * 0.5);
    float scan = 0.92 + 0.08 * sin(uv.y * 900.0 + time * 14.0);
    float vignette = smoothstep(1.15, 0.25, length(uv - 0.5));

    vec3 green = vec3(0.08, 1.0, 0.22) * (lum * 1.35 + noise * 0.08);
    green *= scan * vignette;
    return green;
}

void main()
{
    vec3 color = texture(sceneTex, TexCoord).rgb;

    if (mode == 1)
        color = edgeDetect(TexCoord);
    else if (mode == 2)
        color = gaussianBlur(TexCoord);
    else if (mode == 3)
        color = nightVision(TexCoord);

    FragColor = vec4(color, 1.0);
}
