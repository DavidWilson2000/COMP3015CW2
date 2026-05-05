#pragma once

#include <glm/glm.hpp>
#include <string>

struct GLFWwindow;

const char* GetWeatherName();
std::string GetWeatherEffectText();
float GetWeatherEffect01();
glm::vec3 GetWeatherTint();
float GetWeatherFogNearMultiplier();
float GetWeatherFogFarMultiplier();
float GetWeatherDangerBonus();
float GetWeatherSpeedMultiplier();
float GetWeatherTimeRemaining();
void SetNextWeather(GLFWwindow* window);
void UpdateWeather(float dt, GLFWwindow* window);
