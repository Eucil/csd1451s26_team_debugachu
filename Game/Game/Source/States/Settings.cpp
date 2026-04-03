/*!
@file       Settings.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/Settings.h"

#include <AEEngine.h>

#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "DebugSystem.h"
#include "GameStateManager.h"
#include "States/LevelManager.h"

// Destructible Background
#include "AudioSystem.h"
#include "Terrain.h"
#include "Utils.h"
#include "VFXSystem.h"

static Button buttonIncreaseSfxVolume;
static Button buttonDecreaseSfxVolume;
static Button buttonIncreaseBgmVolume;
static Button buttonDecreaseBgmVolume;
static Button buttonBack;

static s8 titleFont;
static s8 buttonFont;

static TextData headerText;
static TextData sfxVolumeText;
static TextData bgmVolumeText;
static TextData sfxVolumeAmountText;
static TextData bgmVolumeAmountText;

// Destructible Background Variables
static int bgHeight, bgWidth, bgTileSize, bgPortalLimit;
static bool bgFileExist;

static Terrain* bgDirt = nullptr;
static Terrain* bgStone = nullptr;
static Terrain* bgMagic = nullptr;
static AEGfxTexture* pBgDirtTex{nullptr};
static AEGfxTexture* pBgStoneTex{nullptr};
static AEGfxTexture* pBgMagicTex{nullptr};
static TiledBackground bg;
static VFXSystem bgVfxSystem;

// Animations
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

void loadSettings() {
        
    animManager.Clear();
    animManager.Add(&screenFader);
    animManager.Add(&someOtherCoolAnimation);
    animManager.InitializeAll();

    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    buttonIncreaseSfxVolume.loadMesh();
    buttonIncreaseSfxVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseSfxVolume.loadMesh();
    buttonDecreaseSfxVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonIncreaseBgmVolume.loadMesh();
    buttonIncreaseBgmVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseBgmVolume.loadMesh();
    buttonDecreaseBgmVolume.loadTexture("Assets/Textures/brown_square_24_24.png");

    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");

    // Destructible Terrain
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pBgDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pBgStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
    pBgMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");
    bg.loadFromJson("background", "Background");
}

void initializeSettings() {
    buttonIncreaseSfxVolume.initFromJson("settings_buttons", "IncreaseSfxVolume");
    buttonIncreaseSfxVolume.setTextFont(buttonFont);
    buttonDecreaseSfxVolume.initFromJson("settings_buttons", "DecreaseSfxVolume");
    buttonDecreaseSfxVolume.setTextFont(buttonFont);
    buttonIncreaseBgmVolume.initFromJson("settings_buttons", "IncreaseBgmVolume");
    buttonIncreaseBgmVolume.setTextFont(buttonFont);
    buttonDecreaseBgmVolume.initFromJson("settings_buttons", "DecreaseBgmVolume");
    buttonDecreaseBgmVolume.setTextFont(buttonFont);

    buttonBack.initFromJson("settings_buttons", "Back");
    buttonBack.setTextFont(buttonFont);

    headerText.initFromJson("settings_text", "Header");
    headerText.font_ = titleFont;

    sfxVolumeText.initFromJson("settings_text", "SfxVolume");
    sfxVolumeText.font_ = buttonFont;
    bgmVolumeText.initFromJson("settings_text", "BgmVolume");
    bgmVolumeText.font_ = buttonFont;
    sfxVolumeAmountText.initFromJson("settings_text", "SfxVolumeAmount");
    sfxVolumeAmountText.font_ = buttonFont;
    bgmVolumeAmountText.initFromJson("settings_text", "BgmVolumeAmount");
    bgmVolumeAmountText.font_ = buttonFont;

    // Initialize destructible Background
    levelManager.init();
    levelManager.checkLevelData();

    bgVfxSystem.Initialize(800, 20);
    if (levelManager.getLevelData(100)) {
        levelManager.parseMapInfo(bgWidth, bgHeight, bgTileSize, bgPortalLimit);
        bgFileExist = true;
    } else {
        bgWidth = 80;
        bgHeight = 45;
        bgTileSize = 20;
        bgFileExist = false;
    }
    bgDirt = new Terrain(TerrainMaterial::Dirt, pBgDirtTex, {0.0f, 0.0f}, bgHeight, bgWidth,
                         bgTileSize, true);

    if (bgFileExist) {
        levelManager.parseTerrainInfo(bgDirt->getNodes(), "Dirt");
    }
    bgDirt->initCellsTransform();
    bgDirt->initCellsGraphics();
    bgDirt->initCellsCollider();
    bgDirt->updateTerrain();

    g_debugSystem.setScene(bgDirt, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                           &bgVfxSystem);
}

void updateSettings(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        // Check for button presses to change audio group
        if (buttonIncreaseSfxVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("sfx", +10);
        }
        if (buttonDecreaseSfxVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("sfx", -10);
        }
        if (buttonIncreaseBgmVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("bgm", +10);
        }
        if (buttonDecreaseBgmVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("bgm", -10);
        }

        // Check for button press to go back to main menu
        if (buttonBack.checkMouseClick()) {
            screenFader.StartFadeOut(&GSM, StateId::MainMenu);
        }

        // Get volume amounts
        sfxVolumeAmountText.content_ = std::to_string(g_audioSystem.getGroupVolume("sfx")) + "%";
        bgmVolumeAmountText.content_ = std::to_string(g_audioSystem.getGroupVolume("bgm")) + "%";

        buttonIncreaseSfxVolume.updateTransform();
        buttonDecreaseSfxVolume.updateTransform();
        buttonIncreaseBgmVolume.updateTransform();
        buttonDecreaseBgmVolume.updateTransform();

        buttonBack.updateTransform();

        if (AEInputCheckCurr(AEVK_LBUTTON)) {
            bool hitDirt = bgDirt->destroyAtMouse(20.0f);
            if (hitDirt) {
                bgVfxSystem.SpawnContinuous(VFXType::DirtBurst, GetMouseWorldPos(), deltaTime,
                                            0.1f);
                g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
            } else {
                bgVfxSystem.ResetSpawnTimer();
            }
        } else {
            bgVfxSystem.ResetSpawnTimer();
        }
        bgVfxSystem.Update(deltaTime);
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }
    animManager.UpdateAll(deltaTime);
}

void drawSettings() {

    bg.draw();

    bgDirt->renderTerrain();
    bgVfxSystem.Draw();

    buttonIncreaseSfxVolume.draw();
    buttonDecreaseSfxVolume.draw();
    buttonIncreaseBgmVolume.draw();
    buttonDecreaseBgmVolume.draw();

    buttonBack.draw();

    headerText.draw(true);
    sfxVolumeText.draw();
    bgmVolumeText.draw();
    sfxVolumeAmountText.draw();
    bgmVolumeAmountText.draw();

    animManager.DrawAll();

    g_debugSystem.drawAll();
}

void freeSettings() {
    g_debugSystem.clearScene();
    bgVfxSystem.Free();
    animManager.FreeAll();

    delete bgDirt;
    bgDirt = nullptr;
}

void unloadSettings() {
    // Unload fonts
    if (titleFont) {
        AEGfxDestroyFont(titleFont);
        titleFont = 0;
    }
    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }

    buttonIncreaseSfxVolume.unload();
    buttonDecreaseSfxVolume.unload();
    buttonIncreaseBgmVolume.unload();
    buttonDecreaseBgmVolume.unload();

    buttonBack.unload();

    // Unload destructible background
    Terrain::freeMeshLibrary();
    if (pBgDirtTex) {
        AEGfxTextureUnload(pBgDirtTex);
        pBgDirtTex = nullptr;
    }
    if (pBgStoneTex) {
        AEGfxTextureUnload(pBgStoneTex);
        pBgStoneTex = nullptr;
    }
    if (pBgMagicTex) {
        AEGfxTextureUnload(pBgMagicTex);
        pBgMagicTex = nullptr;
    }
    bg.unload();
}