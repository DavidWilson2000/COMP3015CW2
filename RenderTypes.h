#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <cstddef>

// Shared render/model data used by main.cpp and WorldRenderer.cpp.
struct ModelMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    size_t vertexCount = 0;
    bool loaded = false;

    // Bounds are used to auto-scale/ground imported models such as trees.
    glm::vec3 minBounds = glm::vec3(0.0f);
    glm::vec3 maxBounds = glm::vec3(0.0f);
};

enum MaterialType
{
    MAT_WOOD = 0,
    MAT_ROCK = 1,
    MAT_GRASS = 2,
    MAT_BOAT = 3,
    MAT_BUOY = 4,
    MAT_LIGHT = 5
};
