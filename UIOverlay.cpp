#include "UIOverlay.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    struct UIVertex
    {
        float x, y;
        float r, g, b, a;
    };

    GLuint gProgram = 0;
    GLuint gVao = 0;
    GLuint gVbo = 0;
    GLint gProjLoc = -1;

    const std::unordered_map<char, std::array<const char*, 7>> kFont = {
        {'A', {"01110","10001","10001","11111","10001","10001","10001"}},
        {'B', {"11110","10001","10001","11110","10001","10001","11110"}},
        {'C', {"01111","10000","10000","10000","10000","10000","01111"}},
        {'D', {"11110","10001","10001","10001","10001","10001","11110"}},
        {'E', {"11111","10000","10000","11110","10000","10000","11111"}},
        {'F', {"11111","10000","10000","11110","10000","10000","10000"}},
        {'G', {"01111","10000","10000","10111","10001","10001","01110"}},
        {'H', {"10001","10001","10001","11111","10001","10001","10001"}},
        {'I', {"11111","00100","00100","00100","00100","00100","11111"}},
        {'J', {"11111","00010","00010","00010","00010","10010","01100"}},
        {'K', {"10001","10010","10100","11000","10100","10010","10001"}},
        {'L', {"10000","10000","10000","10000","10000","10000","11111"}},
        {'M', {"10001","11011","10101","10101","10001","10001","10001"}},
        {'N', {"10001","11001","10101","10011","10001","10001","10001"}},
        {'O', {"01110","10001","10001","10001","10001","10001","01110"}},
        {'P', {"11110","10001","10001","11110","10000","10000","10000"}},
        {'Q', {"01110","10001","10001","10001","10101","10010","01101"}},
        {'R', {"11110","10001","10001","11110","10100","10010","10001"}},
        {'S', {"01111","10000","10000","01110","00001","00001","11110"}},
        {'T', {"11111","00100","00100","00100","00100","00100","00100"}},
        {'U', {"10001","10001","10001","10001","10001","10001","01110"}},
        {'V', {"10001","10001","10001","10001","10001","01010","00100"}},
        {'W', {"10001","10001","10001","10101","10101","10101","01010"}},
        {'X', {"10001","10001","01010","00100","01010","10001","10001"}},
        {'Y', {"10001","10001","01010","00100","00100","00100","00100"}},
        {'Z', {"11111","00001","00010","00100","01000","10000","11111"}},
        {'0', {"01110","10001","10011","10101","11001","10001","01110"}},
        {'1', {"00100","01100","00100","00100","00100","00100","01110"}},
        {'2', {"01110","10001","00001","00010","00100","01000","11111"}},
        {'3', {"11110","00001","00001","01110","00001","00001","11110"}},
        {'4', {"00010","00110","01010","10010","11111","00010","00010"}},
        {'5', {"11111","10000","10000","11110","00001","00001","11110"}},
        {'6', {"01110","10000","10000","11110","10001","10001","01110"}},
        {'7', {"11111","00001","00010","00100","01000","01000","01000"}},
        {'8', {"01110","10001","10001","01110","10001","10001","01110"}},
        {'9', {"01110","10001","10001","01111","00001","00001","01110"}},
        {' ', {"00000","00000","00000","00000","00000","00000","00000"}},
        {':', {"00000","00100","00100","00000","00100","00100","00000"}},
        {'/', {"00001","00010","00100","01000","10000","00000","00000"}},
        {'.', {"00000","00000","00000","00000","00000","00110","00110"}},
        {'-', {"00000","00000","00000","11111","00000","00000","00000"}},
        {'!', {"00100","00100","00100","00100","00100","00000","00100"}},
        {'|', {"00100","00100","00100","00100","00100","00100","00100"}},
        {'[', {"01110","01000","01000","01000","01000","01000","01110"}},
        {']', {"01110","00010","00010","00010","00010","00010","01110"}},
        {'?', {"01110","10001","00001","00010","00100","00000","00100"}},
    };

    GLuint Compile(GLenum type, const char* source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "UI shader compile error:\n" << infoLog << std::endl;
        }
        return shader;
    }

    void PushQuad(std::vector<UIVertex>& verts, float x, float y, float w, float h, const glm::vec4& c)
    {
        verts.push_back({x, y, c.r, c.g, c.b, c.a});
        verts.push_back({x + w, y, c.r, c.g, c.b, c.a});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a});

        verts.push_back({x, y, c.r, c.g, c.b, c.a});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a});
        verts.push_back({x, y + h, c.r, c.g, c.b, c.a});
    }

    std::string ToUpperSafe(std::string text)
    {
        for (char& ch : text)
        {
            unsigned char uch = static_cast<unsigned char>(ch);
            ch = static_cast<char>(std::toupper(uch));
            if (kFont.find(ch) == kFont.end())
            {
                ch = ' ';
            }
        }
        return text;
    }

    void AddText(std::vector<UIVertex>& verts, float x, float y, float scale, const glm::vec4& color, const std::string& text)
    {
        const std::string safe = ToUpperSafe(text);
        float cursor = x;
        const float pixel = scale;
        const float advance = pixel * 6.0f;

        for (char ch : safe)
        {
            auto it = kFont.find(ch);
            const auto& glyph = (it != kFont.end()) ? it->second : kFont.at(' ');

            for (int row = 0; row < 7; ++row)
            {
                for (int col = 0; col < 5; ++col)
                {
                    if (glyph[row][col] == '1')
                    {
                        PushQuad(verts, cursor + col * pixel, y + row * pixel, pixel, pixel, color);
                    }
                }
            }
            cursor += advance;
        }
    }

    float EstimateTextWidth(const std::string& text, float scale)
    {
        return static_cast<float>(text.size()) * scale * 6.0f;
    }

    void AddBar(std::vector<UIVertex>& verts, float x, float y, float w, float h, float fill, const glm::vec4& bg, const glm::vec4& fg)
    {
        PushQuad(verts, x, y, w, h, bg);
        const float inner = Clamp01(fill);
        PushQuad(verts, x + 3.0f, y + 3.0f, std::max(0.0f, (w - 6.0f) * inner), std::max(0.0f, h - 6.0f), fg);
    }
}

bool InitUIOverlay()
{
    const char* vert = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec4 aColor;
        uniform mat4 uProjection;
        out vec4 vColor;
        void main()
        {
            vColor = aColor;
            gl_Position = uProjection * vec4(aPos.xy, 0.0, 1.0);
        }
    )";

    const char* frag = R"(
        #version 330 core
        in vec4 vColor;
        out vec4 FragColor;
        void main()
        {
            FragColor = vColor;
        }
    )";

    const GLuint vs = Compile(GL_VERTEX_SHADER, vert);
    const GLuint fs = Compile(GL_FRAGMENT_SHADER, frag);

    gProgram = glCreateProgram();
    glAttachShader(gProgram, vs);
    glAttachShader(gProgram, fs);
    glLinkProgram(gProgram);

    GLint success = 0;
    glGetProgramiv(gProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(gProgram, 1024, nullptr, infoLog);
        std::cerr << "UI program link error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &gVao);
    glGenBuffers(1, &gVbo);

    glBindVertexArray(gVao);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(UIVertex) * 60000, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void*>(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    gProjLoc = glGetUniformLocation(gProgram, "uProjection");
    return gProgram != 0;
}

void RenderUIOverlay(const HUDState& state)
{
    if (gProgram == 0) return;

    std::vector<UIVertex> verts;
    verts.reserve(40000);

    const float sw = static_cast<float>(state.screenWidth);
    const float sh = static_cast<float>(state.screenHeight);
    const glm::vec4 panel(0.05f, 0.08f, 0.11f, 0.72f);
    const glm::vec4 panelSoft(0.10f, 0.14f, 0.18f, 0.58f);
    const glm::vec4 outline(0.70f, 0.82f, 0.92f, 0.20f);
    const glm::vec4 text(0.92f, 0.96f, 1.0f, 0.95f);
    const glm::vec4 accent(0.83f, 0.71f, 0.34f, 0.95f);
    const glm::vec4 good(0.36f, 0.78f, 0.50f, 0.95f);
    const glm::vec4 warn(0.92f, 0.45f, 0.32f, 0.95f);
    const glm::vec4 neutral(0.44f, 0.63f, 0.84f, 0.95f);

    // Top-left main panel
    PushQuad(verts, 16.0f, 16.0f, 350.0f, 124.0f, panel);
    PushQuad(verts, 16.0f, 16.0f, 350.0f, 3.0f, outline);
    PushQuad(verts, 16.0f, 137.0f, 350.0f, 3.0f, outline);

    AddText(verts, 28.0f, 28.0f, 3.0f, text, "ZONE: " + state.zoneName);
    AddText(verts, 28.0f, 52.0f, 3.0f, text, "GOLD: " + std::to_string(state.gold));
    AddText(verts, 28.0f, 76.0f, 3.0f, text, "CARGO: " + std::to_string(state.cargoCount) + "/" + std::to_string(state.cargoCapacity));
    AddText(verts, 28.0f, 100.0f, 3.0f, text, "VALUE: " + std::to_string(state.cargoValue) + "G");

    // Top-right stats panel
    PushQuad(verts, sw - 320.0f, 16.0f, 304.0f, 150.0f, panelSoft);
    PushQuad(verts, sw - 320.0f, 16.0f, 304.0f, 3.0f, outline);

    AddText(verts, sw - 304.0f, 28.0f, 3.0f, text, "ROD: " + std::to_string(state.rodLevel));
    AddText(verts, sw - 304.0f, 52.0f, 3.0f, text, "ENGINE: " + std::to_string(state.engineLevel));

    AddText(verts, sw - 304.0f, 80.0f, 3.0f, text, "SPEED");
    AddBar(verts, sw - 304.0f, 104.0f, 250.0f, 16.0f,
        Clamp01(std::abs(state.speed) / 8.0f),
        glm::vec4(0.12f, 0.18f, 0.24f, 0.95f), neutral);

    AddText(verts, sw - 304.0f, 128.0f, 3.0f, text, "DANGER");
    AddBar(verts, sw - 304.0f, 148.0f, 250.0f, 16.0f,
        Clamp01(state.danger),
        glm::vec4(0.20f, 0.14f, 0.12f, 0.95f), warn);

    // Bottom panel
    const std::string dockText = state.atDock
        ? "AT DOCK  SELL:R  ROD:1  ENGINE:2  CARGO:3"
        : "OPEN WATER  PRESS E TO FISH";

    const std::string statusLine =
        (!state.statusText.empty() && state.messageTimer > 0.0f)
        ? state.statusText
        : "SAIL THE FOG AND FILL YOUR HOLD";

    const float dockScale = 2.5f;
    const float hintScale = 2.5f;
    const float statusScale = 2.0f;

    float longestLine = EstimateTextWidth(dockText, dockScale);
    if (EstimateTextWidth(state.hintText, hintScale) > longestLine)
        longestLine = EstimateTextWidth(state.hintText, hintScale);
    if (EstimateTextWidth(statusLine, statusScale) > longestLine)
        longestLine = EstimateTextWidth(statusLine, statusScale);

    float bottomPanelWidth = longestLine + 40.0f;
    if (bottomPanelWidth < 560.0f) bottomPanelWidth = 560.0f;
    if (bottomPanelWidth > sw - 32.0f) bottomPanelWidth = sw - 32.0f;

    PushQuad(verts, 16.0f, sh - 124.0f, bottomPanelWidth, 108.0f, panel);

    AddText(verts, 28.0f, sh - 104.0f, dockScale, state.atDock ? accent : good, dockText);
    AddText(verts, 28.0f, sh - 76.0f, hintScale, text, state.hintText);

    if (!state.statusText.empty() && state.messageTimer > 0.0f)
    {
        glm::vec4 flashColor = glm::mix(text, accent, Clamp01(state.flash * 1.5f));
        AddText(verts, 28.0f, sh - 48.0f, statusScale, flashColor, state.statusText);
    }
    else
    {
        AddText(verts, 28.0f, sh - 48.0f, statusScale, text, statusLine);
    }

    glUseProgram(gProgram);
    const glm::mat4 projection = glm::ortho(0.0f, sw, sh, 0.0f);
    glUniformMatrix4fv(gProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(gVao);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(UIVertex)), verts.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void ShutdownUIOverlay()
{
    if (gVbo != 0) glDeleteBuffers(1, &gVbo);
    if (gVao != 0) glDeleteVertexArrays(1, &gVao);
    if (gProgram != 0) glDeleteProgram(gProgram);

    gVbo = 0;
    gVao = 0;
    gProgram = 0;
}
