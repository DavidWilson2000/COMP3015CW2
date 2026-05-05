#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#define STB_IMAGE_IMPLEMENTATION
#include "helper/stb/stb_image.h"
#include "SimpleOBJLoader.h"
#include "PostProcess.h"
#include "UIOverlay.h"
#include "FishingMinigame.h"
#include "SoundManager.h"
#include "IslandQuest.h"
#include "LostIslandSetpiece.h"
#include "GameSettingsMenu.h"
#include "WorldGen.h"
#include "RenderTypes.h"
#include "WorldRenderer.h"
#include "HarbourMission.h"
#include "ResourceUtils.h"
#include "GeometrySetup.h"
#include "ParticleEffects.h"
#include "GameTypes.h"
#include "WeatherSystem.h"



// Core application configuration, main.cpp acts as the coordinator for the game: it creates the window, loads resources, updates gameplay systems, and runs the render pipeline.
// Larger systems such as world generation, static world rendering, UI, audio, quests and post-processing are split into their own files.


const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
const int MAX_PARTICLES = 300;

int gWindowWidth = SCR_WIDTH;
int gWindowHeight = SCR_HEIGHT;

const std::string BASIC_VERT_PATH = "shader/basic.vert";
const std::string BASIC_FRAG_PATH = "shader/basic.frag";
const std::string WATER_VERT_PATH = "shader/water.vert";
const std::string WATER_FRAG_PATH = "shader/water.frag";
const std::string DEPTH_VERT_PATH = "shader/depth.vert";
const std::string DEPTH_FRAG_PATH = "shader/depth.frag";
const std::string PARTICLE_VERT_PATH = "shader/particle.vert";
const std::string PARTICLE_FRAG_PATH = "shader/particle.frag";
const std::string SKYBOX_VERT_PATH = "shader/skybox.vert";
const std::string SKYBOX_FRAG_PATH = "shader/skybox.frag";
const std::string POSTPROCESS_VERT_PATH = "shader/postprocess.vert";
const std::string POSTPROCESS_FRAG_PATH = "shader/postprocess.frag";

bool keys[1024] = { false };
bool fishPressed = false;
bool shopCloseReady = false;
bool sellPressed = false;
bool upgrade1Pressed = false;
bool upgrade2Pressed = false;
bool upgrade3Pressed = false;
bool repairPressed = false;
bool startPressed = false;
bool pausePressed = false;
bool journalPressed = false;
bool journalLeftPressed = false;
bool journalRightPressed = false;
bool minigameTogglePressed = false;
bool minigameHookPressed = false;
bool cameraModeTogglePressed = false;
bool shadowModeTogglePressed = false;
bool gAnimateSunCycle = true;
bool settingsTogglePressed = false;
bool settingsUpPressed = false;
bool settingsDownPressed = false;
bool settingsLeftPressed = false;
bool settingsRightPressed = false;
bool settingsResetPressed = false;
bool missionInteractPressed = false;
bool missionResetPressed = false;
bool shopLeftPressed = false;
bool shopRightPressed = false;
bool shopUpPressed = false;
bool shopDownPressed = false;
bool shopSelectPressed = false;
bool shopMousePressed = false;
bool shopClosePressed = false;
bool contractTurnInPressed = false;

Boat boat;
std::vector<FishZone> zones;
std::vector<CargoItem> cargo;
std::vector<Particle> particles(MAX_PARTICLES);

std::unordered_map<std::string, FishIconTexture> gFishIconTextures;
std::unordered_map<std::string, FishJournalEntry> gFishJournal;
std::vector<std::string> gFishJournalOrder;
std::string gLastCaughtFishName;
GLuint gLastCaughtFishTexture = 0;
int gLastCaughtFishTextureWidth = 0;
int gLastCaughtFishTextureHeight = 0;
bool gLastCaughtFishTextureMissing = false;
int gLastCaughtFishRarity = 0;
float gCaughtFishCardTimer = 0.0f;
float gCatchBannerTimer = 0.0f;
std::string gCatchBannerText;
float gCameraShakeTimer = 0.0f;
float gCameraShakeStrength = 0.0f;

bool gGameStarted = false;
bool gPaused = false;
bool gJournalOpen = false;
bool gDockShopOpen = false;
int gDockShopSelection = 0;
int gJournalPage = 0;

std::vector<ShopkeeperContract> gShopkeeperContracts;



void setCatchMessage(GLFWwindow* window, const std::string& message, float timer);
void SpawnSplash(std::vector<Particle>& particles, const glm::vec3& pos, const glm::vec4& color, int count);
void InitialiseShopkeeperContracts();
float gSessionPlayTime = 0.0f;
int gTotalFishCaught = 0;
int gTotalGoldEarned = 0;
int gTotalCargoSold = 0;
int gPerfectHooks = 0;

int totalMoney = 0;
int rodLevel = 1;
int engineLevel = 1;
int cargoLevel = 1;
int cargoCapacity = 5;
float fishCooldown = 0.0f;
float catchFlashTimer = 0.0f;
float catchMessageTimer = 0.0f;
float hullIntegrity = 100.0f;
float hullWarningTimer = 0.0f;
float engineStutterTimer = 0.0f;
bool gWinCelebrationTriggered = false;
std::string lastCatchText = "No fish caught yet";

EnvironmentBlendState gEnvironmentBlend;
FishingMinigame gFishingMinigame;
int gFishingMinigameZoneIndex = -1;
FishData gPendingMinigameFish;
bool gHasPendingMinigameFish = false;
SoundManager gSound;
IslandQuest gIslandQuest;
GameSettingsMenu gSettingsMenu;
HarbourMission gHarbourMission;


GLuint cubeVAO = 0, cubeVBO = 0;
GLuint planeVAO = 0, planeVBO = 0;
GLuint skyboxVAO = 0, skyboxVBO = 0;
GLuint particleVAO = 0, particleVBO = 0;

GLuint basicShader = 0;
GLuint waterShader = 0;
GLuint depthShader = 0;
GLuint particleShader = 0;
GLuint skyboxShader = 0;

PostProcessor gPostProcessor;
PostProcessMode gPostMode = PostProcessMode::None;
ShadowFilterMode gShadowFilterMode = ShadowFilterMode::PCSS;
CameraMode gCameraMode = CameraMode::Follow;
float gFreeLookYawOffset = 0.0f;
float gFreeLookPitch = 30.0f;
float gFreeLookDistance = 9.0f;

GLuint shadowFBO = 0;
GLuint shadowMap = 0;
GLuint skyboxCubemap = 0;
GLuint woodTex = 0;
GLuint rockTex = 0;
GLuint grassTex = 0;
GLuint boatTex = 0;
GLuint patrolBoatTex = 0;
GLuint scrapTex = 0;
GLuint buoyTex = 0;
GLuint treeTex = 0;
GLuint zoneMarkerTex[3] = { 0, 0, 0 };
GLuint gShopkeeperTexture = 0;

ModelMesh boatModel;
ModelMesh swordModel;
ModelMesh treeModel;
ModelMesh scrapModel;

float boatVisualY = 0.18f;
float boatVisualPitch = 0.0f;
float boatVisualRoll = 0.0f;

float cubeVertices[] = {
    -0.5f,-0.5f,-0.5f,  0,0,-1,  0,0,
     0.5f,-0.5f,-0.5f,  0,0,-1,  1,0,
     0.5f, 0.5f,-0.5f,  0,0,-1,  1,1,
     0.5f, 0.5f,-0.5f,  0,0,-1,  1,1,
    -0.5f, 0.5f,-0.5f,  0,0,-1,  0,1,
    -0.5f,-0.5f,-0.5f,  0,0,-1,  0,0,

    -0.5f,-0.5f, 0.5f,  0,0,1,  0,0,
     0.5f,-0.5f, 0.5f,  0,0,1,  1,0,
     0.5f, 0.5f, 0.5f,  0,0,1,  1,1,
     0.5f, 0.5f, 0.5f,  0,0,1,  1,1,
    -0.5f, 0.5f, 0.5f,  0,0,1,  0,1,
    -0.5f,-0.5f, 0.5f,  0,0,1,  0,0,

    -0.5f, 0.5f, 0.5f, -1,0,0,  1,0,
    -0.5f, 0.5f,-0.5f, -1,0,0,  1,1,
    -0.5f,-0.5f,-0.5f, -1,0,0,  0,1,
    -0.5f,-0.5f,-0.5f, -1,0,0,  0,1,
    -0.5f,-0.5f, 0.5f, -1,0,0,  0,0,
    -0.5f, 0.5f, 0.5f, -1,0,0,  1,0,

     0.5f, 0.5f, 0.5f,  1,0,0,  1,0,
     0.5f, 0.5f,-0.5f,  1,0,0,  1,1,
     0.5f,-0.5f,-0.5f,  1,0,0,  0,1,
     0.5f,-0.5f,-0.5f,  1,0,0,  0,1,
     0.5f,-0.5f, 0.5f,  1,0,0,  0,0,
     0.5f, 0.5f, 0.5f,  1,0,0,  1,0,

    -0.5f,-0.5f,-0.5f,  0,-1,0, 0,1,
     0.5f,-0.5f,-0.5f,  0,-1,0, 1,1,
     0.5f,-0.5f, 0.5f,  0,-1,0, 1,0,
     0.5f,-0.5f, 0.5f,  0,-1,0, 1,0,
    -0.5f,-0.5f, 0.5f,  0,-1,0, 0,0,
    -0.5f,-0.5f,-0.5f,  0,-1,0, 0,1,

    -0.5f, 0.5f,-0.5f,  0,1,0,  0,1,
     0.5f, 0.5f,-0.5f,  0,1,0,  1,1,
     0.5f, 0.5f, 0.5f,  0,1,0,  1,0,
     0.5f, 0.5f, 0.5f,  0,1,0,  1,0,
    -0.5f, 0.5f, 0.5f,  0,1,0,  0,0,
    -0.5f, 0.5f,-0.5f,  0,1,0,  0,1
};

float planeVertices[] = {
    -0.5f, 0.0f, -0.5f, 0,1,0, 0,0,
     0.5f, 0.0f, -0.5f, 0,1,0, 4,0,
     0.5f, 0.0f,  0.5f, 0,1,0, 4,4,
     0.5f, 0.0f,  0.5f, 0,1,0, 4,4,
    -0.5f, 0.0f,  0.5f, 0,1,0, 0,4,
    -0.5f, 0.0f, -0.5f, 0,1,0, 0,0
};

float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
};


void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    gWindowWidth = width;
    gWindowHeight = height;
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_F4) gPostMode = PostProcessMode::DepthOfField;
        if (key == GLFW_KEY_F5) gPostMode = PostProcessMode::Edge;
        if (key == GLFW_KEY_F6) gPostMode = PostProcessMode::Blur;
        if (key == GLFW_KEY_F7) gPostMode = PostProcessMode::NightVision;
        if (key == GLFW_KEY_F8) gPostMode = PostProcessMode::None;
        if (key == GLFW_KEY_F9) gShadowFilterMode = (gShadowFilterMode == ShadowFilterMode::PCSS) ? ShadowFilterMode::PCF : ShadowFilterMode::PCSS;
        if (key == GLFW_KEY_F10) gAnimateSunCycle = !gAnimateSunCycle;
        if (key == GLFW_KEY_F11) gPostMode = PostProcessMode::GodRays;
        if (key == GLFW_KEY_F12) gPostMode = PostProcessMode::SSAO;
    }

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS) keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
}


std::string makeFishTextureStem(const std::string& fishName)
{
    std::string stem;
    for (char ch : fishName)
    {
        unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch))
        {
            stem.push_back(static_cast<char>(uch));
        }
    }
    return stem;
}

std::vector<std::string> getFishTextureCandidates(const std::string& fishName)
{
    std::vector<std::string> candidates;
    candidates.push_back("media/fish/" + fishName + ".png");

    const std::string compactStem = makeFishTextureStem(fishName);
    if (!compactStem.empty() && compactStem != fishName)
    {
        candidates.push_back("media/fish/" + compactStem + ".png");
    }

    candidates.push_back("media/fish/" + fishName + ".PNG");
    if (!compactStem.empty() && compactStem != fishName)
    {
        candidates.push_back("media/fish/" + compactStem + ".PNG");
    }

    return candidates;
}

// Loads optional fish PNG artwork for the catch card and journal.
// Missing fish images are allowed; the UI can display a fallback state.
FishIconTexture loadFishIconTexture(const std::string& fishName)
{
    FishIconTexture icon;
    icon.attempted = true;

    const std::vector<std::string> candidates = getFishTextureCandidates(fishName);
    std::string resolvedPath;

    for (const auto& candidate : candidates)
    {
        if (resolveResourcePath(candidate, resolvedPath, "fish PNG", false, false))
        {
            std::cout << "[Fish PNG] Found image for " << fishName
                      << ": " << resolvedPath << std::endl;
            break;
        }
    }

    if (resolvedPath.empty())
    {
        std::cout << "No fish PNG found for: " << fishName << std::endl;
        return icon;
    }

    stbi_set_flip_vertically_on_load(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(resolvedPath.c_str(), &width, &height, &channels, 4);

    if (!data)
    {
        std::cerr << "Failed to load fish PNG: " << resolvedPath << std::endl;
        return icon;
    }

    glGenTextures(1, &icon.texture);
    glBindTexture(GL_TEXTURE_2D, icon.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    icon.width = width;
    icon.height = height;
    icon.found = true;

    std::cout << "Loaded fish PNG: " << resolvedPath << " (" << width << "x" << height << ")" << std::endl;
    return icon;
}

const FishIconTexture& getFishIconTexture(const std::string& fishName)
{
    const std::string key = makeFishTextureStem(fishName).empty() ? fishName : makeFishTextureStem(fishName);
    auto it = gFishIconTextures.find(key);
    if (it == gFishIconTextures.end())
    {
        FishIconTexture icon = loadFishIconTexture(fishName);
        it = gFishIconTextures.emplace(key, icon).first;
    }
    return it->second;
}

void ClearCaughtFishDisplay()
{
    gLastCaughtFishName.clear();
    gLastCaughtFishTexture = 0;
    gLastCaughtFishTextureWidth = 0;
    gLastCaughtFishTextureHeight = 0;
    gLastCaughtFishTextureMissing = false;
    gLastCaughtFishRarity = 0;
    gCaughtFishCardTimer = 0.0f;
}

void ShowCaughtFishDisplay(const FishData& fish)
{
    gLastCaughtFishName = fish.name;
    gCaughtFishCardTimer = 3.25f;

    const FishIconTexture& icon = getFishIconTexture(fish.name);
    gLastCaughtFishTexture = icon.texture;
    gLastCaughtFishTextureWidth = icon.width;
    gLastCaughtFishTextureHeight = icon.height;
    gLastCaughtFishTextureMissing = !icon.found;
    gLastCaughtFishRarity = fish.rarity;
}

std::string MakeFishJournalKey(const std::string& fishName)
{
    const std::string compact = makeFishTextureStem(fishName);
    return compact.empty() ? fishName : compact;
}

void TriggerCatchBanner(const std::string& text, float duration, float shakeStrength = 0.0f)
{
    gCatchBannerText = text;
    gCatchBannerTimer = duration;
    if (shakeStrength > 0.0f)
    {
        gCameraShakeTimer = std::max(gCameraShakeTimer, duration * 0.8f);
        gCameraShakeStrength = std::max(gCameraShakeStrength, shakeStrength);
    }
}

// Registers every fish from every zone so the journal can display
// undiscovered entries before the player has caught them.
void RegisterFishJournalEntries()
{
    gFishJournal.clear();
    gFishJournalOrder.clear();

    for (const auto& zone : zones)
    {
        for (const auto& fish : zone.fishPool)
        {
            const std::string key = MakeFishJournalKey(fish.name);
            if (gFishJournal.find(key) != gFishJournal.end())
                continue;

            FishJournalEntry entry;
            entry.baseFish = fish;
            entry.highestRarityCaught = fish.rarity;
            gFishJournal[key] = entry;
            gFishJournalOrder.push_back(key);
        }
    }
}

void RecordFishCaught(const FishData& fish)
{
    const std::string key = MakeFishJournalKey(fish.name);
    auto it = gFishJournal.find(key);
    if (it == gFishJournal.end())
    {
        FishJournalEntry entry;
        entry.baseFish = fish;
        entry.discovered = true;
        entry.caughtCount = 1;
        entry.bestValue = fish.value;
        entry.highestRarityCaught = fish.rarity;
        gFishJournal[key] = entry;
        gFishJournalOrder.push_back(key);
        return;
    }

    it->second.discovered = true;
    it->second.caughtCount += 1;
    it->second.bestValue = std::max(it->second.bestValue, fish.value);
    it->second.highestRarityCaught = std::max(it->second.highestRarityCaught, fish.rarity);
}

int GetJournalPageCount()
{
    const int perPage = 4;
    return std::max(1, static_cast<int>((gFishJournalOrder.size() + perPage - 1) / perPage));
}

std::vector<HUDJournalEntry> BuildJournalPageEntries(int page)
{
    std::vector<HUDJournalEntry> entries;
    const int perPage = 4;
    const int safePage = std::max(0, std::min(page, GetJournalPageCount() - 1));
    const int start = safePage * perPage;
    const int end = std::min(start + perPage, static_cast<int>(gFishJournalOrder.size()));

    for (int i = start; i < end; ++i)
    {
        const auto it = gFishJournal.find(gFishJournalOrder[i]);
        if (it == gFishJournal.end())
            continue;

        const FishJournalEntry& entry = it->second;
        HUDJournalEntry hudEntry;
        hudEntry.name = entry.baseFish.name;
        hudEntry.rarity = entry.discovered ? entry.highestRarityCaught : entry.baseFish.rarity;
        hudEntry.caughtCount = entry.caughtCount;
        hudEntry.bestValue = entry.bestValue;
        hudEntry.discovered = entry.discovered;

        if (entry.discovered)
        {
            const FishIconTexture& icon = getFishIconTexture(entry.baseFish.name);
            hudEntry.texture = icon.texture;
            hudEntry.textureWidth = icon.width;
            hudEntry.textureHeight = icon.height;
            hudEntry.textureMissing = !icon.found;
        }

        entries.push_back(hudEntry);
    }

    return entries;
}

void setupShadowMap()
{
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Fishing zones define the main biome/gameplay regions. Each zone has
// its own fish pool, fog values, water tint, and danger amount.
void setupZones()
{
    zones.push_back({ glm::vec3(12.0f, 0.0f, 10.0f), 6.5f,
        { {"Sardine",0,5}, {"Mackerel",0,6}, {"Crab",1,11} },
        glm::vec3(0.18f, 0.55f, 0.35f), glm::vec3(0.72f, 0.82f, 0.92f), 16.0f, 66.0f, 0.2f, "Harbour Waters" });

    zones.push_back({ glm::vec3(-18.0f, 0.0f, -10.0f), 8.5f,
        { {"SilverBrean",0,8}, {"Lionfish",2,20}, {"Eel",2,25}, {"Grouper",1,13} },
        glm::vec3(0.10f, 0.60f, 0.55f), glm::vec3(0.58f, 0.78f, 0.78f), 13.0f, 53.0f, 0.45f, "Reef Edge" });

    zones.push_back({ glm::vec3(23.0f, 0.0f, -23.0f), 10.0f,
        { {"Anglerfish",2,30}, {"Oarfish",3,60}, {"Ghost Fish",3,100}, {"Black Cod",1,18} },
        glm::vec3(0.35f, 0.18f, 0.55f), glm::vec3(0.36f, 0.42f, 0.58f), 9.0f, 36.0f, 0.8f, "Deep Trench" });
}

const FishZone* getCurrentZone()
{
    for (const auto& zone : zones)
    {
        if (zone.contains(boat.position)) return &zone;
    }
    return nullptr;
}

std::string getCurrentZoneName()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->zoneName : "Open Water";
}

glm::vec3 getDefaultWaterTint()
{
    return glm::vec3(0.0f, 0.10f, 0.16f);
}

glm::vec3 getDefaultFogColor()
{
    return glm::vec3(0.70f, 0.82f, 0.94f);
}

float getDefaultFogNear()
{
    return 18.0f;
}

float getDefaultFogFar()
{
    return 72.0f;
}

float getDefaultDanger()
{
    return 0.1f;
}

glm::vec3 getTargetZoneWaterTint()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->tint : getDefaultWaterTint();
}

glm::vec3 getTargetZoneFogColor()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->fogColor : getDefaultFogColor();
}

float getTargetZoneFogNear()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->fogNear : getDefaultFogNear();
}

float getTargetZoneFogFar()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->fogFar : getDefaultFogFar();
}

float getTargetWorldDanger()
{
    const FishZone* zone = getCurrentZone();
    return zone ? zone->danger : getDefaultDanger();
}

void initialiseEnvironmentBlend()
{
    gEnvironmentBlend.waterTint = getTargetZoneWaterTint();
    gEnvironmentBlend.fogColor = getTargetZoneFogColor();
    gEnvironmentBlend.fogNear = getTargetZoneFogNear();
    gEnvironmentBlend.fogFar = getTargetZoneFogFar();
    gEnvironmentBlend.danger = getTargetWorldDanger();
}

// Smoothly blends the world atmosphere toward the current zone instead
// of instantly snapping fog/water colours when the boat crosses a zone.
void updateEnvironmentBlend(float dt)
{
    const float blendStrength = 1.0f - std::exp(-2.4f * dt);

    gEnvironmentBlend.waterTint = glm::mix(gEnvironmentBlend.waterTint, getTargetZoneWaterTint(), blendStrength);
    gEnvironmentBlend.fogColor = glm::mix(gEnvironmentBlend.fogColor, getTargetZoneFogColor(), blendStrength);
    gEnvironmentBlend.fogNear = glm::mix(gEnvironmentBlend.fogNear, getTargetZoneFogNear(), blendStrength);
    gEnvironmentBlend.fogFar = glm::mix(gEnvironmentBlend.fogFar, getTargetZoneFogFar(), blendStrength);
    gEnvironmentBlend.danger = glm::mix(gEnvironmentBlend.danger, getTargetWorldDanger(), blendStrength);
}

glm::vec3 getZoneWaterTint()
{
    return gEnvironmentBlend.waterTint;
}

glm::vec3 getZoneFogColor()
{
    return gEnvironmentBlend.fogColor;
}

float getZoneFogNear()
{
    return gEnvironmentBlend.fogNear;
}

float getZoneFogFar()
{
    return gEnvironmentBlend.fogFar;
}


float getWorldDanger()
{
    return clampf(gEnvironmentBlend.danger + GetWeatherDangerBonus(), 0.0f, 1.0f);
}

float getQuestFogFactor()
{
    return gIslandQuest.GetLockedFogFactor(boat.position);
}

glm::vec3 getEffectiveFogColor()
{
    glm::vec3 base = glm::mix(getZoneFogColor(), glm::vec3(0.80f, 0.82f, 0.86f), getQuestFogFactor());
    return glm::mix(base, GetWeatherTint(), GetWeatherEffect01() * 0.55f);
}

float getEffectiveFogNear()
{
    return glm::mix(getZoneFogNear() * GetWeatherFogNearMultiplier(), 3.5f, getQuestFogFactor());
}

float getEffectiveFogFar()
{
    return glm::mix(getZoneFogFar() * GetWeatherFogFarMultiplier(), 11.0f, getQuestFogFactor());
}

const char* getShadowFilterLabel()
{
    return gShadowFilterMode == ShadowFilterMode::PCSS ? "PCSS" : "PCF";
}

const char* GetCameraModeLabel()
{
    return gCameraMode == CameraMode::FreeLook ? "FreeLook" : "Follow";
}

const char* getSunCycleLabel()
{
    return gAnimateSunCycle ? "Cycle" : "Fixed";
}

// Calculates the dynamic sun state used by lighting, shadows, water,
// sky colour, and god rays. F10 toggles whether the cycle animates.
SunState EvaluateSunState(float time)
{
    SunState sun;

    if (!gAnimateSunCycle)
    {
        sun.direction = glm::normalize(glm::vec3(0.38f, 0.86f, 0.22f));
        sun.color = glm::vec3(1.0f, 0.96f, 0.90f);
        sun.dayFactor = 1.0f;
        sun.intensity = 1.12f;
        sun.ambient = 0.42f;
        return sun;
    }

    const float cycle = time * 0.16f;
    const float azimuth = cycle * 0.70f + 0.65f;
    const float elevation = 0.16f + 0.84f * (0.5f + 0.5f * std::sin(cycle));
    const float side = 0.28f * std::sin(cycle * 0.47f + 1.1f);

    sun.direction = glm::normalize(glm::vec3(std::cos(azimuth), elevation, std::sin(azimuth) + side));
    sun.dayFactor = saturatef((elevation - 0.16f) / 0.84f);

    const float warmth = 1.0f - sun.dayFactor;
    const glm::vec3 dawn = glm::vec3(1.00f, 0.72f, 0.54f);
    const glm::vec3 noon = glm::vec3(1.00f, 0.97f, 0.90f);
    sun.color = glm::mix(noon, dawn, warmth * 0.85f);
    sun.intensity = glm::mix(0.48f, 1.08f, sun.dayFactor);
    sun.ambient = glm::mix(0.18f, 0.40f, sun.dayFactor);
    return sun;
}

glm::vec3 getCameraTarget()
{
    return boat.position + glm::vec3(0.0f, 0.9f, 0.0f);
}

glm::vec3 getCameraPosition()
{
    glm::vec3 cameraPos(0.0f);

    if (gCameraMode == CameraMode::FreeLook)
    {
        const glm::vec3 target = getCameraTarget();
        const float yaw = glm::radians(boat.rotationY + 180.0f + gFreeLookYawOffset);
        const float pitch = glm::radians(gFreeLookPitch);

        glm::vec3 orbitOffset(
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::cos(yaw) * std::cos(pitch));

        cameraPos = target + orbitOffset * gFreeLookDistance;
    }
    else
    {
        const float radians = glm::radians(boat.rotationY);
        const glm::vec3 behind(-std::sin(radians), 0.0f, -std::cos(radians));
        cameraPos = boat.position + behind * 8.0f + glm::vec3(0.0f, 5.25f, 0.0f);
    }

    if (gCameraShakeTimer > 0.0f)
    {
        const float t = static_cast<float>(glfwGetTime());
        const float fade = clampf(gCameraShakeTimer / std::max(gCameraShakeStrength > 0.0f ? 0.8f : 1.0f, 0.01f), 0.0f, 1.0f);
        cameraPos.x += std::sin(t * 33.0f) * gCameraShakeStrength * 0.12f * fade;
        cameraPos.y += std::cos(t * 41.0f) * gCameraShakeStrength * 0.10f * fade;
        cameraPos.z += std::sin(t * 27.0f) * gCameraShakeStrength * 0.12f * fade;
    }

    return cameraPos;
}

bool isAtDock()
{
    return glm::length(glm::vec2(boat.position.x, boat.position.z)) < 5.5f;
}

int getCargoValue()
{
    int total = 0;
    for (const auto& item : cargo) total += item.fish.value;
    return total;
}

float getHullIntegrity01()
{
    return clampf(hullIntegrity / 100.0f, 0.0f, 1.0f);
}

int getRepairCost()
{
    const float missingHull = std::max(0.0f, 100.0f - hullIntegrity);
    return static_cast<int>(std::ceil(missingHull * 1.4f));
}

void applyHullSpeedPenalty()
{
    const float health01 = getHullIntegrity01();
    const float forwardScale = 0.45f + health01 * 0.55f;
    const float backwardScale = 0.55f + health01 * 0.45f;

    const float weatherSpeed = GetWeatherSpeedMultiplier();
    boat.velocity = glm::clamp(
        boat.velocity,
        -boat.maxBackwardSpeed * backwardScale * weatherSpeed,
        boat.maxForwardSpeed * forwardScale * weatherSpeed);
}

int rodUpgradeCost() { return 20 + (rodLevel - 1) * 15; }
int engineUpgradeCost() { return 25 + (engineLevel - 1) * 20; }
int cargoUpgradeCost() { return 18 + (cargoLevel - 1) * 14; }

void setCatchMessage(GLFWwindow* window, const std::string& message, float timer);
void sellCargo();
void buyRodUpgrade();
void buyEngineUpgrade();
void buyCargoUpgrade();
void repairHull();

struct DockShopFishGroup
{
    std::string name;
    int rarity = 0;
    int value = 0;
    int quantity = 0;
};

std::vector<DockShopFishGroup> BuildDockShopFishGroups()
{
    std::vector<DockShopFishGroup> groups;

    for (const auto& item : cargo)
    {
        bool found = false;
        for (auto& group : groups)
        {
            if (group.name == item.fish.name &&
                group.rarity == item.fish.rarity &&
                group.value == item.fish.value)
            {
                group.quantity += 1;
                found = true;
                break;
            }
        }

        if (!found)
        {
            DockShopFishGroup group;
            group.name = item.fish.name;
            group.rarity = item.fish.rarity;
            group.value = item.fish.value;
            group.quantity = 1;
            groups.push_back(group);
        }
    }

    return groups;
}

int getDockShopFishGroupCount()
{
    return static_cast<int>(BuildDockShopFishGroups().size());
}

int getDockShopSelectionCount()
{
    return getDockShopFishGroupCount() + DOCK_SHOP_ACTION_COUNT;
}

void clampDockShopSelection()
{
    const int total = std::max(1, getDockShopSelectionCount());
    if (gDockShopSelection < 0) gDockShopSelection = 0;
    if (gDockShopSelection >= total) gDockShopSelection = total - 1;
}

void openDockShop(GLFWwindow* window)
{
    if (!isAtDock()) return;

    gDockShopOpen = true;
    shopCloseReady = false; // player must release E before E can close the shop
    gDockShopSelection = 0;
    shopClosePressed = true; // Prevent the same E press that opened the shop from instantly closing it.
    clampDockShopSelection();
    gPaused = false;
    gJournalOpen = false;
    ClearCaughtFishDisplay();
    setCatchMessage(window, "Dock shop opened", 1.0f);
}

void closeDockShop(GLFWwindow* window)
{
    if (!gDockShopOpen) return;
    gDockShopOpen = false;
    setCatchMessage(window, "Dock shop closed", 0.9f);
}

void sellCargoItem(size_t index)
{
    if (!isAtDock() || index >= cargo.size()) return;

    const FishData fish = cargo[index].fish;
    totalMoney += fish.value;
    gTotalGoldEarned += fish.value;
    gTotalCargoSold += 1;
    cargo.erase(cargo.begin() + static_cast<std::ptrdiff_t>(index));

    ClearCaughtFishDisplay();
    lastCatchText = "Sold " + fish.name + " for " + std::to_string(fish.value) + "g";
    catchMessageTimer = 2.0f;
    TriggerCatchBanner("FISH SOLD", 0.9f);
    clampDockShopSelection();
}

void sellCargoGroupAt(int groupIndex)
{
    const std::vector<DockShopFishGroup> groups = BuildDockShopFishGroups();
    if (groupIndex < 0 || groupIndex >= static_cast<int>(groups.size())) return;

    const DockShopFishGroup& group = groups[groupIndex];
    for (size_t i = 0; i < cargo.size(); ++i)
    {
        const FishData& fish = cargo[i].fish;
        if (fish.name == group.name && fish.rarity == group.rarity && fish.value == group.value)
        {
            sellCargoItem(i);
            return;
        }
    }
}

void activateDockShopSelection(GLFWwindow* window)
{
    clampDockShopSelection();
    const int groupCount = getDockShopFishGroupCount();
    if (gDockShopSelection < groupCount)
    {
        sellCargoGroupAt(gDockShopSelection);
        return;
    }

    const DockShopAction action = static_cast<DockShopAction>(gDockShopSelection - groupCount);
    switch (action)
    {
    case DockShopAction::SellAll:
        sellCargo();
        gDockShopSelection = 0;
        break;
    case DockShopAction::UpgradeRod:
        buyRodUpgrade();
        break;
    case DockShopAction::UpgradeEngine:
        buyEngineUpgrade();
        break;
    case DockShopAction::UpgradeCargo:
        buyCargoUpgrade();
        break;
    case DockShopAction::RepairHull:
        repairHull();
        break;
    }
    clampDockShopSelection();
}

std::vector<HUDShopItem> BuildDockShopItems()
{
    std::vector<HUDShopItem> items;
    const std::vector<DockShopFishGroup> groups = BuildDockShopFishGroups();
    items.reserve(groups.size());

    for (const auto& group : groups)
    {
        HUDShopItem hudItem;
        hudItem.name = group.name;
        hudItem.rarity = group.rarity;
        hudItem.value = group.value;
        hudItem.quantity = group.quantity;
        const FishIconTexture& icon = getFishIconTexture(group.name);
        hudItem.texture = icon.texture;
        hudItem.textureWidth = icon.width;
        hudItem.textureHeight = icon.height;
        hudItem.textureMissing = !icon.found;
        items.push_back(hudItem);
    }
    return items;
}

std::vector<HUDShopAction> BuildDockShopActions()
{
    std::vector<HUDShopAction> actions;
    actions.reserve(DOCK_SHOP_ACTION_COUNT);

    HUDShopAction sellAll;
    sellAll.label = "SELL ALL";
    sellAll.valueText = cargo.empty() ? "EMPTY HOLD" : (std::to_string(getCargoValue()) + "G TOTAL");
    sellAll.enabled = !cargo.empty();
    actions.push_back(sellAll);

    HUDShopAction rod;
    rod.label = "ROD";
    rod.valueText = "LV " + std::to_string(rodLevel) + " -> " + std::to_string(rodLevel + 1) + "  " + std::to_string(rodUpgradeCost()) + "G";
    rod.affordable = totalMoney >= rodUpgradeCost();
    actions.push_back(rod);

    HUDShopAction engine;
    engine.label = "ENGINE";
    engine.valueText = "LV " + std::to_string(engineLevel) + " -> " + std::to_string(engineLevel + 1) + "  " + std::to_string(engineUpgradeCost()) + "G";
    engine.affordable = totalMoney >= engineUpgradeCost();
    actions.push_back(engine);

    HUDShopAction cargoAction;
    cargoAction.label = "CARGO";
    cargoAction.valueText = std::to_string(cargoCapacity) + " -> " + std::to_string(cargoCapacity + 2) + "  " + std::to_string(cargoUpgradeCost()) + "G";
    cargoAction.affordable = totalMoney >= cargoUpgradeCost();
    actions.push_back(cargoAction);

    HUDShopAction repair;
    repair.label = "REPAIR";
    repair.enabled = hullIntegrity < 99.9f;
    repair.valueText = repair.enabled ? (std::to_string(getRepairCost()) + "G") : std::string("HULL FULL");
    repair.affordable = !repair.enabled || totalMoney >= getRepairCost();
    actions.push_back(repair);

    return actions;
}


int CountCargoFishByName(const std::string& fishName)
{
    int count = 0;
    for (const auto& item : cargo)
    {
        if (item.fish.name == fishName)
            count += 1;
    }
    return count;
}

bool RemoveCargoFishByName(const std::string& fishName, int quantity)
{
    int removed = 0;
    for (auto it = cargo.begin(); it != cargo.end() && removed < quantity; )
    {
        if (it->fish.name == fishName)
        {
            it = cargo.erase(it);
            removed += 1;
        }
        else
        {
            ++it;
        }
    }
    return removed == quantity;
}

void InitialiseShopkeeperContracts()
{
    gShopkeeperContracts.clear();
    gShopkeeperContracts.push_back({ "Sardine", 2, 30, false });
    gShopkeeperContracts.push_back({ "Lionfish", 1, 65, false });
    gShopkeeperContracts.push_back({ "Ghost Fish", 1, 150, false });
}

int GetReadyContractCount()
{
    int ready = 0;
    for (const auto& contract : gShopkeeperContracts)
    {
        if (!contract.completed && CountCargoFishByName(contract.fishName) >= contract.quantity)
            ready += 1;
    }
    return ready;
}

int GetCompletedContractCount()
{
    int done = 0;
    for (const auto& contract : gShopkeeperContracts)
    {
        if (contract.completed)
            done += 1;
    }
    return done;
}

std::vector<HUDContractEntry> BuildContractHUDEntries()
{
    std::vector<HUDContractEntry> entries;
    entries.reserve(gShopkeeperContracts.size());

    for (const auto& contract : gShopkeeperContracts)
    {
        HUDContractEntry entry;
        entry.title = contract.fishName + " ORDER";
        entry.requirement = "BRING " + contract.fishName + " X" + std::to_string(contract.quantity);
        const int current = CountCargoFishByName(contract.fishName);
        entry.progress = contract.completed ? "DONE" : (std::to_string(std::min(current, contract.quantity)) + "/" + std::to_string(contract.quantity));
        entry.reward = contract.reward;
        entry.ready = !contract.completed && current >= contract.quantity;
        entry.completed = contract.completed;
        entries.push_back(entry);
    }

    return entries;
}

void TurnInReadyContracts(GLFWwindow* window)
{
    if (!isAtDock())
    {
        setCatchMessage(window, "Return to the dock to claim contracts", 1.4f);
        return;
    }

    int completedNow = 0;
    int rewardTotal = 0;
    for (auto& contract : gShopkeeperContracts)
    {
        if (!contract.completed && CountCargoFishByName(contract.fishName) >= contract.quantity)
        {
            if (RemoveCargoFishByName(contract.fishName, contract.quantity))
            {
                contract.completed = true;
                completedNow += 1;
                rewardTotal += contract.reward;
            }
        }
    }

    if (completedNow > 0)
    {
        totalMoney += rewardTotal;
        gTotalGoldEarned += rewardTotal;
        gTotalCargoSold += completedNow;
        ClearCaughtFishDisplay();
        lastCatchText = "Completed " + std::to_string(completedNow) + " contract(s) for " + std::to_string(rewardTotal) + "g";
        catchMessageTimer = 2.8f;
        TriggerCatchBanner("CONTRACT COMPLETE", 1.3f);
        SpawnSplash(particles, glm::vec3(0.0f, 0.35f, 1.2f), glm::vec4(1.0f, 0.86f, 0.35f, 1.0f), 22);
    }
    else
    {
        setCatchMessage(window, "No contracts ready yet", 1.5f);
    }
}

int GetDockShopSelectionFromMouse(double mouseX, double mouseY)
{
    const float sw = static_cast<float>(gWindowWidth);
    const float sh = static_cast<float>(gWindowHeight);
    const float pad = 22.0f;
    const float x = 54.0f;
    const float y = 42.0f;
    const float w = sw - 108.0f;
    const float portraitW = 220.0f;
    const float cargoX = x + pad;
    const float cargoY = y + 114.0f;
    const float cargoW = w - portraitW - pad * 3.0f;
    const float cargoH = 348.0f;
    const int itemsPerPage = 8;
    const int groupCount = getDockShopFishGroupCount();

    int visiblePage = 0;
    if (groupCount > 0)
    {
        if (gDockShopSelection < groupCount)
            visiblePage = std::max(0, gDockShopSelection / itemsPerPage);
        else
            visiblePage = std::max(0, (groupCount - 1) / itemsPerPage);
    }

    const int pageStart = visiblePage * itemsPerPage;
    const int pageEnd = std::min(pageStart + itemsPerPage, groupCount);
    const float gridStartY = cargoY + 44.0f;
    const float cardGap = 14.0f;
    const float cardW = (cargoW - 14.0f * 2.0f - cardGap) * 0.5f;
    const float cardH = 134.0f;

    for (int i = pageStart; i < pageEnd; ++i)
    {
        const int local = i - pageStart;
        const int col = local % 2;
        const int row = local / 2;
        const float cx = cargoX + 14.0f + col * (cardW + cardGap);
        const float cy = gridStartY + row * (cardH + cardGap);
        if (mouseX >= cx && mouseX <= cx + cardW && mouseY >= cy && mouseY <= cy + cardH)
            return i;
    }

    const float actionTitleY = cargoY + cargoH + 18.0f;
    const float actionsY = actionTitleY + 34.0f;
    const float actionsGap = 12.0f;
    const float actionW = (w - pad * 2.0f - actionsGap * 4.0f) / 5.0f;
    const float actionH = 96.0f;

    for (int i = 0; i < DOCK_SHOP_ACTION_COUNT; ++i)
    {
        const float ax = x + pad + i * (actionW + actionsGap);
        const float ay = actionsY;
        if (mouseX >= ax && mouseX <= ax + actionW && mouseY >= ay && mouseY <= ay + actionH)
            return groupCount + i;
    }

    return -1;
}

FishData catchFishFromZone(const FishZone& zone)
{
    int roll = rand() % 100;
    roll -= (rodLevel - 1) * 6;

    if (zone.danger < 0.3f)
    {
        roll += 12;
    }
    else if (zone.danger > 0.7f)
    {
        roll -= 18;
    }
    else if (zone.danger > 0.4f)
    {
        roll -= 8;
    }

    std::vector<FishData> common, uncommon, rare, legendary;
    for (const auto& fish : zone.fishPool)
    {
        if (fish.rarity == 0) common.push_back(fish);
        else if (fish.rarity == 1) uncommon.push_back(fish);
        else if (fish.rarity == 2) rare.push_back(fish);
        else legendary.push_back(fish);
    }

    if (zone.danger < 0.3f && !common.empty() && roll < 58) return common[rand() % common.size()];
    if (zone.danger < 0.3f && !uncommon.empty() && roll < 88) return uncommon[rand() % uncommon.size()];

    if (zone.danger > 0.7f && !rare.empty() && roll < 78) return rare[rand() % rare.size()];
    if (zone.danger > 0.7f && !legendary.empty() && roll < 100) return legendary[rand() % legendary.size()];

    if (roll < 48 && !common.empty()) return common[rand() % common.size()];
    if (roll < 78 && !uncommon.empty()) return uncommon[rand() % uncommon.size()];
    if (roll < 95 && !rare.empty()) return rare[rand() % rare.size()];
    if (!legendary.empty()) return legendary[rand() % legendary.size()];
    return zone.fishPool[rand() % zone.fishPool.size()];
}


void setCatchMessage(GLFWwindow* window, const std::string& message, float timer = 2.5f)
{
    ClearCaughtFishDisplay();
    lastCatchText = message;
    catchMessageTimer = timer;
    std::cout << "[Fish] " << lastCatchText << std::endl;
    if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
}

void breakBoatAndTowToDock(GLFWwindow* window)
{
    const int lostCargo = cargo.empty() ? 0 : std::max(1, static_cast<int>(cargo.size()) / 2);
    if (lostCargo > 0)
    {
        cargo.erase(cargo.end() - lostCargo, cargo.end());
    }

    boat.position = glm::vec3(0.0f, 0.18f, 0.0f);
    boat.velocity = 0.0f;
    hullIntegrity = 40.0f;
    gFishingMinigame.Cancel();
    gHasPendingMinigameFish = false;
    gFishingMinigameZoneIndex = -1;

    catchFlashTimer = 1.0f;
    hullWarningTimer = 4.0f;
    TriggerCatchBanner("HULL BREACHED", 1.8f, 1.1f);
    setCatchMessage(window, lostCargo > 0 ? "HULL BREACHED - TOWED TO DOCK - LOST CARGO" : "HULL BREACHED - TOWED TO DOCK", 3.5f);
    SpawnSplash(particles, glm::vec3(0.0f, 0.25f, 0.0f), glm::vec4(1.0f, 0.74f, 0.30f, 0.95f), 22);
    gSound.PlaySplash();
}


void handleHarbourMissionEvent(GLFWwindow* window, const HarbourMissionEvent& event)
{
    if (event.type == HarbourMissionEventType::None)
    {
        return;
    }

    ClearCaughtFishDisplay();

    if (event.type == HarbourMissionEventType::PartCollected)
    {
        catchFlashTimer = std::max(catchFlashTimer, 0.18f);
        TriggerCatchBanner("SALVAGE RECOVERED", 1.15f, 0.15f);
        setCatchMessage(window,
            event.message + " (" + std::to_string(event.collectedParts) + "/" + std::to_string(event.totalParts) + ")",
            2.2f);
        SpawnSplash(particles, event.position + glm::vec3(0.0f, 0.15f, 0.0f), glm::vec4(0.55f, 0.90f, 1.0f, 0.90f), 14);
        gSound.PlaySplash();
        return;
    }

    if (event.type == HarbourMissionEventType::Delivered)
    {
        gSound.StopPatrolHornLoop();

        const int rewardGold = 45;
        totalMoney += rewardGold;
        hullIntegrity = std::min(100.0f, hullIntegrity + 25.0f);
        catchFlashTimer = std::max(catchFlashTimer, 0.35f);
        TriggerCatchBanner("HARBOUR REPAIRED", 1.8f, 0.25f);
        setCatchMessage(window, "Harbour mission complete: +" + std::to_string(rewardGold) + "g and hull repaired", 3.0f);
        SpawnSplash(particles, glm::vec3(0.0f, 0.35f, 0.0f), glm::vec4(0.95f, 0.86f, 0.35f, 0.95f), 28);
        gSound.PlaySplash();
        return;
    }

    if (event.type == HarbourMissionEventType::PlayerSpotted)
    {
        hullWarningTimer = std::max(hullWarningTimer, 1.2f);
        TriggerCatchBanner("PATROL SPOTTED YOU", 1.0f, 0.35f);
        setCatchMessage(window, event.message + " - evade!", 1.8f);
        return;
    }

    if (event.type == HarbourMissionEventType::PlayerLost)
    {
        setCatchMessage(window, event.message, 1.3f);
        return;
    }

    if (event.type == HarbourMissionEventType::PlayerCaught)
    {
        hullIntegrity -= 22.0f;
        engineStutterTimer = std::max(engineStutterTimer, 1.8f);
        hullWarningTimer = std::max(hullWarningTimer, 3.0f);
        catchFlashTimer = std::max(catchFlashTimer, 0.4f);
        TriggerCatchBanner("PATROL HIT", 1.2f, 0.65f);
        SpawnSplash(particles, event.position + glm::vec3(0.0f, 0.2f, 0.0f), glm::vec4(1.0f, 0.58f, 0.22f, 0.95f), 22);
        gSound.PlaySplash();

        if (hullIntegrity <= 0.0f)
        {
            breakBoatAndTowToDock(window);
        }
        else
        {
            setCatchMessage(window, "Patrol rammed you - hull damaged", 2.3f);
        }
        return;
    }
}

void triggerWinCelebration()
{
    if (gWinCelebrationTriggered || !gIslandQuest.HasWon()) return;

    gWinCelebrationTriggered = true;
    catchFlashTimer = 1.2f;
    TriggerCatchBanner("LOST ISLAND REACHED", 2.6f, 0.8f);

    const glm::vec3 goalPos = gIslandQuest.GetGoalIslandPosition();
    SpawnSplash(particles, goalPos + glm::vec3(0.0f, 1.7f, 0.0f), glm::vec4(1.0f, 0.86f, 0.35f, 0.95f), 36);
    SpawnSplash(particles, goalPos + glm::vec3(0.0f, 1.1f, 0.0f), glm::vec4(0.80f, 0.92f, 1.0f, 0.90f), 28);
    gSound.PlaySplash();
}

// Applies danger-zone gameplay effects such as hull damage, warning
// feedback, and forced recovery if the boat is destroyed.
void updateDangerGameplay(GLFWwindow* window, float dt)
{
    if (hullWarningTimer > 0.0f) hullWarningTimer -= dt;
    if (engineStutterTimer > 0.0f) engineStutterTimer -= dt;

    if (isAtDock())
    {
        applyHullSpeedPenalty();
        return;
    }

    const float danger = getWorldDanger();
    const float speed01 = glm::clamp(std::abs(boat.velocity) / std::max(boat.maxForwardSpeed, 0.01f), 0.0f, 1.0f);

    if (danger >= 0.35f)
    {
        const float damageRate = std::max(0.0f, danger - 0.30f) * (0.65f + speed01 * 0.95f) * 2.6f;
        hullIntegrity = std::max(0.0f, hullIntegrity - damageRate * dt);

        if (danger > 0.55f && hullWarningTimer <= 0.0f && speed01 > 0.18f)
        {
            setCatchMessage(window, hullIntegrity < 35.0f ? "HULL CRITICAL - REPAIR AT DOCK" : "ROUGH WATER - HULL TAKING DAMAGE", 1.4f);
            TriggerCatchBanner(hullIntegrity < 35.0f ? "HULL CRITICAL" : "ROUGH WATER", 1.4f, 0.7f);
            catchFlashTimer = std::max(catchFlashTimer, 0.35f);
            hullWarningTimer = hullIntegrity < 35.0f ? 2.8f : 4.5f;
        }
    }

    if (hullIntegrity < 55.0f && danger > 0.45f && engineStutterTimer <= 0.0f && std::abs(boat.velocity) > 0.5f)
    {
        const float healthPenalty = 1.0f - getHullIntegrity01();
        const float stutterChancePerSecond = (danger - 0.35f) * 0.85f + healthPenalty * 0.65f;
        const float triggerChance = clampf(stutterChancePerSecond * dt, 0.0f, 0.35f);
        const float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        if (roll < triggerChance)
        {
            boat.velocity *= 0.55f;
            engineStutterTimer = 1.1f + healthPenalty * 1.1f;
            setCatchMessage(window, "ENGINE STUTTER - HULL TOO DAMAGED", 1.2f);
            TriggerCatchBanner("ENGINE STUTTER", 1.0f, 0.6f);
        }
    }

    applyHullSpeedPenalty();

    if (hullIntegrity <= 0.0f)
    {
        breakBoatAndTowToDock(window);
    }
}

bool canStartFishing(std::string& failMessage)
{
    if (fishCooldown > 0.0f)
    {
        failMessage = "Line is not ready yet";
        return false;
    }

    const FishZone* zone = getCurrentZone();
    if (!zone)
    {
        failMessage = "No fish here";
        return false;
    }

    if (static_cast<int>(cargo.size()) >= cargoCapacity)
    {
        failMessage = "Cargo full - return to dock to sell";
        return false;
    }

    return true;
}

int getCurrentZoneIndex()
{
    for (size_t i = 0; i < zones.size(); ++i)
    {
        if (zones[i].contains(boat.position)) return static_cast<int>(i);
    }
    return -1;
}

void awardFishCatch(GLFWwindow* window, const FishData& fish, float timingScore = 0.0f)
{
    cargo.push_back({ fish });
    RecordFishCaught(fish);
    gTotalFishCaught += 1;
    if (timingScore > 0.95f) gPerfectHooks += 1;

    std::stringstream ss;
    if (fish.rarity == 3) { ss << "LEGENDARY CATCH! "; catchFlashTimer = 0.9f; }
    else if (fish.rarity == 2) { ss << "RARE CATCH! "; catchFlashTimer = 0.5f; }
    else if (fish.rarity == 1) { ss << "UNCOMMON CATCH! "; catchFlashTimer = std::max(catchFlashTimer, 0.18f); }

    if (timingScore > 0.95f)
        ss << "PERFECT HOOK! ";
    else if (timingScore > 0.70f)
        ss << "GOOD HOOK! ";

    ss << fish.name
       << " | Value: " << fish.value << "g"
       << " | Cargo: " << cargo.size() << "/" << cargoCapacity;

    lastCatchText = ss.str();
    catchMessageTimer = timingScore > 0.95f ? 3.8f : 3.0f;
    ShowCaughtFishDisplay(fish);
    gLastCaughtFishRarity = fish.rarity;

    if (timingScore > 0.95f)
        TriggerCatchBanner("PERFECT HOOK", 1.6f, 0.45f);
    else if (fish.rarity == 3)
        TriggerCatchBanner("LEGENDARY CATCH", 1.9f, 0.9f);
    else if (fish.rarity == 2)
        TriggerCatchBanner("RARE CATCH", 1.5f, 0.65f);

    std::cout << "[Catch] " << fish.name
              << " | Rarity: " << fish.rarity
              << " | Value: " << fish.value << "g"
              << " | Cargo: " << cargo.size() << "/" << cargoCapacity
              << std::endl;

    if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
    const glm::vec3 zoneTint = getZoneWaterTint();
    glm::vec4 splashColor(zoneTint.r + 0.35f, zoneTint.g + 0.28f, zoneTint.b + 0.26f, 0.90f);
    SpawnSplash(particles, boat.position + glm::vec3(0.0f, 0.1f, 1.8f), splashColor, fish.rarity >= 2 ? 16 : 12);
    gSound.PlaySplash();
    if (fish.rarity >= 1) gSound.PlayReel();
    if (fish.rarity == 3) gSound.PlaySplash();
}

FishData catchFishForMinigameResult(FishData fish, float timingScore)
{
    if (timingScore > 0.94f && fish.rarity < 3)
    {
        fish.rarity += 1;
        fish.value = static_cast<int>(std::round(fish.value * 1.35f));
    }
    else if (timingScore > 0.75f)
    {
        fish.value = static_cast<int>(std::round(fish.value * 1.15f));
    }

    return fish;
}

// Attempts to start/carry out fishing depending on the player's zone,
// cooldown, cargo space, rod level, and minigame setting.
void tryFishing(GLFWwindow* window)
{
    std::string failMessage;
    if (!canStartFishing(failMessage))
    {
        setCatchMessage(window, failMessage, 2.3f);
        return;
    }

    const FishZone* zone = getCurrentZone();
    if (!zone)
    {
        setCatchMessage(window, "No fish here", 2.0f);
        return;
    }

    if (!gFishingMinigame.IsEnabled())
    {
        fishCooldown = 1.0f;

        const int zoneIndex = getCurrentZoneIndex();
        if (gIslandQuest.TryCollectKeyForZone(zoneIndex, rodLevel, lastCatchText, catchMessageTimer))
        {
            ClearCaughtFishDisplay();
            if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
            SpawnSplash(particles, boat.position + glm::vec3(0.0f, 0.1f, 1.8f), glm::vec4(0.95f, 0.86f, 0.30f, 0.95f), 16);
            gSound.PlaySplash();
            return;
        }

        FishData fish = catchFishFromZone(*zone);
        awardFishCatch(window, fish, 0.0f);
        return;
    }

    if (gFishingMinigame.IsActive())
        return;

    gFishingMinigameZoneIndex = getCurrentZoneIndex();
    gPendingMinigameFish = catchFishFromZone(*zone);
    gHasPendingMinigameFish = true;
    gFishingMinigame.Start(rodLevel, gPendingMinigameFish.rarity);
    fishCooldown = 0.2f;
    lastCatchText = gFishingMinigame.GetStatusText();
    catchMessageTimer = 10.0f;
}

void resolveFishingMinigame(GLFWwindow* window, const FishingMinigameResult& result)
{
    fishCooldown = result.success ? 0.8f : 0.5f;

    if (!result.success)
    {
        gHasPendingMinigameFish = false;
        catchFlashTimer = 0.0f;
        setCatchMessage(window, result.message, 2.0f);
        return;
    }

    int idx = gFishingMinigameZoneIndex;
    if (idx < 0 || idx >= static_cast<int>(zones.size()))
    {
        gHasPendingMinigameFish = false;
        setCatchMessage(window, "The fish got away", 2.0f);
        return;
    }

    gSound.PlayReel();

    if (gIslandQuest.TryCollectKeyForZone(idx, rodLevel, lastCatchText, catchMessageTimer))
    {
        gHasPendingMinigameFish = false;
        ClearCaughtFishDisplay();
        if (window) glfwSetWindowTitle(window, lastCatchText.c_str());
        SpawnSplash(particles, boat.position + glm::vec3(0.0f, 0.1f, 1.8f), glm::vec4(0.95f, 0.86f, 0.30f, 0.95f), 18);
        gSound.PlaySplash();
        return;
    }

    FishData fish = gHasPendingMinigameFish ? gPendingMinigameFish : catchFishFromZone(zones[idx]);
    gHasPendingMinigameFish = false;
    fish = catchFishForMinigameResult(fish, result.timingScore);
    awardFishCatch(window, fish, result.timingScore);
}

// Converts stored fish into gold when the player is at the dock.
void sellCargo()
{
    if (!isAtDock() || cargo.empty()) return;
    int soldValue = getCargoValue();
    const int soldCount = static_cast<int>(cargo.size());
    totalMoney += soldValue;
    gTotalGoldEarned += soldValue;
    gTotalCargoSold += soldCount;
    cargo.clear();
    ClearCaughtFishDisplay();
    lastCatchText = "Sold cargo for " + std::to_string(soldValue) + "g";
    catchMessageTimer = 2.5f;
    TriggerCatchBanner("CARGO SOLD", 1.1f);
    std::cout << "[Dock] " << lastCatchText << " | Gold: " << totalMoney << std::endl;
    SpawnSplash(particles, glm::vec3(0.0f, 0.3f, 1.2f), glm::vec4(1.0f, 0.86f, 0.35f, 1.0f), 18);
}

void buyRodUpgrade()
{
    int cost = rodUpgradeCost();
    if (!isAtDock() || totalMoney < cost) return;
    totalMoney -= cost;
    ClearCaughtFishDisplay();
    rodLevel++;
    lastCatchText = "Bought rod upgrade for " + std::to_string(cost) + "g";
    catchMessageTimer = 2.5f;
    std::cout << "[Dock] " << lastCatchText << " | Rod: " << rodLevel << std::endl;
    TriggerCatchBanner("ROD UPGRADED", 1.0f);
}

void buyEngineUpgrade()
{
    int cost = engineUpgradeCost();
    if (!isAtDock() || totalMoney < cost) return;
    totalMoney -= cost;
    ClearCaughtFishDisplay();
    engineLevel++;
    boat.maxForwardSpeed += 1.2f;
    boat.acceleration += 0.35f;
    lastCatchText = "Bought engine upgrade for " + std::to_string(cost) + "g";
    catchMessageTimer = 2.5f;
    std::cout << "[Dock] " << lastCatchText << " | Engine: " << engineLevel << std::endl;
    TriggerCatchBanner("ENGINE UPGRADED", 1.0f);
}

void buyCargoUpgrade()
{
    int cost = cargoUpgradeCost();
    if (!isAtDock() || totalMoney < cost) return;
    totalMoney -= cost;
    ClearCaughtFishDisplay();
    cargoLevel++;
    cargoCapacity += 2;
    lastCatchText = "Bought cargo upgrade for " + std::to_string(cost) + "g";
    catchMessageTimer = 2.5f;
    std::cout << "[Dock] " << lastCatchText << " | Capacity: " << cargoCapacity << std::endl;
    TriggerCatchBanner("CARGO EXPANDED", 1.0f);
}

void repairHull()
{
    if (!isAtDock()) return;
    if (hullIntegrity >= 99.9f) return;

    const int cost = getRepairCost();
    if (totalMoney < cost) return;

    totalMoney -= cost;
    hullIntegrity = 100.0f;
    engineStutterTimer = 0.0f;
    hullWarningTimer = 0.0f;
    ClearCaughtFishDisplay();
    lastCatchText = "Repaired hull for " + std::to_string(cost) + "g";
    catchMessageTimer = 2.5f;
    catchFlashTimer = std::max(catchFlashTimer, 0.25f);
    std::cout << "[Dock] " << lastCatchText << " | Hull: " << hullIntegrity << std::endl;
    SpawnSplash(particles, glm::vec3(0.0f, 0.3f, 1.2f), glm::vec4(0.72f, 0.92f, 1.0f, 0.95f), 14);
    gSound.PlaySplash();
    TriggerCatchBanner("HULL REPAIRED", 1.1f);
}

// Material binding helper for the main world shader. The materialType
// uniform lets the shader apply different responses for wood, rock,
// grass, boat, buoy/markers, and emissive-style light objects.
void bindMaterial(GLuint shader, GLuint textureID, int materialType, float tiling)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    setInt(shader, "materialTex", 0);
    setInt(shader, "materialType", materialType);
    setFloat(shader, "uvTiling", tiling);
}

void drawCube(GLuint shader, const glm::mat4& model)
{
    setMat4(shader, "model", model);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawGeneratedMesh(GLuint shader, const GeneratedMesh& mesh, const glm::mat4& model)
{
    if (!mesh.ready || mesh.vao == 0 || mesh.vertexCount == 0)
    {
        return;
    }

    setMat4(shader, "model", model);
    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertexCount));
    glBindVertexArray(0);
}

void drawPlane(GLuint shader, const glm::mat4& model)
{
    setMat4(shader, "model", model);
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawModel(GLuint shader, const ModelMesh& modelMesh, const glm::mat4& model)
{
    if (!modelMesh.loaded)
        return;

    setMat4(shader, "model", model);
    glBindVertexArray(modelMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(modelMesh.vertexCount));
    glBindVertexArray(0);
}

// Builds a transform for small collectible OBJ props. The mesh is centred on
// X/Z, grounded on its lowest Y vertex, and uniformly scaled to a target size.
// This means media/models/scrap.obj can be swapped for a different scrap model
// without manually re-tuning every mission-part position.
glm::mat4 buildGroundedModelTransform(const ModelMesh& modelMesh,
                                      const glm::vec3& worldPosition,
                                      float targetMaxDimension,
                                      float yawRadians)
{
    const glm::vec3 boundsSize = modelMesh.maxBounds - modelMesh.minBounds;
    const float maxDimension = std::max(boundsSize.x, std::max(boundsSize.y, boundsSize.z));
    const float safeDimension = std::max(maxDimension, 0.001f);
    const float uniformScale = targetMaxDimension / safeDimension;

    const glm::vec3 modelCenter(
        (modelMesh.minBounds.x + modelMesh.maxBounds.x) * 0.5f,
        modelMesh.minBounds.y,
        (modelMesh.minBounds.z + modelMesh.maxBounds.z) * 0.5f);

    glm::mat4 model(1.0f);
    model = glm::translate(model, worldPosition);
    model = glm::rotate(model, yawRadians, glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(uniformScale));
    model = glm::translate(model, glm::vec3(-modelCenter.x, -modelMesh.minBounds.y, -modelCenter.z));
    return model;
}

// Renders dynamic world geometry that still lives in main.cpp.
// Static set dressing is handled by WorldRenderer.cpp, while the boat
// is kept here because it depends on movement, cargo, and water bobbing.
void renderWorldGeometry(GLuint shader, bool depthPass, float time)
{
    auto setMat = [&](GLuint tex, int type, float tiling)
    {
        if (!depthPass) bindMaterial(shader, tex, type, tiling);
    };

    RenderStaticWorldGeometry(shader, depthPass, time);

    float centerH = getWaterHeight(boat.position.x, boat.position.z, time);
    float frontH  = getWaterHeight(boat.position.x, boat.position.z + 0.75f, time);
    float backH   = getWaterHeight(boat.position.x, boat.position.z - 0.75f, time);
    float leftH   = getWaterHeight(boat.position.x - 0.45f, boat.position.z, time);
    float rightH  = getWaterHeight(boat.position.x + 0.45f, boat.position.z, time);

    float targetPitch = std::atan2(frontH - backH, 1.5f);
    float targetRoll  = std::atan2(rightH - leftH, 0.9f);

    // make the boat feel heavier than the water
    targetPitch *= 0.10f;
    targetRoll  *= 0.10f;

    // keep only a very small movement-based lean
    float movementRoll = glm::radians(boat.velocity * 0.15f);

    // smooth the visual motion so the boat sits on the water rather than snapping to every wave
    boatVisualY     = glm::mix(boatVisualY, centerH + 0.08f, 0.02f);
    boatVisualPitch = glm::mix(boatVisualPitch, targetPitch, 0.04f);
    boatVisualRoll  = glm::mix(boatVisualRoll, -targetRoll + movementRoll, 0.04f);

    glm::mat4 boatBase(1.0f);
    boatBase = glm::translate(boatBase, glm::vec3(boat.position.x, boatVisualY, boat.position.z));
    boatBase = glm::rotate(boatBase, glm::radians(boat.rotationY), glm::vec3(0, 1, 0));
    boatBase = glm::rotate(boatBase, boatVisualPitch, glm::vec3(1, 0, 0));
    boatBase = glm::rotate(boatBase, boatVisualRoll, glm::vec3(0, 0, 1));

    if (boatModel.loaded)
    {
        setMat(boatTex, MAT_BOAT, 1.0f);

        glm::mat4 boatModelMatrix = boatBase;
        boatModelMatrix = glm::translate(boatModelMatrix, glm::vec3(0.0f, -0.06f, 0.0f));
        boatModelMatrix = glm::rotate(boatModelMatrix, glm::radians(180.0f), glm::vec3(0, 1, 0));
        boatModelMatrix = glm::scale(boatModelMatrix, glm::vec3(0.40f, 0.40f, 0.40f));

        drawModel(shader, boatModel, boatModelMatrix);
    }
    else
    {
        setMat(boatTex, MAT_BOAT, 1.6f);
        glm::mat4 hull = glm::scale(boatBase, glm::vec3(1.25f, 0.4f, 2.35f));
        drawCube(shader, hull);

        setMat(woodTex, MAT_WOOD, 1.3f);
        glm::mat4 cabin = glm::translate(boatBase, glm::vec3(0.0f, 0.48f, -0.1f));
        cabin = glm::scale(cabin, glm::vec3(0.8f, 0.55f, 0.95f));
        drawCube(shader, cabin);

        glm::mat4 mast = glm::translate(boatBase, glm::vec3(0.0f, 1.18f, 0.0f));
        mast = glm::scale(mast, glm::vec3(0.09f, 1.7f, 0.09f));
        drawCube(shader, mast);
    }

    // Cargo crates on boat
    int shownCrates = std::min(static_cast<int>(cargo.size()), 4);
    for (int i = 0; i < shownCrates; ++i)
    {
        setMat(woodTex, MAT_WOOD, 1.0f);
        glm::mat4 crate = boatBase;

        float xoff = 0.0f;
        float zoff = 0.0f;
        float yoff = 0.52f;

        switch (i)
        {
        case 0:
            xoff = -0.10f; zoff = -0.10f;
            break;
        case 1:
            xoff = 0.10f; zoff = -0.10f;
            break;
        case 2:
            xoff = -0.10f; zoff = 0.08f;
            break;
        case 3:
            xoff = 0.10f; zoff = 0.08f;
            break;
        }

        crate = glm::translate(crate, glm::vec3(xoff, yoff, zoff));
        crate = glm::scale(crate, glm::vec3(0.08f, 0.08f, 0.08f));
        drawCube(shader, crate);
    }

    // Side-mission salvage parts: bobbing crates/buoys that the player can
    // collect with Q. These render in both the depth pass and the lit pass so
    // they cast/receive the same shadows as the rest of the scene.
    if (!gHarbourMission.IsComplete())
    {
        int partIndex = 0;
        for (const HarbourMissionPart& part : gHarbourMission.GetParts())
        {
            if (part.collected)
            {
                ++partIndex;
                continue;
            }

            const float bob = std::sin(time * 2.2f + static_cast<float>(partIndex) * 1.3f) * 0.08f;
            const float spin = time * 0.65f + static_cast<float>(partIndex);

            if (scrapModel.loaded)
            {
                // Mission collectibles now use media/models/scrap.obj when it is available.
                // The transform helper auto-centres and auto-scales the mesh so it behaves
                // like a compact floating salvage object regardless of the OBJ's original size.
                glm::mat4 scrapPiece = buildGroundedModelTransform(
                    scrapModel,
                    glm::vec3(part.position.x, part.position.y + bob - 0.08f, part.position.z),
                    0.92f,
                    spin);

                setMat(scrapTex != 0 ? scrapTex : woodTex, MAT_WOOD, 1.0f);
                drawModel(shader, scrapModel, scrapPiece);
            }
            else
            {
                // Fallback if media/models/scrap.obj is missing: keep the old crate/buoy cube.
                glm::mat4 partModel(1.0f);
                partModel = glm::translate(partModel, glm::vec3(part.position.x, part.position.y + bob, part.position.z));
                partModel = glm::rotate(partModel, spin, glm::vec3(0, 1, 0));
                partModel = glm::scale(partModel, glm::vec3(0.72f, 0.42f, 0.72f));

                setMat(partIndex == 0 ? buoyTex : woodTex, partIndex == 0 ? MAT_BOAT : MAT_WOOD, 1.0f);
                drawCube(shader, partModel);
            }

            glm::mat4 marker(1.0f);
            marker = glm::translate(marker, glm::vec3(part.position.x, part.position.y + 0.65f + bob, part.position.z));
            marker = glm::scale(marker, glm::vec3(0.18f, 0.18f, 0.18f));
            setMat(buoyTex, MAT_BOAT, 1.0f);
            drawCube(shader, marker);

            ++partIndex;
        }

        if (gHarbourMission.HasAllParts())
        {
            const float pulse = 0.55f + 0.18f * std::sin(time * 4.0f);
            glm::mat4 dockBeacon(1.0f);
            dockBeacon = glm::translate(dockBeacon, glm::vec3(0.0f, 1.0f + pulse * 0.18f, 2.7f));
            dockBeacon = glm::scale(dockBeacon, glm::vec3(0.35f + pulse * 0.08f, 0.35f + pulse * 0.08f, 0.35f + pulse * 0.08f));
            setMat(buoyTex, MAT_BOAT, 1.0f);
            drawCube(shader, dockBeacon);
        }

        // AI patrol boat. It now reuses the same imported boat model as the
        // player boat, but at a smaller scale and with a separate blue/teal
        // patrol texture so it is readable as a different craft. A small
        // warning-light cube is left on top to make the AI easier to spot.
        const HarbourPatrolEnemy& enemy = gHarbourMission.GetEnemy();
        const float enemyYaw = std::atan2(enemy.forward.x, enemy.forward.z);
        glm::mat4 enemyBase(1.0f);
        enemyBase = glm::translate(enemyBase, glm::vec3(enemy.position.x, getWaterHeight(enemy.position.x, enemy.position.z, time) + 0.08f, enemy.position.z));
        enemyBase = glm::rotate(enemyBase, enemyYaw, glm::vec3(0, 1, 0));

        if (boatModel.loaded)
        {
            setMat(patrolBoatTex != 0 ? patrolBoatTex : boatTex, MAT_BOAT, 1.0f);

            glm::mat4 patrolModel = enemyBase;
            patrolModel = glm::translate(patrolModel, glm::vec3(0.0f, -0.06f, 0.0f));
            patrolModel = glm::rotate(patrolModel, glm::radians(180.0f), glm::vec3(0, 1, 0));
            patrolModel = glm::scale(patrolModel, glm::vec3(0.28f, 0.28f, 0.28f));
            drawModel(shader, boatModel, patrolModel);
        }
        else
        {
            setMat(patrolBoatTex != 0 ? patrolBoatTex : boatTex, MAT_BOAT, 1.0f);
            glm::mat4 enemyHull = glm::scale(enemyBase, glm::vec3(0.78f, 0.26f, 1.35f));
            drawCube(shader, enemyHull);
        }

        setMat(buoyTex, MAT_BOAT, 1.0f);
        glm::mat4 enemyLight = glm::translate(enemyBase, glm::vec3(0.0f, 0.42f, 0.1f));
        enemyLight = glm::scale(enemyLight, glm::vec3(0.24f, 0.24f, 0.24f));
        drawCube(shader, enemyLight);
    }
}

// First render pass: render the world from the sun/light point of view
// into the shadow map. The main shader and water shader sample this
// texture later for PCF/PCSS shadowing.
void renderDepthPass(const glm::mat4& lightSpaceMatrix, float time)
{
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    glUseProgram(depthShader);
    setMat4(depthShader, "lightSpaceMatrix", lightSpaceMatrix);
    renderWorldGeometry(depthShader, true, time);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Draws the atmospheric skybox behind the scene.
void renderSkybox(const glm::mat4& view, const glm::mat4& projection, float time)
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShader);
    glm::mat4 skyView = glm::mat4(glm::mat3(view));
    setMat4(skyboxShader, "view", skyView);
    setMat4(skyboxShader, "projection", projection);
    setFloat(skyboxShader, "time", time);
    setFloat(skyboxShader, "worldDanger", getWorldDanger());
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    setInt(skyboxShader, "skybox", 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

// Draws the animated water surface using its own shader so wave motion,
// zone tint, fog, lighting, and shadow sampling can be handled separately.
void renderWater(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& lightSpaceMatrix, const SunState& sun, const glm::vec3& viewPos, float time)
{
    glUseProgram(waterShader);
    setMat4(waterShader, "view", view);
    setMat4(waterShader, "projection", projection);
    setMat4(waterShader, "lightSpaceMatrix", lightSpaceMatrix);
    setVec3(waterShader, "sunDirection", sun.direction);
    setVec3(waterShader, "sunColor", sun.color);
    setFloat(waterShader, "sunIntensity", sun.intensity);
    setFloat(waterShader, "ambientStrength", sun.ambient);
    setInt(waterShader, "shadowFilterMode", static_cast<int>(gShadowFilterMode));
    setFloat(waterShader, "shadowLightSize", 0.026f);
    setFloat(waterShader, "shadowSoftnessScale", 12.0f);
    setVec3(waterShader, "viewPos", viewPos);
    setVec3(waterShader, "zoneTint", getZoneWaterTint());
    setVec3(waterShader, "fogColor", getEffectiveFogColor());
    setFloat(waterShader, "fogNear", getEffectiveFogNear());
    setFloat(waterShader, "fogFar", getEffectiveFogFar());
    setFloat(waterShader, "worldDanger", getWorldDanger());
    setFloat(waterShader, "time", time);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    setInt(waterShader, "shadowMap", 1);

    glm::mat4 waterModel(1.0f);
    waterModel = glm::scale(waterModel, glm::vec3(120.0f, 1.0f, 120.0f));
    drawPlane(waterShader, waterModel);
}

// Main lit scene pass. This binds lighting, fog, shadow settings and
// then renders the world geometry using the main material shader.
void renderLitScene(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& lightSpaceMatrix, const SunState& sun, const glm::vec3& viewPos, float time)
{
    glUseProgram(basicShader);
    setMat4(basicShader, "view", view);
    setMat4(basicShader, "projection", projection);
    setMat4(basicShader, "lightSpaceMatrix", lightSpaceMatrix);
    setVec3(basicShader, "sunDirection", sun.direction);
    setVec3(basicShader, "sunColor", sun.color);
    setFloat(basicShader, "sunIntensity", sun.intensity);
    setFloat(basicShader, "ambientStrength", sun.ambient);
    setInt(basicShader, "shadowFilterMode", static_cast<int>(gShadowFilterMode));
    setFloat(basicShader, "shadowLightSize", 0.024f);
    setFloat(basicShader, "shadowSoftnessScale", 18.0f);
    setVec3(basicShader, "viewPos", viewPos);
    setVec3(basicShader, "fogColor", getEffectiveFogColor());
    setFloat(basicShader, "fogNear", getEffectiveFogNear());
    setFloat(basicShader, "fogFar", getEffectiveFogFar());
    setFloat(basicShader, "worldDanger", getWorldDanger());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    setInt(basicShader, "shadowMap", 1);

    renderWorldGeometry(basicShader, false, time);
}

// Final transparent particle pass for wake, splashes, ambient particles,
// Lost Island aura particles, and victory celebration effects.
void renderParticles(const glm::mat4& view, const glm::mat4& projection)
{
    std::vector<ParticleVertex> alive;
    alive.reserve(MAX_PARTICLES);
    for (const auto& p : particles)
    {
        if (p.life > 0.0f)
            alive.push_back({ p.position, p.color, p.size });
    }
    if (alive.empty()) return;

    glUseProgram(particleShader);
    setMat4(particleShader, "view", view);
    setMat4(particleShader, "projection", projection);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, alive.size() * sizeof(ParticleVertex), alive.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(alive.size()));
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

int main()
{
    srand(static_cast<unsigned int>(time(nullptr)));
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    // Create a large bordered window and centre it on the primary monitor.
    // This keeps the coursework demo readable while still behaving like
    // a normal desktop game window.

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    int windowWidth = static_cast<int>(mode->width * 0.94f);
    int windowHeight = static_cast<int>(mode->height * 0.90f);

    GLFWwindow* window = glfwCreateWindow(
        windowWidth,
        windowHeight,
        "Dredge-style Fishing Prototype",
        nullptr,
        nullptr
    );

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    int windowPosX = (mode->width - windowWidth) / 2;
    int windowPosY = (mode->height - windowHeight) / 2;
    glfwSetWindowPos(window, windowPosX, windowPosY);

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, windowWidth, windowHeight);
    gWindowWidth = windowWidth;
    gWindowHeight = windowHeight;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    std::cout << "[Startup] OpenGL initialized." << std::endl;
    std::cout << "[Startup] Window size: " << gWindowWidth << "x" << gWindowHeight << std::endl;
    std::cout << "[Startup] Loading shaders, models, textures and audio..." << std::endl;

    // Shader programs are loaded up front. Each one owns a different
    // render stage: world materials, water, depth shadows, particles,
    // skybox, and fullscreen post-processing.
    basicShader = createShaderProgramFromFiles(BASIC_VERT_PATH, BASIC_FRAG_PATH);
    waterShader = createShaderProgramFromFiles(WATER_VERT_PATH, WATER_FRAG_PATH);
    depthShader = createShaderProgramFromFiles(DEPTH_VERT_PATH, DEPTH_FRAG_PATH);
    particleShader = createShaderProgramFromFiles(PARTICLE_VERT_PATH, PARTICLE_FRAG_PATH);
    skyboxShader = createShaderProgramFromFiles(SKYBOX_VERT_PATH, SKYBOX_FRAG_PATH);

    InitUIOverlay();
    InitGameSettingsMenuRenderer();
    if (!gPostProcessor.Init(gWindowWidth, gWindowHeight, POSTPROCESS_VERT_PATH, POSTPROCESS_FRAG_PATH))
    {
        std::cerr << "Failed to initialize post-processing pipeline." << std::endl;
    }

    // Upload reusable meshes and render targets.
    // setupGeneratedIslandMeshes() lives in WorldGen.cpp and creates the
    // procedural island/grass meshes used by WorldRenderer.cpp.
    SetupCube(cubeVAO, cubeVBO, cubeVertices, sizeof(cubeVertices));
    setupGeneratedIslandMeshes();
    SetupPlane(planeVAO, planeVBO, planeVertices, sizeof(planeVertices));
    SetupSkybox(skyboxVAO, skyboxVBO, skyboxVertices, sizeof(skyboxVertices));
    SetupParticleBuffer(particleVAO, particleVBO, MAX_PARTICLES);
    setupShadowMap();
    setupZones();
    RegisterFishJournalEntries();
    InitialiseShopkeeperContracts();
    SetNextWeather(nullptr);
    initialiseEnvironmentBlend();

    // Load imported models. The sword has a fallback path because older
    // project versions stored it in a slightly different folder.
    loadOBJModel("media/models/boat.obj", boatModel);
    if (!loadOBJModel("media/sword.obj", swordModel))
    {
        loadOBJModel("media/models/sword.obj", swordModel);
    }

    // Optional natural tree model used on the procedural mini islands.
    // Keep tree.obj, tree.mtl and treecolorpallet.png in media/models/.
    loadOBJModel("media/models/tree.obj", treeModel);

    // Harbour mission collectible model. Preferred path is media/models/scrap.obj.
    // Fallbacks allow either media/models/scrap/scrap.obj or media/scrap.obj.
    if (!loadOBJModel("media/models/scrap.obj", scrapModel))
    {
        if (!loadOBJModel("media/models/scrap/scrap.obj", scrapModel))
        {
            loadOBJModel("media/scrap.obj", scrapModel);
        }
    }

    // Load material textures. If a file is missing, procedural fallback
    // textures are generated so the scene still remains playable.
    woodTex = loadTextureFromFile("media/wood.jpg");
    if (woodTex == 0)
    {
        std::cout << "[Fallback] Using procedural wood texture." << std::endl;
        woodTex = createWoodTexture();
    }

    rockTex = loadTextureFromFile("media/rock.jpg");
    if (rockTex == 0)
    {
        std::cout << "[Fallback] Using procedural rock texture." << std::endl;
        rockTex = createRockTexture();
    }

    grassTex = loadTextureFromFile("media/grass.jpg");
    if (grassTex == 0)
    {
        std::cout << "[Fallback] Using procedural grass texture." << std::endl;
        grassTex = createGrassTexture();
    }

    treeTex = loadTextureFromFile("media/models/treecolorpallet.png");
    if (treeTex == 0)
    {
        std::cout << "[Fallback] Tree palette missing. Reusing grass texture for tree model." << std::endl;
        treeTex = grassTex;
    }

    boatTex = loadTextureFromFile("textures/boat.jpg");
    if (boatTex == 0)
    {
        std::cout << "[Fallback] Using procedural boat texture." << std::endl;
        boatTex = createBoatTexture();
    }

    // The AI patrol uses the same imported boat model as the player, but with
    // a separate procedural colour texture so it is easy to recognise.
    patrolBoatTex = createPatrolBoatTexture();

    // Optional texture for the scrap collectible model. If no image exists,
    // the procedural rusty-metal texture is used instead.
    scrapTex = loadTextureFromFile("media/models/scrap.png");
    if (scrapTex == 0)
    {
        scrapTex = loadTextureFromFile("media/models/scrap.jpg");
    }
    if (scrapTex == 0)
    {
        std::cout << "[Fallback] Using procedural scrap texture." << std::endl;
        scrapTex = createScrapTexture();
    }

    buoyTex = loadTextureFromFile("textures/buoy.jpg");
    if (buoyTex == 0)
    {
        std::cout << "[Fallback] Using procedural buoy texture." << std::endl;
        buoyTex = createBuoyTexture();
    }

    // Bright marker textures used to identify each fishing island from far away.
    zoneMarkerTex[0] = createSolidColorTexture(255, 190, 70);   // Harbour Waters
    zoneMarkerTex[1] = createSolidColorTexture(60, 230, 180);   // Reef Edge
    zoneMarkerTex[2] = createSolidColorTexture(150, 85, 255);   // Deep Trench

    gShopkeeperTexture = loadTextureFromFile("media/ui/shopkeeper.png", false);
    if (gShopkeeperTexture == 0) gShopkeeperTexture = loadTextureFromFile("media/shopkeeper.png", false);
    if (gShopkeeperTexture == 0) gShopkeeperTexture = loadTextureFromFile("media/ui/shopkeeper.jpg", false);

    skyboxCubemap = createFallbackCubemap();

    // Audio also uses fallback folder attempts so the packaged Debug
    // build and the Visual Studio working directory both have a chance
    // to find media/sounds.
    bool soundReady = false;
    const std::vector<std::string> soundFolders = {
        "media/sounds",
        "../media/sounds",
        "../../media/sounds",
        "../../../media/sounds"
    };

    for (const std::string& soundFolder : soundFolders)
    {
        std::cout << "[Audio] Trying sound folder: " << soundFolder << std::endl;

        if (gSound.Init(soundFolder))
        {
            std::cout << "[Audio] Sound manager initialized using: "
                      << soundFolder << std::endl;

            gSound.PlayBackgroundLoop();
            soundReady = true;
            break;
        }
    }

    if (!soundReady)
    {
        std::cerr << "[Audio] Failed to initialize sound manager with any fallback path." << std::endl;
    }

    float lastFrame = 0.0f;

    // Main game loop: input/menu handling, gameplay updates, atmosphere,
    // audio, render passes, UI construction, and buffer swap.
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // One-shot menu input handling. These flags prevent a single
        // key press from toggling menus multiple times across frames.
        if (keys[GLFW_KEY_ENTER] && !startPressed)
        {

        if (!gGameStarted)
            {
                gGameStarted = true;
                lastCatchText = "Set sail, find keys, and salvage harbour repair parts";
                catchMessageTimer = 2.0f;
                TriggerCatchBanner("VOYAGE STARTED", 1.2f);
            }
            startPressed = true;
        }
        if (!keys[GLFW_KEY_ENTER]) startPressed = false;

        if (gGameStarted && keys[GLFW_KEY_C] && !cameraModeTogglePressed)
        {
            gCameraMode = (gCameraMode == CameraMode::FreeLook) ? CameraMode::Follow : CameraMode::FreeLook;
            if (gCameraMode == CameraMode::FreeLook)
            {
                gFreeLookYawOffset = 0.0f;
                gFreeLookPitch = 30.0f;
                gFreeLookDistance = 9.0f;
            }
            cameraModeTogglePressed = true;
            setCatchMessage(window, std::string("Camera: ") + GetCameraModeLabel(), 1.8f);
        }
        if (!keys[GLFW_KEY_C]) cameraModeTogglePressed = false;

        if (gGameStarted && keys[GLFW_KEY_P] && !pausePressed)
        {
            gPaused = !gPaused;
            if (gPaused) { gJournalOpen = false; gDockShopOpen = false; }
            pausePressed = true;
        }
        if (!keys[GLFW_KEY_P]) pausePressed = false;

        if (gGameStarted && keys[GLFW_KEY_J] && !journalPressed)
        {
            gJournalOpen = !gJournalOpen;
            if (gJournalOpen) { gPaused = false; gDockShopOpen = false; }
            journalPressed = true;
        }
        if (!keys[GLFW_KEY_J]) journalPressed = false;

        if (gGameStarted && keys[GLFW_KEY_O] && !settingsTogglePressed)
        {
            gSettingsMenu.ToggleOpen();
            if (gSettingsMenu.IsOpen())
            {
                gPaused = false;
                gJournalOpen = false;
                gDockShopOpen = false;
            }
            settingsTogglePressed = true;
            gSettingsMenu.ApplyAudio(gSound);
            setCatchMessage(window, gSettingsMenu.IsOpen() ? "Settings opened" : "Settings closed", 1.2f);
        }
        if (!keys[GLFW_KEY_O]) settingsTogglePressed = false;

        if (gSettingsMenu.IsOpen())
        {
            if (keys[GLFW_KEY_UP] && !settingsUpPressed)
            {
                gSettingsMenu.MoveSelection(-1);
                settingsUpPressed = true;
            }
            if (!keys[GLFW_KEY_UP]) settingsUpPressed = false;

            if (keys[GLFW_KEY_DOWN] && !settingsDownPressed)
            {
                gSettingsMenu.MoveSelection(1);
                settingsDownPressed = true;
            }
            if (!keys[GLFW_KEY_DOWN]) settingsDownPressed = false;

            if (keys[GLFW_KEY_LEFT] && !settingsLeftPressed)
            {
                gSettingsMenu.AdjustSelected(-0.05f);
                gSettingsMenu.ApplyAudio(gSound);
                settingsLeftPressed = true;
            }
            if (!keys[GLFW_KEY_LEFT]) settingsLeftPressed = false;

            if (keys[GLFW_KEY_RIGHT] && !settingsRightPressed)
            {
                gSettingsMenu.AdjustSelected(0.05f);
                gSettingsMenu.ApplyAudio(gSound);
                settingsRightPressed = true;
            }
            if (!keys[GLFW_KEY_RIGHT]) settingsRightPressed = false;

            if (keys[GLFW_KEY_R] && !settingsResetPressed)
            {
                gSettingsMenu.ResetDefaults();
                gSettingsMenu.ApplyAudio(gSound);
                settingsResetPressed = true;
            }
            if (!keys[GLFW_KEY_R]) settingsResetPressed = false;
        }
        else
        {
            settingsUpPressed = false;
            settingsDownPressed = false;
            settingsLeftPressed = false;
            settingsRightPressed = false;
            settingsResetPressed = false;
        }

        if (gJournalOpen && keys[GLFW_KEY_LEFT] && !journalLeftPressed)
        {
            gJournalPage = std::max(0, gJournalPage - 1);
            journalLeftPressed = true;
        }
        if (!keys[GLFW_KEY_LEFT]) journalLeftPressed = false;

        if (gJournalOpen && keys[GLFW_KEY_RIGHT] && !journalRightPressed)
        {
            gJournalPage = std::min(GetJournalPageCount() - 1, gJournalPage + 1);
            journalRightPressed = true;
        }
        if (!keys[GLFW_KEY_RIGHT]) journalRightPressed = false;

        if (gDockShopOpen)
        {
            const int groupCount = getDockShopFishGroupCount();
            const int totalCount = std::max(1, getDockShopSelectionCount());

            if (keys[GLFW_KEY_LEFT] && !shopLeftPressed)
            {
                if (gDockShopSelection >= groupCount)
                    gDockShopSelection = std::max(groupCount, gDockShopSelection - 1);
                else
                    gDockShopSelection = std::max(0, gDockShopSelection - 1);
                shopLeftPressed = true;
            }
            if (!keys[GLFW_KEY_LEFT]) shopLeftPressed = false;

            if (keys[GLFW_KEY_RIGHT] && !shopRightPressed)
            {
                if (gDockShopSelection >= groupCount)
                    gDockShopSelection = std::min(totalCount - 1, gDockShopSelection + 1);
                else
                    gDockShopSelection = std::min(std::max(0, groupCount - 1), gDockShopSelection + 1);
                shopRightPressed = true;
            }
            if (!keys[GLFW_KEY_RIGHT]) shopRightPressed = false;

            if (keys[GLFW_KEY_UP] && !shopUpPressed)
            {
                if (gDockShopSelection >= groupCount)
                    gDockShopSelection = (groupCount > 0) ? std::max(0, std::min(groupCount - 1, gDockShopSelection - groupCount)) : groupCount;
                else
                    gDockShopSelection = std::max(0, gDockShopSelection - 2);
                shopUpPressed = true;
            }
            if (!keys[GLFW_KEY_UP]) shopUpPressed = false;

            if (keys[GLFW_KEY_DOWN] && !shopDownPressed)
            {
                if (gDockShopSelection < groupCount)
                {
                    const int nextItem = gDockShopSelection + 2;
                    gDockShopSelection = (nextItem < groupCount) ? nextItem : groupCount;
                }
                else
                {
                    gDockShopSelection = std::min(totalCount - 1, gDockShopSelection + 1);
                }
                shopDownPressed = true;
            }
            if (!keys[GLFW_KEY_DOWN]) shopDownPressed = false;

            if (keys[GLFW_KEY_ENTER] && !shopSelectPressed)
            {
                activateDockShopSelection(window);
                shopSelectPressed = true;
            }
            if (!keys[GLFW_KEY_ENTER]) shopSelectPressed = false;

            if (keys[GLFW_KEY_T] && !contractTurnInPressed)
            {
                TurnInReadyContracts(window);
                contractTurnInPressed = true;
            }
            if (!keys[GLFW_KEY_T]) contractTurnInPressed = false;

            const bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            if (mouseDown && !shopMousePressed)
            {
                double mouseX = 0.0;
                double mouseY = 0.0;
                glfwGetCursorPos(window, &mouseX, &mouseY);
                const int clickedSelection = GetDockShopSelectionFromMouse(mouseX, mouseY);
                if (clickedSelection >= 0)
                {
                    gDockShopSelection = clickedSelection;
                    activateDockShopSelection(window);
                }
                shopMousePressed = true;
            }
            if (!mouseDown) shopMousePressed = false;

            if (!keys[GLFW_KEY_E])
            {
                shopCloseReady = true;
            }

            if (keys[GLFW_KEY_E] && shopCloseReady)
            {
                closeDockShop(window);
                shopCloseReady = false;
                fishPressed = true;
            }
        }
        else
        {
            shopLeftPressed = false;
            shopRightPressed = false;
            shopUpPressed = false;
            shopDownPressed = false;
            shopSelectPressed = false;
            shopMousePressed = false;
            shopCloseReady = false;
        }

        const bool gameplayBlocked = !gGameStarted || gPaused || gJournalOpen || gSettingsMenu.IsOpen() || gDockShopOpen;

        if (!gameplayBlocked)
        {
            // Core gameplay update only runs when start/pause/journal/settings
            // overlays are not blocking player control.
            if (gCameraMode == CameraMode::FreeLook)
            {
                const float yawSpeed = 90.0f * deltaTime;
                const float pitchSpeed = 50.0f * deltaTime;
                if (keys[GLFW_KEY_LEFT]) gFreeLookYawOffset -= yawSpeed;
                if (keys[GLFW_KEY_RIGHT]) gFreeLookYawOffset += yawSpeed;
                if (keys[GLFW_KEY_UP]) gFreeLookPitch = clampf(gFreeLookPitch + pitchSpeed, 10.0f, 65.0f);
                if (keys[GLFW_KEY_DOWN]) gFreeLookPitch = clampf(gFreeLookPitch - pitchSpeed, 10.0f, 65.0f);
            }

            gSessionPlayTime += deltaTime;
            boat.update(deltaTime);
            updateEnvironmentBlend(deltaTime);
            UpdateWeather(deltaTime, window);
            gFishingMinigame.Update(deltaTime);

            const bool missionInteractNow = !gFishingMinigame.IsActive() && keys[GLFW_KEY_Q] && !missionInteractPressed;
            if (keys[GLFW_KEY_Q]) missionInteractPressed = true;
            else missionInteractPressed = false;

            if (keys[GLFW_KEY_H] && !missionResetPressed)
            {
                gHarbourMission.Reset();
                gSound.StopPatrolHornLoop();
                missionResetPressed = true;
                setCatchMessage(window, "Harbour recovery mission reset", 1.8f);
                TriggerCatchBanner("MISSION RESET", 1.0f);
            }
            if (!keys[GLFW_KEY_H]) missionResetPressed = false;

            if (keys[GLFW_KEY_T] && !contractTurnInPressed)
            {
                TurnInReadyContracts(window);
                contractTurnInPressed = true;
            }
            if (!keys[GLFW_KEY_T]) contractTurnInPressed = false;

            const HarbourMissionEvent missionEvent = gHarbourMission.Update(deltaTime, boat.position, missionInteractNow, isAtDock());
            handleHarbourMissionEvent(window, missionEvent);

            const bool patrolIsChasing =
                !gHarbourMission.IsComplete() &&
                gHarbourMission.GetEnemy().state == HarbourEnemyState::Chase;

            gSound.UpdatePatrolHorn(patrolIsChasing);

            const int keysBeforeQuestUpdate = gIslandQuest.GetCollectedKeyCount();
            const bool hadWonBeforeQuestUpdate = gIslandQuest.HasWon();
            const std::string statusBeforeSystemMessages = lastCatchText;
            gIslandQuest.Update(boat.position, lastCatchText, catchMessageTimer);
            // Development shortcut used to speed up video/testing progression.
            // It is documented in the README as a debug helper.
            gIslandQuest.HandleTestGoldHotkey(keys[GLFW_KEY_G], totalMoney, lastCatchText, catchMessageTimer);
            if (gIslandQuest.GetCollectedKeyCount() != keysBeforeQuestUpdate ||
                gIslandQuest.HasWon() != hadWonBeforeQuestUpdate ||
                (keys[GLFW_KEY_G] && lastCatchText != statusBeforeSystemMessages))
            {
                ClearCaughtFishDisplay();
            }

            updateDangerGameplay(window, deltaTime);
            gSound.UpdateBoatMotor(std::abs(boat.velocity), boat.maxForwardSpeed);
            std::string currentAudioZone = getCurrentZoneName();
            if (gIslandQuest.HasWon())
            {
                currentAudioZone = "Lost Island";
            }
            gSound.UpdateZoneLayer(currentAudioZone, deltaTime, getWorldDanger());
            triggerWinCelebration();

            if (fishCooldown > 0.0f) fishCooldown -= deltaTime;
            if (catchFlashTimer > 0.0f) catchFlashTimer -= deltaTime;
            if (catchMessageTimer > 0.0f) catchMessageTimer -= deltaTime;
            if (gCaughtFishCardTimer > 0.0f) gCaughtFishCardTimer -= deltaTime;
            if (gCatchBannerTimer > 0.0f) gCatchBannerTimer -= deltaTime;
            if (gCameraShakeTimer > 0.0f) gCameraShakeTimer -= deltaTime;
            else gCameraShakeStrength = 0.0f;

            SpawnWakeParticles(particles, boat);

            if (keys[GLFW_KEY_M] && !minigameTogglePressed)
            {
                gFishingMinigame.ToggleEnabled();
                if (!gFishingMinigame.IsEnabled())
                {
                    gHasPendingMinigameFish = false;
                    gFishingMinigameZoneIndex = -1;
                }
                minigameTogglePressed = true;
                setCatchMessage(window, gFishingMinigame.IsEnabled() ? "Fishing minigame enabled" : "Fishing minigame disabled", 1.6f);
            }
            if (!keys[GLFW_KEY_M]) minigameTogglePressed = false;

            if (gFishingMinigame.IsActive())
            {
                if (keys[GLFW_KEY_SPACE] && !minigameHookPressed)
                {
                    gFishingMinigame.Hook();
                    minigameHookPressed = true;
                }
                if (!keys[GLFW_KEY_SPACE]) minigameHookPressed = false;
            }
            else
            {
                minigameHookPressed = false;
            }

            FishingMinigameResult minigameResult;
            if (gFishingMinigame.ConsumeResult(minigameResult))
            {
                resolveFishingMinigame(window, minigameResult);
            }

            if (keys[GLFW_KEY_E] && !fishPressed)
            {
                if (isAtDock())
                {
                    openDockShop(window);
                }
                else
                {
                    tryFishing(window);
                }
                fishPressed = true;
            }
            if (!keys[GLFW_KEY_E]) fishPressed = false;

            sellPressed = false;
            upgrade1Pressed = false;
            upgrade2Pressed = false;
            upgrade3Pressed = false;
            repairPressed = false;
        }
        else
        {
            fishPressed = false;
            sellPressed = false;
            upgrade1Pressed = false;
            upgrade2Pressed = false;
            upgrade3Pressed = false;
            repairPressed = false;
            minigameTogglePressed = false;
            minigameHookPressed = false;
            missionInteractPressed = false;
            missionResetPressed = false;
            contractTurnInPressed = false;
            gSound.UpdateBoatMotor(0.0f, boat.maxForwardSpeed);
            std::string currentAudioZone = getCurrentZoneName();
            if (gIslandQuest.HasWon())
            {
                currentAudioZone = "Lost Island";
            }
            gSound.UpdateZoneLayer(currentAudioZone, deltaTime, getWorldDanger());
        }

        UpdateParticles(particles, deltaTime);
        SpawnLostIslandAuraParticles(particles, gIslandQuest);
        SpawnZoneAmbientParticles(particles, zones, currentFrame);

        std::stringstream title;
        title << "Dredge-style Fishing Prototype | Zone: " << getCurrentZoneName()
              << " | Gold: " << totalMoney
              << " | Cargo: " << cargo.size() << "/" << cargoCapacity << " (" << getCargoValue() << "g)"
              << " | Rod: " << rodLevel
              << " | Hull: " << static_cast<int>(std::round(hullIntegrity)) << "%"
              << " | Speed: " << std::fixed << std::setprecision(1) << boat.velocity;

        const char* postLabel = "None";
        if (gPostMode == PostProcessMode::Edge) postLabel = "Edge";
        else if (gPostMode == PostProcessMode::Blur) postLabel = "Blur";
        else if (gPostMode == PostProcessMode::NightVision) postLabel = "NightVision";
        else if (gPostMode == PostProcessMode::GodRays) postLabel = "GodRays";
        else if (gPostMode == PostProcessMode::SSAO) postLabel = "SSAO";
        else if (gPostMode == PostProcessMode::DepthOfField) postLabel = "DepthOfField";
        title << " | PP: " << postLabel
              << " | Shadows: " << getShadowFilterLabel()
              << " | Camera: " << GetCameraModeLabel()
              << " | Shadow: " << getShadowFilterLabel()
              << " | MiniGame: " << (gFishingMinigame.IsEnabled() ? (gFishingMinigame.IsActive() ? "Active" : "On") : "Off")
              << " | Quest: " << gIslandQuest.GetQuestSummary()
              << " | Side: " << gHarbourMission.GetObjectiveSummary()
              << " | Patrol: " << gHarbourMission.GetEnemyStateLabel()
              << " | Weather: " << GetWeatherName()
              << " | Contracts: " << GetCompletedContractCount() << "/" << gShopkeeperContracts.size();
        if (!gGameStarted) title << " | PRESS ENTER TO START";
        if (gPaused) title << " | PAUSED";
        if (gJournalOpen) title << " | JOURNAL";
        if (gSettingsMenu.IsOpen()) title << " | SETTINGS";
        if (gDockShopOpen) title << " | SHOP";
        if (isAtDock())
        {
            title << " | E Open Shop";
        }
        else
        {
            title << " | E Fish";
        }

        if (catchMessageTimer > 0.0f && !lastCatchText.empty())
        {
            title << " | " << lastCatchText;
        }

        glfwSetWindowTitle(window, title.str().c_str());

        SunState sun = EvaluateSunState(currentFrame);
        // The settings menu brightness is applied directly to scene lighting,
        // fog/clear colour, and the rendered world. This makes turning the
        // slider down visibly darken the game instead of only making a very
        // subtle change to the background colour.
        const float settingsBrightness = clampf(gSettingsMenu.GetBrightness(), 0.20f, 1.45f);
        sun.intensity *= settingsBrightness;
        sun.ambient *= settingsBrightness;

        glm::vec3 fogColor = getEffectiveFogColor() * settingsBrightness;
        glm::vec3 clearColor = glm::mix(fogColor, glm::vec3(0.95f, 0.90f, 0.78f) * settingsBrightness, catchFlashTimer * 0.4f);
        clearColor = glm::mix(clearColor, glm::vec3(0.08f, 0.10f, 0.16f), 0.22f * (1.0f - sun.dayFactor));
        if (hullWarningTimer > 0.0f)
        {
            const float dangerPulse = 0.16f + 0.10f * std::sin(currentFrame * 12.0f);
            clearColor = glm::mix(clearColor, glm::vec3(0.80f, 0.18f, 0.12f), dangerPulse);
        }
        if (gIslandQuest.HasWon())
        {
            const float winGlow = 0.14f + 0.06f * std::sin(currentFrame * 3.5f);
            clearColor = glm::mix(clearColor, glm::vec3(0.96f, 0.86f, 0.46f), winGlow);
        }
        clearColor = glm::clamp(clearColor * settingsBrightness, glm::vec3(0.0f), glm::vec3(1.0f));
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);

        glm::vec3 cameraTarget = getCameraTarget();
        glm::vec3 cameraPos = getCameraPosition();
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0, 1, 0));
        gPostProcessor.EnsureSize(gWindowWidth, gWindowHeight);

        glm::mat4 projection = glm::perspective(glm::radians(60.0f), static_cast<float>(gWindowWidth) / static_cast<float>(std::max(gWindowHeight, 1)), 0.1f, 500.0f);

        const glm::vec3 lightTarget = boat.position + glm::vec3(0.0f, 0.8f, 0.0f);
        const float sunDistance = 52.0f;
        glm::vec3 lightPos = lightTarget + sun.direction * sunDistance;
        glm::mat4 lightProjection = glm::ortho(-42.0f, 42.0f, -42.0f, 42.0f, 1.0f, 120.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0, 1, 0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // Render pipeline:
        // 1) shadow depth pass into the shadow map
        // 2) render scene into the post-process framebuffer
        // 3) fullscreen post-process pass
        // 4) custom UI/settings overlay
        renderDepthPass(lightSpaceMatrix, currentFrame);

        gPostProcessor.BeginScene(clearColor);
        glViewport(0, 0, gWindowWidth, gWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderSkybox(view, projection, currentFrame);
        renderWater(view, projection, lightSpaceMatrix, sun, cameraPos, currentFrame);
        renderLitScene(view, projection, lightSpaceMatrix, sun, cameraPos, currentFrame);
        renderParticles(view, projection);
        gPostProcessor.EndScene();

        glViewport(0, 0, gWindowWidth, gWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::vec3 cameraForward = glm::normalize(cameraTarget - cameraPos);
        glm::vec3 sunWorldPos = cameraPos + sun.direction * 250.0f;
        glm::vec4 sunClip = projection * view * glm::vec4(sunWorldPos, 1.0f);
        glm::vec2 sunScreenPos(0.5f, 1.2f);
        float sunVisibility = 0.0f;
        if (sunClip.w > 0.001f)
        {
            glm::vec3 sunNdc = glm::vec3(sunClip) / sunClip.w;
            sunScreenPos = glm::vec2(sunNdc.x * 0.5f + 0.5f, sunNdc.y * 0.5f + 0.5f);
            const float onScreen = 1.0f - saturatef(std::max(std::abs(sunNdc.x), std::abs(sunNdc.y)) - 0.85f);
            const float forwardness = saturatef((glm::dot(cameraForward, sun.direction) - 0.02f) / 0.20f);
            sunVisibility = saturatef(onScreen) * forwardness * sun.dayFactor;
        }
        gPostProcessor.Render(gPostMode, currentFrame, sunScreenPos, sun.color, sunVisibility, sun.dayFactor);

        // Build a single HUDState snapshot for the UI renderer. This keeps
        // UIOverlay from directly owning or mutating gameplay systems.
        HUDState hud;
        hud.screenWidth = gWindowWidth;
        hud.screenHeight = gWindowHeight;
        hud.zoneName = gIslandQuest.HasWon() ? "Lost Island" : getCurrentZoneName();
        hud.statusText = lastCatchText;
        hud.questSummary = gIslandQuest.GetQuestSummary();
        hud.gold = totalMoney;
        hud.cargoCount = static_cast<int>(cargo.size());
        hud.cargoCapacity = cargoCapacity;
        hud.cargoValue = getCargoValue();
        hud.rodLevel = rodLevel;
        hud.engineLevel = engineLevel;
        hud.rodUpgradeCost = rodUpgradeCost();
        hud.engineUpgradeCost = engineUpgradeCost();
        hud.cargoUpgradeCost = cargoUpgradeCost();
        hud.keysCollected = gIslandQuest.GetCollectedKeyCount();
        hud.keysTotal = 3;
        hud.harbourPartsCollected = gHarbourMission.GetCollectedPartCount();
        hud.harbourPartsTotal = gHarbourMission.GetTotalPartCount();
        hud.repairCost = getRepairCost();
        hud.hullIntegrity = hullIntegrity;
        hud.hullCritical = hullIntegrity < 35.0f;
        hud.speed = std::abs(boat.velocity);
        hud.danger = getWorldDanger();
        hud.atDock = isAtDock();
        hud.minigameEnabled = gFishingMinigame.IsEnabled();
        hud.minigameActive = gFishingMinigame.IsActive();
        hud.goalIslandUnlocked = gIslandQuest.IsGoalIslandUnlocked();
        hud.hasWon = gIslandQuest.HasWon();
        hud.messageTimer = catchMessageTimer;
        hud.flash = catchFlashTimer;
        hud.caughtFishName = gLastCaughtFishName;
        hud.caughtFishRarity = gLastCaughtFishRarity;
        hud.caughtFishTexture = gLastCaughtFishTexture;
        hud.caughtFishTextureWidth = gLastCaughtFishTextureWidth;
        hud.caughtFishTextureHeight = gLastCaughtFishTextureHeight;
        hud.showCaughtFishCard = (gCaughtFishCardTimer > 0.0f);
        hud.fishTextureMissing = gLastCaughtFishTextureMissing;
        hud.totalFishCaught = gTotalFishCaught;
        hud.totalGoldEarned = gTotalGoldEarned;
        hud.totalCargoSold = gTotalCargoSold;
        hud.perfectHooks = gPerfectHooks;
        hud.totalPlayTimeSeconds = gSessionPlayTime;
        hud.weatherName = GetWeatherName();
        hud.weatherEffectText = GetWeatherEffectText();
        hud.weatherIntensity = GetWeatherEffect01();
        hud.weatherTimeRemaining = GetWeatherTimeRemaining();
        hud.contractEntries = BuildContractHUDEntries();
        hud.contractsReady = GetReadyContractCount();
        hud.contractsCompleted = GetCompletedContractCount();
        hud.showStartScreen = !gGameStarted;
        hud.showPauseScreen = gPaused;
        hud.showJournal = gJournalOpen;
        hud.showVictoryScreen = gIslandQuest.HasWon();
        hud.showDockShop = gDockShopOpen;
        hud.journalPageCount = GetJournalPageCount();
        gJournalPage = std::max(0, std::min(gJournalPage, hud.journalPageCount - 1));
        hud.journalPage = gJournalPage;
        hud.journalEntries = BuildJournalPageEntries(gJournalPage);
        hud.catchBannerText = gCatchBannerText;
        hud.catchBannerTimer = gCatchBannerTimer;
        hud.shopItems = BuildDockShopItems();
        hud.shopActions = BuildDockShopActions();
        hud.shopSelection = gDockShopSelection;
        hud.shopkeeperTexture = gShopkeeperTexture;
        if (gFishingMinigame.IsActive())
        {
            hud.hintText = gFishingMinigame.GetHintText() + " | " + gIslandQuest.GetQuestSummary();
            hud.statusText = gFishingMinigame.GetStatusText();
            hud.messageTimer = 999.0f;
            hud.flash = 0.0f;
            hud.showCaughtFishCard = false;
        }
        if (hud.showStartScreen || hud.showPauseScreen || hud.showJournal || hud.showDockShop)
        {
            hud.showCaughtFishCard = false;
        }
        else if (hud.atDock)
        {
            hud.hintText = std::string("E:OPEN SHOP  T:TURN IN CONTRACTS  C:") + GetCameraModeLabel() + (gCameraMode == CameraMode::FreeLook ? std::string(" ARROWS LOOK") : std::string(""));
        }
        else if (hud.hasWon)
        {
            hud.hintText = "THE LOST ISLAND HAS BEEN REACHED  C:" + std::string(GetCameraModeLabel()) + (gCameraMode == CameraMode::FreeLook ? std::string(" ARROWS LOOK") : std::string(""));
        }
        else if (hud.goalIslandUnlocked)
        {
            hud.hintText = "E:CAST  M:" + std::string(hud.minigameEnabled ? "ON" : "OFF") + "  C:" + std::string(GetCameraModeLabel()) + (gCameraMode == CameraMode::FreeLook ? std::string(" ARROWS LOOK") : std::string("")) + "  SAIL TO THE LOST ISLAND" + (hud.hullCritical ? std::string("  HULL CRITICAL") : std::string(""));
        }
        else
        {
            hud.hintText = "E:CAST  M:" + std::string(hud.minigameEnabled ? "ON" : "OFF") + "  C:" + std::string(GetCameraModeLabel()) + (gCameraMode == CameraMode::FreeLook ? std::string(" ARROWS LOOK") : std::string("")) + "  KEYS " + std::to_string(hud.keysCollected) + "/" + std::to_string(hud.keysTotal) + (hud.hullCritical ? std::string("  HULL CRITICAL") : std::string(""));
        }
        if (gGameStarted && !hud.showPauseScreen && !hud.showJournal && !gFishingMinigame.IsActive())
        {
            hud.hintText += "  |  " + gHarbourMission.GetCompactHint(boat.position, hud.atDock);
        }


        if (hud.showDockShop)
        {
            hud.hintText = "SHOP OPEN  ARROWS MOVE  ENTER BUY/SELL  E CLOSE  T CONTRACTS";
            hud.statusText = cargo.empty() ? "Sell fish, buy upgrades, and repair at the dock" : "Select a fish to sell one at a time, or use SELL ALL";
            hud.messageTimer = 999.0f;
        }

        if (!gGameStarted)
        {
            hud.hintText = "PRESS ENTER TO START";
            hud.statusText = "FIND 3 KEYS, SALVAGE 3 REPAIR PARTS, AND AVOID THE PATROL";
            hud.messageTimer = 999.0f;
        }
        RenderUIOverlay(hud);
        RenderGameSettingsMenu(gWindowWidth, gWindowHeight, gSettingsMenu.GetState());

        glfwSwapBuffers(window);
    }

    // Cleanup GL resources and shutdown helper systems before exiting.
    if (boatModel.vbo != 0) glDeleteBuffers(1, &boatModel.vbo);
    if (boatModel.vao != 0) glDeleteVertexArrays(1, &boatModel.vao);
    if (swordModel.vbo != 0) glDeleteBuffers(1, &swordModel.vbo);
    if (swordModel.vao != 0) glDeleteVertexArrays(1, &swordModel.vao);
    if (treeModel.vbo != 0) glDeleteBuffers(1, &treeModel.vbo);
    if (treeModel.vao != 0) glDeleteVertexArrays(1, &treeModel.vao);

    if (gShopkeeperTexture != 0) glDeleteTextures(1, &gShopkeeperTexture);

    for (auto& entry : gFishIconTextures)
    {
        if (entry.second.texture != 0)
        {
            glDeleteTextures(1, &entry.second.texture);
            entry.second.texture = 0;
        }
    }

    gPostProcessor.Shutdown();
    ShutdownGameSettingsMenuRenderer();
    ShutdownUIOverlay();
    gSound.Shutdown();

    glfwTerminate();
    return 0;
}
