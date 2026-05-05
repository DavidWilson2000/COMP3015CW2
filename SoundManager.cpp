#include "SoundManager.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>

namespace
{
    bool FileExists(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        return file.good();
    }

    std::string ToLowerNoSpace(std::string value)
    {
        std::string out;
        out.reserve(value.size());
        for (char ch : value)
        {
            unsigned char uch = static_cast<unsigned char>(ch);
            if (std::isalnum(uch))
            {
                out.push_back(static_cast<char>(std::tolower(uch)));
            }
        }
        return out;
    }
}

SoundManager::SoundManager() = default;
SoundManager::~SoundManager() { Shutdown(); }

float SoundManager::Clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

bool SoundManager::Init(const std::string& soundFolder)
{
    Shutdown();
    m_soundFolder = soundFolder;

    if (!m_soundFolder.empty() &&
        m_soundFolder.back() != '/' &&
        m_soundFolder.back() != '\\')
    {
        m_soundFolder += "/";
    }

    m_engine = irrklang::createIrrKlangDevice();
    if (!m_engine)
    {
        std::cerr << "Failed to create irrKlang device." << std::endl;
        return false;
    }

    return true;
}

void SoundManager::Shutdown()
{
    StopBoatMotor();
    StopZoneLayers();
    StopPatrolHornLoop();
    if (m_backgroundLoop)
    {
        m_backgroundLoop->stop();
        m_backgroundLoop->drop();
        m_backgroundLoop = nullptr;
    }

    if (m_engine)
    {
        m_engine->drop();
        m_engine = nullptr;
    }
}

std::string SoundManager::ResolveSoundPath(const std::string& baseName) const
{
    if (baseName.empty()) return "";

    const std::string directPath = m_soundFolder + baseName;
    if (FileExists(directPath))
        return directPath;

    const std::vector<std::string> extensions = { ".wav", ".mp3", ".ogg", ".flac" };
    for (const auto& ext : extensions)
    {
        const std::string testPath = directPath + ext;
        if (FileExists(testPath))
            return testPath;
    }

    return "";
}

std::string SoundManager::ResolveZoneLayerPath(const std::string& zoneName, std::string& outResolvedBaseName) const
{
    outResolvedBaseName.clear();
    const std::string key = ToLowerNoSpace(zoneName);
    std::vector<std::string> candidates;

    if (key.empty() || key == "openwater")
        candidates = { "ambient_open_water", "ambient_openwater", "zone_open_water", "zone_openwater" };
    else if (key == "harbourwaters" || key == "harbour")
        candidates = { "ambient_harbour", "ambient_harbour_waters", "zone_harbour", "zone_harbour_waters" };
    else if (key == "reefedge" || key == "sunkenreef" || key == "reef")
        candidates = { "ambient_reef", "ambient_sunken_reef", "ambient_reef_edge", "zone_reef", "zone_sunken_reef" };
    else if (key == "deeptrench" || key == "abyssaltrench" || key == "trench")
        candidates = { "ambient_trench", "ambient_abyssal_trench", "ambient_deep_trench", "zone_trench", "zone_abyssal_trench" };
    else if (key == "lostisland")
        candidates = { "ambient_lost_island", "ambient_lostisland", "ambient_lost", "zone_lost_island" };
    else if (key == "shroudedshoals" || key == "shoals")
        candidates = { "ambient_shoals", "ambient_shrouded_shoals", "zone_shoals" };
    else
        candidates = { "ambient_open_water", "ambient_openwater", "zone_open_water", "zone_openwater" };

    for (const auto& candidate : candidates)
    {
        const std::string path = ResolveSoundPath(candidate);
        if (!path.empty())
        {
            outResolvedBaseName = candidate;
            return path;
        }
    }
    return "";
}

void SoundManager::SetMasterVolume(float value)
{
    m_masterVolume = Clamp01(value);
    ApplyLoopVolumes();
}

void SoundManager::SetMusicVolume(float value)
{
    m_musicVolume = Clamp01(value);
    ApplyLoopVolumes();
}

void SoundManager::SetSfxVolume(float value)
{
    m_sfxVolume = Clamp01(value);
    ApplyLoopVolumes();
}

float SoundManager::GetMasterVolume() const { return m_masterVolume; }
float SoundManager::GetMusicVolume() const { return m_musicVolume; }
float SoundManager::GetSfxVolume() const { return m_sfxVolume; }

void SoundManager::ApplyLoopVolumes(float boatBaseVolume, float zoneBaseVolume)
{
    if (boatBaseVolume >= 0.0f) m_lastBoatBaseVolume = boatBaseVolume;
    if (zoneBaseVolume >= 0.0f) m_lastZoneBaseVolume = zoneBaseVolume;

    if (m_backgroundLoop)
        m_backgroundLoop->setVolume(0.05f * m_musicVolume * m_masterVolume);

    if (m_boatLoop)
        m_boatLoop->setVolume(m_lastBoatBaseVolume * m_sfxVolume * m_masterVolume);

    if (m_zoneLoop)
        m_zoneLoop->setVolume(m_lastZoneBaseVolume * m_musicVolume * m_masterVolume);

    if (m_nextZoneLoop)
        m_nextZoneLoop->setVolume(std::min(m_nextZoneLoop->getVolume(), m_musicVolume * m_masterVolume));

    if (m_patrolHorn)
        m_patrolHorn->setVolume(0.85f * m_sfxVolume * m_masterVolume);
}

void SoundManager::PlayBackgroundLoop()
{
    if (!m_engine || m_backgroundLoop) return;
    const std::string path = ResolveSoundPath("background");
    if (path.empty()) return;

    m_backgroundLoop = m_engine->play2D(path.c_str(), true, false, true);
    ApplyLoopVolumes();
}

void SoundManager::PlayReel()
{
    if (!m_engine) return;

    const std::string path = ResolveSoundPath("reel");
    if (path.empty()) return;

    irrklang::ISound* sound = m_engine->play2D(path.c_str(), false, false, true);
    if (sound)
    {
        sound->setVolume(0.95f * m_sfxVolume * m_masterVolume);
        sound->drop();
    }
}

void SoundManager::PlaySplash()
{
    if (!m_engine) return;

    const std::string path = ResolveSoundPath("splash");
    if (path.empty()) return;

    irrklang::ISound* sound = m_engine->play2D(path.c_str(), false, false, true);
    if (sound)
    {
        sound->setVolume(0.90f * m_sfxVolume * m_masterVolume);
        sound->drop();
    }
}

void SoundManager::StopBoatMotor()
{
    if (m_boatLoop)
    {
        m_boatLoop->stop();
        m_boatLoop->drop();
        m_boatLoop = nullptr;
    }
}

void SoundManager::StopZoneLayers()
{
    if (m_zoneLoop)
    {
        m_zoneLoop->stop();
        m_zoneLoop->drop();
        m_zoneLoop = nullptr;
    }
    if (m_nextZoneLoop)
    {
        m_nextZoneLoop->stop();
        m_nextZoneLoop->drop();
        m_nextZoneLoop = nullptr;
    }
    m_zoneLayerName.clear();
    m_nextZoneLayerName.clear();
    m_lastZoneBaseVolume = 0.0f;
}

void SoundManager::AdvanceZoneCrossfade(float dt, float targetVolume)
{
    const float fadeSpeed = std::max(dt, 0.0f) * 0.85f;
    const float scaledTarget = targetVolume * m_musicVolume * m_masterVolume;

    if (m_nextZoneLoop)
    {
        const float nextVol = std::min(scaledTarget, m_nextZoneLoop->getVolume() + fadeSpeed);
        m_nextZoneLoop->setVolume(nextVol);

        if (m_zoneLoop)
        {
            const float curVol = std::max(0.0f, m_zoneLoop->getVolume() - fadeSpeed);
            m_zoneLoop->setVolume(curVol);

            if (curVol <= 0.001f && nextVol >= scaledTarget - 0.02f)
            {
                m_zoneLoop->stop();
                m_zoneLoop->drop();
                m_zoneLoop = m_nextZoneLoop;
                m_zoneLayerName = m_nextZoneLayerName;
                m_nextZoneLoop = nullptr;
                m_nextZoneLayerName.clear();
            }
        }
        else
        {
            m_zoneLoop = m_nextZoneLoop;
            m_zoneLayerName = m_nextZoneLayerName;
            m_nextZoneLoop = nullptr;
            m_nextZoneLayerName.clear();
            if (m_zoneLoop)
                m_zoneLoop->setVolume(scaledTarget);
        }
    }
    else if (m_zoneLoop)
    {
        const float currentVolume = m_zoneLoop->getVolume();
        if (currentVolume < scaledTarget)
            m_zoneLoop->setVolume(std::min(scaledTarget, currentVolume + fadeSpeed));
        else if (currentVolume > scaledTarget)
            m_zoneLoop->setVolume(std::max(scaledTarget, currentVolume - fadeSpeed));
    }
}

void SoundManager::UpdateBoatMotor(float speed, float maxSpeed)
{
    if (!m_engine) return;

    const float threshold = 0.18f;
    if (speed <= threshold)
    {
        StopBoatMotor();
        return;
    }

    const std::string path = ResolveSoundPath("boat");
    if (path.empty()) return;

    if (!m_boatLoop)
    {
        m_boatLoop = m_engine->play2D(path.c_str(), true, false, true);
        if (!m_boatLoop) return;
    }

    const float safeMax = std::max(maxSpeed, 0.01f);
    const float speed01 = Clamp01(speed / safeMax);
    const float baseVolume = 0.15f + speed01 * 0.35f;

    m_boatLoop->setPlaybackSpeed(0.75f + speed01 * 0.55f);
    ApplyLoopVolumes(baseVolume);
}

void SoundManager::UpdateZoneLayer(const std::string& zoneName, float dt, float danger, float proximityBlend)
{
    if (!m_engine) return;

    std::string resolvedName;
    const std::string path = ResolveZoneLayerPath(zoneName, resolvedName);
    const std::string key = ToLowerNoSpace(zoneName);

    float targetVolume = 0.0f;
    if (!path.empty())
    {
        const float blend = Clamp01(proximityBlend);
        if (key == "openwater")
            targetVolume = (0.04f + danger * 0.03f) * blend;
        else if (key == "harbourwaters" || key == "harbour")
            targetVolume = (0.10f + danger * 0.04f) * blend;
        else if (key == "reefedge" || key == "sunkenreef" || key == "reef")
            targetVolume = (0.16f + danger * 0.08f) * blend;
        else if (key == "deeptrench" || key == "abyssaltrench" || key == "trench")
            targetVolume = (0.20f + danger * 0.10f) * blend;
        else if (key == "lostisland")
            targetVolume = 0.22f * blend;
        else
            targetVolume = (0.10f + danger * 0.05f) * blend;
    }

    m_lastZoneBaseVolume = targetVolume;

    if (path.empty())
    {
        if (m_nextZoneLoop)
        {
            const float nextVol = std::max(0.0f, m_nextZoneLoop->getVolume() - std::max(dt, 0.0f) * 0.85f);
            m_nextZoneLoop->setVolume(nextVol);
            if (nextVol <= 0.001f)
            {
                m_nextZoneLoop->stop();
                m_nextZoneLoop->drop();
                m_nextZoneLoop = nullptr;
                m_nextZoneLayerName.clear();
            }
        }

        if (m_zoneLoop)
        {
            const float curVol = std::max(0.0f, m_zoneLoop->getVolume() - std::max(dt, 0.0f) * 0.85f);
            m_zoneLoop->setVolume(curVol);
            if (curVol <= 0.001f)
            {
                m_zoneLoop->stop();
                m_zoneLoop->drop();
                m_zoneLoop = nullptr;
                m_zoneLayerName.clear();
            }
        }
        return;
    }

    if (m_zoneLayerName != resolvedName && m_nextZoneLayerName != resolvedName)
    {
        if (!m_zoneLoop)
        {
            m_zoneLoop = m_engine->play2D(path.c_str(), true, false, true);
            if (m_zoneLoop)
            {
                m_zoneLoop->setVolume(0.0f);
                m_zoneLayerName = resolvedName;
            }
        }
        else
        {
            if (m_nextZoneLoop)
            {
                m_nextZoneLoop->stop();
                m_nextZoneLoop->drop();
                m_nextZoneLoop = nullptr;
                m_nextZoneLayerName.clear();
            }

            m_nextZoneLoop = m_engine->play2D(path.c_str(), true, false, true);
            if (m_nextZoneLoop)
            {
                m_nextZoneLoop->setVolume(0.0f);
                m_nextZoneLayerName = resolvedName;
            }
        }
    }

    AdvanceZoneCrossfade(dt, targetVolume);
}
void SoundManager::StartPatrolHornLoop()
{
    if (!m_engine)
    {
        return;
    }

    // Do not start another horn if one is already playing.
    if (m_patrolHorn)
    {
        return;
    }

    std::string hornPath = m_soundFolder + "/horn.wav";

    // Parameters:
    // true  = loop
    // false = do not start paused
    // true  = track the sound so we can stop it later
    m_patrolHorn = m_engine->play2D(hornPath.c_str(), true, false, true);

    if (m_patrolHorn)
    {
        m_patrolHorn->setVolume(0.85f * m_sfxVolume * m_masterVolume);
    }
}
void SoundManager::StopPatrolHornLoop()
{
    if (!m_patrolHorn)
    {
        return;
    }

    m_patrolHorn->stop();
    m_patrolHorn->drop();
    m_patrolHorn = nullptr;
}
void SoundManager::UpdatePatrolHorn(bool shouldPlay)
{
    if (shouldPlay)
    {
        StartPatrolHornLoop();
    }
    else
    {
        StopPatrolHornLoop();
    }
}