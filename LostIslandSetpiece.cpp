#include "LostIslandSetpiece.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

std::vector<glm::mat4> BuildLostIslandAltarTransforms(const glm::vec3& goalPos)
{
    std::vector<glm::mat4> pieces;

    // Lower, broader stepped altar so it stays visually grounded above the water.
    glm::mat4 base0(1.0f);
    base0 = glm::translate(base0, goalPos + glm::vec3(0.0f, -0.06f, 0.0f));
    base0 = glm::scale(base0, glm::vec3(3.55f, 0.16f, 3.55f));
    pieces.push_back(base0);

    glm::mat4 base1(1.0f);
    base1 = glm::translate(base1, goalPos + glm::vec3(0.0f, 0.10f, 0.0f));
    base1 = glm::scale(base1, glm::vec3(3.05f, 0.15f, 3.05f));
    pieces.push_back(base1);

    glm::mat4 base2(1.0f);
    base2 = glm::translate(base2, goalPos + glm::vec3(0.0f, 0.26f, 0.0f));
    base2 = glm::scale(base2, glm::vec3(2.55f, 0.14f, 2.55f));
    pieces.push_back(base2);

    glm::mat4 base3(1.0f);
    base3 = glm::translate(base3, goalPos + glm::vec3(0.0f, 0.42f, 0.0f));
    base3 = glm::scale(base3, glm::vec3(2.05f, 0.13f, 2.05f));
    pieces.push_back(base3);

    glm::mat4 ring0(1.0f);
    ring0 = glm::translate(ring0, goalPos + glm::vec3(0.0f, 0.58f, 0.0f));
    ring0 = glm::scale(ring0, glm::vec3(1.62f, 0.12f, 1.62f));
    pieces.push_back(ring0);

    glm::mat4 ring1(1.0f);
    ring1 = glm::translate(ring1, goalPos + glm::vec3(0.0f, 0.74f, 0.0f));
    ring1 = glm::scale(ring1, glm::vec3(1.18f, 0.11f, 1.18f));
    pieces.push_back(ring1);

    glm::mat4 plinthBase(1.0f);
    plinthBase = glm::translate(plinthBase, goalPos + glm::vec3(0.0f, 0.94f, 0.0f));
    plinthBase = glm::scale(plinthBase, glm::vec3(0.82f, 0.15f, 0.82f));
    pieces.push_back(plinthBase);

    glm::mat4 plinth(1.0f);
    plinth = glm::translate(plinth, goalPos + glm::vec3(0.0f, 1.26f, 0.0f));
    plinth = glm::scale(plinth, glm::vec3(0.40f, 0.72f, 0.40f));
    pieces.push_back(plinth);

    // Larger ceremonial fins around the altar.
    for (int i = 0; i < 4; ++i)
    {
        float angle = glm::radians(45.0f + i * 90.0f);
        glm::vec3 offset(std::cos(angle) * 1.72f, 0.68f, std::sin(angle) * 1.72f);

        glm::mat4 finStone(1.0f);
        finStone = glm::translate(finStone, goalPos + offset);
        finStone = glm::rotate(finStone, angle, glm::vec3(0, 1, 0));
        finStone = glm::rotate(finStone, glm::radians(-18.0f), glm::vec3(0, 0, 1));
        finStone = glm::scale(finStone, glm::vec3(1.12f, 0.25f, 0.32f));
        pieces.push_back(finStone);
    }

    // Four monoliths around the centre to make it feel more like a shrine.
    for (int i = 0; i < 4; ++i)
    {
        float angle = glm::radians(i * 90.0f);
        glm::vec3 offset(std::cos(angle) * 1.34f, 1.12f, std::sin(angle) * 1.34f);

        glm::mat4 monolith(1.0f);
        monolith = glm::translate(monolith, goalPos + offset);
        monolith = glm::rotate(monolith, angle, glm::vec3(0, 1, 0));
        monolith = glm::scale(monolith, glm::vec3(0.24f, 0.70f, 0.24f));
        pieces.push_back(monolith);
    }

    // Small outer corner stones to help it read as a larger platform.
    for (int i = 0; i < 4; ++i)
    {
        float angle = glm::radians(45.0f + i * 90.0f);
        glm::vec3 offset(std::cos(angle) * 2.30f, 0.20f, std::sin(angle) * 2.30f);

        glm::mat4 cornerStone(1.0f);
        cornerStone = glm::translate(cornerStone, goalPos + offset);
        cornerStone = glm::rotate(cornerStone, angle, glm::vec3(0, 1, 0));
        cornerStone = glm::scale(cornerStone, glm::vec3(0.48f, 0.18f, 0.48f));
        pieces.push_back(cornerStone);
    }

    return pieces;
}

glm::mat4 BuildLostIslandSwordTransform(const glm::vec3& goalPos, float time)
{
    glm::mat4 sword(1.0f);

    const float glowLift = std::sin(time * 1.15f) * 0.015f;

    sword = glm::translate(sword, goalPos + glm::vec3(0.0f, 1.86f + glowLift, 0.0f));
    sword = glm::rotate(sword, glm::radians(180.0f), glm::vec3(0, 1, 0));
    sword = glm::rotate(sword, glm::radians(-5.0f), glm::vec3(0, 0, 1));
    sword = glm::scale(sword, glm::vec3(0.05f, 0.05f, 0.05f));

    return sword;
}

glm::mat4 BuildLostIslandSwordFallbackTransform(const glm::vec3& goalPos, float time)
{
    glm::mat4 sword(1.0f);

    const float glowLift = std::sin(time * 1.15f) * 0.015f;

    sword = glm::translate(sword, goalPos + glm::vec3(0.0f, 1.80f + glowLift, 0.0f));
    sword = glm::rotate(sword, glm::radians(-5.0f), glm::vec3(0, 0, 1));
    sword = glm::scale(sword, glm::vec3(0.07f, 0.92f, 0.07f));

    return sword;
}
