#pragma once

#include <glm/glm.hpp>
#include <string>

class IslandQuest
{
public:
    IslandQuest();

    void Reset();

    void HandleTestGoldHotkey(bool keyDown, int& totalMoney, std::string& lastCatchText, float& catchMessageTimer);
    bool TryCollectKeyForZone(int zoneIndex, int rodLevel, std::string& lastCatchText, float& catchMessageTimer);
    void Update(const glm::vec3& boatPosition, std::string& lastCatchText, float& catchMessageTimer);

    int GetCollectedKeyCount() const;
    bool HasAllKeys() const;
    bool IsGoalIslandUnlocked() const;
    bool HasWon() const;

    glm::vec3 GetGoalIslandPosition() const;
    float GetGoalIslandRadius() const;

    float GetLockedFogFactor(const glm::vec3& boatPosition) const;
    std::string GetQuestSummary() const;

private:
    void UnlockGoalIsland(std::string& lastCatchText, float& catchMessageTimer);
    void WinGame(std::string& lastCatchText, float& catchMessageTimer);

    bool m_testGoldPressed = false;
    bool m_keysCollected[3] = { false, false, false };
    bool m_goalIslandUnlocked = false;
    bool m_hasWon = false;

    glm::vec3 m_goalIslandPosition = glm::vec3(0.0f, 0.35f, 42.0f);
    float m_goalIslandRadius = 5.5f;
};
