#pragma once

#include <string>

struct HUDState
{
    int screenWidth = 1280;
    int screenHeight = 720;

    std::string zoneName;
    std::string statusText;
    std::string hintText;

    int gold = 0;
    int cargoCount = 0;
    int cargoCapacity = 0;
    int cargoValue = 0;
    int rodLevel = 1;
    int engineLevel = 1;
    float speed = 0.0f;
    float danger = 0.0f;

    bool atDock = false;
    float messageTimer = 0.0f;
    float flash = 0.0f;
};

bool InitUIOverlay();
void RenderUIOverlay(const HUDState& state);
void ShutdownUIOverlay();
