#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

// Small generated mesh used for procedural islands and grass caps.
// Vertex layout matches the main shader layout:
// location 0 = vec3 position
// location 1 = vec3 normal
// location 2 = vec2 texCoord
struct GeneratedMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    size_t vertexCount = 0;
    bool ready = false;
};

extern std::vector<GeneratedMesh> gIslandRockMeshes;
extern std::vector<GeneratedMesh> gIslandGrassMeshes;

// Used by main.cpp to place trees, grass tufts and island markers onto the island surface.
float islandSurfaceHeight(float x, float z, float radius, float maxHeight, float seed);

// Builds the procedural island meshes once after the OpenGL context has been created.
void setupGeneratedIslandMeshes();
