#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

struct FishData
{
    std::string name;
    int rarity = 0; // 0 common, 1 uncommon, 2 rare, 3 legendary
    int value = 0;
};

struct FishZone
{
    glm::vec3 center{0.0f};
    float radius = 0.0f;
    std::vector<FishData> fishPool;
    glm::vec3 tint{0.0f};
    glm::vec3 fogColor{0.0f};
    float fogNear = 0.0f;
    float fogFar = 0.0f;
    float danger = 0.0f;
    std::string zoneName;

    bool contains(const glm::vec3& pos) const
    {
        return glm::length(glm::vec2(pos.x - center.x, pos.z - center.z)) <= radius;
    }
};

enum class WorldMaterial
{
    Rock,
    Grass,
    Wood,
    Light
};

enum class IslandBiome
{
    Harbour,
    Reef,
    Trench,
    Shoals
};

struct WorldRenderPiece
{
    glm::mat4 transform{1.0f};
    WorldMaterial material = WorldMaterial::Rock;
    float texScale = 1.0f;
};

struct WorldIsland
{
    glm::vec3 center{0.0f};
    IslandBiome biome = IslandBiome::Harbour;
    std::string name;
    std::vector<WorldRenderPiece> pieces;
};

struct WorldLayout
{
    std::vector<FishZone> zones;
    std::vector<WorldIsland> islands;
};

WorldLayout BuildDefaultWorldLayout();
