#include "UIOverlay.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
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
        float u, v;
    };

    struct TexturedQuad
    {
        GLuint texture = 0;
        std::vector<UIVertex> verts;
    };

    GLuint gProgram = 0;
    GLuint gVao = 0;
    GLuint gVbo = 0;
    GLint gProjLoc = -1;
    GLint gUseTextureLoc = -1;
    GLint gTextureLoc = -1;

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
        {'+', {"00000","00100","00100","11111","00100","00100","00000"}},
        {'(', {"00010","00100","01000","01000","01000","00100","00010"}},
        {')', {"01000","00100","00010","00010","00010","00100","01000"}},
        {'%', {"11001","11010","00100","01000","10110","00110","00000"}}
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
        verts.push_back({x,     y,     c.r, c.g, c.b, c.a, 0.0f, 0.0f});
        verts.push_back({x + w, y,     c.r, c.g, c.b, c.a, 1.0f, 0.0f});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a, 1.0f, 1.0f});

        verts.push_back({x,     y,     c.r, c.g, c.b, c.a, 0.0f, 0.0f});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a, 1.0f, 1.0f});
        verts.push_back({x,     y + h, c.r, c.g, c.b, c.a, 0.0f, 1.0f});
    }

    void PushTexturedQuad(std::vector<UIVertex>& verts, float x, float y, float w, float h, const glm::vec4& c)
    {
        verts.push_back({x,     y,     c.r, c.g, c.b, c.a, 0.0f, 0.0f});
        verts.push_back({x + w, y,     c.r, c.g, c.b, c.a, 1.0f, 0.0f});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a, 1.0f, 1.0f});

        verts.push_back({x,     y,     c.r, c.g, c.b, c.a, 0.0f, 0.0f});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a, 1.0f, 1.0f});
        verts.push_back({x,     y + h, c.r, c.g, c.b, c.a, 0.0f, 1.0f});
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
        return static_cast<float>(ToUpperSafe(text).size()) * scale * 6.0f;
    }

    float TextHeight(float scale)
    {
        return scale * 7.0f;
    }

    void AddBar(std::vector<UIVertex>& verts, float x, float y, float w, float h, float fill, const glm::vec4& bg, const glm::vec4& fg)
    {
        PushQuad(verts, x, y, w, h, bg);
        const float inner = Clamp01(fill);
        PushQuad(verts, x + 3.0f, y + 3.0f, std::max(0.0f, (w - 6.0f) * inner), std::max(0.0f, h - 6.0f), fg);
    }

    void AddOutlinedPanel(std::vector<UIVertex>& verts, float x, float y, float w, float h, const glm::vec4& fill, const glm::vec4& outline)
    {
        PushQuad(verts, x, y, w, h, fill);
        PushQuad(verts, x, y, w, 3.0f, outline);
        PushQuad(verts, x, y + h - 3.0f, w, 3.0f, outline);
        PushQuad(verts, x, y, 3.0f, h, outline);
        PushQuad(verts, x + w - 3.0f, y, 3.0f, h, outline);
    }

    glm::vec4 RarityColor(int rarity)
    {
        switch (rarity)
        {
        case 3: return glm::vec4(1.00f, 0.84f, 0.36f, 0.98f);
        case 2: return glm::vec4(0.56f, 0.74f, 1.00f, 0.98f);
        case 1: return glm::vec4(0.42f, 0.86f, 0.58f, 0.98f);
        default: return glm::vec4(0.85f, 0.88f, 0.92f, 0.95f);
        }
    }

    std::string RarityName(int rarity)
    {
        switch (rarity)
        {
        case 3: return "LEGENDARY";
        case 2: return "RARE";
        case 1: return "UNCOMMON";
        default: return "COMMON";
        }
    }

    std::string FormatTime(float seconds)
    {
        int total = std::max(0, static_cast<int>(std::round(seconds)));
        int mins = total / 60;
        int secs = total % 60;
        std::string result = std::to_string(mins) + ":";
        if (secs < 10) result += "0";
        result += std::to_string(secs);
        return result;
    }

    void QueueTextureQuad(std::vector<TexturedQuad>& textureQuads, GLuint texture, float x, float y, float w, float h, const glm::vec4& color)
    {
        if (texture == 0) return;
        auto it = std::find_if(textureQuads.begin(), textureQuads.end(), [texture](const TexturedQuad& q) { return q.texture == texture; });
        if (it == textureQuads.end())
        {
            textureQuads.push_back(TexturedQuad{});
            textureQuads.back().texture = texture;
            it = textureQuads.end() - 1;
        }
        PushTexturedQuad(it->verts, x, y, w, h, color);
    }

    void DrawBatch(const std::vector<UIVertex>& verts, bool textured, GLuint texture = 0)
    {
        if (verts.empty()) return;
        glBindVertexArray(gVao);
        glBindBuffer(GL_ARRAY_BUFFER, gVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(UIVertex)), verts.data());
        glUniform1i(gUseTextureLoc, textured ? 1 : 0);
        if (textured)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glUniform1i(gTextureLoc, 0);
        }
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));
    }

    float MaxWidth(const std::vector<std::string>& lines, float scale)
    {
        float w = 0.0f;
        for (const auto& line : lines) w = std::max(w, EstimateTextWidth(line, scale));
        return w;
    }

    void DrawOverlayLines(std::vector<UIVertex>& verts,
                          float sw,
                          float sh,
                          const std::string& title,
                          const std::vector<std::pair<std::string, glm::vec4>>& lines,
                          const glm::vec4& fill,
                          const glm::vec4& border,
                          float titleScale,
                          float bodyScale)
    {
        const float pad = 34.0f;
        const float gap = 16.0f;

        float maxWidth = EstimateTextWidth(title, titleScale);
        for (const auto& line : lines)
        {
            maxWidth = std::max(maxWidth, EstimateTextWidth(line.first, bodyScale));
        }

        float panelW = std::max(420.0f, maxWidth + pad * 2.0f);
        panelW = std::min(panelW, sw - 80.0f);
        float panelH = pad * 2.0f + TextHeight(titleScale) + gap;
        panelH += static_cast<float>(lines.size()) * TextHeight(bodyScale);
        if (!lines.empty()) panelH += static_cast<float>(lines.size() - 1) * 10.0f;

        const float x = (sw - panelW) * 0.5f;
        const float y = (sh - panelH) * 0.5f;
        AddOutlinedPanel(verts, x, y, panelW, panelH, fill, border);

        float cy = y + pad;
        AddText(verts, x + pad, cy, titleScale, border, title);
        cy += TextHeight(titleScale) + gap;
        for (const auto& line : lines)
        {
            AddText(verts, x + pad, cy, bodyScale, line.second, line.first);
            cy += TextHeight(bodyScale) + 10.0f;
        }
    }
}

bool InitUIOverlay()
{
    const char* vert = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec4 aColor;
        layout (location = 2) in vec2 aUV;
        uniform mat4 uProjection;
        out vec4 vColor;
        out vec2 vUV;
        void main()
        {
            vColor = aColor;
            vUV = aUV;
            gl_Position = uProjection * vec4(aPos.xy, 0.0, 1.0);
        }
    )";

    const char* frag = R"(
        #version 330 core
        in vec4 vColor;
        in vec2 vUV;
        uniform int uUseTexture;
        uniform sampler2D uTexture;
        out vec4 FragColor;
        void main()
        {
            vec4 color = vColor;
            if (uUseTexture == 1)
            {
                color *= texture(uTexture, vUV);
            }
            FragColor = color;
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(UIVertex) * 200000, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void*>(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void*>(sizeof(float) * 6));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    gProjLoc = glGetUniformLocation(gProgram, "uProjection");
    gUseTextureLoc = glGetUniformLocation(gProgram, "uUseTexture");
    gTextureLoc = glGetUniformLocation(gProgram, "uTexture");
    return gProgram != 0;
}

void RenderUIOverlay(const HUDState& state)
{
    if (gProgram == 0) return;

    std::vector<UIVertex> verts;
    std::vector<TexturedQuad> textureQuads;
    verts.reserve(140000);
    textureQuads.reserve(32);

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
    const glm::vec4 shadow(0.01f, 0.02f, 0.04f, 0.72f);

    const float titleScale = 3.0f;
    const float bodyScale = 2.5f;
    const float smallScale = 2.0f;
    const float panelPad = 12.0f;
    const float leftX = 16.0f;
    const float topY = 16.0f;

    const std::string zoneLine = "ZONE: " + state.zoneName;
    const std::string goldLine = "GOLD: " + std::to_string(state.gold);
    const std::string cargoLine = "CARGO: " + std::to_string(state.cargoCount) + "/" + std::to_string(state.cargoCapacity);
    const std::string valueLine = "VALUE: " + std::to_string(state.cargoValue) + "G";

    float leftTopTextWidth = std::max(
        std::max(EstimateTextWidth(zoneLine, titleScale), EstimateTextWidth(goldLine, titleScale)),
        std::max(EstimateTextWidth(cargoLine, titleScale), EstimateTextWidth(valueLine, titleScale)));
    float leftPanelWidth = std::max(350.0f, leftTopTextWidth + panelPad * 2.0f + 16.0f);
    leftPanelWidth = std::min(leftPanelWidth, sw * 0.45f);

    const float lineGapTop = 6.0f;
    const float leftPanelHeight = panelPad * 2.0f + TextHeight(titleScale) * 4.0f + lineGapTop * 3.0f;
    AddOutlinedPanel(verts, leftX, topY, leftPanelWidth, leftPanelHeight, panel, outline);

    float cursorY = topY + panelPad;
    AddText(verts, leftX + panelPad, cursorY, titleScale, text, zoneLine);
    cursorY += TextHeight(titleScale) + lineGapTop;
    AddText(verts, leftX + panelPad, cursorY, titleScale, text, goldLine);
    cursorY += TextHeight(titleScale) + lineGapTop;
    AddText(verts, leftX + panelPad, cursorY, titleScale, text, cargoLine);
    cursorY += TextHeight(titleScale) + lineGapTop;
    AddText(verts, leftX + panelPad, cursorY, titleScale, text, valueLine);

    const std::string questTitle = "QUEST";
    const std::string questSummary = state.questSummary.empty() ? "KEYS " + std::to_string(state.keysCollected) + "/" + std::to_string(state.keysTotal) : state.questSummary;
    const std::string keysLine = "KEYS: " + std::to_string(state.keysCollected) + "/" + std::to_string(state.keysTotal);

    float questTextWidth = std::max(
        EstimateTextWidth(questTitle, titleScale),
        std::max(EstimateTextWidth(questSummary, bodyScale), EstimateTextWidth(keysLine, bodyScale)));
    float questBarWidth = std::max(200.0f, questTextWidth);
    float questPanelWidth = std::max(leftPanelWidth, questBarWidth + panelPad * 2.0f);
    questPanelWidth = std::min(questPanelWidth, sw * 0.45f);
    questBarWidth = questPanelWidth - panelPad * 2.0f;

    const float questPanelY = topY + leftPanelHeight + 12.0f;
    const float questGap = 8.0f;
    const float barHeight = 18.0f;
    const float questPanelHeight = panelPad * 2.0f + TextHeight(titleScale) + TextHeight(bodyScale) + TextHeight(bodyScale) + barHeight + questGap * 3.0f;
    AddOutlinedPanel(verts, leftX, questPanelY, questPanelWidth, questPanelHeight, panelSoft, outline);

    cursorY = questPanelY + panelPad;
    AddText(verts, leftX + panelPad, cursorY, titleScale, accent, questTitle);
    cursorY += TextHeight(titleScale) + questGap;
    AddText(verts, leftX + panelPad, cursorY, bodyScale, text, questSummary);
    cursorY += TextHeight(bodyScale) + questGap;
    AddText(verts, leftX + panelPad, cursorY, bodyScale, text, keysLine);
    cursorY += TextHeight(bodyScale) + questGap;

    const glm::vec4 questBarColor = state.hasWon ? good : (state.goalIslandUnlocked ? accent : neutral);
    AddBar(verts, leftX + panelPad, cursorY, questBarWidth, barHeight,
        state.keysTotal > 0 ? static_cast<float>(state.keysCollected) / static_cast<float>(state.keysTotal) : 0.0f,
        glm::vec4(0.12f, 0.18f, 0.24f, 0.95f), questBarColor);

    const std::string rodLine = "ROD: " + std::to_string(state.rodLevel);
    const std::string engineLine = "ENGINE: " + std::to_string(state.engineLevel);
    const std::string minigameLine = std::string("MINIGAME: ") + (state.minigameEnabled ? (state.minigameActive ? "ACTIVE" : "ON") : "OFF");
    const std::string hullLine = "HULL: " + std::to_string(static_cast<int>(std::round(state.hullIntegrity))) + "%";

    float rightTextWidth = std::max(
        std::max(EstimateTextWidth(rodLine, titleScale), EstimateTextWidth(engineLine, titleScale)),
        std::max(EstimateTextWidth(minigameLine, bodyScale), EstimateTextWidth(hullLine, bodyScale)));
    float rightBarWidth = std::max(220.0f, rightTextWidth - 10.0f);
    float rightPanelWidth = std::max(304.0f, std::max(rightTextWidth, EstimateTextWidth("DANGER", titleScale) + rightBarWidth) + panelPad * 2.0f);
    rightPanelWidth = std::min(rightPanelWidth, sw * 0.42f);
    rightBarWidth = rightPanelWidth - panelPad * 2.0f;

    const float rightX = sw - rightPanelWidth - 16.0f;
    const float rightGap = 8.0f;
    const float rightPanelHeight = panelPad * 2.0f + TextHeight(titleScale) * 2.0f + TextHeight(bodyScale) * 2.0f
                                 + TextHeight(titleScale) * 2.0f + 16.0f * 2.0f + rightGap * 5.0f;
    AddOutlinedPanel(verts, rightX, topY, rightPanelWidth, rightPanelHeight, panelSoft, outline);

    cursorY = topY + panelPad;
    AddText(verts, rightX + panelPad, cursorY, titleScale, text, rodLine);
    cursorY += TextHeight(titleScale) + rightGap;
    AddText(verts, rightX + panelPad, cursorY, titleScale, text, engineLine);
    cursorY += TextHeight(titleScale) + rightGap;
    AddText(verts, rightX + panelPad, cursorY, bodyScale, state.minigameEnabled ? good : warn, minigameLine);
    cursorY += TextHeight(bodyScale) + rightGap;
    AddText(verts, rightX + panelPad, cursorY, bodyScale, state.hullCritical ? warn : text, hullLine);
    cursorY += TextHeight(bodyScale) + rightGap;
    AddText(verts, rightX + panelPad, cursorY, titleScale, text, "SPEED");
    cursorY += TextHeight(titleScale) + 4.0f;
    AddBar(verts, rightX + panelPad, cursorY, rightBarWidth, 16.0f,
        Clamp01(std::abs(state.speed) / 8.0f),
        glm::vec4(0.12f, 0.18f, 0.24f, 0.95f), neutral);
    cursorY += 16.0f + rightGap;
    AddText(verts, rightX + panelPad, cursorY, titleScale, text, "DANGER");
    cursorY += TextHeight(titleScale) + 4.0f;
    AddBar(verts, rightX + panelPad, cursorY, rightBarWidth, 16.0f,
        Clamp01(state.danger),
        glm::vec4(0.20f, 0.14f, 0.12f, 0.95f), warn);

    const std::string dockText = state.atDock
        ? "AT DOCK  SELL:R  ROD:1(" + std::to_string(state.rodUpgradeCost) + "G)  ENGINE:2(" + std::to_string(state.engineUpgradeCost) + "G)  CARGO:3(" + std::to_string(state.cargoUpgradeCost) + "G)  REPAIR:4(" + std::to_string(state.repairCost) + "G)"
        : std::string("OPEN WATER  PRESS E TO ") + (state.minigameEnabled ? "CAST" : "FISH");

    const std::string statusLine =
        (!state.statusText.empty() && state.messageTimer > 0.0f)
        ? state.statusText
        : (state.hasWon ? "THE SHRINE IS YOURS" : "SAIL THE FOG AND FILL YOUR HOLD");

    const float dockScale = 2.5f;
    const float hintScale = 2.5f;
    const float statusScale = 2.0f;

    float longestLine = std::max({
        EstimateTextWidth(dockText, dockScale),
        EstimateTextWidth(state.hintText, hintScale),
        EstimateTextWidth(statusLine, statusScale)
    });

    float bottomPanelWidth = std::max(620.0f, longestLine + 40.0f);
    bottomPanelWidth = std::min(bottomPanelWidth, sw - 32.0f);
    const float bottomPanelHeight = 108.0f;
    const float bottomPanelY = sh - bottomPanelHeight - 16.0f;
    AddOutlinedPanel(verts, 16.0f, bottomPanelY, bottomPanelWidth, bottomPanelHeight, panel, outline);

    AddText(verts, 28.0f, bottomPanelY + 20.0f, dockScale, state.atDock ? accent : good, dockText);
    AddText(verts, 28.0f, bottomPanelY + 48.0f, hintScale, text, state.hintText);

    if (!state.statusText.empty() && state.messageTimer > 0.0f)
    {
        glm::vec4 flashColor = glm::mix(text, accent, Clamp01(state.flash * 1.5f));
        if (state.hullCritical) flashColor = glm::mix(flashColor, warn, 0.55f);
        AddText(verts, 28.0f, bottomPanelY + 76.0f, statusScale, flashColor, state.statusText);
    }
    else
    {
        AddText(verts, 28.0f, bottomPanelY + 76.0f, statusScale, state.hullCritical ? warn : text, statusLine);
    }

    if (state.catchBannerTimer > 0.0f && !state.catchBannerText.empty())
    {
        const float s = 3.5f;
        const float pad = 18.0f;
        const float w = std::min(sw - 60.0f, std::max(280.0f, EstimateTextWidth(state.catchBannerText, s) + pad * 2.0f));
        const float h = TextHeight(s) + pad * 2.0f;
        const float x = (sw - w) * 0.5f;
        const float y = 22.0f;
        AddOutlinedPanel(verts, x, y, w, h, shadow, RarityColor(state.caughtFishRarity));
        AddText(verts, x + pad, y + pad, s, RarityColor(state.caughtFishRarity), state.catchBannerText);
    }

    if (state.showCaughtFishCard)
    {
        const float cardW = 260.0f;
        const float cardH = 252.0f;
        const float x = sw - cardW - 24.0f;
        const float y = (sh - cardH) * 0.5f - 20.0f;
        AddOutlinedPanel(verts, x, y, cardW, cardH, panel, RarityColor(state.caughtFishRarity));
        AddText(verts, x + 16.0f, y + 14.0f, 2.8f, RarityColor(state.caughtFishRarity), state.caughtFishName.empty() ? "FISH CAUGHT" : state.caughtFishName);
        AddText(verts, x + 16.0f, y + 40.0f, 2.1f, text, RarityName(state.caughtFishRarity));

        const float artX = x + 18.0f;
        const float artY = y + 70.0f;
        const float artW = cardW - 36.0f;
        const float artH = 150.0f;
        PushQuad(verts, artX, artY, artW, artH, glm::vec4(0.02f, 0.03f, 0.05f, 0.85f));
        if (state.caughtFishTexture != 0 && !state.fishTextureMissing)
        {
            float imgW = static_cast<float>(std::max(1, state.caughtFishTextureWidth));
            float imgH = static_cast<float>(std::max(1, state.caughtFishTextureHeight));
            float fit = std::min(artW / imgW, artH / imgH);
            float drawW = imgW * fit;
            float drawH = imgH * fit;
            QueueTextureQuad(textureQuads, state.caughtFishTexture,
                             artX + (artW - drawW) * 0.5f,
                             artY + (artH - drawH) * 0.5f,
                             drawW, drawH,
                             glm::vec4(1,1,1,1));
        }
        else
        {
            AddText(verts, artX + 26.0f, artY + 58.0f, 2.2f, warn, "NO FISH PNG");
        }
    }

    if (state.showStartScreen || state.showPauseScreen || state.showJournal || state.showVictoryScreen)
    {
        PushQuad(verts, 0.0f, 0.0f, sw, sh, glm::vec4(0.01f, 0.02f, 0.04f, 0.42f));
    }

    if (state.showStartScreen)
    {
        const std::string title = "DREDGE STYLE FISHING PROTOTYPE";
        std::vector<std::pair<std::string, glm::vec4>> lines = {
            {"FIND 3 KEYS AND REACH THE LOST ISLAND", text},
            {"WASD SAIL   E FISH   M TOGGLE MINIGAME", text},
            {"R SELL   1 2 3 UPGRADES   4 REPAIR   J JOURNAL", text},
            {"PRESS ENTER TO START", good}
        };
        DrawOverlayLines(verts, sw, sh, title, lines, glm::vec4(0.02f, 0.05f, 0.09f, 0.92f), accent, 4.0f, 3.0f);
    }

    if (state.showPauseScreen)
    {
        const std::string title = "PAUSED / HELP";
        std::vector<std::pair<std::string, glm::vec4>> lines = {
            {"WASD MOVE   E FISH   SPACE HOOK   M MINIGAME", text},
            {"R SELL   1 2 3 UPGRADES   4 REPAIR", text},
            {"J JOURNAL   P CLOSE HELP   F5-F8 POST FX", text},
            {"PRESS P TO RESUME", good}
        };
        DrawOverlayLines(verts, sw, sh, title, lines, glm::vec4(0.02f, 0.05f, 0.09f, 0.92f), outline, 4.0f, 3.0f);
    }

    if (state.showJournal)
    {
        const float pad = 24.0f;
        const float x = 80.0f;
        const float y = 56.0f;
        const float w = sw - 160.0f;
        const float h = sh - 112.0f;
        AddOutlinedPanel(verts, x, y, w, h, glm::vec4(0.02f, 0.05f, 0.09f, 0.94f), neutral);
        AddText(verts, x + pad, y + pad, 3.6f, accent, "FISH JOURNAL");
        AddText(verts, x + w - 220.0f, y + pad + 4.0f, 2.2f, text,
                "PAGE " + std::to_string(state.journalPage + 1) + "/" + std::to_string(std::max(1, state.journalPageCount)));

        const float cardGap = 18.0f;
        const float cardW = (w - pad * 2.0f - cardGap) * 0.5f;
        const float cardH = 170.0f;
        const float startY = y + 70.0f;

        for (size_t i = 0; i < state.journalEntries.size(); ++i)
        {
            const HUDJournalEntry& e = state.journalEntries[i];
            const int col = static_cast<int>(i % 2);
            const int row = static_cast<int>(i / 2);
            const float cx = x + pad + col * (cardW + cardGap);
            const float cy = startY + row * (cardH + cardGap);
            AddOutlinedPanel(verts, cx, cy, cardW, cardH, panel, e.discovered ? RarityColor(e.rarity) : outline);

            if (e.discovered)
            {
                AddText(verts, cx + 16.0f, cy + 16.0f, 2.6f, RarityColor(e.rarity), e.name);
                AddText(verts, cx + 16.0f, cy + 42.0f, 2.0f, text, RarityName(e.rarity));
                AddText(verts, cx + 16.0f, cy + 68.0f, 2.0f, text, "CAUGHT: " + std::to_string(e.caughtCount));
                AddText(verts, cx + 16.0f, cy + 90.0f, 2.0f, text, "BEST: " + std::to_string(e.bestValue) + "G");

                const float artX = cx + cardW - 128.0f;
                const float artY = cy + 18.0f;
                const float artW = 100.0f;
                const float artH = 100.0f;
                PushQuad(verts, artX, artY, artW, artH, glm::vec4(0.02f, 0.03f, 0.05f, 0.9f));
                if (e.texture != 0 && !e.textureMissing)
                {
                    float imgW = static_cast<float>(std::max(1, e.textureWidth));
                    float imgH = static_cast<float>(std::max(1, e.textureHeight));
                    float fit = std::min(artW / imgW, artH / imgH);
                    float drawW = imgW * fit;
                    float drawH = imgH * fit;
                    QueueTextureQuad(textureQuads, e.texture,
                                     artX + (artW - drawW) * 0.5f,
                                     artY + (artH - drawH) * 0.5f,
                                     drawW, drawH,
                                     glm::vec4(1,1,1,1));
                }
                else
                {
                    AddText(verts, artX + 8.0f, artY + 40.0f, 1.6f, warn, "NO PNG");
                }
            }
            else
            {
                AddText(verts, cx + 16.0f, cy + 18.0f, 2.6f, outline, "UNKNOWN FISH");
                AddText(verts, cx + 16.0f, cy + 52.0f, 2.0f, text, "CATCH THIS FISH TO");
                AddText(verts, cx + 16.0f, cy + 74.0f, 2.0f, text, "UNLOCK THE ENTRY");
                PushQuad(verts, cx + cardW - 128.0f, cy + 18.0f, 100.0f, 100.0f, glm::vec4(0.12f, 0.14f, 0.18f, 0.9f));
                AddText(verts, cx + cardW - 112.0f, cy + 56.0f, 2.2f, outline, "???");
            }
        }

        AddText(verts, x + pad, y + h - 32.0f, 2.2f, good, "LEFT / RIGHT CHANGE PAGE    J CLOSE JOURNAL");
    }

    if (state.showVictoryScreen)
    {
        const std::string title = "LOST ISLAND REACHED";
        std::vector<std::pair<std::string, glm::vec4>> lines = {
            {"FISH CAUGHT: " + std::to_string(state.totalFishCaught), text},
            {"GOLD EARNED: " + std::to_string(state.totalGoldEarned), text},
            {"CARGO SOLD: " + std::to_string(state.totalCargoSold), text},
            {"PERFECT HOOKS: " + std::to_string(state.perfectHooks), text},
            {"TIME PLAYED: " + FormatTime(state.totalPlayTimeSeconds), text},
            {"J OPEN JOURNAL   P HELP", good}
        };
        DrawOverlayLines(verts, sw, sh, title, lines, glm::vec4(0.06f, 0.05f, 0.02f, 0.94f), accent, 3.8f, 2.8f);
    }

    glUseProgram(gProgram);
    const glm::mat4 projection = glm::ortho(0.0f, sw, sh, 0.0f);
    glUniformMatrix4fv(gProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    DrawBatch(verts, false);
    for (const auto& q : textureQuads)
    {
        DrawBatch(q.verts, true, q.texture);
    }

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
