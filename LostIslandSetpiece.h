#pragma once

#include <glm/glm.hpp>
#include <vector>

std::vector<glm::mat4> BuildLostIslandAltarTransforms(const glm::vec3& goalPos);
glm::mat4 BuildLostIslandSwordTransform(const glm::vec3& goalPos, float time);
glm::mat4 BuildLostIslandSwordFallbackTransform(const glm::vec3& goalPos, float time);
