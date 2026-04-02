#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cmath>

// ------------------------------------------------------------
// Window settings
// ------------------------------------------------------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ------------------------------------------------------------
// Input
// ------------------------------------------------------------
bool keys[1024] = { false };
bool fishPressed = false;

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

    bool contains(const glm::vec3& pos) const
    {
        float distance = glm::length(glm::vec2(pos.x - center.x, pos.z - center.z));
        return distance <= radius;
    }

    FishData catchFish() const
    {
        int roll = rand() % 100;

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

        if (roll < 55 && !commons.empty())
            return commons[rand() % commons.size()];
        else if (roll < 80 && !uncommons.empty())
            return uncommons[rand() % uncommons.size()];
        else if (roll < 95 && !rares.empty())
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

        float speedFactor = glm::clamp(std::abs(velocity) / maxForwardSpeed, 0.2f, 1.0f);
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

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
Boat boat;
std::vector<FishZone> zones;
std::string lastCatchText = "No fish caught yet";
int totalMoney = 0;
float fishCooldown = 0.0f;
float catchFlashTimer = 0.0f;

// ------------------------------------------------------------
// Shader helpers
// ------------------------------------------------------------
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram(const char* vertexSrc, const char* fragmentSrc)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

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
        std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// ------------------------------------------------------------
// Shaders
// ------------------------------------------------------------
const char* basicVertexShader = R"(
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* basicFragmentShader = R"(
#version 460 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vec3 ambient = 0.25 * objectColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor;

    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}
)";

const char* waterVertexShader = R"(
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

out vec3 FragPos;
out float waveHeight;

void main()
{
    vec3 pos = aPos;

    pos.y += sin(pos.x * 0.35 + time * 1.5) * 0.12;
    pos.y += cos(pos.z * 0.28 + time * 1.2) * 0.10;

    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = vec3(worldPos);
    waveHeight = pos.y;

    gl_Position = projection * view * worldPos;
}
)";

const char* waterFragmentShader = R"(
#version 460 core

in vec3 FragPos;
in float waveHeight;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 zoneTint;

void main()
{
    vec3 deepColor = vec3(0.02, 0.18, 0.35);
    vec3 shallowColor = vec3(0.05, 0.42, 0.62);

    float t = clamp((waveHeight + 0.2) * 1.8, 0.0, 1.0);
    vec3 baseColor = mix(deepColor, shallowColor, t);

    vec3 color = mix(baseColor, zoneTint, 0.25);

    float dist = length(viewPos - FragPos);
    float fog = clamp(dist / 120.0, 0.0, 1.0);
    vec3 fogColor = vec3(0.6, 0.8, 1.0);

    vec3 finalColor = mix(color, fogColor, fog);

    FragColor = vec4(finalColor, 1.0);
}
)";

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

void drawCube(GLuint shader, const glm::mat4& model, const glm::vec3& colour)
{
    glUseProgram(shader);
    setMat4(shader, "model", model);
    setVec3(shader, "objectColor", colour);

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

    zones.push_back({ glm::vec3(15.0f, 0.0f, 10.0f), 5.5f, harbourFish, glm::vec3(0.2f, 0.8f, 0.3f), "Harbour Waters" });
    zones.push_back({ glm::vec3(-18.0f, 0.0f, -8.0f), 6.5f, reefFish, glm::vec3(0.9f, 0.7f, 0.2f), "Reef Edge" });
    zones.push_back({ glm::vec3(25.0f, 0.0f, -22.0f), 7.5f, deepSeaFish, glm::vec3(0.7f, 0.2f, 0.9f), "Deep Trench" });
}

std::string getCurrentZoneName()
{
    for (const auto& zone : zones)
    {
        if (zone.contains(boat.position))
            return zone.zoneName;
    }
    return "Open Water";
}

glm::vec3 getZoneWaterTint()
{
    for (const auto& zone : zones)
    {
        if (zone.contains(boat.position))
            return zone.colour;
    }
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

void tryFishing(GLFWwindow* window)
{
    if (fishCooldown > 0.0f)
        return;

    fishCooldown = 1.0f;

    for (const auto& zone : zones)
    {
        if (zone.contains(boat.position))
        {
            FishData fish = zone.catchFish();
            totalMoney += fish.value;

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
                << " | Gold: " << totalMoney;

            lastCatchText = ss.str();
            std::cout << lastCatchText << std::endl;
            glfwSetWindowTitle(window, lastCatchText.c_str());
            return;
        }
    }

    lastCatchText = "No fish here";
    std::cout << lastCatchText << std::endl;
    glfwSetWindowTitle(window, lastCatchText.c_str());
}

glm::vec3 getCameraPosition()
{
    float radians = glm::radians(boat.rotationY);
    glm::vec3 behind(-std::sin(radians), 0.0f, -std::cos(radians));

    return boat.position + behind * 8.0f + glm::vec3(0.0f, 5.0f, 0.0f);
}

void renderScene(const glm::mat4& view, const glm::mat4& projection, float time)
{
    glm::vec3 lightPos(15.0f, 20.0f, 10.0f);
    glm::vec3 cameraPos = getCameraPosition();
    float boatBob = std::sin(time * 2.5f) * 0.08f;

    // Water
    glUseProgram(waterShader);
    setMat4(waterShader, "view", view);
    setMat4(waterShader, "projection", projection);
    setVec3(waterShader, "lightPos", lightPos);
    setVec3(waterShader, "viewPos", cameraPos);
    setVec3(waterShader, "zoneTint", getZoneWaterTint());
    setFloat(waterShader, "time", time);

    glm::mat4 waterModel = glm::mat4(1.0f);
    waterModel = glm::scale(waterModel, glm::vec3(120.0f, 1.0f, 120.0f));
    drawPlane(waterShader, waterModel);

    // Basic shader scene setup
    glUseProgram(basicShader);
    setMat4(basicShader, "view", view);
    setMat4(basicShader, "projection", projection);
    setVec3(basicShader, "lightPos", lightPos);
    setVec3(basicShader, "viewPos", cameraPos);

    // Dock
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.1f, 0.0f));
        model = glm::scale(model, glm::vec3(5.0f, 0.2f, 3.0f));
        drawCube(basicShader, model, glm::vec3(0.45f, 0.28f, 0.12f));
    }

    // Islands
    {
        std::vector<glm::vec3> islandPositions = {
            glm::vec3(15.0f, 0.4f, 10.0f),
            glm::vec3(-18.0f, 0.4f, -8.0f),
            glm::vec3(25.0f, 0.4f, -22.0f),
            glm::vec3(-10.0f, 0.4f, 18.0f)
        };

        for (const auto& pos : islandPositions)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::scale(model, glm::vec3(4.0f, 1.2f, 4.0f));
            drawCube(basicShader, model, glm::vec3(0.35f, 0.55f, 0.25f));
        }
    }

    // Fishing zone markers
    for (const auto& zone : zones)
    {
        glm::mat4 model = glm::mat4(1.0f);
        float pulse = 1.0f + std::sin(time * 2.0f + zone.center.x) * 0.1f;
        model = glm::translate(model, zone.center + glm::vec3(0.0f, 0.8f, 0.0f));
        model = glm::scale(model, glm::vec3(zone.radius * 0.35f * pulse, 1.6f, zone.radius * 0.35f * pulse));
        drawCube(basicShader, model, zone.colour);
    }

    float tilt = boat.velocity * 0.8f;

    // Boat body
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, boat.position + glm::vec3(0.0f, boatBob, 0.0f));
        model = glm::rotate(model, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.2f, 0.4f, 2.2f));
        drawCube(basicShader, model, glm::vec3(0.7f, 0.15f, 0.1f));
    }

    // Boat cabin
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, boat.position + glm::vec3(0.0f, 0.45f + boatBob, -0.1f));
        model = glm::rotate(model, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.75f, 0.5f, 0.9f));
        drawCube(basicShader, model, glm::vec3(0.9f, 0.9f, 0.85f));
    }

    // Boat mast
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, boat.position + glm::vec3(0.0f, 1.1f + boatBob, 0.0f));
        model = glm::rotate(model, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.08f, 1.6f, 0.08f));
        drawCube(basicShader, model, glm::vec3(0.35f, 0.2f, 0.1f));
    }

    // Wake effect
    if (std::abs(boat.velocity) > 0.5f)
    {
        float radians = glm::radians(boat.rotationY);
        glm::vec3 forward(std::sin(radians), 0.0f, std::cos(radians));
        glm::vec3 wakePos = boat.position - forward * 1.8f + glm::vec3(0.0f, -0.05f, 0.0f);

        glm::mat4 wake = glm::mat4(1.0f);
        wake = glm::translate(wake, wakePos);
        wake = glm::rotate(wake, glm::radians(boat.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        wake = glm::scale(wake, glm::vec3(1.0f, 0.03f, 2.5f + std::abs(boat.velocity) * 0.2f));

        drawCube(basicShader, wake, glm::vec3(0.8f, 0.9f, 1.0f));
    }
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

    basicShader = createShaderProgram(basicVertexShader, basicFragmentShader);
    waterShader = createShaderProgram(waterVertexShader, waterFragmentShader);

    setupCube();
    setupPlane();
    setupZones();

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

        if (catchFlashTimer > 0.0f)
            catchFlashTimer -= deltaTime;

        if (keys[GLFW_KEY_E] && !fishPressed)
        {
            tryFishing(window);
            fishPressed = true;
        }

        if (!keys[GLFW_KEY_E])
            fishPressed = false;

        std::stringstream title;
        title << "Fishing Game CW2 | Zone: " << getCurrentZoneName()
            << " | Gold: " << totalMoney
            << " | Speed: " << static_cast<int>(boat.velocity * 10.0f) / 10.0f
            << " | Press E to Fish";
        glfwSetWindowTitle(window, title.str().c_str());

        glm::vec3 base = glm::vec3(0.52f, 0.78f, 0.95f);
        if (catchFlashTimer > 0.0f)
            base = glm::mix(base, glm::vec3(1.0f, 0.95f, 0.7f), glm::clamp(catchFlashTimer * 2.0f, 0.0f, 1.0f));

        glClearColor(base.r, base.g, base.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 cameraPos = getCameraPosition();
        glm::mat4 view = glm::lookAt(cameraPos, boat.position, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);

        renderScene(view, projection, currentFrame);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteProgram(basicShader);
    glDeleteProgram(waterShader);

    glfwTerminate();
    return 0;
}