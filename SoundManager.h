#pragma once

#include <string>
#include <irrKlang/irrKlang.h>

class SoundManager
{
public:
    SoundManager();
    ~SoundManager();

    bool Init(const std::string& soundFolder);
    void Shutdown();

    void PlayBackgroundLoop();
    void PlayReel();
    void PlaySplash();
    void UpdateBoatMotor(float speed, float maxSpeed);

private:
    std::string ResolveSoundPath(const std::string& baseName) const;
    void StopBoatMotor();
    static float Clamp01(float value);

    irrklang::ISoundEngine* m_engine = nullptr;
    irrklang::ISound* m_backgroundLoop = nullptr;
    irrklang::ISound* m_boatLoop = nullptr;
    std::string m_soundFolder;
};
