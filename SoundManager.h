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

    void UpdateZoneLayer(const std::string& zoneName, float dt, float danger, float proximityBlend = 1.0f);

    void SetMasterVolume(float value);
    void SetMusicVolume(float value);
    void SetSfxVolume(float value);

    float GetMasterVolume() const;
    float GetMusicVolume() const;
    float GetSfxVolume() const;

private:
    std::string ResolveSoundPath(const std::string& baseName) const;
    std::string ResolveZoneLayerPath(const std::string& zoneName, std::string& outResolvedBaseName) const;
    void StopBoatMotor();
    void StopZoneLayers();
    void AdvanceZoneCrossfade(float dt, float targetVolume);
    void ApplyLoopVolumes(float boatBaseVolume = -1.0f, float zoneBaseVolume = -1.0f);
    static float Clamp01(float value);

    irrklang::ISoundEngine* m_engine = nullptr;
    irrklang::ISound* m_backgroundLoop = nullptr;
    irrklang::ISound* m_boatLoop = nullptr;
    irrklang::ISound* m_zoneLoop = nullptr;
    irrklang::ISound* m_nextZoneLoop = nullptr;

    std::string m_zoneLayerName;
    std::string m_nextZoneLayerName;
    std::string m_soundFolder;

    float m_masterVolume = 1.0f;
    float m_musicVolume = 1.0f;
    float m_sfxVolume = 1.0f;

    float m_lastBoatBaseVolume = 0.0f;
    float m_lastZoneBaseVolume = 0.0f;
};