#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cstddef>

// ------------------------------------------------------------
// Window settings
// ------------------------------------------------------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// ------------------------------------------------------------
// Input
// ------------------------------------------------------------
bool keys[1024] = { false };
bool fishPressed = false;
bool sellPressed = false;
bool rodUpgradePressed = false;
bool engineUpgradePressed = false;
bool cargoUpgradePressed = false;

// ------------------------------------------------------------
// Fish data
// ------------------------------------------------------------
enum class FishRarity
{
    Common,
    Uncommon,
    Rare,
    Legendary
};

struct FishData
{
    std::string name;
    FishRarity rarity;
    int value;
};

std::string rarityToString(FishRarity rarity)
{
    switch (rarity)
    {
    case FishRarity::Common: return "Common";
    case FishRarity::Uncommon: return "Uncommon";
    case FishRarity::Rare: return "Rare";
    case FishRarity::Legendary: return "Legendary";
    default: return "Unknown";
    }
}

struct FishZone
{
    glm::vec3 center;
    float radius;
    std::vector<FishData> fishPool;
    glm::vec3 colour;
    std::string zoneName;
    float danger;

    bool contains(const glm::vec3& pos) const
    {
        float distance = glm::length(glm::vec2(pos.x - center.x, pos.z - center.z));
        return distance <= radius;
    }

    FishData catchFish(int rodLevel) const
    {
        int roll = rand() % 100;
        int luckBonus = (rodLevel - 1) * 6;
        roll = std::max(0, roll - luckBonus);

        std::vector<FishData> commons;
        std::vector<FishData> uncommons;
        std::vector<FishData> rares;
        std::vector<FishData> legendaries;

        for (const auto& fish : fishPool)
        {
            switch (fish.rarity)
            {
            case FishRarity::Common: commons.push_back(fish); break;
            case FishRarity::Uncommon: uncommons.push_back(fish); break;
            case FishRarity::Rare: rares.push_back(fish); break;
            case FishRarity::Legendary: legendaries.push_back(fish); break;
            }
        }

        if (roll < 45 && !commons.empty())
            return commons[rand() % commons.size()];
        else if (roll < 75 && !uncommons.empty())
            return uncommons[rand() % uncommons.size()];
        else if (roll < 94 && !rares.empty())
            return rares[rand() % rares.size()];
        else if (!legendaries.empty())
            return legendaries[rand() % legendaries.size()];

        return fishPool[rand() % fishPool.size()];
    }
};

// ------------------------------------------------------------
// Boat
// ------------------------------------------------------------
class Boat
{
public:
    Boat()
        : position(0.0f, 0.2f, 0.0f),
        rotationY(0.0f),
        velocity(0.0f),
        maxForwardSpeed(7.0f),
        maxBackwardSpeed(3.5f),
        acceleration(4.5f),
        deceleration(3.0f),
        turnSpeed(85.0f)
    {
    }

    void update(float dt)
    {
        if (keys[GLFW_KEY_W])
        {
            velocity += acceleration * dt;
            if (velocity > maxForwardSpeed)
                velocity = maxForwardSpeed;
        }
        else if (keys[GLFW_KEY_S])
        {
            velocity -= acceleration * dt;
            if (velocity < -maxBackwardSpeed)
                velocity = -maxBackwardSpeed;
        }
        else
        {
            if (velocity > 0.0f)
            {
                velocity -= deceleration * dt;
                if (velocity < 0.0f)
                    velocity = 0.0f;
            }
            else if (velocity < 0.0f)
            {
                velocity += deceleration * dt;
                if (velocity > 0.0f)
                    velocity = 0.0f;
            }
        }

        float turnAmount = 0.0f;
        if (keys[GLFW_KEY_A])
            turnAmount += 1.0f;
        if (keys[GLFW_KEY_D])
            turnAmount -= 1.0f;

        float speedFactor = glm::clamp(std::abs(velocity) / std::max(0.1f, maxForwardSpeed), 0.2f, 1.0f);
        rotationY += turnAmount * turnSpeed * speedFactor * dt;

        float radians = glm::radians(rotationY);
        glm::vec3 forward(std::sin(radians), 0.0f, std::cos(radians));
        position += forward * velocity * dt;

        float limit = 55.0f;
        position.x = glm::clamp(position.x, -limit, limit);
        position.z = glm::clamp(position.z, -limit, limit);
    }

    glm::vec3 position;
    float rotationY;
    float velocity;
    float maxForwardSpeed;
    float maxBackwardSpeed;
    float acceleration;
    float deceleration;
    float turnSpeed;
};

const struct FishZone* getCurrentZone();
glm::vec3 getCurrentFogColor();
float getCurrentFogDensity();

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
Boat boat;
std::vector<FishZone> zones;
std::vector<FishData> cargoHold;
std::string lastCatchText = "No fish caught yet";
int totalMoney = 0;
int cargoCapacity = 5;
int rodLevel = 1;
int engineLevel = 1;
int cargoLevel = 1;
float fishCooldown = 0.0f;
float catchFlashTimer = 0.0f;
float statusMessageTimer = 0.0f;
float worldDanger = 0.0f;
const glm::vec3 dockCenter(0.0f, 0.0f, 0.0f);
const float dockRadius = 6.0f;

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float size;
    float life;
    float maxLife;
};

struct ParticleVertex
{
    glm::vec3 position;
    glm::vec4 color;
    float size;
};

std::vector<Particle> particles;
const size_t MAX_PARTICLES = 600;

glm::vec3 getCurrentFogColor()
{
    const FishZone* zone = getCurrentZone();
    if (!zone)
        return glm::vec3(0.58f, 0.72f, 0.84f);

    if (zone->danger < 0.25f)
        return glm::vec3(0.66f, 0.76f, 0.84f);
    if (zone->danger < 0.6f)
        return glm::vec3(0.52f, 0.68f, 0.70f);
    return glm::vec3(0.34f, 0.40f, 0.48f);
}

float getCurrentFogDensity()
{
    const FishZone* zone = getCurrentZone();
    if (!zone)
        return 0.016f;
    return 0.018f + zone->danger * 0.030f;
}

// ------------------------------------------------------------
// Shader helpers
// ------------------------------------------------------------
std::string readTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const std::string& source, const std::string& debugName)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Shader compilation failed (" << debugName << "):\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSrc = readTextFile(vertexPath);
    std::string fragmentSrc = readTextFile(fragmentPath);

    if (vertexSrc.empty() || fragmentSrc.empty())
        return 0;

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc, vertexPath);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc, fragmentPath);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "Shader linking failed (" << vertexPath << ", " << fragmentPath << "):\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// ------------------------------------------------------------
// External shader files
// ------------------------------------------------------------
const std::string BASIC_VERT_PATH = "shader/basic.vert";
const std::string BASIC_FRAG_PATH = "shader/basic.frag";
const std::string WATER_VERT_PATH = "shader/water.vert";
const std::string WATER_FRAG_PATH = "shader/water.frag";
const std::string DEPTH_VERT_PATH = "shader/depth.vert";
const std::string DEPTH_FRAG_PATH = "shader/depth.frag";
const std::string PARTICLE_VERT_PATH = "shader/particle.vert";
const std::string PARTICLE_FRAG_PATH = "shader/particle.frag";

// ------------------------------------------------------------
// Mesh data
// ------------------------------------------------------------
float cubeVertices[] = {
    -0.5f,-0.5f,-0.5f,    0.0f, 0.0f,-1.0f,
     0.5f,-0.5f,-0.5f,    0.0f, 0.0f,-1.0f,
     0.5f, 0.5f,-0.5f,    0.0f, 0.0f,-1.0f,
     0.5f, 0.5f,-0.5f,    0.0f, 0.0f,-1.0f,
    -0.5f, 0.5f,-0.5f,    0.0f, 0.0f,-1.0f,
    -0.5f,-0.5f,-0.5f,    0.0f, 0.0f,-1.0f,

    -0.5f,-0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
     0.5f,-0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
    -0.5f,-0.5f, 0.5f,    0.0f, 0.0f, 1.0f,

    -0.5f, 0.5f, 0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f,-0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f,-0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f,-0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f, 0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f,   -1.0f, 0.0f, 0.0f,

     0.5f, 0.5f, 0.5f,    1.0f, 0.0f, 0.0f,
     0.5f, 0.5f,-0.5f,    1.0f, 0.0f, 0.0f,
     0.5f,-0.5f,-0.5f,    1.0f, 0.0f, 0.0f,
     0.5f,-0.5f,-0.5f,    1.0f, 0.0f, 0.0f,
     0.5f,-0.5f, 0.5f,    1.0f, 0.0f, 0.0f,
     0.5f, 0.5f, 0.5f,    1.0f, 0.0f, 0.0f,

    -0.5f,-0.5f,-0.5f,    0.0f,-1.0f, 0.0f,
     0.5f,-0.5f,-0.5f,    0.0f,-1.0f, 0.0f,
     0.5f,-0.5f, 0.5f,    0.0f,-1.0f, 0.0f,
     0.5f,-0.5f, 0.5f,    0.0f,-1.0f, 0.0f,
    -0.5f,-0.5f, 0.5f,    0.0f,-1.0f, 0.0f,
    -0.5f,-0.5f,-0.5f,    0.0f,-1.0f, 0.0f,

    -0.5f, 0.5f,-0.5f,    0.0f, 1.0f, 0.0f,
     0.5f, 0.5f,-0.5f,    0.0f, 1.0f, 0.0f,
     0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,
     0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f,-0.5f,    0.0f, 1.0f, 0.0f
};

float planeVertices[] = {
    -0.5f, 0.0f, -0.5f,    0.0f, 1.0f, 0.0f,
     0.5f, 0.0f, -0.5f,    0.0f, 1.0f, 0.0f,
     0.5f, 0.0f,  0.5f,    0.0f, 1.0f, 0.0f,

     0.5f, 0.0f,  0.5f,    0.0f, 1.0f, 0.0f,
    -0.5f, 0.0f,  0.5f,    0.0f, 1.0f, 0.0f,
    -0.5f, 0.0f, -0.5f,    0.0f, 1.0f, 0.0f
};

GLuint cubeVAO = 0, cubeVBO = 0;
GLuint planeVAO = 0, planeVBO = 0;

GLuint basicShader = 0;
GLuint waterShader = 0;
GLuint depthShader = 0;
GLuint particleShader = 0;
GLuint shadowFBO = 0;
GLuint shadowMap = 0;
GLuint particleVAO = 0, particleVBO = 0;

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

void setupCube()
{
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void setupPlane()
{
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void setupParticleSystem()
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

void setFloat(GLuint shader, const char* name, float value)
{
    glUniform1f(glGetUniformLocation(shader, name), value);
}

void setInt(GLuint shader, const char* name, int value)
{
    glUniform1i(glGetUniformLocation(shader, name), value);
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

void drawCube(GLuint shader, const glm::mat4& model, const glm::vec3& colour)
{
    glUseProgram(shader);
    GLint colorLoc = glGetUniformLocation(shader, "objectColor");
    if (colorLoc != -1)
        setVec3(shader, "objectColor", colour);
    setMat4(shader, "model", model);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void drawPlane(GLuint shader, const glm::mat4& model)
{
    glUseProgram(shader);
    setMat4(shader, "model", model);

    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

float randomRange(float minV, float maxV)
{
    return minV + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (maxV - minV);
}

void spawnParticle(const glm::vec3& pos, const glm::vec3& vel, const glm::vec4& color, float size, float life)
{
    if (particles.size() >= MAX_PARTICLES)
        particles.erase(particles.begin());

    particles.push_back({ pos, vel, color, size, life, life });
}

void emitWakeParticles()
{
    if (std::abs(boat.velocity) < 0.35f)
        return;

    float radians = glm::radians(boat.rotationY);
    glm::vec3 forward(std::sin(radians), 0.0f, std::cos(radians));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 basePos = boat.position - forward * 1.55f + glm::vec3(0.0f, 0.02f, 0.0f);

    float speedFactor = glm::clamp(std::abs(boat.velocity) / std::max(0.1f, boat.maxForwardSpeed), 0.0f, 1.0f);
    int count = 1 + static_cast<int>(speedFactor * 2.0f);

    for (int i = 0; i < count; ++i)
    {
        float side = (i % 2 == 0) ? -1.0f : 1.0f;
        glm::vec3 pos = basePos + right * side * randomRange(0.15f, 0.55f);
        glm::vec3 vel = -forward * randomRange(0.5f, 1.6f) + right * side * randomRange(0.1f, 0.35f);
        vel.y = randomRange(0.18f, 0.36f);
        spawnParticle(pos, vel, glm::vec4(0.85f, 0.93f, 1.0f, 0.55f), randomRange(8.0f, 15.0f), randomRange(0.7f, 1.2f));
    }
}

void emitSplashParticles(const glm::vec3& origin, int count, const glm::vec4& color, float sizeScale = 1.0f)
{
    for (int i = 0; i < count; ++i)
    {
        float angle = randomRange(0.0f, 6.28318f);
        float radius = randomRange(0.05f, 0.75f);
        glm::vec3 dir(std::cos(angle), 0.0f, std::sin(angle));
        glm::vec3 pos = origin + dir * radius + glm::vec3(0.0f, 0.05f, 0.0f);
        glm::vec3 vel = dir * randomRange(0.2f, 1.0f);
        vel.y = randomRange(0.35f, 0.95f);
        spawnParticle(pos, vel, color, randomRange(10.0f, 20.0f) * sizeScale, randomRange(0.6f, 1.1f));
    }
}

void updateParticles(float dt)
{
    for (size_t i = 0; i < particles.size();)
    {
        Particle& p = particles[i];
        p.life -= dt;
        if (p.life <= 0.0f)
        {
            particles.erase(particles.begin() + static_cast<long long>(i));
            continue;
        }

        p.velocity.y -= 1.35f * dt;
        p.position += p.velocity * dt;
        p.velocity *= (1.0f - 0.55f * dt);

        float lifeT = glm::clamp(p.life / std::max(0.01f, p.maxLife), 0.0f, 1.0f);
        p.color.a = lifeT * 0.6f;
        ++i;
    }
}

void renderParticles(const glm::mat4& view, const glm::mat4& projection)
{
    if (particles.empty())
        return;

    std::vector<ParticleVertex> verts;
    verts.reserve(particles.size());
    for (const auto& p : particles)
        verts.push_back({ p.position, p.color, p.size });

    glUseProgram(particleShader);
    setMat4(particleShader, "view", view);
    setMat4(particleShader, "projection", projection);

    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(ParticleVertex), verts.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(verts.size()));
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void setupZones()
{
    std::vector<FishData> harbourFish = {
        { "Sardine", FishRarity::Common, 5 },
        { "Mackerel", FishRarity::Common, 6 },
        { "Crab", FishRarity::Uncommon, 10 }
    };

    std::vector<FishData> reefFish = {
        { "Snapper", FishRarity::Common, 8 },
        { "Lionfish", FishRarity::Rare, 20 },
        { "Eel", FishRarity::Rare, 25 }
    };

    std::vector<FishData> deepSeaFish = {
        { "Anglerfish", FishRarity::Rare, 30 },
        { "Oarfish", FishRarity::Legendary, 60 },
        { "Ghost Fish", FishRarity::Legendary, 100 }
    };

    zones.push_back({ glm::vec3(15.0f, 0.0f, 10.0f), 6.2f, harbourFish, glm::vec3(0.22f, 0.72f, 0.36f), "Harbour Waters", 0.15f });
    zones.push_back({ glm::vec3(-18.0f, 0.0f, -8.0f), 7.2f, reefFish, glm::vec3(0.15f, 0.65f, 0.55f), "Reef Edge", 0.35f });
    zones.push_back({ glm::vec3(25.0f, 0.0f, -22.0f), 8.4f, deepSeaFish, glm::vec3(0.32f, 0.20f, 0.60f), "Deep Trench", 0.85f });
}

const FishZone* getCurrentZone()
{
    for (const auto& zone : zones)
    {
        if (zone.contains(boat.position))
            return &zone;
    }
    return nullptr;
}

std::string getCurrentZoneName()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->zoneName : "Open Water";
}

glm::vec3 getZoneWaterTint()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->colour : glm::vec3(0.0f, 0.08f, 0.10f);
}

bool isAtDock()
{
    float distance = glm::length(glm::vec2(boat.position.x - dockCenter.x, boat.position.z - dockCenter.z));
    return distance <= dockRadius && std::abs(boat.velocity) < 1.25f;
}

int getCargoValue()
{
    int total = 0;
    for (const auto& fish : cargoHold)
        total += fish.value;
    return total;
}

int getRodUpgradeCost()
{
    return 20 + (rodLevel - 1) * 30;
}

int getEngineUpgradeCost()
{
    return 25 + (engineLevel - 1) * 35;
}

int getCargoUpgradeCost()
{
    return 18 + (cargoLevel - 1) * 28;
}

void applyEngineUpgradeStats()
{
    boat.maxForwardSpeed = 7.0f + (engineLevel - 1) * 1.4f;
    boat.maxBackwardSpeed = 3.5f + (engineLevel - 1) * 0.5f;
    boat.acceleration = 4.5f + (engineLevel - 1) * 0.8f;
}

void setStatusMessage(const std::string& message, float duration = 2.2f)
{
    lastCatchText = message;
    statusMessageTimer = duration;
    std::cout << message << std::endl;
}

void sellCargo()
{
    if (!isAtDock())
    {
        setStatusMessage("Move closer to the dock and slow down to sell.");
        return;
    }

    if (cargoHold.empty())
    {
        setStatusMessage("Cargo hold is empty.");
        return;
    }

    int saleValue = getCargoValue();
    totalMoney += saleValue;
    cargoHold.clear();
    emitSplashParticles(dockCenter + glm::vec3(0.0f, 0.15f, 0.0f), 18, glm::vec4(0.95f, 0.85f, 0.35f, 0.75f), 0.9f);

    std::stringstream ss;
    ss << "Sold all cargo for " << saleValue << " gold. Total gold: " << totalMoney;
    setStatusMessage(ss.str(), 3.0f);
}

void buyRodUpgrade()
{
    int cost = getRodUpgradeCost();
    if (!isAtDock())
    {
        setStatusMessage("Dock to buy upgrades.");
        return;
    }
    if (totalMoney < cost)
    {
        std::stringstream ss;
        ss << "Not enough gold for rod upgrade. Need " << cost << ".";
        setStatusMessage(ss.str());
        return;
    }

    totalMoney -= cost;
    rodLevel++;
    std::stringstream ss;
    ss << "Rod upgraded to Lv " << rodLevel << ". Better odds for rare fish.";
    setStatusMessage(ss.str(), 3.0f);
}

void buyEngineUpgrade()
{
    int cost = getEngineUpgradeCost();
    if (!isAtDock())
    {
        setStatusMessage("Dock to buy upgrades.");
        return;
    }
    if (totalMoney < cost)
    {
        std::stringstream ss;
        ss << "Not enough gold for engine upgrade. Need " << cost << ".";
        setStatusMessage(ss.str());
        return;
    }

    totalMoney -= cost;
    engineLevel++;
    applyEngineUpgradeStats();
    std::stringstream ss;
    ss << "Engine upgraded to Lv " << engineLevel << ". Boat speed increased.";
    setStatusMessage(ss.str(), 3.0f);
}

void buyCargoUpgrade()
{
    int cost = getCargoUpgradeCost();
    if (!isAtDock())
    {
        setStatusMessage("Dock to buy upgrades.");
        return;
    }
    if (totalMoney < cost)
    {
        std::stringstream ss;
        ss << "Not enough gold for cargo upgrade. Need " << cost << ".";
        setStatusMessage(ss.str());
        return;
    }

    totalMoney -= cost;
    cargoLevel++;
    cargoCapacity += 2;
    std::stringstream ss;
    ss << "Cargo hold upgraded to Lv " << cargoLevel << ". Capacity is now " << cargoCapacity << ".";
    setStatusMessage(ss.str(), 3.0f);
}

void tryFishing()
{
    if (fishCooldown > 0.0f)
        return;

    if (cargoHold.size() >= static_cast<size_t>(cargoCapacity))
    {
        setStatusMessage("Cargo hold full. Return to dock to sell.");
        fishCooldown = 0.5f;
        return;
    }

    fishCooldown = 1.0f;

    for (const auto& zone : zones)
    {
        if (zone.contains(boat.position))
        {
            FishData fish = zone.catchFish(rodLevel);
            cargoHold.push_back(fish);

            std::stringstream ss;
            if (fish.rarity == FishRarity::Legendary)
            {
                ss << "LEGENDARY CATCH! ";
                catchFlashTimer = 0.6f;
            }
            else if (fish.rarity == FishRarity::Rare)
            {
                ss << "RARE CATCH! ";
                catchFlashTimer = 0.3f;
            }

            ss << fish.name
               << " | " << rarityToString(fish.rarity)
               << " | Value: " << fish.value
               << " | Cargo: " << cargoHold.size() << "/" << cargoCapacity;

            emitSplashParticles(boat.position + glm::vec3(0.0f, 0.1f, 0.0f), 14, glm::vec4(0.88f, 0.95f, 1.0f, 0.7f));
            setStatusMessage(ss.str());
            return;
        }
    }

    setStatusMessage("No fish here.");
}

glm::vec3 getCameraPosition()
{
    float radians = glm::radians(boat.rotationY);
    glm::vec3 behind(-std::sin(radians), 0.0f, -std::cos(radians));
    return boat.position + behind * 8.0f + glm::vec3(0.0f, 5.0f, 0.0f);
}

void drawIslandCluster(GLuint shader, const glm::vec3& pos, const glm::vec3& grassColor, bool stylisedPalms = true)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos + glm::vec3(0.0f, 0.30f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f, 0.9f, 4.8f));
    drawCube(shader, model, glm::vec3(0.27f, 0.24f, 0.22f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, pos + glm::vec3(0.8f, 0.95f, -0.3f));
    model = glm::scale(model, glm::vec3(3.8f, 0.8f, 3.4f));
    drawCube(shader, model, glm::vec3(0.34f, 0.31f, 0.28f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, pos + glm::vec3(-0.3f, 1.45f, 0.5f));
    model = glm::scale(model, glm::vec3(2.8f, 0.45f, 2.6f));
    drawCube(shader, model, grassColor);

    if (stylisedPalms)
    {
        for (int i = 0; i < 2; ++i)
        {
            float offset = i == 0 ? -0.9f : 0.9f;
            model = glm::mat4(1.0f);
            model = glm::translate(model, pos + glm::vec3(offset, 2.0f, 0.2f * (i == 0 ? -1.0f : 1.0f)));
            model = glm::scale(model, glm::vec3(0.16f, 1.7f, 0.16f));
            drawCube(shader, model, glm::vec3(0.35f, 0.24f, 0.12f));

            model = glm::mat4(1.0f);
            model = glm::translate(model, pos + glm::vec3(offset, 2.95f, 0.0f));
            model = glm::scale(model, glm::vec3(1.2f, 0.18f, 0.45f));
            drawCube(shader, model, glm::vec3(0.18f, 0.52f, 0.20f));

            model = glm::mat4(1.0f);
            model = glm::translate(model, pos + glm::vec3(offset, 2.95f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(1.2f, 0.18f, 0.45f));
            drawCube(shader, model, glm::vec3(0.18f, 0.52f, 0.20f));
        }
    }
}

void drawDockStructures(GLuint shader)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.12f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f, 0.18f, 3.0f));
    drawCube(shader, model, glm::vec3(0.45f, 0.28f, 0.12f));

    std::vector<glm::vec3> pylons = {
        {-2.0f, -0.8f, -1.1f}, {2.0f, -0.8f, -1.1f}, {-2.0f, -0.8f, 1.1f}, {2.0f, -0.8f, 1.1f}
    };
    for (const auto& p : pylons)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, p);
        model = glm::scale(model, glm::vec3(0.22f, 1.8f, 0.22f));
        drawCube(shader, model, glm::vec3(0.23f, 0.16f, 0.08f));
    }

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.6f, 0.65f, 0.0f));
    model = glm::scale(model, glm::vec3(1.8f, 0.8f, 1.6f));
    drawCube(shader, model, glm::vec3(0.52f, 0.38f, 0.22f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.6f, 1.35f, 0.0f));
    model = glm::scale(model, glm::vec3(2.2f, 0.2f, 2.0f));
    drawCube(shader, model, glm::vec3(0.28f, 0.16f, 0.12f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.6f, 0.45f, 0.85f));
    model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));
    drawCube(shader, model, glm::vec3(0.58f, 0.38f, 0.18f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.4f, -0.85f));
    model = glm::scale(model, glm::vec3(0.6f, 0.6f, 0.6f));
    drawCube(shader, model, glm::vec3(0.52f, 0.32f, 0.15f));
}

void drawBoat(GLuint shader, float time)
{
    float boatBob = std::sin(time * 2.5f) * 0.08f;
    float tilt = boat.velocity * 0.8f;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, boat.position + glm::vec3(0.0f, boatBob, 0.0f));
    model = glm::rotate(model, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(1.4f, 0.45f, 2.5f));
    drawCube(shader, model, glm::vec3(0.45f, 0.18f, 0.12f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, boat.position + glm::vec3(0.0f, 0.42f + boatBob, -0.10f));
    model = glm::rotate(model, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.85f, 0.55f, 1.0f));
    drawCube(shader, model, glm::vec3(0.82f, 0.82f, 0.78f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, boat.position + glm::vec3(0.0f, 1.1f + boatBob, 0.0f));
    model = glm::rotate(model, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.08f, 1.6f, 0.08f));
    drawCube(shader, model, glm::vec3(0.35f, 0.2f, 0.1f));

    int visibleCrates = static_cast<int>(cargoHold.size());
    visibleCrates = std::min(visibleCrates, 6);
    for (int i = 0; i < visibleCrates; ++i)
    {
        float localX = -0.35f + (i % 3) * 0.35f;
        float localZ = 0.35f + (i / 3) * 0.35f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, boat.position + glm::vec3(0.0f, 0.42f + boatBob, 0.0f));
        model = glm::rotate(model, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(localX, 0.15f, localZ));
        model = glm::scale(model, glm::vec3(0.22f, 0.22f, 0.22f));
        drawCube(shader, model, glm::vec3(0.55f, 0.38f, 0.20f));
    }
}

void drawWorldGeometry(GLuint shader, float time, bool includeMarkers)
{
    drawDockStructures(shader);

    drawIslandCluster(shader, glm::vec3(15.0f, 0.0f, 10.0f), glm::vec3(0.34f, 0.56f, 0.26f));
    drawIslandCluster(shader, glm::vec3(-18.0f, 0.0f, -8.0f), glm::vec3(0.28f, 0.52f, 0.24f));
    drawIslandCluster(shader, glm::vec3(25.0f, 0.0f, -22.0f), glm::vec3(0.24f, 0.44f, 0.20f));
    drawIslandCluster(shader, glm::vec3(-10.0f, 0.0f, 18.0f), glm::vec3(0.36f, 0.58f, 0.28f), false);

    if (includeMarkers)
    {
        for (const auto& zone : zones)
        {
            float pulse = 1.0f + std::sin(time * 2.0f + zone.center.x) * 0.08f;
            glm::mat4 buoy = glm::mat4(1.0f);
            buoy = glm::translate(buoy, zone.center + glm::vec3(0.0f, 0.55f, 0.0f));
            buoy = glm::scale(buoy, glm::vec3(0.35f * pulse, 0.9f, 0.35f * pulse));
            drawCube(shader, buoy, zone.colour);

            glm::mat4 pole = glm::mat4(1.0f);
            pole = glm::translate(pole, zone.center + glm::vec3(0.0f, 1.15f, 0.0f));
            pole = glm::scale(pole, glm::vec3(0.06f, 0.7f, 0.06f));
            drawCube(shader, pole, glm::vec3(0.25f, 0.18f, 0.10f));
        }
    }

    drawBoat(shader, time);
}

void renderScene(const glm::mat4& view, const glm::mat4& projection, float time)
{
    glm::vec3 cameraPos = getCameraPosition();
    glm::vec3 lightPos(24.0f, 28.0f, 12.0f);
    glm::mat4 lightProjection = glm::ortho(-65.0f, 65.0f, -65.0f, 65.0f, 1.0f, 90.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(depthShader);
    setMat4(depthShader, "lightSpaceMatrix", lightSpaceMatrix);
    drawWorldGeometry(depthShader, time, false);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    glm::vec3 fogColor = getCurrentFogColor();
    float fogDensity = getCurrentFogDensity();

    glUseProgram(waterShader);
    setMat4(waterShader, "view", view);
    setMat4(waterShader, "projection", projection);
    setMat4(waterShader, "lightSpaceMatrix", lightSpaceMatrix);
    setVec3(waterShader, "lightPos", lightPos);
    setVec3(waterShader, "viewPos", cameraPos);
    setVec3(waterShader, "zoneTint", getZoneWaterTint());
    setVec3(waterShader, "fogColor", fogColor);
    setFloat(waterShader, "fogDensity", fogDensity);
    setFloat(waterShader, "time", time);
    setFloat(waterShader, "waveStrength", 1.0f + worldDanger * 0.55f);
    setFloat(waterShader, "worldDanger", worldDanger);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    setInt(waterShader, "shadowMap", 0);
    glm::mat4 waterModel = glm::mat4(1.0f);
    waterModel = glm::scale(waterModel, glm::vec3(120.0f, 1.0f, 120.0f));
    drawPlane(waterShader, waterModel);

    glUseProgram(basicShader);
    setMat4(basicShader, "view", view);
    setMat4(basicShader, "projection", projection);
    setMat4(basicShader, "lightSpaceMatrix", lightSpaceMatrix);
    setVec3(basicShader, "lightPos", lightPos);
    setVec3(basicShader, "viewPos", cameraPos);
    setVec3(basicShader, "fogColor", fogColor);
    setFloat(basicShader, "fogDensity", fogDensity);
    setFloat(basicShader, "worldDanger", worldDanger);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    setInt(basicShader, "shadowMap", 0);
    drawWorldGeometry(basicShader, time, true);


    renderParticles(view, projection);
}


std::string buildWindowTitle()
{
    std::stringstream title;
    title << "Dredge-style Fishing Prototype"
          << " | Zone: " << getCurrentZoneName()
          << " | Gold: " << totalMoney
          << " | Cargo: " << cargoHold.size() << "/" << cargoCapacity << " (" << getCargoValue() << "g)"
          << " | Rod " << rodLevel
          << " | Engine " << engineLevel
          << " | Dock: ";

    if (isAtDock())
    {
        title << "Sell[R]  Rod[1:" << getRodUpgradeCost() << "g]"
              << "  Engine[2:" << getEngineUpgradeCost() << "g]"
              << "  Cargo[3:" << getCargoUpgradeCost() << "g]";
    }
    else
    {
        title << "Return to dock";
    }

    if (statusMessageTimer > 0.0f)
        title << " | " << lastCatchText;
    else
        title << " | Fish[E]";

    return title.str();
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    if (!glfwInit())
    {
        std::cerr << "Failed to initialise GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Fishing Game CW2 Template", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialise GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    basicShader = createShaderProgram(BASIC_VERT_PATH, BASIC_FRAG_PATH);
    waterShader = createShaderProgram(WATER_VERT_PATH, WATER_FRAG_PATH);
    depthShader = createShaderProgram(DEPTH_VERT_PATH, DEPTH_FRAG_PATH);
    particleShader = createShaderProgram(PARTICLE_VERT_PATH, PARTICLE_FRAG_PATH);

    if (basicShader == 0 || waterShader == 0 || depthShader == 0 || particleShader == 0)
    {
        std::cerr << "One or more shaders failed to load. Check your shader file paths and compile errors above." << std::endl;
        glfwTerminate();
        return -1;
    }

    setupCube();
    setupPlane();
    setupParticleSystem();
    setupShadowMap();
    setupZones();
    applyEngineUpgradeStats();

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        boat.update(deltaTime);

        if (fishCooldown > 0.0f)
            fishCooldown -= deltaTime;
        if (statusMessageTimer > 0.0f)
            statusMessageTimer -= deltaTime;
        if (catchFlashTimer > 0.0f)
            catchFlashTimer -= deltaTime;

        const FishZone* currentZone = getCurrentZone();
        worldDanger = currentZone ? currentZone->danger : 0.0f;
        emitWakeParticles();
        updateParticles(deltaTime);

        if (keys[GLFW_KEY_E] && !fishPressed)
        {
            tryFishing();
            fishPressed = true;
        }
        if (!keys[GLFW_KEY_E])
            fishPressed = false;

        if (keys[GLFW_KEY_R] && !sellPressed)
        {
            sellCargo();
            sellPressed = true;
        }
        if (!keys[GLFW_KEY_R])
            sellPressed = false;

        if (keys[GLFW_KEY_1] && !rodUpgradePressed)
        {
            buyRodUpgrade();
            rodUpgradePressed = true;
        }
        if (!keys[GLFW_KEY_1])
            rodUpgradePressed = false;

        if (keys[GLFW_KEY_2] && !engineUpgradePressed)
        {
            buyEngineUpgrade();
            engineUpgradePressed = true;
        }
        if (!keys[GLFW_KEY_2])
            engineUpgradePressed = false;

        if (keys[GLFW_KEY_3] && !cargoUpgradePressed)
        {
            buyCargoUpgrade();
            cargoUpgradePressed = true;
        }
        if (!keys[GLFW_KEY_3])
            cargoUpgradePressed = false;

        glfwSetWindowTitle(window, buildWindowTitle().c_str());

        glm::vec3 base = glm::mix(getCurrentFogColor(), glm::vec3(0.58f, 0.76f, 0.94f), 0.35f);
        if (catchFlashTimer > 0.0f)
            base = glm::mix(base, glm::vec3(1.0f, 0.95f, 0.7f), glm::clamp(catchFlashTimer * 2.0f, 0.0f, 1.0f));

        glClearColor(base.r, base.g, base.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 cameraPos = getCameraPosition();
        glm::mat4 view = glm::lookAt(cameraPos, boat.position, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 500.0f);

        renderScene(view, projection, currentFrame);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteProgram(basicShader);
    glDeleteProgram(waterShader);
    glDeleteProgram(depthShader);
    glDeleteProgram(particleShader);
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);
    glDeleteFramebuffers(1, &shadowFBO);
    glDeleteTextures(1, &shadowMap);

    glfwTerminate();
    return 0;
}
