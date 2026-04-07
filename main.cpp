#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION

#include "helper/stb/stb_image.h"
#include "SimpleOBJLoader.h"
#include "PostProcess.h"
#include "UIOverlay.h"
#include "FishingMinigame.h"
#include "SoundManager.h"
#include "IslandQuest.h"
#include "LostIslandSetpiece.h"


const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
const int MAX_PARTICLES = 300;

int gWindowWidth = SCR_WIDTH;
int gWindowHeight = SCR_HEIGHT;

const std::string BASIC_VERT_PATH = "shader/basic.vert";
const std::string BASIC_FRAG_PATH = "shader/basic.frag";
const std::string WATER_VERT_PATH = "shader/water.vert";
const std::string WATER_FRAG_PATH = "shader/water.frag";
const std::string DEPTH_VERT_PATH = "shader/depth.vert";
const std::string DEPTH_FRAG_PATH = "shader/depth.frag";
const std::string PARTICLE_VERT_PATH = "shader/particle.vert";
const std::string PARTICLE_FRAG_PATH = "shader/particle.frag";
const std::string SKYBOX_VERT_PATH = "shader/skybox.vert";
const std::string SKYBOX_FRAG_PATH = "shader/skybox.frag";
const std::string POSTPROCESS_VERT_PATH = "shader/postprocess.vert";
const std::string POSTPROCESS_FRAG_PATH = "shader/postprocess.frag";

bool keys[1024] = { false };
bool fishPressed = false;
bool sellPressed = false;
bool upgrade1Pressed = false;
bool upgrade2Pressed = false;
bool upgrade3Pressed = false;
bool minigameTogglePressed = false;
bool minigameHookPressed = false;

struct ModelMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    size_t vertexCount = 0;
    bool loaded = false;
};

struct FishData
{
    std::string name;
    int rarity; // 0 common, 1 uncommon, 2 rare, 3 legendary
    int value;
};

struct FishZone
{
    glm::vec3 center;
    float radius;
    std::vector<FishData> fishPool;
    glm::vec3 tint;
    glm::vec3 fogColor;
    float fogNear;
    float fogFar;
    float danger;
    std::string zoneName;

    bool contains(const glm::vec3& pos) const
    {
        return glm::length(glm::vec2(pos.x - center.x, pos.z - center.z)) <= radius;
    }
};

struct CargoItem
{
    FishData fish;
};

struct Boat
{
    glm::vec3 position = glm::vec3(0.0f, 0.18f, 0.0f);
    float rotationY = 0.0f;
    float velocity = 0.0f;
    float maxForwardSpeed = 7.0f;
    float maxBackwardSpeed = 3.5f;
    float acceleration = 4.5f;
    float deceleration = 3.0f;
    float turnSpeed = 85.0f;

    void update(float dt)
    {
        if (keys[GLFW_KEY_W])
        {
            velocity += acceleration * dt;
            if (velocity > maxForwardSpeed) velocity = maxForwardSpeed;
        }
        else if (keys[GLFW_KEY_S])
        {
            velocity -= acceleration * dt;
            if (velocity < -maxBackwardSpeed) velocity = -maxBackwardSpeed;
        }
        else
        {
            if (velocity > 0.0f)
            {
                velocity -= deceleration * dt;
                if (velocity < 0.0f) velocity = 0.0f;
            }
            else if (velocity < 0.0f)
            {
                velocity += deceleration * dt;
                if (velocity > 0.0f) velocity = 0.0f;
            }
        }

        float turnAmount = 0.0f;
        if (keys[GLFW_KEY_A]) turnAmount += 1.0f;
        if (keys[GLFW_KEY_D]) turnAmount -= 1.0f;

        float speedFactor = glm::clamp(std::abs(velocity) / std::max(maxForwardSpeed, 0.01f), 0.2f, 1.0f);
        rotationY += turnAmount * turnSpeed * speedFactor * dt;

        float radians = glm::radians(rotationY);
        glm::vec3 forward(std::sin(radians), 0.0f, std::cos(radians));
        position += forward * velocity * dt;

        float limit = 56.0f;
        position.x = glm::clamp(position.x, -limit, limit);
        position.z = glm::clamp(position.z, -limit, limit);
    }
};

float getWaterHeight(float x, float z, float time)
{
    float y = 0.0f;
    y += std::sin(x * 0.30f + time * 1.35f) * 0.16f;
    y += std::cos(z * 0.22f + time * 1.10f) * 0.12f;
    y += std::sin((x + z) * 0.12f + time * 0.80f) * 0.10f;
    return y;
}
float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

struct Particle
{
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec4 color{1.0f};
    float life = 0.0f;
    float size = 6.0f;
};

struct ParticleVertex
{
    glm::vec3 position;
    glm::vec4 color;
    float size;
};

enum MaterialType
{
    MAT_WOOD = 0,
    MAT_ROCK = 1,
    MAT_GRASS = 2,
    MAT_BOAT = 3,
    MAT_BUOY = 4,
    MAT_LIGHT = 5
};

Boat boat;
std::vector<FishZone> zones;
std::vector<CargoItem> cargo;
std::vector<Particle> particles(MAX_PARTICLES);

int totalMoney = 0;
int rodLevel = 1;
int engineLevel = 1;
int cargoLevel = 1;
int cargoCapacity = 5;
float fishCooldown = 0.0f;
float catchFlashTimer = 0.0f;
float catchMessageTimer = 0.0f;
std::string lastCatchText = "No fish caught yet";

struct EnvironmentBlendState
{
    glm::vec3 waterTint = glm::vec3(0.0f, 0.10f, 0.16f);
    glm::vec3 fogColor = glm::vec3(0.70f, 0.82f, 0.94f);
    float fogNear = 18.0f;
    float fogFar = 72.0f;
    float danger = 0.1f;
};

EnvironmentBlendState gEnvironmentBlend;
FishingMinigame gFishingMinigame;
int gFishingMinigameZoneIndex = -1;
SoundManager gSound;
IslandQuest gIslandQuest;


GLuint cubeVAO = 0, cubeVBO = 0;
GLuint planeVAO = 0, planeVBO = 0;
GLuint skyboxVAO = 0, skyboxVBO = 0;
GLuint particleVAO = 0, particleVBO = 0;

GLuint basicShader = 0;
GLuint waterShader = 0;
GLuint depthShader = 0;
GLuint particleShader = 0;
GLuint skyboxShader = 0;

PostProcessor gPostProcessor;
PostProcessMode gPostMode = PostProcessMode::None;

GLuint shadowFBO = 0;
GLuint shadowMap = 0;
GLuint skyboxCubemap = 0;
GLuint woodTex = 0;
GLuint rockTex = 0;
GLuint grassTex = 0;
GLuint boatTex = 0;
GLuint buoyTex = 0;

ModelMesh boatModel;
ModelMesh swordModel;

float boatVisualY = 0.18f;
float boatVisualPitch = 0.0f;
float boatVisualRoll = 0.0f;

float cubeVertices[] = {
    -0.5f,-0.5f,-0.5f,  0,0,-1,  0,0,
     0.5f,-0.5f,-0.5f,  0,0,-1,  1,0,
     0.5f, 0.5f,-0.5f,  0,0,-1,  1,1,
     0.5f, 0.5f,-0.5f,  0,0,-1,  1,1,
    -0.5f, 0.5f,-0.5f,  0,0,-1,  0,1,
    -0.5f,-0.5f,-0.5f,  0,0,-1,  0,0,

    -0.5f,-0.5f, 0.5f,  0,0,1,  0,0,
     0.5f,-0.5f, 0.5f,  0,0,1,  1,0,
     0.5f, 0.5f, 0.5f,  0,0,1,  1,1,
     0.5f, 0.5f, 0.5f,  0,0,1,  1,1,
    -0.5f, 0.5f, 0.5f,  0,0,1,  0,1,
    -0.5f,-0.5f, 0.5f,  0,0,1,  0,0,

    -0.5f, 0.5f, 0.5f, -1,0,0,  1,0,
    -0.5f, 0.5f,-0.5f, -1,0,0,  1,1,
    -0.5f,-0.5f,-0.5f, -1,0,0,  0,1,
    -0.5f,-0.5f,-0.5f, -1,0,0,  0,1,
    -0.5f,-0.5f, 0.5f, -1,0,0,  0,0,
    -0.5f, 0.5f, 0.5f, -1,0,0,  1,0,

     0.5f, 0.5f, 0.5f,  1,0,0,  1,0,
     0.5f, 0.5f,-0.5f,  1,0,0,  1,1,
     0.5f,-0.5f,-0.5f,  1,0,0,  0,1,
     0.5f,-0.5f,-0.5f,  1,0,0,  0,1,
     0.5f,-0.5f, 0.5f,  1,0,0,  0,0,
     0.5f, 0.5f, 0.5f,  1,0,0,  1,0,

    -0.5f,-0.5f,-0.5f,  0,-1,0, 0,1,
     0.5f,-0.5f,-0.5f,  0,-1,0, 1,1,
     0.5f,-0.5f, 0.5f,  0,-1,0, 1,0,
     0.5f,-0.5f, 0.5f,  0,-1,0, 1,0,
    -0.5f,-0.5f, 0.5f,  0,-1,0, 0,0,
    -0.5f,-0.5f,-0.5f,  0,-1,0, 0,1,

    -0.5f, 0.5f,-0.5f,  0,1,0,  0,1,
     0.5f, 0.5f,-0.5f,  0,1,0,  1,1,
     0.5f, 0.5f, 0.5f,  0,1,0,  1,0,
     0.5f, 0.5f, 0.5f,  0,1,0,  1,0,
    -0.5f, 0.5f, 0.5f,  0,1,0,  0,0,
    -0.5f, 0.5f,-0.5f,  0,1,0,  0,1
};

float planeVertices[] = {
    -0.5f, 0.0f, -0.5f, 0,1,0, 0,0,
     0.5f, 0.0f, -0.5f, 0,1,0, 4,0,
     0.5f, 0.0f,  0.5f, 0,1,0, 4,4,
     0.5f, 0.0f,  0.5f, 0,1,0, 4,4,
    -0.5f, 0.0f,  0.5f, 0,1,0, 0,4,
    -0.5f, 0.0f, -0.5f, 0,1,0, 0,0
};

float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
};

std::string readTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

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

GLuint createShaderProgramFromFiles(const std::string& vertPath, const std::string& fragPath)
{
    std::string vertSrc = readTextFile(vertPath);
    std::string fragSrc = readTextFile(fragPath);
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
        std::cerr << "Program link error:\n" << infoLog << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    gWindowWidth = width;
    gWindowHeight = height;
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_F5) gPostMode = PostProcessMode::Edge;
        if (key == GLFW_KEY_F6) gPostMode = PostProcessMode::Blur;
        if (key == GLFW_KEY_F7) gPostMode = PostProcessMode::NightVision;
        if (key == GLFW_KEY_F8) gPostMode = PostProcessMode::None;
    }

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS) keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
}

void setupCube()
{
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void setupPlane()
{
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void setupSkybox()
{
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void setupParticles()
{
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(ParticleVertex), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, color));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, size));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
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

bool loadOBJModel(const std::string& path, ModelMesh& outModel)
{
    std::vector<float> vertices;
    std::string warning;
    std::string error;

    if (!SimpleOBJ::loadOBJInterleaved(path, vertices, &warning, &error))
    {
        if (!warning.empty()) std::cout << "OBJ warning: " << warning << std::endl;
        if (!error.empty()) std::cerr << "OBJ error: " << error << std::endl;
        std::cerr << "Failed to load OBJ: " << path << std::endl;
        return false;
    }

    if (!warning.empty())
        std::cout << "OBJ warning: " << warning << std::endl;

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

    std::cout << "Loaded OBJ model: " << path
              << " | vertices: " << outModel.vertexCount << std::endl;

    return true;
}

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

GLuint loadTextureFromFile(const std::string& path, bool flipVertically = true)
{
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
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data)
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
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

    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", channels: " << channels << ")" << std::endl;

    return tex;
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
            data[i + 0] = static_cast<unsigned char>(clampf(58.0f + shade * 82.0f, 0.0f, 255.0f));
            data[i + 1] = static_cast<unsigned char>(clampf(32.0f + shade * 52.0f, 0.0f, 255.0f));
            data[i + 2] = static_cast<unsigned char>(clampf(18.0f + shade * 28.0f, 0.0f, 255.0f));
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
            data[i + 0] = static_cast<unsigned char>(clampf(c, 0, 255));
            data[i + 1] = static_cast<unsigned char>(clampf(c + 5, 0, 255));
            data[i + 2] = static_cast<unsigned char>(clampf(c + 10, 0, 255));
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

void setupShadowMap()
{
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupZones()
{
    zones.push_back({ glm::vec3(12.0f, 0.0f, 10.0f), 6.5f,
        { {"Sardine",0,5}, {"Mackerel",0,6}, {"Crab",1,11} },
        glm::vec3(0.18f, 0.55f, 0.35f), glm::vec3(0.72f, 0.82f, 0.92f), 16.0f, 66.0f, 0.2f, "Harbour Waters" });

    zones.push_back({ glm::vec3(-18.0f, 0.0f, -10.0f), 8.5f,
        { {"Snapper",0,8}, {"Lionfish",2,20}, {"Eel",2,25}, {"Grouper",1,13} },
        glm::vec3(0.10f, 0.60f, 0.55f), glm::vec3(0.58f, 0.78f, 0.78f), 13.0f, 53.0f, 0.45f, "Reef Edge" });

    zones.push_back({ glm::vec3(23.0f, 0.0f, -23.0f), 10.0f,
        { {"Anglerfish",2,30}, {"Oarfish",3,60}, {"Ghost Fish",3,100}, {"Black Cod",1,18} },
        glm::vec3(0.35f, 0.18f, 0.55f), glm::vec3(0.36f, 0.42f, 0.58f), 9.0f, 36.0f, 0.8f, "Deep Trench" });
}

const FishZone* getCurrentZone()
{
    for (const auto& zone : zones)
    {
        if (zone.contains(boat.position)) return &zone;
    }
    return nullptr;
}

std::string getCurrentZoneName()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->zoneName : "Open Water";
}

glm::vec3 getDefaultWaterTint()
{
    return glm::vec3(0.0f, 0.10f, 0.16f);
}

glm::vec3 getDefaultFogColor()
{
    return glm::vec3(0.70f, 0.82f, 0.94f);
}

float getDefaultFogNear()
{
    return 18.0f;
}

float getDefaultFogFar()
{
    return 72.0f;
}

float getDefaultDanger()
{
    return 0.1f;
}

glm::vec3 getTargetZoneWaterTint()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->tint : getDefaultWaterTint();
}

glm::vec3 getTargetZoneFogColor()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->fogColor : getDefaultFogColor();
}

float getTargetZoneFogNear()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->fogNear : getDefaultFogNear();
}

float getTargetZoneFogFar()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->fogFar : getDefaultFogFar();
}

float getTargetWorldDanger()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->danger : getDefaultDanger();
}

void initialiseEnvironmentBlend()
{
    gEnvironmentBlend.waterTint = getTargetZoneWaterTint();
    gEnvironmentBlend.fogColor = getTargetZoneFogColor();
    gEnvironmentBlend.fogNear = getTargetZoneFogNear();
    gEnvironmentBlend.fogFar = getTargetZoneFogFar();
    gEnvironmentBlend.danger = getTargetWorldDanger();
}

void updateEnvironmentBlend(float dt)
{
    const float blendStrength = 1.0f - std::exp(-2.4f * dt);

    gEnvironmentBlend.waterTint = glm::mix(gEnvironmentBlend.waterTint, getTargetZoneWaterTint(), blendStrength);
    gEnvironmentBlend.fogColor = glm::mix(gEnvironmentBlend.fogColor, getTargetZoneFogColor(), blendStrength);
    gEnvironmentBlend.fogNear = glm::mix(gEnvironmentBlend.fogNear, getTargetZoneFogNear(), blendStrength);
    gEnvironmentBlend.fogFar = glm::mix(gEnvironmentBlend.fogFar, getTargetZoneFogFar(), blendStrength);
    gEnvironmentBlend.danger = glm::mix(gEnvironmentBlend.danger, getTargetWorldDanger(), blendStrength);
}

glm::vec3 getZoneWaterTint()
{
    return gEnvironmentBlend.waterTint;
}

glm::vec3 getZoneFogColor()
{
    return gEnvironmentBlend.fogColor;
}

float getZoneFogNear()
{
    return gEnvironmentBlend.fogNear;
}

float getZoneFogFar()
{
    return gEnvironmentBlend.fogFar;
}

float getWorldDanger()
{
    return gEnvironmentBlend.danger;
}

float getQuestFogFactor()
{
    return gIslandQuest.GetLockedFogFactor(boat.position);
}

glm::vec3 getEffectiveFogColor()
{
    return glm::mix(getZoneFogColor(), glm::vec3(0.80f, 0.82f, 0.86f), getQuestFogFactor());
}

float getEffectiveFogNear()
{
    return glm::mix(getZoneFogNear(), 3.5f, getQuestFogFactor());
}

float getEffectiveFogFar()
{
    return glm::mix(getZoneFogFar(), 11.0f, getQuestFogFactor());
}

glm::vec3 getCameraPosition()
{
    float radians = glm::radians(boat.rotationY);
    glm::vec3 behind(-std::sin(radians), 0.0f, -std::cos(radians));
    return boat.position + behind * 8.0f + glm::vec3(0.0f, 5.25f, 0.0f);
}

bool isAtDock()
{
    return glm::length(glm::vec2(boat.position.x, boat.position.z)) < 5.5f;
}

int getCargoValue()
{
    int total = 0;
    for (const auto& item : cargo) total += item.fish.value;
    return total;
}

int rodUpgradeCost() { return 20 + (rodLevel - 1) * 15; }
int engineUpgradeCost() { return 25 + (engineLevel - 1) * 20; }
int cargoUpgradeCost() { return 18 + (cargoLevel - 1) * 14; }

void spawnParticle(const glm::vec3& pos, const glm::vec3& vel, const glm::vec4& color, float life, float size)
{
    for (auto& p : particles)
    {
        if (p.life <= 0.0f)
        {
            p.position = pos;
            p.velocity = vel;
            p.color = color;
            p.life = life;
            p.size = size;
            return;
        }
    }
}

void spawnWakeParticles()
{
    if (std::abs(boat.velocity) < 0.4f) return;

    float radians = glm::radians(boat.rotationY);
    glm::vec3 forward(std::sin(radians), 0.0f, std::cos(radians));
    glm::vec3 side(forward.z, 0.0f, -forward.x);
    glm::vec3 base = boat.position - forward * 1.8f + glm::vec3(0.0f, 0.02f, 0.0f);

    for (int i = 0; i < 2; ++i)
    {
        float lateral = (i == 0 ? -0.45f : 0.45f);
        glm::vec3 pos = base + side * lateral;
        glm::vec3 vel = -forward * (0.6f + static_cast<float>(rand() % 40) * 0.01f) + glm::vec3(0.0f, 0.2f, 0.0f);
        spawnParticle(pos, vel, glm::vec4(0.86f, 0.93f, 1.0f, 0.85f), 0.8f, 11.0f);
    }
}

void spawnSplash(const glm::vec3& pos, const glm::vec4& color, int count)
{
    for (int i = 0; i < count; ++i)
    {
        glm::vec3 vel(
            (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 1.2f,
            0.6f + static_cast<float>(rand() % 100) / 140.0f,
            (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 1.2f);
        spawnParticle(pos, vel, color, 0.9f, 10.0f + static_cast<float>(rand() % 6));
    }
}

void updateParticles(float dt)
{
    for (auto& p : particles)
    {
        if (p.life <= 0.0f) continue;
        p.life -= dt;
        if (p.life <= 0.0f) continue;
        p.velocity += glm::vec3(0.0f, -1.8f, 0.0f) * dt;
        p.position += p.velocity * dt;
        p.color.a = glm::clamp(p.life, 0.0f, 1.0f);
    }
}

FishData catchFishFromZone(const FishZone& zone)
{
    int roll = rand() % 100;
    roll -= (rodLevel - 1) * 6;

    std::vector<FishData> common, uncommon, rare, legendary;
    for (const auto& fish : zone.fishPool)
    {
        if (fish.rarity == 0) common.push_back(fish);
        else if (fish.rarity == 1) uncommon.push_back(fish);
        else if (fish.rarity == 2) rare.push_back(fish);
        else legendary.push_back(fish);
    }

    if (roll < 48 && !common.empty()) return common[rand() % common.size()];
    if (roll < 78 && !uncommon.empty()) return uncommon[rand() % uncommon.size()];
    if (roll < 95 && !rare.empty()) return rare[rand() % rare.size()];
    if (!legendary.empty()) return legendary[rand() % legendary.size()];
    return zone.fishPool[rand() % zone.fishPool.size()];
}

void setCatchMessage(GLFWwindow* window, const std::string& message, float timer = 2.5f)
{
    lastCatchText = message;
    catchMessageTimer = timer;
    std::cout << "[Fish] " << lastCatchText << std::endl;
    if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
}

bool canStartFishing(std::string& failMessage)
{
    if (fishCooldown > 0.0f)
    {
        failMessage = "Line is not ready yet";
        return false;
    }

    const FishZone* zone = getCurrentZone();
    if (!zone)
    {
        failMessage = "No fish here";
        return false;
    }

    if (static_cast<int>(cargo.size()) >= cargoCapacity)
    {
        failMessage = "Cargo full - return to dock to sell";
        return false;
    }

    return true;
}

int getCurrentZoneIndex()
{
    for (size_t i = 0; i < zones.size(); ++i)
    {
        if (zones[i].contains(boat.position)) return static_cast<int>(i);
    }
    return -1;
}

void awardFishCatch(GLFWwindow* window, const FishData& fish, float timingScore = 0.0f)
{
    cargo.push_back({ fish });

    std::stringstream ss;
    if (fish.rarity == 3) { ss << "LEGENDARY CATCH! "; catchFlashTimer = 0.7f; }
    else if (fish.rarity == 2) { ss << "RARE CATCH! "; catchFlashTimer = 0.35f; }
    else if (fish.rarity == 1) { ss << "UNCOMMON CATCH! "; }

    if (timingScore > 0.95f)
        ss << "PERFECT HOOK! ";
    else if (timingScore > 0.70f)
        ss << "GOOD HOOK! ";

    ss << fish.name
       << " | Value: " << fish.value << "g"
       << " | Cargo: " << cargo.size() << "/" << cargoCapacity;

    lastCatchText = ss.str();
    catchMessageTimer = 3.0f;

    std::cout << "[Catch] " << fish.name
              << " | Rarity: " << fish.rarity
              << " | Value: " << fish.value << "g"
              << " | Cargo: " << cargo.size() << "/" << cargoCapacity
              << std::endl;

    if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
    spawnSplash(boat.position + glm::vec3(0.0f, 0.1f, 1.8f), glm::vec4(0.82f, 0.94f, 1.0f, 0.85f), 12);
    gSound.PlaySplash();
}

FishData catchFishForMinigameResult(const FishZone& zone, float timingScore)
{
    FishData fish = catchFishFromZone(zone);

    if (timingScore > 0.94f && fish.rarity < 3)
    {
        fish.rarity += 1;
        fish.value = static_cast<int>(std::round(fish.value * 1.35f));
    }
    else if (timingScore > 0.75f)
    {
        fish.value = static_cast<int>(std::round(fish.value * 1.15f));
    }

    return fish;
}

void tryFishing(GLFWwindow* window)
{
    std::string failMessage;
    if (!canStartFishing(failMessage))
    {
        setCatchMessage(window, failMessage, 2.3f);
        return;
    }

    if (!gFishingMinigame.IsEnabled())
    {
        fishCooldown = 1.0f;
        const FishZone* zone = getCurrentZone();
        if (!zone)
        {
            setCatchMessage(window, "No fish here", 2.0f);
            return;
        }

        const int zoneIndex = getCurrentZoneIndex();
        if (gIslandQuest.TryCollectKeyForZone(zoneIndex, rodLevel, lastCatchText, catchMessageTimer))
        {
            if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
            spawnSplash(boat.position + glm::vec3(0.0f, 0.1f, 1.8f), glm::vec4(0.95f, 0.86f, 0.30f, 0.95f), 16);
            gSound.PlaySplash();
            return;
        }

        FishData fish = catchFishFromZone(*zone);
        awardFishCatch(window, fish, 0.0f);
        return;
    }

    if (gFishingMinigame.IsActive())
        return;

    gFishingMinigameZoneIndex = getCurrentZoneIndex();
    gFishingMinigame.Start();
    fishCooldown = 0.2f;
    lastCatchText = gFishingMinigame.GetStatusText();
    catchMessageTimer = 10.0f;
}

void resolveFishingMinigame(GLFWwindow* window, const FishingMinigameResult& result)
{
    fishCooldown = result.success ? 0.8f : 0.5f;

    if (!result.success)
    {
        catchFlashTimer = 0.0f;
        setCatchMessage(window, result.message, 2.0f);
        return;
    }

    int idx = gFishingMinigameZoneIndex;
    if (idx < 0 || idx >= static_cast<int>(zones.size()))
    {
        setCatchMessage(window, "The fish got away", 2.0f);
        return;
    }

    gSound.PlayReel();

    if (gIslandQuest.TryCollectKeyForZone(idx, rodLevel, lastCatchText, catchMessageTimer))
    {
        if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
        spawnSplash(boat.position + glm::vec3(0.0f, 0.1f, 1.8f), glm::vec4(0.95f, 0.86f, 0.30f, 0.95f), 18);
        gSound.PlaySplash();
        return;
    }

    FishData fish = catchFishForMinigameResult(zones[idx], result.timingScore);
    awardFishCatch(window, fish, result.timingScore);
}

void sellCargo()
{
    if (!isAtDock() || cargo.empty()) return;
    int soldValue = getCargoValue();
    totalMoney += soldValue;
    cargo.clear();
    lastCatchText = "Sold cargo for " + std::to_string(soldValue) + "g";
    catchMessageTimer = 2.5f;
    std::cout << "[Dock] " << lastCatchText << " | Gold: " << totalMoney << std::endl;
    spawnSplash(glm::vec3(0.0f, 0.3f, 1.2f), glm::vec4(1.0f, 0.86f, 0.35f, 1.0f), 18);
}

void buyRodUpgrade()
{
    int cost = rodUpgradeCost();
    if (!isAtDock() || totalMoney < cost) return;
    totalMoney -= cost;
    rodLevel++;
    lastCatchText = "Bought rod upgrade for " + std::to_string(cost) + "g";
    catchMessageTimer = 2.5f;
    std::cout << "[Dock] " << lastCatchText << " | Rod: " << rodLevel << std::endl;
}

void buyEngineUpgrade()
{
    int cost = engineUpgradeCost();
    if (!isAtDock() || totalMoney < cost) return;
    totalMoney -= cost;
    engineLevel++;
    boat.maxForwardSpeed += 1.2f;
    boat.acceleration += 0.35f;
    lastCatchText = "Bought engine upgrade for " + std::to_string(cost) + "g";
    catchMessageTimer = 2.5f;
    std::cout << "[Dock] " << lastCatchText << " | Engine: " << engineLevel << std::endl;
}

void buyCargoUpgrade()
{
    int cost = cargoUpgradeCost();
    if (!isAtDock() || totalMoney < cost) return;
    totalMoney -= cost;
    cargoLevel++;
    cargoCapacity += 2;
    lastCatchText = "Bought cargo upgrade for " + std::to_string(cost) + "g";
    catchMessageTimer = 2.5f;
    std::cout << "[Dock] " << lastCatchText << " | Capacity: " << cargoCapacity << std::endl;
}

void bindMaterial(GLuint shader, GLuint textureID, int materialType, float tiling)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    setInt(shader, "materialTex", 0);
    setInt(shader, "materialType", materialType);
    setFloat(shader, "uvTiling", tiling);
}

void drawCube(GLuint shader, const glm::mat4& model)
{
    setMat4(shader, "model", model);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawPlane(GLuint shader, const glm::mat4& model)
{
    setMat4(shader, "model", model);
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawModel(GLuint shader, const ModelMesh& modelMesh, const glm::mat4& model)
{
    if (!modelMesh.loaded)
        return;

    setMat4(shader, "model", model);
    glBindVertexArray(modelMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(modelMesh.vertexCount));
    glBindVertexArray(0);
}

void renderWorldGeometry(GLuint shader, bool depthPass, float time)
{
    auto setMat = [&](GLuint tex, int type, float tiling)
    {
        if (!depthPass) bindMaterial(shader, tex, type, tiling);
    };

    // Dock base
    {
        setMat(woodTex, MAT_WOOD, 3.0f);
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.28f, 0.0f));
        model = glm::scale(model, glm::vec3(5.0f, 0.22f, 3.0f));
        drawCube(shader, model);

        for (int x = -1; x <= 1; ++x)
        {
            for (int z = -1; z <= 1; z += 2)
            {
                glm::mat4 post(1.0f);
                post = glm::translate(post, glm::vec3(static_cast<float>(x) * 1.5f, -0.72f, static_cast<float>(z) * 1.0f));
                post = glm::scale(post, glm::vec3(0.22f, 2.0f, 0.22f));
                drawCube(shader, post);
            }
        }
    }

    // Dock shed
    {
        setMat(woodTex, MAT_WOOD, 2.0f);
        glm::mat4 base(1.0f);
        base = glm::translate(base, glm::vec3(-1.2f, 0.91f, 0.0f));
        base = glm::scale(base, glm::vec3(1.4f, 1.0f, 1.2f));
        drawCube(shader, base);

        glm::mat4 roof(1.0f);
        roof = glm::translate(roof, glm::vec3(-1.2f, 1.71f, 0.0f));
        roof = glm::rotate(roof, glm::radians(25.0f), glm::vec3(0, 0, 1));
        roof = glm::scale(roof, glm::vec3(1.9f, 0.16f, 1.5f));
        drawCube(shader, roof);
    }

    // Crates at dock
    {
        setMat(woodTex, MAT_WOOD, 1.5f);

        glm::mat4 crate1(1.0f);
        crate1 = glm::translate(crate1, glm::vec3(1.55f, 0.49f, -0.55f));
        crate1 = glm::scale(crate1, glm::vec3(0.24f, 0.24f, 0.24f));
        drawCube(shader, crate1);

        glm::mat4 crate2(1.0f);
        crate2 = glm::translate(crate2, glm::vec3(1.88f, 0.49f, -0.34f));
        crate2 = glm::scale(crate2, glm::vec3(0.20f, 0.20f, 0.20f));
        drawCube(shader, crate2);

        glm::mat4 crate3(1.0f);
        crate3 = glm::translate(crate3, glm::vec3(2.10f, 0.49f, -0.58f));
        crate3 = glm::scale(crate3, glm::vec3(0.18f, 0.18f, 0.18f));
        drawCube(shader, crate3);
    }

    // Islands layered
    std::vector<glm::vec3> islandPositions = {
        glm::vec3(12.0f, 0.35f, 10.0f),
        glm::vec3(-18.0f, 0.35f, -10.0f),
        glm::vec3(23.0f, 0.35f, -23.0f),
        glm::vec3(-10.0f, 0.35f, 18.0f)
    };

    for (size_t i = 0; i < islandPositions.size(); ++i)
    {
        glm::vec3 pos = islandPositions[i];
        setMat(rockTex, MAT_ROCK, 2.7f);
        for (int layer = 0; layer < 3; ++layer)
        {
            glm::mat4 rock(1.0f);
            rock = glm::translate(rock, pos + glm::vec3(0.25f * layer, layer * 0.35f, -0.15f * layer));
            rock = glm::scale(rock, glm::vec3(4.4f - layer * 0.9f, 0.9f, 4.0f - layer * 0.7f));
            drawCube(shader, rock);
        }

        setMat(grassTex, MAT_GRASS, 3.0f);
        glm::mat4 grass(1.0f);
        grass = glm::translate(grass, pos + glm::vec3(0.1f, 1.15f, 0.0f));
        grass = glm::scale(grass, glm::vec3(2.9f, 0.32f, 2.5f));
        drawCube(shader, grass);

        // simple palms/masts
        setMat(woodTex, MAT_WOOD, 1.0f);
        for (int trunk = 0; trunk < 2; ++trunk)
        {
            glm::mat4 t(1.0f);
            float xoff = trunk == 0 ? -0.6f : 0.65f;
            float zoff = trunk == 0 ? 0.55f : -0.5f;
            t = glm::translate(t, pos + glm::vec3(xoff, 2.1f, zoff));
            t = glm::rotate(t, glm::radians(trunk == 0 ? 8.0f : -10.0f), glm::vec3(0, 0, 1));
            t = glm::scale(t, glm::vec3(0.18f, 2.1f, 0.18f));
            drawCube(shader, t);

            setMat(grassTex, MAT_GRASS, 1.0f);
            glm::mat4 leaf(1.0f);
            leaf = glm::translate(leaf, pos + glm::vec3(xoff, 3.1f, zoff));
            leaf = glm::rotate(leaf, glm::radians(22.0f + trunk * 18.0f), glm::vec3(0, 1, 0));
            leaf = glm::scale(leaf, glm::vec3(1.25f, 0.12f, 0.34f));
            drawCube(shader, leaf);
        }
    }

    // Lost Island quest altar with sword
    if (gIslandQuest.IsGoalIslandUnlocked() || gIslandQuest.HasWon())
    {
        glm::vec3 goalPos = gIslandQuest.GetGoalIslandPosition();

        setMat(rockTex, MAT_ROCK, 2.8f);
        for (int layer = 0; layer < 3; ++layer)
        {
            glm::mat4 rock(1.0f);
            rock = glm::translate(rock, goalPos + glm::vec3(0.2f * layer, layer * 0.42f, -0.12f * layer));
            rock = glm::scale(rock, glm::vec3(4.8f - layer * 0.9f, 1.0f, 4.4f - layer * 0.8f));
            drawCube(shader, rock);
        }

        setMat(grassTex, MAT_GRASS, 3.0f);
        glm::mat4 grass(1.0f);
        grass = glm::translate(grass, goalPos + glm::vec3(0.0f, 1.35f, 0.0f));
        grass = glm::scale(grass, glm::vec3(3.2f, 0.34f, 2.9f));
        drawCube(shader, grass);

        std::vector<glm::mat4> altarPieces = BuildLostIslandAltarTransforms(goalPos);
        setMat(rockTex, MAT_ROCK, 1.7f);
        for (const auto& piece : altarPieces)
        {
            drawCube(shader, piece);
        }

        if (swordModel.loaded)
        {
            setMat(boatTex, MAT_LIGHT, 1.0f);
            glm::mat4 swordModelMatrix = BuildLostIslandSwordTransform(goalPos, time);
            drawModel(shader, swordModel, swordModelMatrix);
        }
        else
        {
            setMat(boatTex, MAT_LIGHT, 1.0f);
            glm::mat4 swordFallback = BuildLostIslandSwordFallbackTransform(goalPos, time);
            drawCube(shader, swordFallback);
        }
    }

    // Zone buoys
    for (const auto& zone : zones)
    {
        float pulse = 1.0f + std::sin(time * 2.1f + zone.center.x) * 0.05f;
        setMat(buoyTex, MAT_BUOY, 1.0f);
        glm::mat4 buoy(1.0f);
        buoy = glm::translate(buoy, zone.center + glm::vec3(0.0f, 0.7f + std::sin(time * 1.4f + zone.center.z) * 0.08f, 0.0f));
        buoy = glm::scale(buoy, glm::vec3(0.4f * pulse, 1.2f * pulse, 0.4f * pulse));
        drawCube(shader, buoy);

        setMat(woodTex, MAT_WOOD, 1.0f);
        glm::mat4 mast(1.0f);
        mast = glm::translate(mast, zone.center + glm::vec3(0.0f, 1.75f, 0.0f));
        mast = glm::scale(mast, glm::vec3(0.08f, 1.2f, 0.08f));
        drawCube(shader, mast);
    }

    float centerH = getWaterHeight(boat.position.x, boat.position.z, time);
    float frontH  = getWaterHeight(boat.position.x, boat.position.z + 0.75f, time);
    float backH   = getWaterHeight(boat.position.x, boat.position.z - 0.75f, time);
    float leftH   = getWaterHeight(boat.position.x - 0.45f, boat.position.z, time);
    float rightH  = getWaterHeight(boat.position.x + 0.45f, boat.position.z, time);

    float targetPitch = std::atan2(frontH - backH, 1.5f);
    float targetRoll  = std::atan2(rightH - leftH, 0.9f);

    // make the boat feel heavier than the water
    targetPitch *= 0.10f;
    targetRoll  *= 0.10f;

    // keep only a very small movement-based lean
    float movementRoll = glm::radians(boat.velocity * 0.15f);

    // smooth the visual motion so the boat sits on the water rather than snapping to every wave
    boatVisualY     = glm::mix(boatVisualY, centerH + 0.08f, 0.02f);
    boatVisualPitch = glm::mix(boatVisualPitch, targetPitch, 0.04f);
    boatVisualRoll  = glm::mix(boatVisualRoll, -targetRoll + movementRoll, 0.04f);

    glm::mat4 boatBase(1.0f);
    boatBase = glm::translate(boatBase, glm::vec3(boat.position.x, boatVisualY, boat.position.z));
    boatBase = glm::rotate(boatBase, glm::radians(boat.rotationY), glm::vec3(0, 1, 0));
    boatBase = glm::rotate(boatBase, boatVisualPitch, glm::vec3(1, 0, 0));
    boatBase = glm::rotate(boatBase, boatVisualRoll, glm::vec3(0, 0, 1));

    if (boatModel.loaded)
    {
        setMat(boatTex, MAT_BOAT, 1.0f);

        glm::mat4 boatModelMatrix = boatBase;
        boatModelMatrix = glm::translate(boatModelMatrix, glm::vec3(0.0f, -0.06f, 0.0f));
        boatModelMatrix = glm::rotate(boatModelMatrix, glm::radians(180.0f), glm::vec3(0, 1, 0));
        boatModelMatrix = glm::scale(boatModelMatrix, glm::vec3(0.40f, 0.40f, 0.40f));

        drawModel(shader, boatModel, boatModelMatrix);
    }
    else
    {
        setMat(boatTex, MAT_BOAT, 1.6f);
        glm::mat4 hull = glm::scale(boatBase, glm::vec3(1.25f, 0.4f, 2.35f));
        drawCube(shader, hull);

        setMat(woodTex, MAT_WOOD, 1.3f);
        glm::mat4 cabin = glm::translate(boatBase, glm::vec3(0.0f, 0.48f, -0.1f));
        cabin = glm::scale(cabin, glm::vec3(0.8f, 0.55f, 0.95f));
        drawCube(shader, cabin);

        glm::mat4 mast = glm::translate(boatBase, glm::vec3(0.0f, 1.18f, 0.0f));
        mast = glm::scale(mast, glm::vec3(0.09f, 1.7f, 0.09f));
        drawCube(shader, mast);
    }

    // Cargo crates on boat
    int shownCrates = std::min(static_cast<int>(cargo.size()), 4);
    for (int i = 0; i < shownCrates; ++i)
    {
        setMat(woodTex, MAT_WOOD, 1.0f);
        glm::mat4 crate = boatBase;

        float xoff = 0.0f;
        float zoff = 0.0f;
        float yoff = 0.52f;

        switch (i)
        {
        case 0:
            xoff = -0.10f; zoff = -0.10f;
            break;
        case 1:
            xoff = 0.10f; zoff = -0.10f;
            break;
        case 2:
            xoff = -0.10f; zoff = 0.08f;
            break;
        case 3:
            xoff = 0.10f; zoff = 0.08f;
            break;
        }

        crate = glm::translate(crate, glm::vec3(xoff, yoff, zoff));
        crate = glm::scale(crate, glm::vec3(0.08f, 0.08f, 0.08f));
        drawCube(shader, crate);
    } 
}

void renderDepthPass(const glm::mat4& lightSpaceMatrix, float time)
{
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(depthShader);
    setMat4(depthShader, "lightSpaceMatrix", lightSpaceMatrix);
    renderWorldGeometry(depthShader, true, time);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderSkybox(const glm::mat4& view, const glm::mat4& projection, float time)
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShader);
    glm::mat4 skyView = glm::mat4(glm::mat3(view));
    setMat4(skyboxShader, "view", skyView);
    setMat4(skyboxShader, "projection", projection);
    setFloat(skyboxShader, "time", time);
    setFloat(skyboxShader, "worldDanger", getWorldDanger());
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    setInt(skyboxShader, "skybox", 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void renderWater(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos, const glm::vec3& viewPos, float time)
{
    glUseProgram(waterShader);
    setMat4(waterShader, "view", view);
    setMat4(waterShader, "projection", projection);
    setMat4(waterShader, "lightSpaceMatrix", lightSpaceMatrix);
    setVec3(waterShader, "lightPos", lightPos);
    setVec3(waterShader, "viewPos", viewPos);
    setVec3(waterShader, "zoneTint", getZoneWaterTint());
    setVec3(waterShader, "fogColor", getEffectiveFogColor());
    setFloat(waterShader, "fogNear", getEffectiveFogNear());
    setFloat(waterShader, "fogFar", getEffectiveFogFar());
    setFloat(waterShader, "worldDanger", getWorldDanger());
    setFloat(waterShader, "time", time);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    setInt(waterShader, "shadowMap", 1);

    glm::mat4 waterModel(1.0f);
    waterModel = glm::scale(waterModel, glm::vec3(120.0f, 1.0f, 120.0f));
    drawPlane(waterShader, waterModel);
}

void renderLitScene(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos, const glm::vec3& viewPos, float time)
{
    glUseProgram(basicShader);
    setMat4(basicShader, "view", view);
    setMat4(basicShader, "projection", projection);
    setMat4(basicShader, "lightSpaceMatrix", lightSpaceMatrix);
    setVec3(basicShader, "lightPos", lightPos);
    setVec3(basicShader, "viewPos", viewPos);
    setVec3(basicShader, "fogColor", getEffectiveFogColor());
    setFloat(basicShader, "fogNear", getEffectiveFogNear());
    setFloat(basicShader, "fogFar", getEffectiveFogFar());
    setFloat(basicShader, "worldDanger", getWorldDanger());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    setInt(basicShader, "shadowMap", 1);

    renderWorldGeometry(basicShader, false, time);
}

void renderParticles(const glm::mat4& view, const glm::mat4& projection)
{
    std::vector<ParticleVertex> alive;
    alive.reserve(MAX_PARTICLES);
    for (const auto& p : particles)
    {
        if (p.life > 0.0f)
            alive.push_back({ p.position, p.color, p.size });
    }
    if (alive.empty()) return;

    glUseProgram(particleShader);
    setMat4(particleShader, "view", view);
    setMat4(particleShader, "projection", projection);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, alive.size() * sizeof(ParticleVertex), alive.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(alive.size()));
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

int main()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Dredge-style Fishing Prototype", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    basicShader = createShaderProgramFromFiles(BASIC_VERT_PATH, BASIC_FRAG_PATH);
    waterShader = createShaderProgramFromFiles(WATER_VERT_PATH, WATER_FRAG_PATH);
    depthShader = createShaderProgramFromFiles(DEPTH_VERT_PATH, DEPTH_FRAG_PATH);
    particleShader = createShaderProgramFromFiles(PARTICLE_VERT_PATH, PARTICLE_FRAG_PATH);
    skyboxShader = createShaderProgramFromFiles(SKYBOX_VERT_PATH, SKYBOX_FRAG_PATH);

    InitUIOverlay();
    if (!gPostProcessor.Init(gWindowWidth, gWindowHeight, POSTPROCESS_VERT_PATH, POSTPROCESS_FRAG_PATH))
    {
        std::cerr << "Failed to initialize post-processing pipeline." << std::endl;
    }

    setupCube();
    setupPlane();
    setupSkybox();
    setupParticles();
    setupShadowMap();
    setupZones();
    initialiseEnvironmentBlend();

    loadOBJModel("media/models/boat.obj", boatModel);
    if (!loadOBJModel("media/sword.obj", swordModel))
    {
        loadOBJModel("media/models/sword.obj", swordModel);
    }

    woodTex = loadTextureFromFile("media/wood.jpg");
    if (woodTex == 0) woodTex = createWoodTexture();

    rockTex = loadTextureFromFile("media/rock.jpg");
    if (rockTex == 0) rockTex = createRockTexture();

    grassTex = loadTextureFromFile("media/grass.jpg");
    if (grassTex == 0) grassTex = createGrassTexture();

    boatTex = loadTextureFromFile("textures/boat.jpg");
    if (boatTex == 0) boatTex = createBoatTexture();

    buoyTex = loadTextureFromFile("textures/buoy.jpg");
    if (buoyTex == 0) buoyTex = createBuoyTexture();

    skyboxCubemap = createFallbackCubemap();

    if (!gSound.Init("media/sounds"))
    {
        std::cerr << "Failed to initialize sound manager." << std::endl;
    }
    else
    {
        gSound.PlayBackgroundLoop();
    }

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        boat.update(deltaTime);
        gSound.UpdateBoatMotor(std::abs(boat.velocity), boat.maxForwardSpeed);
        updateEnvironmentBlend(deltaTime);
        gFishingMinigame.Update(deltaTime);
        gIslandQuest.Update(boat.position, lastCatchText, catchMessageTimer);
        gIslandQuest.HandleTestGoldHotkey(keys[GLFW_KEY_G], totalMoney, lastCatchText, catchMessageTimer);

        if (fishCooldown > 0.0f) fishCooldown -= deltaTime;
        if (catchFlashTimer > 0.0f) catchFlashTimer -= deltaTime;
        if (catchMessageTimer > 0.0f) catchMessageTimer -= deltaTime;

        spawnWakeParticles();
        updateParticles(deltaTime);

        if (keys[GLFW_KEY_M] && !minigameTogglePressed)
        {
            gFishingMinigame.ToggleEnabled();
            minigameTogglePressed = true;
            setCatchMessage(window, gFishingMinigame.IsEnabled() ? "Fishing minigame enabled" : "Fishing minigame disabled", 1.6f);
        }
        if (!keys[GLFW_KEY_M]) minigameTogglePressed = false;

        if (gFishingMinigame.IsActive())
        {
            if (keys[GLFW_KEY_SPACE] && !minigameHookPressed)
            {
                gFishingMinigame.Hook();
                minigameHookPressed = true;
            }
            if (!keys[GLFW_KEY_SPACE]) minigameHookPressed = false;
        }
        else
        {
            minigameHookPressed = false;
        }

        FishingMinigameResult minigameResult;
        if (gFishingMinigame.ConsumeResult(minigameResult))
        {
            resolveFishingMinigame(window, minigameResult);
        }

        if (keys[GLFW_KEY_E] && !fishPressed) { tryFishing(window); fishPressed = true; }
        if (!keys[GLFW_KEY_E]) fishPressed = false;

        if (keys[GLFW_KEY_R] && !sellPressed) { sellCargo(); sellPressed = true; }
        if (!keys[GLFW_KEY_R]) sellPressed = false;

        if (keys[GLFW_KEY_1] && !upgrade1Pressed) { buyRodUpgrade(); upgrade1Pressed = true; }
        if (!keys[GLFW_KEY_1]) upgrade1Pressed = false;

        if (keys[GLFW_KEY_2] && !upgrade2Pressed) { buyEngineUpgrade(); upgrade2Pressed = true; }
        if (!keys[GLFW_KEY_2]) upgrade2Pressed = false;

        if (keys[GLFW_KEY_3] && !upgrade3Pressed) { buyCargoUpgrade(); upgrade3Pressed = true; }
        if (!keys[GLFW_KEY_3]) upgrade3Pressed = false;

        std::stringstream title;
        title << "Dredge-style Fishing Prototype | Zone: " << getCurrentZoneName()
              << " | Gold: " << totalMoney
              << " | Cargo: " << cargo.size() << "/" << cargoCapacity << " (" << getCargoValue() << "g)"
              << " | Rod: " << rodLevel
              << " | Speed: " << std::fixed << std::setprecision(1) << boat.velocity;

        const char* postLabel = "None";
        if (gPostMode == PostProcessMode::Edge) postLabel = "Edge";
        else if (gPostMode == PostProcessMode::Blur) postLabel = "Blur";
        else if (gPostMode == PostProcessMode::NightVision) postLabel = "NightVision";
        title << " | PP: " << postLabel
              << " | MiniGame: " << (gFishingMinigame.IsEnabled() ? (gFishingMinigame.IsActive() ? "Active" : "On") : "Off")
              << " | Quest: " << gIslandQuest.GetQuestSummary();
        if (isAtDock())
        {
            title << " | Sell[R] | Rod[1:" << rodUpgradeCost() << "g]"
                  << " | Engine[2:" << engineUpgradeCost() << "g]"
                  << " | Cargo[3:" << cargoUpgradeCost() << "g]";
        }
        else
        {
            title << " | E Fish";
        }

        if (catchMessageTimer > 0.0f && !lastCatchText.empty())
        {
            title << " | " << lastCatchText;
        }

        glfwSetWindowTitle(window, title.str().c_str());

        glm::vec3 fogColor = getEffectiveFogColor();
        glm::vec3 clearColor = glm::mix(fogColor, glm::vec3(0.95f, 0.90f, 0.78f), catchFlashTimer * 0.4f);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);

        glm::vec3 cameraPos = getCameraPosition();
        glm::mat4 view = glm::lookAt(cameraPos, boat.position + glm::vec3(0.0f, 0.9f, 0.0f), glm::vec3(0, 1, 0));
        gPostProcessor.EnsureSize(gWindowWidth, gWindowHeight);

        glm::mat4 projection = glm::perspective(glm::radians(60.0f), static_cast<float>(gWindowWidth) / static_cast<float>(std::max(gWindowHeight, 1)), 0.1f, 500.0f);

        glm::vec3 lightPos(18.0f, 26.0f, 12.0f);
        glm::mat4 lightProjection = glm::ortho(-65.0f, 65.0f, -65.0f, 65.0f, 1.0f, 100.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        renderDepthPass(lightSpaceMatrix, currentFrame);

        gPostProcessor.BeginScene(clearColor);
        glViewport(0, 0, gWindowWidth, gWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderSkybox(view, projection, currentFrame);
        renderWater(view, projection, lightSpaceMatrix, lightPos, cameraPos, currentFrame);
        renderLitScene(view, projection, lightSpaceMatrix, lightPos, cameraPos, currentFrame);
        renderParticles(view, projection);
        gPostProcessor.EndScene();

        glViewport(0, 0, gWindowWidth, gWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gPostProcessor.Render(gPostMode, currentFrame);

        HUDState hud;
        hud.screenWidth = gWindowWidth;
        hud.screenHeight = gWindowHeight;
        hud.zoneName = gIslandQuest.HasWon() ? "Lost Island" : getCurrentZoneName();
        hud.statusText = lastCatchText;
        hud.gold = totalMoney;
        hud.cargoCount = static_cast<int>(cargo.size());
        hud.cargoCapacity = cargoCapacity;
        hud.cargoValue = getCargoValue();
        hud.rodLevel = rodLevel;
        hud.engineLevel = engineLevel;
        hud.speed = std::abs(boat.velocity);
        hud.danger = getWorldDanger();
        hud.atDock = isAtDock();
        hud.messageTimer = catchMessageTimer;
        hud.flash = catchFlashTimer;
        if (gFishingMinigame.IsActive())
        {
            hud.hintText = gFishingMinigame.GetHintText() + " | " + gIslandQuest.GetQuestSummary();
            hud.statusText = gFishingMinigame.GetStatusText();
            hud.messageTimer = 999.0f;
            hud.flash = 0.0f;
        }
        else if (hud.atDock)
        {
            hud.hintText = "SELL:R  ROD:1  ENGINE:2  CARGO:3  G +1000  M MINIGAME  | " + gIslandQuest.GetQuestSummary();
        }
        else
        {
            hud.hintText = "CURRENT SPEED " + std::to_string(static_cast<int>(std::abs(boat.velocity))) + " | M MINIGAME | G +1000 | " + gIslandQuest.GetQuestSummary();
        }
        RenderUIOverlay(hud);

        glfwSwapBuffers(window);
    }

    if (boatModel.vbo != 0) glDeleteBuffers(1, &boatModel.vbo);
    if (boatModel.vao != 0) glDeleteVertexArrays(1, &boatModel.vao);
    if (swordModel.vbo != 0) glDeleteBuffers(1, &swordModel.vbo);
    if (swordModel.vao != 0) glDeleteVertexArrays(1, &swordModel.vao);

    gPostProcessor.Shutdown();
    ShutdownUIOverlay();
    gSound.Shutdown();

    glfwTerminate();
    return 0;
}
