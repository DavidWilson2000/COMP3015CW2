#include "WorldGen.h"

#include <array>
#include <cmath>

namespace
{
    constexpr float kDegToRad = 3.14159265359f / 180.0f;

    glm::mat4 MakeTransform(const glm::vec3& base,
                            const glm::vec3& offset,
                            const glm::vec3& scale,
                            float yawDeg = 0.0f,
                            float pitchDeg = 0.0f,
                            float rollDeg = 0.0f)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, base + offset);
        if (yawDeg != 0.0f)
            model = glm::rotate(model, yawDeg * kDegToRad, glm::vec3(0, 1, 0));
        if (pitchDeg != 0.0f)
            model = glm::rotate(model, pitchDeg * kDegToRad, glm::vec3(1, 0, 0));
        if (rollDeg != 0.0f)
            model = glm::rotate(model, rollDeg * kDegToRad, glm::vec3(0, 0, 1));
        model = glm::scale(model, scale);
        return model;
    }

    void AddPiece(WorldIsland& island,
                  WorldMaterial material,
                  float texScale,
                  const glm::vec3& offset,
                  const glm::vec3& scale,
                  float yawDeg = 0.0f,
                  float pitchDeg = 0.0f,
                  float rollDeg = 0.0f)
    {
        island.pieces.push_back({ MakeTransform(island.center, offset, scale, yawDeg, pitchDeg, rollDeg), material, texScale });
    }

    void AddPalm(WorldIsland& island, const glm::vec3& baseOffset, float leanDeg, float yawDeg, float height)
    {
        AddPiece(island, WorldMaterial::Wood, 1.0f,
                 baseOffset + glm::vec3(0.0f, height * 0.5f, 0.0f),
                 glm::vec3(0.18f, height, 0.18f),
                 yawDeg, 0.0f, leanDeg);

        for (int i = 0; i < 4; ++i)
        {
            const float fanYaw = yawDeg + i * 90.0f + (i % 2 == 0 ? 12.0f : -10.0f);
            AddPiece(island, WorldMaterial::Grass, 1.0f,
                     baseOffset + glm::vec3(0.0f, height + 0.7f, 0.0f),
                     glm::vec3(1.3f, 0.14f, 0.34f),
                     fanYaw, -4.0f, (i % 2 == 0 ? 18.0f : -18.0f));
        }
    }

    void AddReefFan(WorldIsland& island, const glm::vec3& offset, float yawDeg, float height)
    {
        AddPiece(island, WorldMaterial::Grass, 1.0f,
                 offset + glm::vec3(0.0f, height * 0.5f, 0.0f),
                 glm::vec3(0.14f, height, 0.62f),
                 yawDeg, 0.0f, 16.0f);
        AddPiece(island, WorldMaterial::Grass, 1.0f,
                 offset + glm::vec3(0.0f, height * 0.38f, 0.0f),
                 glm::vec3(0.10f, height * 0.75f, 0.52f),
                 yawDeg + 18.0f, 0.0f, -12.0f);
    }

    void AddBasaltSpire(WorldIsland& island, const glm::vec3& offset, float height, float yawDeg)
    {
        AddPiece(island, WorldMaterial::Rock, 2.6f,
                 offset + glm::vec3(0.0f, height * 0.45f, 0.0f),
                 glm::vec3(0.44f, height, 0.44f),
                 yawDeg, 0.0f, 4.0f);
        AddPiece(island, WorldMaterial::Rock, 2.6f,
                 offset + glm::vec3(0.08f, height * 0.78f, -0.06f),
                 glm::vec3(0.24f, height * 0.55f, 0.24f),
                 yawDeg + 26.0f, -6.0f, -10.0f);
    }

    WorldIsland BuildHarbourIsland()
    {
        WorldIsland island;
        island.center = glm::vec3(12.0f, 0.38f, 10.0f);
        island.biome = IslandBiome::Harbour;
        island.name = "Harbour Waters";

        const std::array<glm::vec3, 6> baseOffsets = {
            glm::vec3(0.0f, -0.08f, 0.0f),
            glm::vec3(-1.55f, 0.02f, 0.85f),
            glm::vec3(1.70f, -0.02f, -0.90f),
            glm::vec3(1.10f, 0.05f, 1.55f),
            glm::vec3(-1.10f, -0.04f, -1.50f),
            glm::vec3(0.45f, 0.10f, -1.95f)
        };
        const std::array<glm::vec3, 6> baseScales = {
            glm::vec3(4.5f, 0.55f, 4.2f),
            glm::vec3(2.8f, 0.46f, 2.5f),
            glm::vec3(2.4f, 0.42f, 2.2f),
            glm::vec3(1.9f, 0.38f, 1.8f),
            glm::vec3(1.7f, 0.36f, 1.6f),
            glm::vec3(1.4f, 0.32f, 1.5f)
        };

        for (size_t i = 0; i < baseOffsets.size(); ++i)
        {
            AddPiece(island, WorldMaterial::Rock, 2.9f, baseOffsets[i], baseScales[i], 18.0f + static_cast<float>(i) * 14.0f, 0.0f, (i % 2 == 0 ? 5.0f : -4.0f));
        }

        AddPiece(island, WorldMaterial::Rock, 2.4f, glm::vec3(-0.25f, 0.48f, 0.20f), glm::vec3(2.8f, 0.42f, 2.4f), 24.0f, 0.0f, 3.0f);
        AddPiece(island, WorldMaterial::Rock, 2.4f, glm::vec3(0.95f, 0.70f, -0.50f), glm::vec3(1.5f, 0.36f, 1.3f), 42.0f, 0.0f, -4.0f);
        AddPiece(island, WorldMaterial::Grass, 3.0f, glm::vec3(-0.30f, 0.88f, 0.18f), glm::vec3(3.0f, 0.26f, 2.6f), 16.0f);
        AddPiece(island, WorldMaterial::Grass, 3.0f, glm::vec3(1.05f, 1.04f, -0.25f), glm::vec3(1.2f, 0.18f, 1.0f), 30.0f);
        AddPiece(island, WorldMaterial::Grass, 3.0f, glm::vec3(-1.05f, 0.98f, 0.88f), glm::vec3(0.92f, 0.14f, 0.86f), -12.0f);

        AddPalm(island, glm::vec3(-0.75f, 1.02f, -0.25f), 8.0f, 18.0f, 1.8f);
        AddPalm(island, glm::vec3(1.05f, 1.04f, 0.60f), -9.0f, -26.0f, 1.6f);

        AddPiece(island, WorldMaterial::Wood, 1.6f, glm::vec3(-2.65f, 0.58f, -0.55f), glm::vec3(1.4f, 0.14f, 0.46f), -12.0f);
        for (int i = 0; i < 3; ++i)
        {
            AddPiece(island, WorldMaterial::Wood, 1.0f,
                     glm::vec3(-3.05f + i * 0.42f, 0.05f, -0.55f),
                     glm::vec3(0.14f, 1.0f, 0.14f),
                     0.0f, 0.0f, (i == 1 ? 3.0f : -4.0f));
        }

        AddPiece(island, WorldMaterial::Grass, 1.2f, glm::vec3(0.55f, 1.18f, 1.35f), glm::vec3(0.72f, 0.18f, 0.72f), 10.0f);
        AddPiece(island, WorldMaterial::Grass, 1.2f, glm::vec3(-1.55f, 1.08f, -0.95f), glm::vec3(0.66f, 0.18f, 0.60f), -8.0f);
        return island;
    }

    WorldIsland BuildReefIsland()
    {
        WorldIsland island;
        island.center = glm::vec3(-18.0f, 0.32f, -10.0f);
        island.biome = IslandBiome::Reef;
        island.name = "Reef Edge";

        for (int i = 0; i < 7; ++i)
        {
            const float angle = 35.0f + i * 48.0f;
            const float radius = (i % 2 == 0) ? 2.0f : 3.0f;
            const float rad = angle * kDegToRad;
            AddPiece(island, WorldMaterial::Rock, 2.7f,
                     glm::vec3(std::cos(rad) * radius, -0.06f + 0.05f * (i % 3), std::sin(rad) * (radius * 0.9f)),
                     glm::vec3(2.5f - 0.18f * i, 0.44f, 2.2f - 0.16f * i),
                     angle, 0.0f, (i % 2 == 0 ? 4.0f : -5.0f));
        }

        AddPiece(island, WorldMaterial::Rock, 2.3f, glm::vec3(0.10f, 0.42f, 0.0f), glm::vec3(2.8f, 0.34f, 2.5f), 22.0f);
        AddPiece(island, WorldMaterial::Grass, 3.4f, glm::vec3(0.10f, 0.72f, 0.12f), glm::vec3(2.7f, 0.20f, 2.2f), 16.0f);
        AddPiece(island, WorldMaterial::Grass, 3.2f, glm::vec3(-0.95f, 0.88f, 0.85f), glm::vec3(1.1f, 0.14f, 0.9f), -10.0f);
        AddPiece(island, WorldMaterial::Grass, 3.2f, glm::vec3(1.25f, 0.84f, -0.55f), glm::vec3(1.0f, 0.14f, 0.8f), 18.0f);

        AddReefFan(island, glm::vec3(-1.55f, 0.78f, 1.25f), 18.0f, 0.92f);
        AddReefFan(island, glm::vec3(1.70f, 0.74f, 0.95f), -28.0f, 0.84f);
        AddReefFan(island, glm::vec3(1.20f, 0.74f, -1.35f), 44.0f, 0.96f);
        AddReefFan(island, glm::vec3(-1.80f, 0.72f, -0.75f), -42.0f, 0.76f);

        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(0.0f, 1.10f, -1.45f), glm::vec3(0.42f, 1.1f, 0.42f), 0.0f, 0.0f, -8.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(-0.85f, 1.05f, -1.15f), glm::vec3(0.30f, 0.85f, 0.30f), 22.0f, 0.0f, 9.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(0.95f, 0.96f, 1.25f), glm::vec3(0.26f, 0.74f, 0.26f), -18.0f, 0.0f, -7.0f);

        AddPiece(island, WorldMaterial::Light, 1.0f, glm::vec3(0.05f, 1.22f, 0.18f), glm::vec3(0.18f, 0.18f, 0.18f));
        AddPiece(island, WorldMaterial::Light, 1.0f, glm::vec3(-1.32f, 0.88f, 0.42f), glm::vec3(0.14f, 0.14f, 0.14f));
        return island;
    }

    WorldIsland BuildTrenchIsland()
    {
        WorldIsland island;
        island.center = glm::vec3(23.0f, 0.28f, -23.0f);
        island.biome = IslandBiome::Trench;
        island.name = "Deep Trench";

        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(0.0f, -0.08f, 0.0f), glm::vec3(4.2f, 0.52f, 3.6f), 12.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(-1.75f, -0.04f, 1.15f), glm::vec3(2.4f, 0.42f, 2.0f), -18.0f, 0.0f, 5.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(1.65f, 0.02f, -1.25f), glm::vec3(2.1f, 0.46f, 1.8f), 28.0f, 0.0f, -6.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(0.45f, 0.34f, 0.60f), glm::vec3(2.4f, 0.32f, 1.7f), 40.0f);
        AddPiece(island, WorldMaterial::Rock, 2.6f, glm::vec3(-0.50f, 0.64f, -0.35f), glm::vec3(1.5f, 0.24f, 1.1f), 18.0f);

        AddBasaltSpire(island, glm::vec3(-0.75f, 0.72f, 0.15f), 2.2f, 8.0f);
        AddBasaltSpire(island, glm::vec3(0.88f, 0.78f, -0.32f), 2.8f, 26.0f);
        AddBasaltSpire(island, glm::vec3(-1.35f, 0.58f, -1.10f), 1.9f, -18.0f);
        AddBasaltSpire(island, glm::vec3(1.55f, 0.56f, 0.95f), 1.7f, 38.0f);

        AddPiece(island, WorldMaterial::Wood, 1.0f, glm::vec3(-2.15f, 1.02f, 0.35f), glm::vec3(0.16f, 2.2f, 0.16f), 22.0f, 0.0f, 16.0f);
        AddPiece(island, WorldMaterial::Wood, 1.0f, glm::vec3(-1.86f, 1.78f, 0.35f), glm::vec3(1.0f, 0.10f, 0.10f), 62.0f, 0.0f, -18.0f);

        AddPiece(island, WorldMaterial::Light, 1.0f, glm::vec3(0.65f, 2.22f, -0.22f), glm::vec3(0.18f, 0.42f, 0.18f), 12.0f, 0.0f, -10.0f);
        AddPiece(island, WorldMaterial::Light, 1.0f, glm::vec3(-0.35f, 1.90f, 0.22f), glm::vec3(0.14f, 0.30f, 0.14f), -8.0f, 0.0f, 10.0f);
        AddPiece(island, WorldMaterial::Light, 1.0f, glm::vec3(1.12f, 1.36f, 1.08f), glm::vec3(0.12f, 0.24f, 0.12f), 22.0f, 0.0f, -6.0f);
        return island;
    }

    WorldIsland BuildShoalsIsland()
    {
        WorldIsland island;
        island.center = glm::vec3(-10.0f, 0.30f, 18.0f);
        island.biome = IslandBiome::Shoals;
        island.name = "Shrouded Shoals";

        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(0.0f, -0.06f, 0.0f), glm::vec3(4.0f, 0.46f, 3.2f), -12.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(-1.65f, 0.02f, -0.55f), glm::vec3(2.4f, 0.34f, 1.7f), 18.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(1.85f, -0.01f, 0.95f), glm::vec3(1.8f, 0.32f, 1.5f), -22.0f);
        AddPiece(island, WorldMaterial::Rock, 2.6f, glm::vec3(0.30f, 0.34f, -1.25f), glm::vec3(2.1f, 0.26f, 1.3f), 42.0f);
        AddPiece(island, WorldMaterial::Grass, 3.0f, glm::vec3(-0.15f, 0.70f, -0.10f), glm::vec3(2.8f, 0.18f, 2.0f), 8.0f);
        AddPiece(island, WorldMaterial::Grass, 3.0f, glm::vec3(1.15f, 0.84f, 0.75f), glm::vec3(0.95f, 0.14f, 0.8f), -10.0f);
        AddPiece(island, WorldMaterial::Grass, 3.0f, glm::vec3(-1.40f, 0.82f, -0.20f), glm::vec3(0.92f, 0.14f, 0.72f), 20.0f);

        // Small rock arch to break up the silhouette.
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(1.85f, 0.90f, -1.15f), glm::vec3(0.42f, 1.16f, 0.42f), 0.0f, 0.0f, 3.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(2.55f, 0.84f, -1.05f), glm::vec3(0.38f, 1.02f, 0.38f), 0.0f, 0.0f, -4.0f);
        AddPiece(island, WorldMaterial::Rock, 2.8f, glm::vec3(2.18f, 1.58f, -1.08f), glm::vec3(1.02f, 0.20f, 0.34f), 10.0f, 0.0f, 8.0f);

        AddPalm(island, glm::vec3(-0.65f, 0.88f, 0.40f), 6.0f, 18.0f, 1.5f);
        AddPalm(island, glm::vec3(0.95f, 0.90f, -0.25f), -7.0f, -26.0f, 1.35f);
        return island;
    }
}

WorldLayout BuildDefaultWorldLayout()
{
    WorldLayout layout;

    layout.zones.push_back({ glm::vec3(12.0f, 0.0f, 10.0f), 6.5f,
        { {"Sardine",0,5}, {"Mackerel",0,6}, {"Crab",1,11} },
        glm::vec3(0.18f, 0.55f, 0.35f), glm::vec3(0.72f, 0.82f, 0.92f), 16.0f, 66.0f, 0.2f, "Harbour Waters" });

    layout.zones.push_back({ glm::vec3(-18.0f, 0.0f, -10.0f), 8.5f,
        { {"Snapper",0,8}, {"Lionfish",2,20}, {"Eel",2,25}, {"Grouper",1,13} },
        glm::vec3(0.10f, 0.60f, 0.55f), glm::vec3(0.58f, 0.78f, 0.78f), 13.0f, 53.0f, 0.45f, "Sunken Reef" });

    layout.zones.push_back({ glm::vec3(23.0f, 0.0f, -23.0f), 10.0f,
        { {"Anglerfish",2,30}, {"Oarfish",3,60}, {"Ghost Fish",3,100}, {"Black Cod",1,18} },
        glm::vec3(0.35f, 0.18f, 0.55f), glm::vec3(0.36f, 0.42f, 0.58f), 9.0f, 36.0f, 0.8f, "Abyssal Trench" });

    layout.islands.push_back(BuildHarbourIsland());
    layout.islands.push_back(BuildReefIsland());
    layout.islands.push_back(BuildTrenchIsland());
    layout.islands.push_back(BuildShoalsIsland());
    return layout;
}
