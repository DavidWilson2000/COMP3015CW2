#include "GameSettingsMenu.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "SoundManager.h"

float GameSettingsMenu::Clamp(float value, float minValue, float maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

void GameSettingsMenu::ToggleOpen()
{
    m_state.open = !m_state.open;
}

bool GameSettingsMenu::IsOpen() const
{
    return m_state.open;
}

void GameSettingsMenu::MoveSelection(int delta)
{
    const int itemCount = 4;
    m_state.selectedIndex += delta;
    if (m_state.selectedIndex < 0) m_state.selectedIndex = itemCount - 1;
    if (m_state.selectedIndex >= itemCount) m_state.selectedIndex = 0;
}

void GameSettingsMenu::AdjustSelected(float delta)
{
    switch (m_state.selectedIndex)
    {
    case 0: m_state.brightness = Clamp(m_state.brightness + delta, 0.55f, 1.45f); break;
    case 1: m_state.masterVolume = Clamp(m_state.masterVolume + delta, 0.0f, 1.0f); break;
    case 2: m_state.musicVolume = Clamp(m_state.musicVolume + delta, 0.0f, 1.0f); break;
    case 3: m_state.sfxVolume = Clamp(m_state.sfxVolume + delta, 0.0f, 1.0f); break;
    default: break;
    }
}

void GameSettingsMenu::ResetDefaults()
{
    m_state.brightness = 1.0f;
    m_state.masterVolume = 1.0f;
    m_state.musicVolume = 1.0f;
    m_state.sfxVolume = 1.0f;
    m_state.selectedIndex = 0;
}

float GameSettingsMenu::GetBrightness() const { return m_state.brightness; }
float GameSettingsMenu::GetMasterVolume() const { return m_state.masterVolume; }
float GameSettingsMenu::GetMusicVolume() const { return m_state.musicVolume; }
float GameSettingsMenu::GetSfxVolume() const { return m_state.sfxVolume; }

const GameSettingsState& GameSettingsMenu::GetState() const
{
    return m_state;
}

void GameSettingsMenu::ApplyAudio(SoundManager& sound) const
{
    sound.SetMasterVolume(m_state.masterVolume);
    sound.SetMusicVolume(m_state.musicVolume);
    sound.SetSfxVolume(m_state.sfxVolume);
}

namespace
{
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
        {'.', {"00000","00000","00000","00000","00000","00110","00110"}},
        {'-', {"00000","00000","00000","11111","00000","00000","00000"}},
        {'<', {"00001","00010","00100","01000","00100","00010","00001"}},
        {'>', {"10000","01000","00100","00010","00100","01000","10000"}},
        {'%', {"11001","11010","00100","01000","10110","00110","00000"}}
    };

    float Clamp01(float v)
    {
        return std::max(0.0f, std::min(1.0f, v));
    }

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
            std::cerr << "Settings UI shader compile error:\n" << infoLog << std::endl;
        }
        return shader;
    }

    void PushQuad(std::vector<UIVertex>& verts, float x, float y, float w, float h, const glm::vec4& c)
    {
        verts.push_back({x,     y,     c.r, c.g, c.b, c.a});
        verts.push_back({x + w, y,     c.r, c.g, c.b, c.a});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a});
        verts.push_back({x,     y,     c.r, c.g, c.b, c.a});
        verts.push_back({x + w, y + h, c.r, c.g, c.b, c.a});
        verts.push_back({x,     y + h, c.r, c.g, c.b, c.a});
    }

    std::string ToUpperSafe(std::string text)
    {
        for (char& ch : text)
        {
            unsigned char uch = static_cast<unsigned char>(ch);
            ch = static_cast<char>(std::toupper(uch));
            if (kFont.find(ch) == kFont.end())
                ch = ' ';
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
            const auto it = kFont.find(ch);
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

    void AddBar(std::vector<UIVertex>& verts, float x, float y, float w, float h, float fill, const glm::vec4& bg, const glm::vec4& fg)
    {
        PushQuad(verts, x, y, w, h, bg);
        PushQuad(verts, x + 3.0f, y + 3.0f, std::max(0.0f, (w - 6.0f) * Clamp01(fill)), std::max(0.0f, h - 6.0f), fg);
    }

    void DrawBatch(const std::vector<UIVertex>& verts)
    {
        if (verts.empty()) return;
        glBindVertexArray(gVao);
        glBindBuffer(GL_ARRAY_BUFFER, gVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(verts.size() * sizeof(UIVertex)), verts.data());
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));
    }
}

bool InitGameSettingsMenuRenderer()
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

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(gProgram, 1024, nullptr, infoLog);
        std::cerr << "Settings UI program link error:\n" << infoLog << std::endl;
    }

    glGenVertexArrays(1, &gVao);
    glGenBuffers(1, &gVbo);

    glBindVertexArray(gVao);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(UIVertex) * 40000, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), reinterpret_cast<void*>(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    gProjLoc = glGetUniformLocation(gProgram, "uProjection");
    return gProgram != 0;
}

void RenderGameSettingsMenu(int screenWidth, int screenHeight, const GameSettingsState& state)
{
    if (!state.open || gProgram == 0) return;

    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);
    std::vector<UIVertex> verts;
    verts.reserve(16000);

    const glm::vec4 panel(0.02f, 0.05f, 0.09f, 0.94f);
    const glm::vec4 border(0.70f, 0.82f, 0.92f, 0.95f);
    const glm::vec4 text(0.88f, 0.92f, 0.96f, 0.95f);
    const glm::vec4 accent(0.92f, 0.76f, 0.24f, 0.98f);
    const glm::vec4 good(0.36f, 0.90f, 0.58f, 0.98f);
    const glm::vec4 muted(0.16f, 0.22f, 0.28f, 0.92f);

    const float w = 820.0f;
    const float h = 520.0f;
    const float x = (sw - w) * 0.5f;
    const float y = (sh - h) * 0.5f;

    PushQuad(verts, x, y, w, h, panel);
    PushQuad(verts, x, y, w, 3.0f, border);
    PushQuad(verts, x, y + h - 3.0f, w, 3.0f, border);
    PushQuad(verts, x, y, 3.0f, h, border);
    PushQuad(verts, x + w - 3.0f, y, 3.0f, h, border);

    AddText(verts, x + 28.0f, y + 26.0f, 4.0f, accent, "SETTINGS");
    AddText(verts, x + 28.0f, y + 66.0f, 2.2f, text, "UP / DOWN SELECT   LEFT / RIGHT CHANGE   R RESET   O CLOSE");

    struct Row
    {
        const char* label;
        float value;
    };

    Row rows[4] = {
        {"BRIGHTNESS", state.brightness},
        {"MASTER VOLUME", state.masterVolume},
        {"MUSIC VOLUME", state.musicVolume},
        {"SFX VOLUME", state.sfxVolume}
    };

    float rowY = y + 132.0f;
    for (int i = 0; i < 4; ++i)
    {
        const bool selected = (state.selectedIndex == i);
        const glm::vec4 rowColor = selected ? good : text;
        const glm::vec4 barFill = selected ? good : border;

        if (selected)
        {
            PushQuad(verts, x + 22.0f, rowY - 10.0f, w - 44.0f, 72.0f, glm::vec4(0.10f, 0.16f, 0.20f, 0.62f));
        }

        AddText(verts, x + 40.0f, rowY, 3.0f, rowColor, rows[i].label);
        AddBar(verts, x + 360.0f, rowY + 2.0f, 300.0f, 24.0f, rows[i].value, muted, barFill);

        const int pct = static_cast<int>(rows[i].value * 100.0f + 0.5f);
        AddText(verts, x + 690.0f, rowY, 2.6f, rowColor, std::to_string(pct) + "%");

        if (selected)
        {
            AddText(verts, x + 18.0f, rowY, 2.6f, rowColor, ">");
        }

        rowY += 78.0f;
    }

    AddText(verts, x + 28.0f, y + h - 82.0f, 2.2f, text, "TIP: USE BRIGHTNESS TO LIFT DARKER SCENES");
    AddText(verts, x + 28.0f, y + h - 52.0f, 2.2f, text, "WITHOUT CHANGING YOUR SHADER MODES");
    glUseProgram(gProgram);
    const glm::mat4 projection = glm::ortho(0.0f, sw, sh, 0.0f);
    glUniformMatrix4fv(gProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    DrawBatch(verts);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void ShutdownGameSettingsMenuRenderer()
{
    if (gVbo != 0) glDeleteBuffers(1, &gVbo);
    if (gVao != 0) glDeleteVertexArrays(1, &gVao);
    if (gProgram != 0) glDeleteProgram(gProgram);

    gVbo = 0;
    gVao = 0;
    gProgram = 0;
}
