#include "ResourceUtils.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include "SimpleOBJLoader.h"
#include "helper/stb/stb_image.h"

namespace
{
    float LocalClampf(float value, float minValue, float maxValue)
    {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }
}

// Resource loading helpers.
// These helpers make the packaged build more robust by trying the
// requested path first, then a few parent-folder fallbacks. This is
// useful when the game is launched from Visual Studio or directly
// from the x64/Debug folder.


bool rawFileExists(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

std::vector<std::string> makeResourcePathCandidates(const std::string& path)
{
    return {
        path,
        "../" + path,
        "../../" + path,
        "../../../" + path
    };
}

bool resolveResourcePath(
    const std::string& requestedPath,
    std::string& resolvedPath,
    const std::string& label,
    bool logSuccess,
    bool logFailure)
{
    const std::vector<std::string> candidates = makeResourcePathCandidates(requestedPath);

    for (const std::string& candidate : candidates)
    {
        if (rawFileExists(candidate))
        {
            resolvedPath = candidate;

            if (logSuccess)
            {
                if (candidate == requestedPath)
                {
                    std::cout << "[Resource] Loaded " << label << ": " << candidate << std::endl;
                }
                else
                {
                    std::cout << "[Resource fallback] Loaded " << label
                              << ": " << candidate
                              << " (requested: " << requestedPath << ")"
                              << std::endl;
                }
            }

            return true;
        }
    }

    if (logFailure)
    {
        std::cerr << "[Resource missing] Could not find " << label
                  << ": " << requestedPath << std::endl;

        std::cerr << "Tried:" << std::endl;
        for (const std::string& candidate : candidates)
        {
            std::cerr << "  - " << candidate << std::endl;
        }
    }

    return false;
}

std::string readTextFile(const std::string& path)
{
    std::string resolvedPath;
    if (!resolveResourcePath(path, resolvedPath, "text/shader file"))
    {
        return "";
    }

    std::ifstream file(resolvedPath);
    if (!file)
    {
        std::cerr << "Failed to open resolved file: " << resolvedPath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::cout << "[Shader/Text] Read " << resolvedPath
              << " (" << buffer.str().size() << " bytes)" << std::endl;

    return buffer.str();
}

// Compiles one GLSL shader stage and prints compiler errors to the
// console. Keeping these logs visible makes shader issues easier to
// diagnose during marking and video demonstration.
GLuint compileShader(GLenum type, const std::string& src)
{
    GLuint shader = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[2048];
        glGetShaderInfoLog(shader, 2048, nullptr, infoLog);
        std::cerr << "Shader compile error:\n" << infoLog << std::endl;
    }
    return shader;
}

// Builds a complete shader program from vertex/fragment shader files.
// The debug logs show exactly which shader pair was loaded and linked.
GLuint createShaderProgramFromFiles(const std::string& vertPath, const std::string& fragPath)
{
    std::cout << "[Shader] Creating program from: "
              << vertPath << " + " << fragPath << std::endl;

    std::string vertSrc = readTextFile(vertPath);
    std::string fragSrc = readTextFile(fragPath);

    if (vertSrc.empty() || fragSrc.empty())
    {
        std::cerr << "[Shader] Missing shader source. Program may fail: "
                  << vertPath << " + " << fragPath << std::endl;
    }

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[2048];
        glGetProgramInfoLog(program, 2048, nullptr, infoLog);
        std::cerr << "Program link error for "
                  << vertPath << " + " << fragPath
                  << ":\n" << infoLog << std::endl;
    }
    else
    {
        std::cout << "[Shader] Linked program successfully: "
                  << vertPath << " + " << fragPath << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}


void setMat4(GLuint shader, const char* name, const glm::mat4& mat)
{
    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void setVec3(GLuint shader, const char* name, const glm::vec3& vec)
{
    glUniform3fv(glGetUniformLocation(shader, name), 1, glm::value_ptr(vec));
}

void setVec4(GLuint shader, const char* name, const glm::vec4& vec)
{
    glUniform4fv(glGetUniformLocation(shader, name), 1, glm::value_ptr(vec));
}

void setFloat(GLuint shader, const char* name, float value)
{
    glUniform1f(glGetUniformLocation(shader, name), value);
}

void setInt(GLuint shader, const char* name, int value)
{
    glUniform1i(glGetUniformLocation(shader, name), value);
}

// Loads an OBJ model into an OpenGL VAO/VBO using the module-compatible
// SimpleOBJ loader. The fallback resolver allows models to be found
// from the packaged build folder as well as from source-run paths.
bool loadOBJModel(const std::string& path, ModelMesh& outModel)
{
    std::string resolvedPath;
    if (!resolveResourcePath(path, resolvedPath, "OBJ model"))
    {
        return false;
    }

    std::vector<float> vertices;
    std::string warning;
    std::string error;

    if (!SimpleOBJ::loadOBJInterleaved(resolvedPath, vertices, &warning, &error))
    {
        if (!warning.empty()) std::cout << "OBJ warning: " << warning << std::endl;
        if (!error.empty()) std::cerr << "OBJ error: " << error << std::endl;

        std::cerr << "[OBJ] Failed to load model: " << resolvedPath
                  << " (requested: " << path << ")" << std::endl;

        return false;
    }

    if (!warning.empty())
        std::cout << "OBJ warning: " << warning << std::endl;

    // Calculate model bounds from the interleaved vertex data.
    // WorldRenderer.cpp uses these bounds to auto-scale and ground imported
    // models such as tree.obj. Without this, min/max stay at 0 and trees can
    // appear missing, huge, or incorrectly placed.
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(-std::numeric_limits<float>::max());

    for (size_t i = 0; i + 7 < vertices.size(); i += 8)
    {
        glm::vec3 p(vertices[i + 0], vertices[i + 1], vertices[i + 2]);
        minBounds = glm::min(minBounds, p);
        maxBounds = glm::max(maxBounds, p);
    }

    outModel.minBounds = minBounds;
    outModel.maxBounds = maxBounds;

    glGenVertexArrays(1, &outModel.vao);
    glGenBuffers(1, &outModel.vbo);

    glBindVertexArray(outModel.vao);
    glBindBuffer(GL_ARRAY_BUFFER, outModel.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    outModel.vertexCount = vertices.size() / 8;
    outModel.loaded = true;

    std::cout << "[OBJ] Loaded model: " << resolvedPath
              << " | vertices: " << outModel.vertexCount
              << " | bounds min(" << outModel.minBounds.x << ", "
              << outModel.minBounds.y << ", " << outModel.minBounds.z << ")"
              << " max(" << outModel.maxBounds.x << ", "
              << outModel.maxBounds.y << ", " << outModel.maxBounds.z << ")"
              << std::endl;

    return true;
}

// Creates a GL texture from raw RGB data. This is used by the
// procedural fallback textures when image files are missing.
GLuint createTextureFromRGBData(const std::vector<unsigned char>& data, int width, int height)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

GLuint createSolidColorTexture(unsigned char r, unsigned char g, unsigned char b)
{
    const int w = 4;
    const int h = 4;
    std::vector<unsigned char> data(w * h * 3);

    for (int i = 0; i < w * h; ++i)
    {
        data[i * 3 + 0] = r;
        data[i * 3 + 1] = g;
        data[i * 3 + 2] = b;
    }

    return createTextureFromRGBData(data, w, h);
}

// Loads a texture from disk with fallback path resolution. If this
// returns 0, the caller can use one of the procedural texture fallbacks.
GLuint loadTextureFromFile(const std::string& path, bool flipVertically)
{
    std::string resolvedPath;
    if (!resolveResourcePath(path, resolvedPath, "texture"))
    {
        return 0;
    }

    if (flipVertically)
    {
        stbi_set_flip_vertically_on_load(true);
    }
    else
    {
        stbi_set_flip_vertically_on_load(false);
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(resolvedPath.c_str(), &width, &height, &channels, 0);

    if (!data)
    {
        std::cerr << "[Texture] Failed to load resolved texture: " << resolvedPath
                  << " (requested: " << path << ")" << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    std::cout << "[Texture] Loaded: " << resolvedPath
              << " (" << width << "x" << height
              << ", channels: " << channels << ")" << std::endl;

    return tex;
}


// Lightweight wrapper used by asset lookups such as fish PNG loading.
bool fileExists(const std::string& path)
{
    std::string resolvedPath;
    return resolveResourcePath(path, resolvedPath, "file", false, false);
}


GLuint createWoodTexture()
{
    const int w = 128, h = 128;
    std::vector<unsigned char> data(w * h * 3);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float xf = static_cast<float>(x) / static_cast<float>(w);
            float yf = static_cast<float>(y) / static_cast<float>(h);

            float warp = std::sin(yf * 24.0f + xf * 5.0f) * 0.08f + std::cos(yf * 11.0f) * 0.05f;
            float grain = std::sin((xf + warp) * 54.0f) * 0.5f + 0.5f;
            float fine = std::sin((xf + yf * 0.18f) * 140.0f) * 0.5f + 0.5f;

            float knotDx = xf - 0.32f;
            float knotDy = yf - 0.58f;
            float knotDist = std::sqrt(knotDx * knotDx * 4.0f + knotDy * knotDy);
            float knot = std::sin(knotDist * 120.0f + yf * 12.0f) * 0.5f + 0.5f;
            knot *= std::exp(-knotDist * 9.0f);

            float shade = 0.55f + grain * 0.35f + fine * 0.07f + knot * 0.25f;

            int i = (y * w + x) * 3;
            data[i + 0] = static_cast<unsigned char>(LocalClampf(58.0f + shade * 82.0f, 0.0f, 255.0f));
            data[i + 1] = static_cast<unsigned char>(LocalClampf(32.0f + shade * 52.0f, 0.0f, 255.0f));
            data[i + 2] = static_cast<unsigned char>(LocalClampf(18.0f + shade * 28.0f, 0.0f, 255.0f));
        }
    }
    return createTextureFromRGBData(data, w, h);
}

GLuint createRockTexture()
{
    const int w = 64, h = 64;
    std::vector<unsigned char> data(w * h * 3);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float noise = std::sin(x * 0.51f + y * 0.37f) * std::cos(y * 0.23f) * 0.5f + 0.5f;
            float crack = (std::sin((x - y) * 0.25f) > 0.8f) ? 0.55f : 0.0f;
            int c = static_cast<int>(85 + noise * 70 - crack * 40);
            int i = (y * w + x) * 3;
            data[i + 0] = static_cast<unsigned char>(LocalClampf(c, 0, 255));
            data[i + 1] = static_cast<unsigned char>(LocalClampf(c + 5, 0, 255));
            data[i + 2] = static_cast<unsigned char>(LocalClampf(c + 10, 0, 255));
        }
    }
    return createTextureFromRGBData(data, w, h);
}

GLuint createGrassTexture()
{
    const int w = 64, h = 64;
    std::vector<unsigned char> data(w * h * 3);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float stripe = std::sin(x * 0.55f) * 0.5f + 0.5f;
            float noise = std::cos((x + y) * 0.27f) * 0.5f + 0.5f;
            int i = (y * w + x) * 3;
            data[i + 0] = static_cast<unsigned char>(30 + stripe * 25);
            data[i + 1] = static_cast<unsigned char>(100 + noise * 90);
            data[i + 2] = static_cast<unsigned char>(35 + stripe * 20);
        }
    }
    return createTextureFromRGBData(data, w, h);
}

GLuint createBoatTexture()
{
    const int w = 64, h = 64;
    std::vector<unsigned char> data(w * h * 3);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            bool line = (x % 16 == 0) || (y % 16 == 0);
            int i = (y * w + x) * 3;
            data[i + 0] = static_cast<unsigned char>(140 + (line ? 18 : 0));
            data[i + 1] = static_cast<unsigned char>(32 + (line ? 10 : 0));
            data[i + 2] = static_cast<unsigned char>(28 + (line ? 10 : 0));
        }
    }
    return createTextureFromRGBData(data, w, h);
}

// Distinct patrol-boat texture used on the same imported boat mesh as the
// player boat. This keeps the patrol visually consistent with the world while
// making it readable as a separate AI/enemy object.
GLuint createPatrolBoatTexture()
{
    const int w = 64, h = 64;
    std::vector<unsigned char> data(w * h * 3);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const bool panelLine = (x % 16 == 0) || (y % 16 == 0);
            const bool warningStripe = ((x + y) / 10) % 2 == 0;
            const int i = (y * w + x) * 3;

            // Dark blue/teal base with a subtle yellow safety-stripe pattern.
            data[i + 0] = static_cast<unsigned char>(24  + (panelLine ? 20 : 0) + (warningStripe ? 18 : 0));
            data[i + 1] = static_cast<unsigned char>(82  + (panelLine ? 24 : 0) + (warningStripe ? 30 : 0));
            data[i + 2] = static_cast<unsigned char>(116 + (panelLine ? 25 : 0));
        }
    }

    return createTextureFromRGBData(data, w, h);
}


// Rusty/metallic-looking fallback texture for the harbour mission scrap model.
// This is used if no scrap texture image is provided in media/models/.
GLuint createScrapTexture()
{
    const int w = 64, h = 64;
    std::vector<unsigned char> data(w * h * 3);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const float noise = std::sin(x * 0.73f + y * 0.41f) * 0.5f + 0.5f;
            const bool seam = (x % 17 == 0) || (y % 23 == 0);
            const bool rustPatch = std::sin(x * 0.31f + y * 0.67f) > 0.55f;
            const int i = (y * w + x) * 3;

            // Cool grey metal with orange-brown rust patches.
            data[i + 0] = static_cast<unsigned char>(LocalClampf((rustPatch ? 120.0f : 82.0f) + noise * 35.0f + (seam ? 25.0f : 0.0f), 0.0f, 255.0f));
            data[i + 1] = static_cast<unsigned char>(LocalClampf((rustPatch ? 68.0f  : 82.0f) + noise * 24.0f + (seam ? 16.0f : 0.0f), 0.0f, 255.0f));
            data[i + 2] = static_cast<unsigned char>(LocalClampf((rustPatch ? 30.0f  : 86.0f) + noise * 18.0f + (seam ? 12.0f : 0.0f), 0.0f, 255.0f));
        }
    }

    return createTextureFromRGBData(data, w, h);
}

GLuint createBuoyTexture()
{
    const int w = 32, h = 32;
    std::vector<unsigned char> data(w * h * 3);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            bool stripe = ((x / 8) % 2) == 0;
            int i = (y * w + x) * 3;
            data[i + 0] = stripe ? 210 : 230;
            data[i + 1] = stripe ? 80 : 190;
            data[i + 2] = stripe ? 50 : 60;
        }
    }
    return createTextureFromRGBData(data, w, h);
}

GLuint createFallbackCubemap()
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    const unsigned char faces[6][3] = {
        {118, 164, 214}, // right
        {110, 156, 206}, // left
        {76,  112, 170}, // top
        {150, 170, 190}, // bottom
        {124, 170, 220}, // front
        {110, 150, 200}  // back
    };

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, faces[i]);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return tex;
}


