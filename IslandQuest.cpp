#include "IslandQuest.h"

#include <algorithm>
#include <cmath>

namespace
{
    float Distance2D(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
    }
}

IslandQuest::IslandQuest()
{
    Reset();
}

void IslandQuest::Reset()
{
    m_testGoldPressed = false;
    m_keysCollected[0] = false;
    m_keysCollected[1] = false;
    m_keysCollected[2] = false;
    m_goalIslandUnlocked = false;
    m_hasWon = false;
}

void IslandQuest::HandleTestGoldHotkey(bool keyDown, int& totalMoney, std::string& lastCatchText, float& catchMessageTimer)
{
    if (keyDown && !m_testGoldPressed)
    {
        totalMoney += 1000;
        lastCatchText = "DEBUG: +1000 gold added";
        catchMessageTimer = 2.0f;
        m_testGoldPressed = true;
    }
    else if (!keyDown)
    {
        m_testGoldPressed = false;
    }
}

bool IslandQuest::TryCollectKeyForZone(int zoneIndex, int rodLevel, std::string& lastCatchText, float& catchMessageTimer)
{
    if (rodLevel < 10) return false;
    if (zoneIndex < 0 || zoneIndex > 2) return false;
    if (m_keysCollected[zoneIndex]) return false;

    static const char* kKeyNames[3] =
    {
        "Harbour Key",
        "Reef Key",
        "Trench Key"
    };

    m_keysCollected[zoneIndex] = true;
    lastCatchText = std::string("You fished up the ") + kKeyNames[zoneIndex] + "!";
    catchMessageTimer = 3.0f;

    if (HasAllKeys())
    {
        UnlockGoalIsland(lastCatchText, catchMessageTimer);
    }

    return true;
}

void IslandQuest::Update(const glm::vec3& boatPosition, std::string& lastCatchText, float& catchMessageTimer)
{
    if (m_hasWon || !m_goalIslandUnlocked) return;

    if (Distance2D(boatPosition, m_goalIslandPosition) <= m_goalIslandRadius)
    {
        WinGame(lastCatchText, catchMessageTimer);
    }
}

int IslandQuest::GetCollectedKeyCount() const
{
    return static_cast<int>(m_keysCollected[0]) +
           static_cast<int>(m_keysCollected[1]) +
           static_cast<int>(m_keysCollected[2]);
}

bool IslandQuest::HasAllKeys() const
{
    return GetCollectedKeyCount() == 3;
}

bool IslandQuest::IsGoalIslandUnlocked() const
{
    return m_goalIslandUnlocked;
}

bool IslandQuest::HasWon() const
{
    return m_hasWon;
}

glm::vec3 IslandQuest::GetGoalIslandPosition() const
{
    return m_goalIslandPosition;
}

float IslandQuest::GetGoalIslandRadius() const
{
    return m_goalIslandRadius;
}

float IslandQuest::GetLockedFogFactor(const glm::vec3& boatPosition) const
{
    if (m_goalIslandUnlocked) return 0.0f;

    const float distance = Distance2D(boatPosition, m_goalIslandPosition);
    const float outerRadius = 18.0f;
    const float innerRadius = 7.0f;

    if (distance >= outerRadius) return 0.0f;
    if (distance <= innerRadius) return 1.0f;

    const float t = 1.0f - ((distance - innerRadius) / (outerRadius - innerRadius));
    return std::max(0.0f, std::min(1.0f, t));
}

std::string IslandQuest::GetQuestSummary() const
{
    if (m_hasWon)
    {
        return "LOST ISLAND REACHED";
    }

    if (m_goalIslandUnlocked)
    {
        return "GO TO THE LOST ISLAND";
    }

    return "KEYS " + std::to_string(GetCollectedKeyCount()) + "/3";
}

void IslandQuest::UnlockGoalIsland(std::string& lastCatchText, float& catchMessageTimer)
{
    m_goalIslandUnlocked = true;
    lastCatchText = "All 3 keys collected! The Lost Island has appeared.";
    catchMessageTimer = 4.0f;
}

void IslandQuest::WinGame(std::string& lastCatchText, float& catchMessageTimer)
{
    m_hasWon = true;
    lastCatchText = "YOU REACHED THE LOST ISLAND! YOU WIN!";
    catchMessageTimer = 999.0f;
}
