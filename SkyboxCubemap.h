#pragma once

#include <glad/glad.h>
#include <string>

GLuint LoadSkyboxCubemapOrFallback(const std::string& folderPath);
GLuint CreateFallbackSkyboxCubemap();
