#pragma once

class SoundManager;

struct GameSettingsState
{
    bool open = false;
    int selectedIndex = 0;
    float brightness = 1.0f;
    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
};

class GameSettingsMenu
{
public:
    void ToggleOpen();
    bool IsOpen() const;

    void MoveSelection(int delta);
    void AdjustSelected(float delta);
    void ResetDefaults();

    float GetBrightness() const;
    float GetMasterVolume() const;
    float GetMusicVolume() const;
    float GetSfxVolume() const;

    const GameSettingsState& GetState() const;
    void ApplyAudio(SoundManager& sound) const;

private:
    static float Clamp(float value, float minValue, float maxValue);
    GameSettingsState m_state;
};

bool InitGameSettingsMenuRenderer();
void RenderGameSettingsMenu(int screenWidth, int screenHeight, const GameSettingsState& state);
void ShutdownGameSettingsMenuRenderer();
