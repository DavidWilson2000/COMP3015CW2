#pragma once

#include <string>
#include <vector>

struct HUDJournalEntry
{
    std::string name;
    int rarity = 0;
    int caughtCount = 0;
    int bestValue = 0;
    bool discovered = false;
    bool textureMissing = false;
    unsigned int texture = 0;
    int textureWidth = 0;
    int textureHeight = 0;
};

struct HUDShopItem
{
    std::string name;
    int rarity = 0;
    int value = 0;
    int quantity = 1;
    bool textureMissing = false;
    unsigned int texture = 0;
    int textureWidth = 0;
    int textureHeight = 0;
};

struct HUDShopAction
{
    std::string label;
    std::string valueText;
    bool enabled = true;
    bool affordable = true;
};

struct HUDContractEntry
{
    std::string title;
    std::string requirement;
    std::string progress;
    int reward = 0;
    bool ready = false;
    bool completed = false;
};

struct HUDState
{
    int screenWidth = 1280;
    int screenHeight = 720;

    std::string zoneName;
    std::string statusText;
    std::string hintText;
    std::string questSummary;

    int gold = 0;
    int cargoCount = 0;
    int cargoCapacity = 0;
    int cargoValue = 0;
    int rodLevel = 1;
    int engineLevel = 1;
    int rodUpgradeCost = 0;
    int engineUpgradeCost = 0;
    int cargoUpgradeCost = 0;
    int repairCost = 0;
    int keysCollected = 0;
    int keysTotal = 3;
    int harbourPartsCollected = 0;
    int harbourPartsTotal = 3;
    int caughtFishRarity = 0;
    int totalFishCaught = 0;
    int totalGoldEarned = 0;
    int totalCargoSold = 0;
    int perfectHooks = 0;
    float speed = 0.0f;
    float danger = 0.0f;
    float hullIntegrity = 100.0f;
    float totalPlayTimeSeconds = 0.0f;
    float weatherIntensity = 0.0f;
    float weatherTimeRemaining = 0.0f;

    std::string weatherName;
    std::string weatherEffectText;

    bool atDock = false;
    bool minigameEnabled = false;
    bool minigameActive = false;
    bool goalIslandUnlocked = false;
    bool hasWon = false;
    bool hullCritical = false;
    bool showCaughtFishCard = false;
    bool fishTextureMissing = false;
    bool showStartScreen = false;
    bool showPauseScreen = false;
    bool showJournal = false;
    bool showVictoryScreen = false;
    bool showDockShop = false;
    unsigned int caughtFishTexture = 0;
    int caughtFishTextureWidth = 0;
    int caughtFishTextureHeight = 0;
    int journalPage = 0;
    int journalPageCount = 1;
    float messageTimer = 0.0f;
    float flash = 0.0f;
    float catchBannerTimer = 0.0f;
    std::string caughtFishName;
    std::string catchBannerText;
    std::vector<HUDJournalEntry> journalEntries;

    std::vector<HUDShopItem> shopItems;
    std::vector<HUDShopAction> shopActions;
    std::vector<HUDContractEntry> contractEntries;
    unsigned int shopkeeperTexture = 0;
    int shopSelection = 0;
    int contractsReady = 0;
    int contractsCompleted = 0;
};

bool InitUIOverlay();
void RenderUIOverlay(const HUDState& state);
void ShutdownUIOverlay();
