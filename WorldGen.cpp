#include "WorldGen.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

// These are owned by the world generation system and used by main.cpp when rendering.
std::vector<GeneratedMesh> gIslandRockMeshes;
std::vector<GeneratedMesh> gIslandGrassMeshes;

static float islandBoundaryNoise(float angle, float seed)
{
    return 1.0f
        + 0.12f * std::sin(angle * 3.0f + seed * 1.7f)
        + 0.08f * std::cos(angle * 5.0f + seed * 0.6f)
        + 0.05f * std::sin(angle * 9.0f + seed * 2.3f);
}

float islandSurfaceHeight(float x, float z, float radius, float maxHeight, float seed)
{
    const float angle = std::atan2(z, x);
    const float boundary = std::max(0.68f, islandBoundaryNoise(angle, seed));
    const float r = std::sqrt(x * x + z * z) / std::max(radius * boundary, 0.001f);

    if (r > 1.0f)
    {
        // Pull the outer rim slightly below the waterline so the island blends into the sea.
        return -0.28f - (r - 1.0f) * 0.42f;
    }

    float falloff = std::max(0.0f, 1.0f - r);
    float rockyNoise =
        0.13f * std::sin(x * 2.35f + seed * 1.9f) +
        0.10f * std::cos(z * 2.10f + seed * 1.1f) +
        0.06f * std::sin((x + z) * 3.45f + seed * 2.8f);

    // A broad mound with irregular low-poly bumps.
    float h = std::pow(falloff, 1.45f) * maxHeight + rockyNoise * falloff;

    // Softly flatten the upper area so it reads like a small playable island, not a spike.
    const float plateau = maxHeight * 0.72f;
    if (h > plateau)
    {
        h = glm::mix(h, plateau, 0.38f);
    }

    return std::max(h, -0.32f);
}

static void pushGeneratedVertex(std::vector<float>& vertices, const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& uv)
{
    vertices.push_back(pos.x);
    vertices.push_back(pos.y);
    vertices.push_back(pos.z);

    vertices.push_back(normal.x);
    vertices.push_back(normal.y);
    vertices.push_back(normal.z);

    vertices.push_back(uv.x);
    vertices.push_back(uv.y);
}

static void pushGeneratedTriangle(std::vector<float>& vertices,
                           const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                           const glm::vec2& uva, const glm::vec2& uvb, const glm::vec2& uvc)
{
    glm::vec3 normal = glm::cross(b - a, c - a);
    if (glm::length(normal) < 0.0001f)
    {
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    else
    {
        normal = glm::normalize(normal);
    }

    pushGeneratedVertex(vertices, a, normal, uva);
    pushGeneratedVertex(vertices, b, normal, uvb);
    pushGeneratedVertex(vertices, c, normal, uvc);
}

static GeneratedMesh uploadGeneratedMesh(const std::vector<float>& vertices)
{
    GeneratedMesh mesh;
    if (vertices.empty())
    {
        return mesh;
    }

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.vertexCount = vertices.size() / 8;
    mesh.ready = true;
    return mesh;
}

static GeneratedMesh createProceduralIslandRockMesh(int resolution, float radius, float maxHeight, float seed)
{
    std::vector<float> vertices;
    vertices.reserve(static_cast<size_t>(resolution * resolution * 6 * 8));

    auto makePoint = [&](int ix, int iz) -> glm::vec3
    {
        float u = ix / static_cast<float>(resolution);
        float v = iz / static_cast<float>(resolution);

        float x = (u * 2.0f - 1.0f) * radius;
        float z = (v * 2.0f - 1.0f) * radius;
        float y = islandSurfaceHeight(x, z, radius, maxHeight, seed);

        return glm::vec3(x, y, z);
    };

    auto edgeAmount = [&](const glm::vec3& p) -> float
    {
        const float angle = std::atan2(p.z, p.x);
        const float boundary = std::max(0.68f, islandBoundaryNoise(angle, seed));
        return std::sqrt(p.x * p.x + p.z * p.z) / std::max(radius * boundary, 0.001f);
    };

    for (int z = 0; z < resolution; ++z)
    {
        for (int x = 0; x < resolution; ++x)
        {
            glm::vec3 p00 = makePoint(x, z);
            glm::vec3 p10 = makePoint(x + 1, z);
            glm::vec3 p01 = makePoint(x, z + 1);
            glm::vec3 p11 = makePoint(x + 1, z + 1);

            // Skip fully outside cells so the mesh has a round, uneven silhouette instead of a square one.
            if (edgeAmount(p00) > 1.12f && edgeAmount(p10) > 1.12f &&
                edgeAmount(p01) > 1.12f && edgeAmount(p11) > 1.12f)
            {
                continue;
            }

            glm::vec2 uv00(x / static_cast<float>(resolution), z / static_cast<float>(resolution));
            glm::vec2 uv10((x + 1) / static_cast<float>(resolution), z / static_cast<float>(resolution));
            glm::vec2 uv01(x / static_cast<float>(resolution), (z + 1) / static_cast<float>(resolution));
            glm::vec2 uv11((x + 1) / static_cast<float>(resolution), (z + 1) / static_cast<float>(resolution));

            pushGeneratedTriangle(vertices, p00, p11, p10, uv00, uv11, uv10);
            pushGeneratedTriangle(vertices, p00, p01, p11, uv00, uv01, uv11);
        }
    }

    return uploadGeneratedMesh(vertices);
}

static GeneratedMesh createProceduralGrassPatchMesh(int rings, int segments, float islandRadius, float patchRadius, float maxHeight, float seed)
{
    std::vector<float> vertices;
    vertices.reserve(static_cast<size_t>(rings * segments * 6 * 8));

    auto makePoint = [&](int ring, int segment) -> glm::vec3
    {
        float ringT = ring / static_cast<float>(rings);
        float angle = (segment / static_cast<float>(segments)) * 6.2831853f;

        // A smaller, uneven grass patch that follows the rock height instead of
        // cutting through the island as a flat green disk.
        float outline = 1.0f
            + 0.10f * std::sin(angle * 3.0f + seed * 0.7f)
            + 0.06f * std::cos(angle * 6.0f + seed * 1.3f)
            + 0.035f * std::sin(angle * 11.0f + seed * 2.1f);

        float r = patchRadius * ringT * outline;
        float x = std::cos(angle) * r;
        float z = std::sin(angle) * r;

        float surfaceY = islandSurfaceHeight(x, z, islandRadius, maxHeight, seed);
        float softEdge = ringT * ringT;
        float bump = 0.025f * (1.0f - softEdge) * std::sin(angle * 5.0f + seed * 1.8f);

        // Slightly raise the grass above the rock to avoid z-fighting, while
        // dipping the edge so it blends into the stone.
        return glm::vec3(x, surfaceY + 0.045f + bump - softEdge * 0.018f, z);
    };

    auto makeUV = [&](const glm::vec3& p) -> glm::vec2
    {
        return glm::vec2((p.x / patchRadius) * 0.5f + 0.5f, (p.z / patchRadius) * 0.5f + 0.5f);
    };

    for (int ring = 0; ring < rings; ++ring)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            int nextSegment = (segment + 1) % segments;

            glm::vec3 p00 = makePoint(ring, segment);
            glm::vec3 p10 = makePoint(ring + 1, segment);
            glm::vec3 p01 = makePoint(ring, nextSegment);
            glm::vec3 p11 = makePoint(ring + 1, nextSegment);

            pushGeneratedTriangle(vertices, p00, p11, p10, makeUV(p00), makeUV(p11), makeUV(p10));
            pushGeneratedTriangle(vertices, p00, p01, p11, makeUV(p00), makeUV(p01), makeUV(p11));
        }
    }

    return uploadGeneratedMesh(vertices);
}

void setupGeneratedIslandMeshes()
{
    gIslandRockMeshes.clear();
    gIslandGrassMeshes.clear();

    // Each island uses a different seed/shape so the repeated locations do not look copied.
    const float seeds[] = { 1.0f, 2.35f, 4.7f, 7.1f };
    const float radii[] = { 3.3f, 2.9f, 3.8f, 2.7f };
    const float heights[] = { 1.65f, 1.35f, 1.85f, 1.25f };

    for (int i = 0; i < 4; ++i)
    {
        gIslandRockMeshes.push_back(createProceduralIslandRockMesh(34, radii[i], heights[i], seeds[i]));
        gIslandGrassMeshes.push_back(createProceduralGrassPatchMesh(6, 42, radii[i], radii[i] * 0.43f, heights[i], seeds[i]));
    }
}
