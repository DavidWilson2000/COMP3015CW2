#include "PostProcess.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
    std::string ReadTextFile(const std::string& path)
    {
        const std::vector<std::string> candidates = {
            path,
            "../" + path,
            "../../" + path,
            "../../../" + path
        };

        for (const std::string& candidate : candidates)
        {
            std::ifstream file(candidate);
            if (file)
            {
                std::cout << "Loaded post-process shader: " << candidate << std::endl;
                std::stringstream ss;
                ss << file.rdbuf();
                return ss.str();
            }
        }

        std::cerr << "Failed to open post-process shader file: " << path << std::endl;
        return {};
    }
}

GLuint PostProcessor::LoadShaderFromFile(GLenum type, const std::string& path)
{
    std::string source = ReadTextFile(path);
    if (source.empty())
        return 0;

    const char* src = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[2048];
        glGetShaderInfoLog(shader, 2048, nullptr, infoLog);
        std::cerr << "Post-process shader compile error in " << path << ":\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint PostProcessor::LoadProgramFromFiles(const std::string& vertPath, const std::string& fragPath)
{
    GLuint vs = LoadShaderFromFile(GL_VERTEX_SHADER, vertPath);
    GLuint fs = LoadShaderFromFile(GL_FRAGMENT_SHADER, fragPath);
    if (vs == 0 || fs == 0)
        return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[2048];
        glGetProgramInfoLog(program, 2048, nullptr, infoLog);
        std::cerr << "Post-process program link error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void PostProcessor::CreateBuffers()
{
    DestroyBuffers();

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &colorTex_);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex_, 0);

    // This must be a depth texture rather than a renderbuffer because SSAO and
    // depth of field need to sample scene depth in the post-process shader.
    glGenTextures(1, &depthTex_);
    glBindTexture(GL_TEXTURE_2D, depthTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width_, height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex_, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Post-process framebuffer is incomplete." << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::DestroyBuffers()
{
    if (depthTex_ != 0)
    {
        glDeleteTextures(1, &depthTex_);
        depthTex_ = 0;
    }
    if (colorTex_ != 0)
    {
        glDeleteTextures(1, &colorTex_);
        colorTex_ = 0;
    }
    if (fbo_ != 0)
    {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
}

bool PostProcessor::Init(int width, int height, const std::string& vertPath, const std::string& fragPath)
{
    width_ = width;
    height_ = height;
    shader_ = LoadProgramFromFiles(vertPath, fragPath);
    if (shader_ == 0)
        return false;

    const float quadVertices[] = {
        // positions   // texcoords
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVao_);
    glGenBuffers(1, &quadVbo_);
    glBindVertexArray(quadVao_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    CreateBuffers();
    return true;
}

void PostProcessor::Shutdown()
{
    DestroyBuffers();

    if (quadVbo_ != 0)
    {
        glDeleteBuffers(1, &quadVbo_);
        quadVbo_ = 0;
    }
    if (quadVao_ != 0)
    {
        glDeleteVertexArrays(1, &quadVao_);
        quadVao_ = 0;
    }
    if (shader_ != 0)
    {
        glDeleteProgram(shader_);
        shader_ = 0;
    }
}

void PostProcessor::EnsureSize(int width, int height)
{
    if (width == width_ && height == height_)
        return;

    width_ = width;
    height_ = height;
    CreateBuffers();
}

void PostProcessor::BeginScene(const glm::vec3& clearColor)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    glEnable(GL_DEPTH_TEST);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessor::EndScene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::Render(PostProcessMode mode, float time)
{
    Render(
        mode,
        time,
        glm::vec2(0.5f, 0.5f),
        glm::vec3(1.0f, 0.95f, 0.85f),
        0.0f,
        1.0f
    );
}

void PostProcessor::Render(PostProcessMode mode, float time,
    const glm::vec2& sunScreenPos,
    const glm::vec3& sunColor,
    float sunVisibility,
    float dayFactor)
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shader_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    glUniform1i(glGetUniformLocation(shader_, "sceneTex"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTex_);
    glUniform1i(glGetUniformLocation(shader_, "depthTex"), 1);

    glUniform1i(glGetUniformLocation(shader_, "mode"), static_cast<int>(mode));
    glUniform2f(glGetUniformLocation(shader_, "texelSize"),
        1.0f / static_cast<float>(width_),
        1.0f / static_cast<float>(height_));
    glUniform1f(glGetUniformLocation(shader_, "time"), time);
    glUniform2f(glGetUniformLocation(shader_, "sunScreenPos"), sunScreenPos.x, sunScreenPos.y);
    glUniform3f(glGetUniformLocation(shader_, "sunColor"), sunColor.r, sunColor.g, sunColor.b);
    glUniform1f(glGetUniformLocation(shader_, "sunVisibility"), sunVisibility);
    glUniform1f(glGetUniformLocation(shader_, "dayFactor"), dayFactor);

    // Tuned for the current third-person boat camera. The boat/near water stay
    // relatively sharp while distant islands/sky soften in DepthOfField mode.
    glUniform1f(glGetUniformLocation(shader_, "dofFocusDistance"), 9.6f);
    glUniform1f(glGetUniformLocation(shader_, "dofFocusRange"), 4.0f);
    glUniform1f(glGetUniformLocation(shader_, "dofMaxBlurPixels"), 8.0f);

    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
