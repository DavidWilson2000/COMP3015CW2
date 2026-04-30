#include "WorldRenderer.h"

#include "RenderTypes.h"
#include "WorldGen.h"
#include "IslandQuest.h"
#include "LostIslandSetpiece.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Resources owned/loaded by main.cpp.
extern GLuint woodTex;
extern GLuint rockTex;
extern GLuint grassTex;
extern GLuint boatTex;
extern GLuint buoyTex;
extern GLuint treeTex;
extern GLuint zoneMarkerTex[3];

extern ModelMesh swordModel;
extern ModelMesh treeModel;

extern IslandQuest gIslandQuest;

// Draw/material helpers still live in main.cpp for now.
extern void bindMaterial(GLuint shader, GLuint textureID, int materialType, float tiling);
extern void drawCube(GLuint shader, const glm::mat4& model);
extern void drawGeneratedMesh(GLuint shader, const GeneratedMesh& mesh, const glm::mat4& model);
extern void drawModel(GLuint shader, const ModelMesh& modelMesh, const glm::mat4& model);

void RenderStaticWorldGeometry(GLuint shader, bool depthPass, float time)
{
    auto setMat = [&](GLuint tex, int type, float tiling)
    {
        if (!depthPass) bindMaterial(shader, tex, type, tiling);
    };

    // Clean improved dock setpiece
    // Built from simple cubes, but kept readable: planks, beams, posts, one shed roof and a few props.
    {
        auto drawBox = [&](GLuint tex, int material, float tiling,
                           const glm::vec3& position,
                           const glm::vec3& scale,
                           float yawDegrees = 0.0f,
                           float rollDegrees = 0.0f)
        {
            setMat(tex, material, tiling);
            glm::mat4 box(1.0f);
            box = glm::translate(box, position);
            box = glm::rotate(box, glm::radians(yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
            box = glm::rotate(box, glm::radians(rollDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
            box = glm::scale(box, scale);
            drawCube(shader, box);
        };

        // Main timber platform, lowered slightly into the water so it feels heavier.
        drawBox(woodTex, MAT_WOOD, 3.0f, glm::vec3(0.0f, 0.24f, 0.0f), glm::vec3(5.2f, 0.18f, 3.1f));

        // Individual surface planks with tiny height/rotation variation.
        for (int plank = 0; plank < 8; ++plank)
        {
            float z = -1.30f + plank * 0.37f;
            float y = 0.39f + (plank % 2 == 0 ? 0.012f : 0.0f);
            float yaw = (plank % 3 - 1) * 0.7f;
            drawBox(woodTex, MAT_WOOD, 1.7f,
                    glm::vec3(0.0f, y, z),
                    glm::vec3(5.35f, 0.045f, 0.14f),
                    yaw);
        }

        // Thick outer frame beams.
        drawBox(woodTex, MAT_WOOD, 1.5f, glm::vec3(0.0f, 0.47f, 1.63f),  glm::vec3(5.45f, 0.14f, 0.12f));
        drawBox(woodTex, MAT_WOOD, 1.5f, glm::vec3(0.0f, 0.47f, -1.63f), glm::vec3(5.45f, 0.14f, 0.12f));
        drawBox(woodTex, MAT_WOOD, 1.5f, glm::vec3(2.73f, 0.47f, 0.0f),  glm::vec3(0.12f, 0.14f, 3.25f));
        drawBox(woodTex, MAT_WOOD, 1.5f, glm::vec3(-2.73f, 0.47f, 0.0f), glm::vec3(0.12f, 0.14f, 3.25f));

        // Under-dock support posts reaching below the water.
        const glm::vec3 postPositions[] = {
            glm::vec3(-2.25f, -0.64f, -1.20f), glm::vec3(0.0f, -0.68f, -1.22f), glm::vec3(2.25f, -0.64f, -1.20f),
            glm::vec3(-2.25f, -0.64f,  1.20f), glm::vec3(0.0f, -0.68f,  1.22f), glm::vec3(2.25f, -0.64f,  1.20f)
        };

        for (const glm::vec3& postPos : postPositions)
        {
            drawBox(woodTex, MAT_WOOD, 1.0f, postPos, glm::vec3(0.20f, 2.15f, 0.20f));
        }

        // A cleaner short rail along the back edge only.
        const float railZ = -1.48f;
        for (int rail = 0; rail < 3; ++rail)
        {
            float x = -2.05f + rail * 2.05f;
            drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(x, 0.98f, railZ), glm::vec3(0.12f, 0.95f, 0.12f));
        }

        // Simple horizontal rail. Kept low so it does not clutter the silhouette.
        drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(0.0f, 1.15f, railZ), glm::vec3(4.25f, 0.045f, 0.055f));

        // Small side jetty where the boat can visually line up with the dock.
        drawBox(woodTex, MAT_WOOD, 2.0f, glm::vec3(-3.05f, 0.23f, 0.72f), glm::vec3(1.30f, 0.13f, 0.72f), -8.0f);

        // A simple ladder on the front edge for dock detail.
        drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(2.25f, 0.08f, 1.78f), glm::vec3(0.055f, 0.82f, 0.055f));
        drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(2.55f, 0.08f, 1.78f), glm::vec3(0.055f, 0.82f, 0.055f));
        drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(2.40f, 0.18f, 1.82f), glm::vec3(0.34f, 0.045f, 0.045f));
        drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(2.40f, -0.08f, 1.82f), glm::vec3(0.34f, 0.045f, 0.045f));

        // Clean dock shed with one roof only.
        drawBox(woodTex, MAT_WOOD, 2.0f, glm::vec3(-1.22f, 0.93f, 0.04f), glm::vec3(1.38f, 1.02f, 1.08f));
        drawBox(woodTex, MAT_WOOD, 1.3f, glm::vec3(-1.22f, 0.48f, 0.04f), glm::vec3(1.52f, 0.12f, 1.20f));

        // One broad roof panel with a small overhang. This removes the crossed/double-roof look.
        drawBox(woodTex, MAT_WOOD, 1.55f, glm::vec3(-1.22f, 1.58f, 0.04f), glm::vec3(1.95f, 0.16f, 1.48f));

        // Small trim under the roof to make the shed read clearly without adding a second roof.
        drawBox(woodTex, MAT_WOOD, 1.1f, glm::vec3(-1.22f, 1.43f, -0.70f), glm::vec3(1.70f, 0.10f, 0.08f));
        drawBox(woodTex, MAT_WOOD, 1.1f, glm::vec3(-1.22f, 1.43f, 0.78f), glm::vec3(1.70f, 0.10f, 0.08f));

        // Small dark doorway so the shed does not look like a plain cube.
        drawBox(rockTex, MAT_ROCK, 1.0f, glm::vec3(-0.50f, 0.77f, -0.55f), glm::vec3(0.34f, 0.52f, 0.035f));

        // Cargo props grouped naturally. Lantern removed for a cleaner dock.
        drawBox(woodTex, MAT_WOOD, 1.3f, glm::vec3(1.46f, 0.58f, -0.40f), glm::vec3(0.26f, 0.26f, 0.26f), 8.0f);
        drawBox(woodTex, MAT_WOOD, 1.3f, glm::vec3(1.77f, 0.55f, -0.24f), glm::vec3(0.22f, 0.22f, 0.22f), -13.0f);
        drawBox(woodTex, MAT_WOOD, 1.3f, glm::vec3(1.63f, 0.83f, -0.30f), glm::vec3(0.18f, 0.18f, 0.18f), 21.0f);

        // Barrel-like stacked blocks beside the shed.
        drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(-2.18f, 0.61f, 0.88f), glm::vec3(0.22f, 0.34f, 0.22f), 18.0f);
        drawBox(woodTex, MAT_WOOD, 1.0f, glm::vec3(-2.46f, 0.58f, 0.72f), glm::vec3(0.19f, 0.30f, 0.19f), -11.0f);
    }

    // Procedural mini islands
    // These replace the old stacked-cube islands with generated low-poly rock mounds.
    // The grass is now a smaller surface-following patch instead of a flat disk, and
    // tree bases are placed using the same height function as the island mesh.
    std::vector<glm::vec3> islandPositions = {
        glm::vec3(12.0f, 0.12f, 10.0f),
        glm::vec3(-18.0f, 0.12f, -10.0f),
        glm::vec3(23.0f, 0.12f, -23.0f),
        glm::vec3(-10.0f, 0.12f, 18.0f)
    };

    const float islandSeeds[] = { 1.0f, 2.35f, 4.7f, 7.1f };
    const float islandRadii[] = { 3.3f, 2.9f, 3.8f, 2.7f };
    const float islandHeights[] = { 1.65f, 1.35f, 1.85f, 1.25f };

    for (size_t i = 0; i < islandPositions.size(); ++i)
    {
        glm::vec3 pos = islandPositions[i];
        float rotation = glm::radians(35.0f * static_cast<float>(i));
        float islandScale = 1.0f + 0.08f * std::sin(static_cast<float>(i) * 1.73f);
        float seed = islandSeeds[i % 4];
        float islandRadius = islandRadii[i % 4];
        float islandHeight = islandHeights[i % 4];

        glm::mat4 islandModel(1.0f);
        islandModel = glm::translate(islandModel, pos);
        islandModel = glm::rotate(islandModel, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        islandModel = glm::scale(islandModel, glm::vec3(islandScale, 1.0f, islandScale));

        setMat(rockTex, MAT_ROCK, 3.0f);
        if (!gIslandRockMeshes.empty())
        {
            drawGeneratedMesh(shader, gIslandRockMeshes[i % gIslandRockMeshes.size()], islandModel);
        }

        setMat(grassTex, MAT_GRASS, 2.1f);
        if (!gIslandGrassMeshes.empty())
        {
            drawGeneratedMesh(shader, gIslandGrassMeshes[i % gIslandGrassMeshes.size()], islandModel);
        }

        auto localToWorld = [&](float localX, float localZ, float extraY = 0.0f) -> glm::vec3
        {
            glm::vec4 world = islandModel * glm::vec4(localX, 0.0f, localZ, 1.0f);
            float groundY = pos.y + islandSurfaceHeight(localX, localZ, islandRadius, islandHeight, seed) + extraY;
            return glm::vec3(world.x, groundY, world.z);
        };

        // Small grass tufts hide hard edges and make the top feel less like one flat texture patch.
        setMat(grassTex, MAT_GRASS, 1.2f);
        for (int tuft = 0; tuft < 9; ++tuft)
        {
            float angle = static_cast<float>(tuft) * 0.91f + seed;
            float dist = 0.35f + 0.85f * (static_cast<float>((tuft * 37) % 100) / 100.0f);
            float localX = std::cos(angle) * dist;
            float localZ = std::sin(angle) * dist * 0.82f;
            glm::vec3 tuftPos = localToWorld(localX, localZ, 0.09f);

            glm::mat4 blade(1.0f);
            blade = glm::translate(blade, tuftPos);
            blade = glm::rotate(blade, rotation + angle, glm::vec3(0.0f, 1.0f, 0.0f));
            blade = glm::rotate(blade, glm::radians(7.0f + tuft * 3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            blade = glm::scale(blade, glm::vec3(0.045f, 0.20f + 0.025f * (tuft % 3), 0.035f));
            drawCube(shader, blade);
        }

        // Imported tree model.
        // The tree is automatically centred and grounded using the OBJ bounds calculated in loadOBJModel().
        // If tree.obj is missing, the old cube-palm fallback is still drawn so the scene remains usable.
        if (treeModel.loaded)
        {
            GLuint activeTreeTexture = treeTex != 0 ? treeTex : grassTex;
            setMat(activeTreeTexture, MAT_WOOD, 1.0f);

            auto buildGroundedTreeTransform = [&](const glm::vec3& basePosition,
                                                  float yawRadians,
                                                  float desiredHeight,
                                                  float extraScale = 1.0f) -> glm::mat4
            {
                glm::vec3 boundsSize = treeModel.maxBounds - treeModel.minBounds;
                float sourceHeight = std::max(boundsSize.y, 0.001f);
                float scaleAmount = (desiredHeight / sourceHeight) * extraScale;

                float centreX = (treeModel.minBounds.x + treeModel.maxBounds.x) * 0.5f;
                float centreZ = (treeModel.minBounds.z + treeModel.maxBounds.z) * 0.5f;

                glm::mat4 treeMatrix(1.0f);
                treeMatrix = glm::translate(treeMatrix, basePosition);
                treeMatrix = glm::rotate(treeMatrix, yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
                treeMatrix = glm::scale(treeMatrix, glm::vec3(scaleAmount));
                treeMatrix = glm::translate(treeMatrix, glm::vec3(-centreX, -treeModel.minBounds.y, -centreZ));
                return treeMatrix;
            };

            struct TreeSpot
            {
                float x;
                float z;
                float height;
                float yawOffset;
                float scale;
            };

            const TreeSpot treeSpots[] = {
                { -0.62f,  0.40f, 1.75f, -18.0f, 1.00f },
                {  0.64f, -0.34f, 1.55f,  24.0f, 0.88f },
                {  0.10f,  0.70f, 1.35f,  67.0f, 0.78f }
            };

            for (const TreeSpot& spot : treeSpots)
            {
                glm::vec3 treeBase = localToWorld(spot.x, spot.z, 0.035f);
                glm::mat4 treeMatrix = buildGroundedTreeTransform(
                    treeBase,
                    rotation + glm::radians(spot.yawOffset + static_cast<float>(i) * 19.0f),
                    spot.height,
                    spot.scale
                );

                drawModel(shader, treeModel, treeMatrix);
            }
        }
        else
        {
            // Fallback cube palms in case the OBJ is not included in the submitted repo.
            setMat(woodTex, MAT_WOOD, 1.0f);
            for (int trunk = 0; trunk < 2; ++trunk)
            {
                float xoff = trunk == 0 ? -0.56f : 0.62f;
                float zoff = trunk == 0 ? 0.42f : -0.34f;
                float lean = trunk == 0 ? 7.0f : -8.5f;
                float trunkHeight = trunk == 0 ? 1.42f : 1.26f;
                float trunkWidth = trunk == 0 ? 0.13f : 0.115f;

                glm::vec3 treeBase = localToWorld(xoff, zoff, 0.03f);
                glm::vec3 trunkCentre = treeBase + glm::vec3(0.0f, trunkHeight * 0.5f, 0.0f);

                glm::mat4 t(1.0f);
                t = glm::translate(t, trunkCentre);
                t = glm::rotate(t, rotation + glm::radians(trunk == 0 ? -6.0f : 14.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                t = glm::rotate(t, glm::radians(lean), glm::vec3(0.0f, 0.0f, 1.0f));
                t = glm::scale(t, glm::vec3(trunkWidth, trunkHeight, trunkWidth));
                drawCube(shader, t);

                glm::vec3 leafCentre = treeBase + glm::vec3(0.0f, trunkHeight + 0.04f, 0.0f);
                setMat(grassTex, MAT_GRASS, 1.0f);
                for (int leafIndex = 0; leafIndex < 6; ++leafIndex)
                {
                    float leafYaw = rotation + glm::radians(leafIndex * 60.0f + trunk * 17.0f);
                    float leafLength = 0.58f + 0.08f * static_cast<float>(leafIndex % 2);

                    glm::mat4 leaf(1.0f);
                    leaf = glm::translate(leaf, leafCentre);
                    leaf = glm::rotate(leaf, leafYaw, glm::vec3(0.0f, 1.0f, 0.0f));
                    leaf = glm::rotate(leaf, glm::radians(-13.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                    leaf = glm::translate(leaf, glm::vec3(leafLength * 0.38f, -0.035f, 0.0f));
                    leaf = glm::scale(leaf, glm::vec3(leafLength, 0.045f, 0.13f));
                    drawCube(shader, leaf);
                }

                glm::mat4 crown(1.0f);
                crown = glm::translate(crown, leafCentre + glm::vec3(0.0f, -0.025f, 0.0f));
                crown = glm::rotate(crown, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
                crown = glm::scale(crown, glm::vec3(0.16f, 0.10f, 0.16f));
                drawCube(shader, crown);

                setMat(woodTex, MAT_WOOD, 1.0f);
            }
        }

        // Clean island indicators.
        // Kept intentionally simple so they read from far away without looking like chunky blocks:
        // 0 Harbour Waters = small square harbour sign
        // 1 Reef Edge      = slim V-fin marker
        // 2 Deep Trench    = tall narrow pennant / spike
        if (i < 3)
        {
            GLuint markerTex = zoneMarkerTex[i] != 0 ? zoneMarkerTex[i] : buoyTex;

            float markerLocalX = 0.0f;
            float markerLocalZ = -islandRadius * 0.56f;
            glm::vec3 markerBase = localToWorld(markerLocalX, markerLocalZ, 0.06f);

            setMat(woodTex, MAT_WOOD, 1.0f);

            // Shared slim post.
            glm::mat4 post(1.0f);
            post = glm::translate(post, markerBase + glm::vec3(0.0f, 0.62f, 0.0f));
            post = glm::rotate(post, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
            post = glm::scale(post, glm::vec3(0.06f, 1.24f, 0.06f));
            drawCube(shader, post);

            glm::vec3 iconCentre = markerBase + glm::vec3(0.0f, 1.34f, 0.0f);
            setMat(markerTex, MAT_LIGHT, 1.0f);

            if (i == 0)
            {
                // Harbour Waters: a clean flat signboard.
                glm::mat4 sign(1.0f);
                sign = glm::translate(sign, iconCentre + glm::vec3(0.0f, 0.02f, 0.0f));
                sign = glm::rotate(sign, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
                sign = glm::scale(sign, glm::vec3(0.28f, 0.22f, 0.05f));
                drawCube(shader, sign);

                glm::mat4 trim(1.0f);
                trim = glm::translate(trim, iconCentre + glm::vec3(0.0f, -0.16f, 0.0f));
                trim = glm::rotate(trim, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
                trim = glm::scale(trim, glm::vec3(0.18f, 0.035f, 0.035f));
                drawCube(shader, trim);
            }
            else if (i == 1)
            {
                // Reef Edge: slim twin fins in a V shape.
                for (int fin = 0; fin < 2; ++fin)
                {
                    glm::mat4 reefFin(1.0f);
                    reefFin = glm::translate(reefFin, iconCentre + glm::vec3(fin == 0 ? -0.08f : 0.08f, 0.04f, 0.0f));
                    reefFin = glm::rotate(reefFin, rotation + glm::radians(fin == 0 ? -18.0f : 18.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    reefFin = glm::rotate(reefFin, glm::radians(fin == 0 ? -10.0f : 10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                    reefFin = glm::scale(reefFin, glm::vec3(0.07f, 0.42f, 0.09f));
                    drawCube(shader, reefFin);
                }
            }
            else
            {
                // Deep Trench: narrow tall marker with a tiny crossbar.
                glm::mat4 spike(1.0f);
                spike = glm::translate(spike, iconCentre + glm::vec3(0.0f, 0.12f, 0.0f));
                spike = glm::rotate(spike, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
                spike = glm::scale(spike, glm::vec3(0.08f, 0.62f, 0.08f));
                drawCube(shader, spike);

                glm::mat4 crossbar(1.0f);
                crossbar = glm::translate(crossbar, iconCentre + glm::vec3(0.0f, 0.28f, 0.0f));
                crossbar = glm::rotate(crossbar, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
                crossbar = glm::scale(crossbar, glm::vec3(0.18f, 0.03f, 0.03f));
                drawCube(shader, crossbar);
            }
        }
    }

    // Lost Island altar with sword
    if (gIslandQuest.IsGoalIslandUnlocked() || gIslandQuest.HasWon())
    {
        glm::vec3 goalPos = gIslandQuest.GetGoalIslandPosition();

        std::vector<glm::mat4> altarPieces = BuildLostIslandAltarTransforms(goalPos);
        setMat(rockTex, MAT_ROCK, 1.9f);
        for (const auto& piece : altarPieces)
        {
            drawCube(shader, piece);
        }

        if (swordModel.loaded)
        {
            setMat(boatTex, MAT_LIGHT, 1.0f);
            glm::mat4 swordModelMatrix = BuildLostIslandSwordTransform(goalPos, time);
            drawModel(shader, swordModel, swordModelMatrix);
        }
        else
        {
            setMat(boatTex, MAT_LIGHT, 1.0f);
            glm::mat4 swordFallback = BuildLostIslandSwordFallbackTransform(goalPos, time);
            drawCube(shader, swordFallback);
        }
    }

    // Old zone buoys removed.
    // They were drawn at the exact fishing-zone centres, which are also the island centres,
    // so they appeared as unexplained blocky objects on top of the islands.
    // The island identifier markers are now drawn inside the procedural island loop above.
}
