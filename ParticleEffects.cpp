#include "ParticleEffects.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

void SpawnParticle(std::vector<Particle>& particles, const glm::vec3& pos, const glm::vec3& vel, const glm::vec4& color, float life, float size)
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

void SpawnWakeParticles(std::vector<Particle>& particles, const Boat& boat)
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
        SpawnParticle(particles, pos, vel, glm::vec4(0.86f, 0.93f, 1.0f, 0.85f), 0.8f, 11.0f);
    }
}

void SpawnSplash(std::vector<Particle>& particles, const glm::vec3& pos, const glm::vec4& color, int count)
{
    for (int i = 0; i < count; ++i)
    {
        glm::vec3 vel(
            (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 1.2f,
            0.6f + static_cast<float>(rand() % 100) / 140.0f,
            (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 1.2f);
        SpawnParticle(particles, pos, vel, color, 0.9f, 10.0f + static_cast<float>(rand() % 6));
    }
}

void UpdateParticles(std::vector<Particle>& particles, float dt)
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



void SpawnLostIslandAuraParticles(std::vector<Particle>& particles, IslandQuest& islandQuest)
{
    if (!(islandQuest.IsGoalIslandUnlocked() || islandQuest.HasWon())) return;

    const glm::vec3 goalPos = islandQuest.GetGoalIslandPosition();
    const int particleCount = islandQuest.HasWon() ? 4 : 2;

    for (int i = 0; i < particleCount; ++i)
    {
        const float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 6.28318f;
        const float radius = islandQuest.HasWon() ? (0.45f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 1.15f)
                                                 : (0.30f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.85f);

        glm::vec3 pos = goalPos + glm::vec3(std::cos(angle) * radius, 1.30f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.85f, std::sin(angle) * radius);
        glm::vec3 vel(std::cos(angle) * 0.04f, 0.30f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.20f, std::sin(angle) * 0.04f);
        glm::vec4 color = islandQuest.HasWon() ? glm::vec4(1.0f, 0.84f, 0.36f, 0.82f)
                                                : glm::vec4(0.72f, 0.88f, 1.0f, 0.70f);

        SpawnParticle(particles, pos, vel, color, 0.95f, islandQuest.HasWon() ? 10.0f : 8.0f);
    }

    if (islandQuest.HasWon())
    {
        glm::vec3 beamPos = goalPos + glm::vec3(0.0f, 1.95f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.65f, 0.0f);
        SpawnParticle(particles, beamPos, glm::vec3(0.0f, 0.55f, 0.0f), glm::vec4(1.0f, 0.94f, 0.60f, 0.78f), 0.9f, 13.0f);
    }
}

void SpawnZoneAmbientParticles(std::vector<Particle>& particles, const std::vector<FishZone>& zones, float time)
{
    for (size_t i = 0; i < zones.size(); ++i)
    {
        const FishZone& zone = zones[i];
        float angle = time * (0.6f + 0.18f * static_cast<float>(i)) + static_cast<float>(i) * 1.9f;
        glm::vec3 pos = zone.center + glm::vec3(std::cos(angle) * (0.6f + zone.radius * 0.08f), 1.15f + 0.22f * std::sin(time * 1.8f + static_cast<float>(i)), std::sin(angle) * (0.6f + zone.radius * 0.08f));
        glm::vec3 vel(0.0f, 0.10f + zone.danger * 0.16f, 0.0f);
        glm::vec4 color(zone.tint.r, zone.tint.g, zone.tint.b, 0.60f + zone.danger * 0.20f);
        SpawnParticle(particles, pos, vel, color, 0.65f + zone.danger * 0.35f, 6.0f + zone.danger * 4.0f);
    }
}


