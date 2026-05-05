#pragma once

#include <glad/glad.h>
#include <cstddef>

void SetupCube(GLuint& vao, GLuint& vbo, const float* vertices, std::size_t byteSize);
void SetupPlane(GLuint& vao, GLuint& vbo, const float* vertices, std::size_t byteSize);
void SetupSkybox(GLuint& vao, GLuint& vbo, const float* vertices, std::size_t byteSize);
void SetupParticleBuffer(GLuint& vao, GLuint& vbo, int maxParticles);
