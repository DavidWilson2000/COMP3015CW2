#pragma once

#include <glad/glad.h>

// Draws static world set dressing: dock, generated islands, tree models,
// clean island markers, and the Lost Island altar/sword.
// The boat is still rendered from main.cpp because it depends on boat/cargo gameplay state.
void RenderStaticWorldGeometry(GLuint shader, bool depthPass, float time);
