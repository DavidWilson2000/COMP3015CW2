#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "GameTypes.h"
#include "IslandQuest.h"

void SpawnParticle(std::vector<Particle>& particles, const glm::vec3& pos, const glm::vec3& vel, const glm::vec4& color, float life, float size);
void SpawnWakeParticles(std::vector<Particle>& particles, const Boat& boat);
void SpawnSplash(std::vector<Particle>& particles, const glm::vec3& pos, const glm::vec4& color, int count);
void UpdateParticles(std::vector<Particle>& particles, float dt);
void SpawnLostIslandAuraParticles(std::vector<Particle>& particles, IslandQuest& islandQuest);
void SpawnZoneAmbientParticles(std::vector<Particle>& particles, const std::vector<FishZone>& zones, float time);
