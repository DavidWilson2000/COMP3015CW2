#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

enum class PostProcessMode
{
    None = 0,
    Edge = 1,
    Blur = 2,
    NightVision = 3,
    GodRays = 4
};

class PostProcessor
{
public:
    bool Init(int width, int height, const std::string& vertPath, const std::string& fragPath);
    void Shutdown();
    void EnsureSize(int width, int height);
    void BeginScene(const glm::vec3& clearColor);
    void EndScene();

    void Render(PostProcessMode mode, float time);
    void Render(PostProcessMode mode, float time,
        const glm::vec2& sunScreenPos,
        const glm::vec3& sunColor,
        float sunVisibility,
        float dayFactor);

private:
    void CreateBuffers();
    void DestroyBuffers();
    GLuint LoadShaderFromFile(GLenum type, const std::string& path);
    GLuint LoadProgramFromFiles(const std::string& vertPath, const std::string& fragPath);

    int width_ = 0;
    int height_ = 0;
    GLuint fbo_ = 0;
    GLuint colorTex_ = 0;
    GLuint depthRbo_ = 0;
    GLuint quadVao_ = 0;
    GLuint quadVbo_ = 0;
    GLuint shader_ = 0;
};