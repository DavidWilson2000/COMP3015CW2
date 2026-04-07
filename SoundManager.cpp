#include "SoundManager.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace
{
    bool FileExists(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        return file.good();
    }
}

SoundManager::SoundManager() = default;

SoundManager::~SoundManager()
{
    Shutdown();
}

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
    if (!m_soundFolder.empty() && m_soundFolder.back() != '/' && m_soundFolder.back() != '\\')
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
    {
        return directPath;
    }

    const std::vector<std::string> extensions = {
        ".wav", ".mp3", ".ogg", ".flac"
    };

    for (const auto& ext : extensions)
    {
        const std::string testPath = directPath + ext;
        if (FileExists(testPath))
        {
            return testPath;
        }
    }

    std::cerr << "Could not find sound file for base name: " << baseName
              << " inside " << m_soundFolder << std::endl;
    return "";
}

void SoundManager::PlayBackgroundLoop()
{
    if (!m_engine || m_backgroundLoop) return;

    const std::string path = ResolveSoundPath("background");
    if (path.empty()) return;

    m_backgroundLoop = m_engine->play2D(path.c_str(), true, false, true);
    if (m_backgroundLoop)
    {
        m_backgroundLoop->setVolume(0.45f);
    }
}

void SoundManager::PlayReel()
{
    if (!m_engine) return;

    const std::string path = ResolveSoundPath("reel");
    if (path.empty()) return;

    m_engine->play2D(path.c_str(), false, false, false);
}

void SoundManager::PlaySplash()
{
    if (!m_engine) return;

    const std::string path = ResolveSoundPath("splash");
    if (path.empty()) return;

    m_engine->play2D(path.c_str(), false, false, false);
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

    m_boatLoop->setVolume(0.15f + speed01 * 0.35f);
    m_boatLoop->setPlaybackSpeed(0.75f + speed01 * 0.55f);
}
