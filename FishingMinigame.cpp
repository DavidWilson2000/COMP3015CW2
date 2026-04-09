#include "FishingMinigame.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace
{
    float Clamp01(float value)
    {
        if (value < 0.0f) return 0.0f;
        if (value > 1.0f) return 1.0f;
        return value;
    }

    float ClampRange(float value, float minValue, float maxValue)
    {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

    const char* RarityName(int rarity)
    {
        switch (rarity)
        {
        case 1: return "UNCOMMON";
        case 2: return "RARE";
        case 3: return "LEGENDARY";
        default: return "COMMON";
        }
    }
}

FishingMinigame::FishingMinigame() = default;

void FishingMinigame::ToggleEnabled()
{
    m_enabled = !m_enabled;
    if (!m_enabled)
    {
        Cancel();
    }
}

bool FishingMinigame::IsEnabled() const
{
    return m_enabled;
}

bool FishingMinigame::IsActive() const
{
    return m_active;
}

void FishingMinigame::Start(int rodLevel, int fishRarity)
{
    if (!m_enabled) return;

    m_active = true;
    m_hasPendingResult = false;
    m_marker = 0.0f;

    m_rodLevel = std::max(1, rodLevel);
    m_fishRarity = std::max(0, std::min(3, fishRarity));

    const float rodEase = Clamp01(static_cast<float>(m_rodLevel - 1) / 9.0f);
    const float rarityHardness = static_cast<float>(m_fishRarity) / 3.0f;

    const float random01 = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    const float randomWidth = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);

    const float baseSpeed = 1.18f - rodEase * 0.45f + rarityHardness * 0.42f;
    const float baseHalfWidth = 0.075f + rodEase * 0.085f - rarityHardness * 0.030f;
    const float baseDuration = 3.10f + rodEase * 0.55f - rarityHardness * 0.35f;

    m_markerVelocity = ClampRange(baseSpeed, 0.45f, 1.45f);
    m_totalDuration = ClampRange(baseDuration, 2.40f, 4.20f);
    m_timeRemaining = m_totalDuration;

    m_targetCenter = 0.18f + random01 * 0.64f;
    m_targetHalfWidth = ClampRange(baseHalfWidth + (randomWidth - 0.5f) * 0.020f, 0.050f, 0.220f);
}

void FishingMinigame::Cancel()
{
    m_active = false;
    m_hasPendingResult = false;
    m_timeRemaining = 0.0f;
}

void FishingMinigame::Update(float dt)
{
    if (!m_active) return;

    m_timeRemaining -= dt;
    if (m_timeRemaining <= 0.0f)
    {
        QueueResult(false, false, 0.0f, "The fish slipped away");
        return;
    }

    m_marker += m_markerVelocity * dt;
    if (m_marker >= 1.0f)
    {
        m_marker = 1.0f;
        m_markerVelocity = -std::abs(m_markerVelocity);
    }
    else if (m_marker <= 0.0f)
    {
        m_marker = 0.0f;
        m_markerVelocity = std::abs(m_markerVelocity);
    }
}

void FishingMinigame::Hook()
{
    if (!m_active) return;

    const float distance = std::abs(m_marker - m_targetCenter);
    const bool success = distance <= m_targetHalfWidth;
    const bool perfect = distance <= (m_targetHalfWidth * 0.45f);
    const float timingScore = success ? Clamp01(1.0f - (distance / std::max(m_targetHalfWidth, 0.0001f))) : 0.0f;

    if (success)
    {
        QueueResult(true, perfect, timingScore, perfect ? "Perfect hook!" : "Good hook!");
    }
    else
    {
        QueueResult(false, false, 0.0f, "Missed the bite");
    }
}

bool FishingMinigame::ConsumeResult(FishingMinigameResult& outResult)
{
    if (!m_hasPendingResult) return false;

    outResult = m_pendingResult;
    m_hasPendingResult = false;
    return true;
}

std::string FishingMinigame::GetHintText() const
{
    if (!m_enabled) return "MINIGAME OFF | PRESS M TO ENABLE";
    if (!m_active) return "MINIGAME ON | PRESS E TO START | SPACE TO HOOK";
    return "SPACE TO HOOK | BIGGER ROD = BIGGER TARGET | RARER FISH = FASTER";
}

std::string FishingMinigame::GetStatusText() const
{
    if (!m_enabled) return "MINIGAME DISABLED";
    if (!m_active) return "PRESS E TO CAST INTO THE MINIGAME";

    const int width = 22;
    int markerIndex = static_cast<int>(std::round(m_marker * static_cast<float>(width - 1)));
    int targetStart = static_cast<int>(std::round((m_targetCenter - m_targetHalfWidth) * static_cast<float>(width - 1)));
    int targetEnd = static_cast<int>(std::round((m_targetCenter + m_targetHalfWidth) * static_cast<float>(width - 1)));
    markerIndex = std::max(0, std::min(width - 1, markerIndex));
    targetStart = std::max(0, std::min(width - 1, targetStart));
    targetEnd = std::max(0, std::min(width - 1, targetEnd));

    std::string bar(width, '-');
    for (int i = targetStart; i <= targetEnd && i < width; ++i)
    {
        if (i >= 0) bar[i] = 'O';
    }
    bar[markerIndex] = '|';

    std::stringstream ss;
    ss << "HOOK " << '[' << bar << ']'
       << " TIME " << static_cast<int>(std::ceil(m_timeRemaining))
       << " " << RarityName(m_fishRarity);
    return ss.str();
}

void FishingMinigame::QueueResult(bool success, bool perfect, float timingScore, const std::string& message)
{
    m_active = false;
    m_hasPendingResult = true;
    m_pendingResult.success = success;
    m_pendingResult.perfect = perfect;
    m_pendingResult.timingScore = timingScore;
    m_pendingResult.message = message;
}
