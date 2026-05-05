#include "WeatherSystem.h"
#include "GameTypes.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdlib>
#include <string>

// main.cpp owns these user-facing feedback functions. The weather system
// stays small and focused by only calling back when a new weather state starts.
void setCatchMessage(GLFWwindow* window, const std::string& message, float timer);
void TriggerCatchBanner(const std::string& text, float duration, float shakeStrength);

enum class WeatherType
{
    Calm = 0,
    Fog,
    Rain,
    Storm
};

struct WeatherRuntimeState
{
    WeatherType type = WeatherType::Calm;
    float timer = 0.0f;
    float duration = 40.0f;
    float intensity = 0.0f;
};

namespace
{
    WeatherRuntimeState gWeather;
}

const char* GetWeatherName()
{
    switch (gWeather.type)
    {
    case WeatherType::Fog: return "FOG";
    case WeatherType::Rain: return "RAIN";
    case WeatherType::Storm: return "STORM";
    default: return "CALM";
    }
}

std::string GetWeatherEffectText()
{
    switch (gWeather.type)
    {
    case WeatherType::Fog: return "LOW VISIBILITY";
    case WeatherType::Rain: return "SLIPPERY WATER";
    case WeatherType::Storm: return "ROUGH WATER DANGER";
    default: return "CLEAR SAILING";
    }
}

float GetWeatherEffect01()
{
    return gWeather.intensity;
}

glm::vec3 GetWeatherTint()
{
    switch (gWeather.type)
    {
    case WeatherType::Fog: return glm::vec3(0.72f, 0.76f, 0.78f);
    case WeatherType::Rain: return glm::vec3(0.42f, 0.48f, 0.56f);
    case WeatherType::Storm: return glm::vec3(0.18f, 0.22f, 0.30f);
    default: return glm::vec3(0.70f, 0.82f, 0.94f);
    }
}

float GetWeatherFogNearMultiplier()
{
    switch (gWeather.type)
    {
    case WeatherType::Fog: return glm::mix(1.0f, 0.42f, gWeather.intensity);
    case WeatherType::Rain: return glm::mix(1.0f, 0.75f, gWeather.intensity);
    case WeatherType::Storm: return glm::mix(1.0f, 0.55f, gWeather.intensity);
    default: return 1.0f;
    }
}

float GetWeatherFogFarMultiplier()
{
    switch (gWeather.type)
    {
    case WeatherType::Fog: return glm::mix(1.0f, 0.48f, gWeather.intensity);
    case WeatherType::Rain: return glm::mix(1.0f, 0.72f, gWeather.intensity);
    case WeatherType::Storm: return glm::mix(1.0f, 0.50f, gWeather.intensity);
    default: return 1.0f;
    }
}

float GetWeatherDangerBonus()
{
    switch (gWeather.type)
    {
    case WeatherType::Rain: return 0.10f * gWeather.intensity;
    case WeatherType::Storm: return 0.30f * gWeather.intensity;
    default: return 0.0f;
    }
}

float GetWeatherSpeedMultiplier()
{
    switch (gWeather.type)
    {
    case WeatherType::Rain: return glm::mix(1.0f, 0.92f, gWeather.intensity);
    case WeatherType::Storm: return glm::mix(1.0f, 0.78f, gWeather.intensity);
    default: return 1.0f;
    }
}

float GetWeatherTimeRemaining()
{
    return std::max(0.0f, gWeather.timer);
}

void SetNextWeather(GLFWwindow* window)
{
    const int roll = rand() % 100;
    WeatherType next = WeatherType::Calm;
    if (roll < 30) next = WeatherType::Calm;
    else if (roll < 55) next = WeatherType::Fog;
    else if (roll < 80) next = WeatherType::Rain;
    else next = WeatherType::Storm;

    gWeather.type = next;
    gWeather.duration = 42.0f + static_cast<float>(rand() % 34);
    gWeather.timer = gWeather.duration;
    gWeather.intensity = 0.0f;

    if (window && next != WeatherType::Calm)
    {
        setCatchMessage(window, std::string("Weather changing: ") + GetWeatherName(), 2.0f);
        TriggerCatchBanner(std::string("WEATHER: ") + GetWeatherName(), 1.1f, next == WeatherType::Storm ? 0.5f : 0.0f);
    }
}

void UpdateWeather(float dt, GLFWwindow* window)
{
    gWeather.timer -= dt;
    if (gWeather.timer <= 0.0f)
    {
        SetNextWeather(window);
    }

    const float elapsed = std::max(0.0f, gWeather.duration - gWeather.timer);
    const float fadeIn = clampf(elapsed / 8.0f, 0.0f, 1.0f);
    const float fadeOut = clampf(gWeather.timer / 8.0f, 0.0f, 1.0f);
    gWeather.intensity = (gWeather.type == WeatherType::Calm) ? 0.0f : std::min(fadeIn, fadeOut);
}
