#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

extern bool keys[1024];

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

enum class DockShopAction
{
    SellAll = 0,
    UpgradeRod,
    UpgradeEngine,
    UpgradeCargo,
    RepairHull
};

constexpr int DOCK_SHOP_ACTION_COUNT = 5;

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

inline float getWaterHeight(float x, float z, float time)
{
    float y = 0.0f;
    y += std::sin(x * 0.30f + time * 1.35f) * 0.05f;
    y += std::cos(z * 0.22f + time * 1.10f) * 0.04f;
    y += std::sin((x + z) * 0.12f + time * 0.80f) * 0.03f;
    return y;
}

inline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

inline float saturatef(float v)
{
    return clampf(v, 0.0f, 1.0f);
}

struct SunState
{
    glm::vec3 direction = glm::normalize(glm::vec3(0.35f, 0.82f, 0.20f));
    glm::vec3 color = glm::vec3(1.0f, 0.96f, 0.90f);
    float dayFactor = 1.0f;
    float intensity = 1.0f;
    float ambient = 0.42f;
};

enum class ShadowFilterMode
{
    PCF = 0,
    PCSS = 1
};

enum class CameraMode
{
    Follow = 0,
    FreeLook = 1
};

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

struct FishIconTexture
{
    GLuint texture = 0;
    int width = 0;
    int height = 0;
    bool attempted = false;
    bool found = false;
};

struct FishJournalEntry
{
    FishData baseFish;
    bool discovered = false;
    int caughtCount = 0;
    int bestValue = 0;
    int highestRarityCaught = 0;
};

struct ShopkeeperContract
{
    std::string fishName;
    int quantity = 1;
    int reward = 0;
    bool completed = false;
};

struct EnvironmentBlendState
{
    glm::vec3 waterTint = glm::vec3(0.0f, 0.10f, 0.16f);
    glm::vec3 fogColor = glm::vec3(0.70f, 0.82f, 0.94f);
    float fogNear = 18.0f;
    float fogFar = 72.0f;
    float danger = 0.1f;
};
