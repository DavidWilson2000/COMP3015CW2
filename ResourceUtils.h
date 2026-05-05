#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "RenderTypes.h"

bool rawFileExists(const std::string& path);
std::vector<std::string> makeResourcePathCandidates(const std::string& path);
bool resolveResourcePath(
    const std::string& requestedPath,
    std::string& resolvedPath,
    const std::string& label,
    bool logSuccess = true,
    bool logFailure = true);
std::string readTextFile(const std::string& path);
GLuint compileShader(GLenum type, const std::string& src);
GLuint createShaderProgramFromFiles(const std::string& vertPath, const std::string& fragPath);

void setMat4(GLuint shader, const char* name, const glm::mat4& mat);
void setVec3(GLuint shader, const char* name, const glm::vec3& vec);
void setVec4(GLuint shader, const char* name, const glm::vec4& vec);
void setFloat(GLuint shader, const char* name, float value);
void setInt(GLuint shader, const char* name, int value);

bool loadOBJModel(const std::string& path, ModelMesh& outModel);
GLuint createTextureFromRGBData(const std::vector<unsigned char>& data, int width, int height);
GLuint createSolidColorTexture(unsigned char r, unsigned char g, unsigned char b);
GLuint loadTextureFromFile(const std::string& path, bool flipVertically = true);
bool fileExists(const std::string& path);

GLuint createWoodTexture();
GLuint createRockTexture();
GLuint createGrassTexture();
GLuint createBoatTexture();
GLuint createPatrolBoatTexture();
GLuint createScrapTexture();
GLuint createBuoyTexture();
GLuint createFallbackCubemap();
