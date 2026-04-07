#include "LostIslandSetpiece.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

std::vector<glm::mat4> BuildLostIslandAltarTransforms(const glm::vec3& goalPos)
{
    std::vector<glm::mat4> pieces;

    // broad ocean altar base
    glm::mat4 tideBase(1.0f);
    tideBase = glm::translate(tideBase, goalPos + glm::vec3(0.0f, 1.86f, 0.0f));
    tideBase = glm::scale(tideBase, glm::vec3(2.15f, 0.16f, 2.15f));
    pieces.push_back(tideBase);

    glm::mat4 lowerRing(1.0f);
    lowerRing = glm::translate(lowerRing, goalPos + glm::vec3(0.0f, 2.08f, 0.0f));
    lowerRing = glm::scale(lowerRing, glm::vec3(1.62f, 0.13f, 1.62f));
    pieces.push_back(lowerRing);

    glm::mat4 upperRing(1.0f);
    upperRing = glm::translate(upperRing, goalPos + glm::vec3(0.0f, 2.28f, 0.0f));
    upperRing = glm::scale(upperRing, glm::vec3(1.08f, 0.11f, 1.08f));
    pieces.push_back(upperRing);

    // central plinth for the sword
    glm::mat4 plinthBase(1.0f);
    plinthBase = glm::translate(plinthBase, goalPos + glm::vec3(0.0f, 2.54f, 0.0f));
    plinthBase = glm::scale(plinthBase, glm::vec3(0.64f, 0.16f, 0.64f));
    pieces.push_back(plinthBase);

    glm::mat4 plinth(1.0f);
    plinth = glm::translate(plinth, goalPos + glm::vec3(0.0f, 2.96f, 0.0f));
    plinth = glm::scale(plinth, glm::vec3(0.34f, 0.70f, 0.34f));
    pieces.push_back(plinth);

    // four ocean-fin stones around the altar
    for (int i = 0; i < 4; ++i)
    {
        float angle = glm::radians(45.0f + i * 90.0f);
        glm::vec3 offset(std::cos(angle) * 1.12f, 2.42f, std::sin(angle) * 1.12f);

        glm::mat4 finStone(1.0f);
        finStone = glm::translate(finStone, goalPos + offset);
        finStone = glm::rotate(finStone, angle, glm::vec3(0, 1, 0));
        finStone = glm::rotate(finStone, glm::radians(-18.0f), glm::vec3(0, 0, 1));
        finStone = glm::scale(finStone, glm::vec3(0.68f, 0.22f, 0.26f));
        pieces.push_back(finStone);
    }

    // small standing monoliths to make the altar feel more ceremonial
    for (int i = 0; i < 4; ++i)
    {
        float angle = glm::radians(i * 90.0f);
        glm::vec3 offset(std::cos(angle) * 0.92f, 2.86f, std::sin(angle) * 0.92f);

        glm::mat4 monolith(1.0f);
        monolith = glm::translate(monolith, goalPos + offset);
        monolith = glm::rotate(monolith, angle, glm::vec3(0, 1, 0));
        monolith = glm::scale(monolith, glm::vec3(0.20f, 0.55f, 0.20f));
        pieces.push_back(monolith);
    }

    return pieces;
}

glm::mat4 BuildLostIslandSwordTransform(const glm::vec3& goalPos, float time)
{
    glm::mat4 sword(1.0f);

    const float glowLift = std::sin(time * 1.15f) * 0.02f;

    sword = glm::translate(sword, goalPos + glm::vec3(0.0f, 3.66f + glowLift, 0.0f));
    sword = glm::rotate(sword, glm::radians(180.0f), glm::vec3(0, 1, 0));
    sword = glm::rotate(sword, glm::radians(-5.0f), glm::vec3(0, 0, 1));
    sword = glm::scale(sword, glm::vec3(0.05f, 0.05f, 0.05f));

    return sword;
}

glm::mat4 BuildLostIslandSwordFallbackTransform(const glm::vec3& goalPos, float time)
{
    glm::mat4 sword(1.0f);

    const float glowLift = std::sin(time * 1.15f) * 0.02f;

    sword = glm::translate(sword, goalPos + glm::vec3(0.0f, 3.60f + glowLift, 0.0f));
    sword = glm::rotate(sword, glm::radians(-5.0f), glm::vec3(0, 0, 1));
    sword = glm::scale(sword, glm::vec3(0.08f, 1.0f, 0.08f));

    return sword;
}
