#pragma once

#include <string>

struct FishingMinigameResult
{
    bool success = false;
    bool perfect = false;
    float timingScore = 0.0f;
    std::string message;
};

class FishingMinigame
{
public:
    FishingMinigame();

    void ToggleEnabled();
    bool IsEnabled() const;
    bool IsActive() const;

    void Start(int rodLevel, int fishRarity);
    void Cancel();
    void Update(float dt);
    void Hook();

    bool ConsumeResult(FishingMinigameResult& outResult);

    std::string GetHintText() const;
    std::string GetStatusText() const;

private:
    void QueueResult(bool success, bool perfect, float timingScore, const std::string& message);

    bool m_enabled = false;
    bool m_active = false;
    bool m_hasPendingResult = false;

    int m_rodLevel = 1;
    int m_fishRarity = 0;

    float m_marker = 0.0f;
    float m_markerVelocity = 0.75f;
    float m_targetCenter = 0.5f;
    float m_targetHalfWidth = 0.14f;
    float m_timeRemaining = 0.0f;
    float m_totalDuration = 0.0f;

    FishingMinigameResult m_pendingResult;
};
