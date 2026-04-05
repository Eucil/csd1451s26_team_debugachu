/*!
@file       Level.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This source file implements the Level game state,
            handling terrain, fluid simulation, portals, collectibles,
            moss, HUD rendering, the level editor, and win condition logic.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/Level.h"

// Standard library
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Third-party
#include <AEEngine.h>

// Project
#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "Collectible.h"
#include "CollisionSystem.h"
#include "Components.h"
#include "ConfigManager.h"
#include "Confirmation.h"
#include "DebugSystem.h"
#include "FluidSystem.h"
#include "GameStateManager.h"
#include "LevelManager.h"
#include "Moss.h"
#include "MouseUtils.h"
#include "Pause.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "States/MainMenu.h"
#include "Terrain.h"
#include "TileBackground.h"
#include "VFXSystem.h"
#include "WinScreen.h"

// ==========================================
// Terrain Static Variables
// ==========================================
static Terrain* dirt = nullptr;
static Terrain* stone = nullptr;
static Terrain* magic = nullptr;
static AEGfxTexture* pTerrainDirtTex{nullptr};
static AEGfxTexture* pTerrainStoneTex{nullptr};
static AEGfxTexture* pTerrainMagicTex{nullptr};

// ==========================================
// Background Static Variables
// ==========================================
static TiledBackground bg;

// ==========================================
// Game Systems Static Variables
// ==========================================
static FluidSystem fluidSystem;
static StartEndPoint startEndPointSystem;
static PortalSystem portalSystem;
static MossSystem mossSystem;
static PauseSystem pauseSystem;
static ConfirmationSystem confirmationSystem;
static CollectibleSystem collectibleSystem;
static VFXSystem vfxSystem;
static WinScreen winScreen;

// ==========================================
// UI / Buttons Static Variables
// ==========================================
static Button buttonPause;
static Button buttonResume;
static Button buttonRestart;
static Button buttonQuit;
static TextData pauseHeaderText;

// ==========================================
// HUD Static Variables
// ==========================================
static AEGfxTexture* pHudWaterIconTex = nullptr;
static AEGfxTexture* pHudGoalIconTex = nullptr;
static AEGfxTexture* pHudPortalIconTex = nullptr;
static AEGfxVertexList* g_hudIconMesh = nullptr;
static AEGfxVertexList* s_flowerIconMesh = nullptr;
static AEGfxVertexList* g_barMesh = nullptr;
static TextData totalWaterText;
static TextData goalText;
static TextData portalLimitText;
static f32 totalWaterRemaining = 0.0f;
static f32 totalWaterCapacity = 0.0f;
static f32 goalPercentage = 0.0f;
static f32 goalFlowerFrameTimer_ = 0.0f;
static int goalFlowerFrame_ = 0;
static int kGoalFlowerFrames = 4;
static f32 kGoalFlowerFrameTime = 0.25f;
static int winThresholdDivisor = 4;
static bool winTriggered = false;

// ==========================================
// Fonts
// ==========================================
static s8 titleFont = 0;
static s8 font = 0;
static s8 buttonFont = 0;

// ==========================================
// Level config
// ==========================================
static int height, width, tileSize, portalLimit;
static bool fileExist;

// ==========================================
// Animations
// ==========================================
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

// ==========================================
// Static Functions Declarations
// ==========================================
static void spawnWaterWithLimit(f32 deltaTime);
static void drawHudIcon(AEGfxTexture* tex, AEGfxVertexList* mesh, f32 worldX, f32 worldY,
                        f32 iconSize, f32 uvOffsetX = 0.0f);
static void drawPixelBar(f32 worldX, f32 worldY, f32 barWidth, f32 barHeight, f32 fillR, f32 fillG,
                         f32 fillB, f32 borderR, f32 borderG, f32 borderB, f32 percentage);
static void drawPortalLimitUI(f32 x, f32 y);
static void drawTotalWaterBar(f32 x, f32 y, f32 remaining, f32 capacity);
static void drawGoalBar(f32 x, f32 y, f32 percentage);

// =========================================================
//
// loadLevel()
//
// - Loads all persistent assets needed for the level:
// - terrain meshes/textures, fonts, systems, HUD text, and UI buttons.
// - Called once per level session (not on restart).
//
// =========================================================
void loadLevel() {
    // Terrain & Background
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pTerrainDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pTerrainStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
    pTerrainMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");
    bg.loadFromJson("background", "Background");

    // Fonts
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // Systems
    collectibleSystem.load(font);
    winScreen.load(font);
    mossSystem.load(font);
    startEndPointSystem.initializeUI(font);

    // Setup HUD
    totalWaterText.x_ = g_configManager.getFloat("Level", "hud", "waterTextX", -0.69f);
    totalWaterText.y_ = g_configManager.getFloat("Level", "hud", "waterTextY", 0.92f);
    totalWaterText.scale_ = g_configManager.getFloat("Level", "hud", "waterTextScale", 0.5f);
    totalWaterText.content_ = "Water: 0/0";
    totalWaterText.font_ = font;

    goalText.x_ = g_configManager.getFloat("Level", "hud", "goalTextX", -0.12f);
    goalText.y_ = g_configManager.getFloat("Level", "hud", "goalTextY", 0.92f);
    goalText.scale_ = g_configManager.getFloat("Level", "hud", "goalTextScale", 0.5f);
    goalText.content_ = "Goal: 0%";
    goalText.font_ = font;

    portalLimitText.x_ = g_configManager.getFloat("Level", "hud", "portalTextX", 0.42f);
    portalLimitText.y_ = g_configManager.getFloat("Level", "hud", "portalTextY", 0.92f);
    portalLimitText.scale_ = g_configManager.getFloat("Level", "hud", "portalTextScale", 0.5f);
    portalLimitText.content_ = "Portals: 0/0";
    portalLimitText.font_ = font;

    // Level Data
    levelManager.initEditorUI(font);
    if (levelManager.getLevelData(levelManager.getCurrentLevel())) {
        levelManager.parseMapInfo(width, height, tileSize, portalLimit);
        fileExist = true;
    } else {
        std::cout << "Failed to load level data\n";
        std::cout << "Using default values\n";
        width = g_configManager.getInt("Level", "defaults", "width", 80);
        height = g_configManager.getInt("Level", "defaults", "height", 45);
        tileSize = g_configManager.getInt("Level", "defaults", "tileSize", 20);
        portalLimit = g_configManager.getInt("Level", "defaults", "portalLimit", 0);
        fileExist = false;
    }

    // UI buttons
    buttonPause.loadMesh();
    buttonPause.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonResume.loadMesh();
    buttonResume.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    buttonRestart.loadMesh();
    buttonRestart.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    buttonQuit.loadMesh();
    buttonQuit.loadTexture("Assets/Textures/brown_rectangle_80_24.png");

    // Pause and Confirmation Systems
    pauseSystem.loadMesh();
    pauseSystem.resume();
    confirmationSystem.load();
    confirmationSystem.hide();
}

// =========================================================
//
// initializeLevel()
//
// - Initializes all game objects and systems for gameplay
// - terrain, start/end points, collectibles, moss, portals,
// - VFX, HUD textures, UI buttons, and animations.
// - Called on both first load and every restart.
//
// =========================================================
void initializeLevel() {
    // Systems
    fluidSystem.initialize();
    portalSystem.initialize(portalLimit);
    mossSystem.initialize();

    // Terrain
    dirt = new Terrain(TerrainMaterial::Dirt, pTerrainDirtTex, {0.0f, 0.0f}, height, width,
                       tileSize, true);
    stone = new Terrain(TerrainMaterial::Stone, pTerrainStoneTex, {0.0f, 0.0f}, height, width,
                        tileSize, true);
    magic = new Terrain(TerrainMaterial::Magic, pTerrainMagicTex, {0.0f, 0.0f}, height, width,
                        tileSize, false);
    if (fileExist) {
        levelManager.parseTerrainInfo(dirt->getNodes(), "Dirt");
    }
    dirt->initCellsTransform();
    dirt->initCellsGraphics();
    dirt->initCellsCollider();
    dirt->updateTerrain();

    if (fileExist) {
        levelManager.parseTerrainInfo(stone->getNodes(), "Stone");
    }
    stone->initCellsTransform();
    stone->initCellsGraphics();
    stone->initCellsCollider();
    stone->updateTerrain();

    if (fileExist) {
        levelManager.parseTerrainInfo(magic->getNodes(), "Magic");
    }
    magic->initCellsTransform();
    magic->initCellsGraphics();
    magic->initCellsCollider();
    magic->updateTerrain();

    // Game Objects
    startEndPointSystem.initialize();
    if (fileExist) {
        levelManager.parseStartEndInfo(startEndPointSystem);
    }

    collectibleSystem.initialize();
    if (fileExist) {
        levelManager.parseCollectibleInfo(collectibleSystem);
    }
    if (fileExist) {
        levelManager.parseMossInfo(mossSystem);
    }
    if (fileExist) {
        levelManager.parsePortalInfo(portalSystem);
    }

    // VFX Systems
    vfxSystem.initialize(g_configManager.getInt("Level", "vfx", "poolSize", 800),
                         g_configManager.getInt("Level", "vfx", "typeCount", 20));

    // HUD
    winTriggered = false; // reset win latch for this level

    // Reset HUD animation state every level start (including Restart)
    kGoalFlowerFrames = g_configManager.getInt("Level", "hud", "flowerFrameCount", 4);
    kGoalFlowerFrameTime = g_configManager.getFloat("Level", "hud", "flowerFrameTime", 0.25f);
    winThresholdDivisor = g_configManager.getInt("Level", "gameplay", "winThresholdDivisor", 4);
    goalFlowerFrameTimer_ = 0.0f;
    goalFlowerFrame_ = 0;

    // Recreate for restarting
    if (pHudWaterIconTex) {
        AEGfxTextureUnload(pHudWaterIconTex);
        pHudWaterIconTex = nullptr;
    }
    if (pHudGoalIconTex) {
        AEGfxTextureUnload(pHudGoalIconTex);
        pHudGoalIconTex = nullptr;
    }
    if (pHudPortalIconTex) {
        AEGfxTextureUnload(pHudPortalIconTex);
        pHudPortalIconTex = nullptr;
    }
    if (g_hudIconMesh) {
        AEGfxMeshFree(g_hudIconMesh);
        g_hudIconMesh = nullptr;
    }

    pHudWaterIconTex = AEGfxTextureLoad("Assets/Textures/hud_water_icon.png");
    pHudGoalIconTex = AEGfxTextureLoad("Assets/Textures/pink_flower_sprite_sheet.png");
    pHudPortalIconTex = AEGfxTextureLoad("Assets/Textures/wormhole.png");

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    g_hudIconMesh = AEGfxMeshEnd();

    // UI buttons
    buttonPause.initFromJson("level_buttons", "Pause");
    buttonPause.setTextFont(buttonFont);
    buttonResume.initFromJson("level_buttons", "Resume");
    buttonResume.setTextFont(buttonFont);
    buttonRestart.initFromJson("level_buttons", "Restart");
    buttonRestart.setTextFont(buttonFont);
    buttonQuit.initFromJson("level_buttons", "MainMenu");
    buttonQuit.setTextFont(buttonFont);

    // Pause & Confirmation system
    pauseSystem.initFromJson("pause_system", "Background");
    pauseHeaderText.initFromJson("pause_system", "Header");
    pauseHeaderText.font_ = titleFont;
    confirmationSystem.init(buttonFont);

    // Animations
    animManager.clear();
    animManager.add(&screenFader);
    animManager.add(&someOtherCoolAnimation);
    animManager.initializeAll();

    g_debugSystem.setScene(dirt, stone, magic, &fluidSystem, &collectibleSystem, &portalSystem,
                           &startEndPointSystem, &vfxSystem);
}

// =========================================================
//
// updateLevel(GameStateManager& GSM, f32 deltaTime)
//
// - Drives all per-frame gameplay logic
// - input handling, system simulation, HUD value updates,
// - win condition check, and animation updates.
//
// =========================================================
void updateLevel(GameStateManager& GSM, f32 deltaTime) {

    if (!g_debugSystem.isOpen()) {
        // =========================
        // Debug system not open
        // =========================
        if (AEInputCheckTriggered(AEVK_Z) || 0 == AESysDoesWindowExist()) {
            std::cout << "Z triggered\n";
            g_debugSystem.open();
        }

        if (pauseSystem.isPaused()) {
            // ====================
            // Game is paused
            // ====================

            // Pause Menu Inputs
            if (!confirmationSystem.isShowing()) {
                if (buttonResume.checkMouseClick()) {
                    pauseSystem.resume();
                }
                if (buttonRestart.checkMouseClick()) {
                    confirmationSystem.show();
                    confirmationSystem.setTask(ConfirmationTask::Restart);
                }
                if (buttonQuit.checkMouseClick()) {
                    confirmationSystem.show();
                    confirmationSystem.setTask(ConfirmationTask::MainMenu);
                }
            }

            if (confirmationSystem.confirmationYesClicked()) {
                if (confirmationSystem.getTask() == ConfirmationTask::Restart) {
                    screenFader.startFadeOut(&GSM, StateId::Restart);
                    pauseSystem.resume();
                } else if (confirmationSystem.getTask() == ConfirmationTask::MainMenu) {
                    screenFader.startFadeOut(&GSM, StateId::MainMenu);
                }
            }
            if (confirmationSystem.confirmationNoClicked()) {
                confirmationSystem.hide();
                confirmationSystem.setTask(ConfirmationTask::No);
            }

            // UI buttons
            buttonResume.updateTransform();
            buttonRestart.updateTransform();
            buttonQuit.updateTransform();

        } else {
            // ====================
            // Game is not paused
            // ====================

            // Click pause button
            if (buttonPause.checkMouseClick()) {
                pauseSystem.pause();
            }
            buttonPause.updateTransform();

            // Keyboard/Mouse inputs for level editor and gameplay
            if (levelManager.getLevelEditorMode() == EditorMode::Edit) {
                // ====================
                // Level editor mode
                // ====================

                // System updates for editor mode
                levelManager.updateLevelEditor();
                collectibleSystem.update(deltaTime, fluidSystem.getParticlePool(FluidType::Water),
                                         vfxSystem);
                mossSystem.update(deltaTime, fluidSystem.getParticlePool(FluidType::Water),
                                  startEndPointSystem, vfxSystem);
                portalSystem.rotatePortal();

                // Inputs to build level
                if (!levelManager.getDisplayBuilderContainer()) {
                    f32 brush_size = levelManager.brushRadius_;
                    switch (levelManager.getCurrentGameBlock()) {
                    case GameBlock::Dirt:
                        if (AEInputCheckCurr(AEVK_LBUTTON)) {
                            dirt->buildAtMouse(brush_size);
                        } else if (AEInputCheckCurr(AEVK_RBUTTON)) {
                            dirt->destroyAtMouse(brush_size);
                        }
                        break;
                    case GameBlock::Stone:
                        if (AEInputCheckCurr(AEVK_LBUTTON)) {
                            stone->buildAtMouse(brush_size);
                        } else if (AEInputCheckCurr(AEVK_RBUTTON)) {
                            stone->destroyAtMouse(brush_size);
                        }
                        break;
                    case GameBlock::Magic:
                        if (AEInputCheckCurr(AEVK_LBUTTON)) {
                            magic->buildAtMouse(brush_size);
                        } else if (AEInputCheckCurr(AEVK_RBUTTON)) {
                            magic->destroyAtMouse(brush_size);
                        }
                        break;
                    case GameBlock::StartPoint:
                        if (AEInputCheckReleased(AEVK_LBUTTON)) {
                            startEndPointSystem.spawnAtMousePos(StartEndType::Pipe,
                                                                GoalDirection::Down);
                        } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                            startEndPointSystem.deleteAtMousePos();
                        }
                        break;
                    case GameBlock::EndPoint:
                        if (AEInputCheckReleased(AEVK_LBUTTON)) {
                            startEndPointSystem.spawnAtMousePos(StartEndType::Flower,
                                                                GoalDirection::Up);
                        } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                            startEndPointSystem.deleteAtMousePos();
                        }
                        break;
                    case GameBlock::Collectible:
                        if (AEInputCheckReleased(AEVK_LBUTTON)) {
                            collectibleSystem.spawnAtMousePos();
                        } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                            collectibleSystem.destroyAtMousePos();
                        }
                        break;
                    case GameBlock::Moss:
                        if (AEInputCheckReleased(AEVK_LBUTTON)) {
                            mossSystem.spawnAtMousePos();
                        } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                            mossSystem.destroyAtMousePos();
                        }
                        break;
                    case GameBlock::Portal:
                        if (magic->isNearestNodeToMouseAtThreshold() == true) {
                            if (AEInputCheckTriggered(AEVK_RBUTTON) ||
                                0 == AESysDoesWindowExist()) {
                                portalSystem.checkMouseClick();
                            } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                                portalSystem.resetIframe();
                            }
                        }
                        if (AEInputCheckTriggered(AEVK_MBUTTON)) {
                            portalSystem.rotatePortal();
                        }
                        break;
                    default:
                        break;
                    }
                }
                // Inputs to save level
                if (AEInputCheckReleased(AEVK_S)) {
                    levelManager.saveMapInfo(width, height, tileSize,
                                             portalSystem.getPortalLimit());
                    levelManager.saveTerrainInfo(dirt->getNodes(), "Dirt");
                    levelManager.saveTerrainInfo(stone->getNodes(), "Stone");
                    levelManager.saveTerrainInfo(magic->getNodes(), "Magic");
                    levelManager.saveStartEndInfo(startEndPointSystem.startPoints_,
                                                  startEndPointSystem.endPoint_);
                    levelManager.saveCollectibleInfo(collectibleSystem.getCollectibles());
                    levelManager.saveMossInfo(mossSystem.getMosses());
                    levelManager.savePortalInfo(portalSystem);
                    levelManager.writeToFile(levelManager.getCurrentLevel());
                }
            } else {
                // ====================
                // Gameplay mode
                // ====================

                // Input for gameplay
                if (AEInputCheckCurr(AEVK_LBUTTON)) {
                    bool hitDirt = dirt->destroyAtMouse(20.0f);
                    // Only run the VFX timer if we actually dug through dirt
                    if (hitDirt) {
                        vfxSystem.spawnContinuous(VFXType::DirtBurst, getMouseWorldPos(), deltaTime,
                                                  0.1f);
                        g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
                    } else {
                        vfxSystem.resetSpawnTimer();
                    }
                } else {
                    vfxSystem.resetSpawnTimer();
                }

                if (AEInputCheckTriggered(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {

                    startEndPointSystem.checkMouseClick();

                } else if (AEInputCheckReleased(AEVK_LBUTTON)) {

                    startEndPointSystem.resetIframe();
                }

                // If magic terrain is near mouse click, place portal
                if (magic->isNearestNodeToMouseAtThreshold() == true) {
                    if (AEInputCheckTriggered(AEVK_RBUTTON) || 0 == AESysDoesWindowExist()) {
                        portalSystem.checkMouseClick();
                    } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                        portalSystem.resetIframe();
                    }
                }
                portalSystem.rotatePortal();

                // HUD Updates
                totalWaterRemaining = 0.0f;
                totalWaterCapacity = 0.0f;
                for (const auto& startPoint : startEndPointSystem.startPoints_) {
                    if (startPoint.active_ && startPoint.type_ == StartEndType::Pipe) {
                        totalWaterRemaining += startPoint.waterRemaining_;
                        totalWaterCapacity += startPoint.waterCapacity_;
                    }
                }
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "Water: %.0f/%.0f", totalWaterRemaining,
                         totalWaterCapacity);
                totalWaterText.content_ = buffer;

                // Calculate
                // percentage based on particles collected vs max particles
                goalPercentage =
                    (static_cast<f32>(startEndPointSystem.particlesCollected_) /
                     static_cast<f32>(fluidSystem.particleMaxCount_ / winThresholdDivisor)) *
                    100.0f;
                if (goalPercentage > 100.0f)
                    goalPercentage = 100.0f;

                char goalBuffer[32];
                snprintf(goalBuffer, sizeof(goalBuffer), "Goal: %.0f%%", goalPercentage);
                goalText.content_ = goalBuffer;

                int portalsUsed = portalSystem.getPortalCount();
                int portalsLimit = portalSystem.getPortalLimit();
                portalLimitText.content_ =
                    "Portals: " + std::to_string(portalsUsed) + "/" + std::to_string(portalsLimit);

                // System updates for gameplay
                spawnWaterWithLimit(deltaTime);

                collectibleSystem.update(deltaTime, fluidSystem.getParticlePool(FluidType::Water),
                                         vfxSystem);
                mossSystem.update(deltaTime, fluidSystem.getParticlePool(FluidType::Water),
                                  startEndPointSystem, vfxSystem);

                fluidSystem.update(deltaTime, {dirt, stone});
                startEndPointSystem.update(deltaTime, fluidSystem.getParticlePool(FluidType::Water),
                                           vfxSystem);
                portalSystem.update(deltaTime, fluidSystem.getParticlePool(FluidType::Water),
                                    vfxSystem);
                vfxSystem.update(deltaTime);

                // Animate goal bar icon
                goalFlowerFrameTimer_ += deltaTime;
                if (goalFlowerFrameTimer_ >= kGoalFlowerFrameTime) {
                    goalFlowerFrameTimer_ -= kGoalFlowerFrameTime;
                    goalFlowerFrame_ = (goalFlowerFrame_ + 1) % kGoalFlowerFrames;
                }

                // Win condition: fire exactly when the goal bar display hits 100%.
                // goalPercentage uses particleMaxCount_/4 as its threshold.
                // CheckWinCondition uses particleMaxCount_/3 which is a higher
                // bar than the display, causing the win to fire after the bar
                // already shows 100%. Using goalPercentage >= 100 keeps both in sync.
                if (goalPercentage >= 100.0f && !winTriggered) {
                    winTriggered = true;

                    // Save highscore
                    levelManager.saveLevelProgress(levelManager.getCurrentLevel(),
                                                   collectibleSystem.getCollectedCount());
                    winScreen.show(collectibleSystem.getCollectedCount(),
                                   collectibleSystem.getTotalCount(),
                                   levelManager.getCurrentLevel());
                }

                // Win Screen
                winScreen.update(GSM);
                // Pause system
                pauseSystem.setTransformFillScreen();
                pauseSystem.updateTransform();
            }
        }
    } else {
        // =========================
        // Debug system open
        // =========================
        if (AEInputCheckTriggered(AEVK_Z) || 0 == AESysDoesWindowExist()) {
            std::cout << "Z triggered\n";
            g_debugSystem.close();
        }

        g_debugSystem.update();
    }
    // Always update these
    animManager.updateAll(deltaTime);
    confirmationSystem.update();
}

// =========================================================
//
// drawLevel()
//
// - Renders all visual layers each frame
// - background, terrain, game objects, editor overlays,
// - HUD bars, pause menu, debug visuals, and animations.
//
// =========================================================
void drawLevel() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    bg.draw();

    // ====================
    // Gameplay mode
    // ====================
    fluidSystem.drawColor();
    startEndPointSystem.drawTexture(fluidSystem.particleMaxCount_);

    dirt->renderTerrain();
    stone->renderTerrain();
    magic->renderTerrain();
    portalSystem.draw();
    vfxSystem.draw();

    collectibleSystem.draw();
    mossSystem.draw();

    // Add preview render in gameplay
    if (levelManager.getLevelEditorMode() == EditorMode::None) {
        // Only show portal preview if mouse is near magic terrain, otherwise default dirt
        if (magic->isNearestNodeToMouseAtThreshold() == true) {
            portalSystem.drawPreview();
        } else {
            levelManager.drawBrushPreview(TerrainMaterial::Dirt, 20.f);
        }
    }

    // ====================
    // Editor Mode
    // ====================
    if (levelManager.getLevelEditorMode() == EditorMode::Edit) {
        levelManager.renderLevelEditorUI();
        switch (levelManager.getCurrentGameBlock()) {
        case GameBlock::Dirt:;
            levelManager.drawBrushPreview(TerrainMaterial::Dirt);
            break;
        case GameBlock::Stone:
            levelManager.drawBrushPreview(TerrainMaterial::Stone);
            break;
        case GameBlock::Magic:
            levelManager.drawBrushPreview(TerrainMaterial::Magic);
            break;
        case GameBlock::StartPoint:
            startEndPointSystem.drawPreview(StartEndType::Pipe);
            break;
        case GameBlock::EndPoint:
            startEndPointSystem.drawPreview(StartEndType::Flower);
            break;
        case GameBlock::Collectible:
            collectibleSystem.drawPreview();
            break;
        case GameBlock::Moss:
            mossSystem.drawPreview();
            break;
        case GameBlock::Portal:
            portalSystem.drawPreview();
            break;
        default:
            break;
        }
    }

    // ====================
    // UI Elements
    // ====================
    collectibleSystem.drawUI();
    totalWaterText.draw();
    goalText.draw();

    drawTotalWaterBar(totalWaterText.x_ + 0.35f, totalWaterText.y_ - 0.09f, totalWaterRemaining,
                      totalWaterCapacity);
    drawGoalBar(goalText.x_ + 0.35f, goalText.y_ - 0.09f, goalPercentage);

    portalLimitText.draw();
    drawPortalLimitUI(portalLimitText.x_ + 0.06f, portalLimitText.y_ - 0.09f);

    buttonPause.draw();

    winScreen.draw();

    // When paused
    if (pauseSystem.isPaused()) {
        // Background
        if (!confirmationSystem.isShowing()) {

            pauseSystem.renderBackground();

            pauseHeaderText.draw(true);

            // UI buttons
            buttonResume.draw();
            buttonRestart.draw();
            buttonQuit.draw();
        }
    }

    confirmationSystem.draw();

    g_debugSystem.drawAll();

    // Animations
    animManager.drawAll();
}

// =========================================================
//
// freeLevel()
//
// - Frees all per-session runtime resources
// - game systems, HUD meshes, and terrain objects.
// - Called on every restart and on level exit.
//
// =========================================================
void freeLevel() {
    // Systems
    g_debugSystem.clearScene();
    winScreen.free();
    fluidSystem.free();
    startEndPointSystem.free();
    portalSystem.free();
    vfxSystem.free();
    animManager.freeAll();
    mossSystem.free();
    collectibleSystem.free();

    // HUD Meshes
    if (g_barMesh) {
        AEGfxMeshFree(g_barMesh);
        g_barMesh = nullptr;
    }
    if (g_hudIconMesh) {
        AEGfxMeshFree(g_hudIconMesh);
        g_hudIconMesh = nullptr;
    }
    if (s_flowerIconMesh) {
        AEGfxMeshFree(s_flowerIconMesh);
        s_flowerIconMesh = nullptr;
    }

    // Terrain
    delete dirt;
    dirt = nullptr;
    delete stone;
    stone = nullptr;
    delete magic;
    magic = nullptr;
}

// =========================================================
//
// unloadLevel()
//
// - Unloads all persistent assets loaded in loadLevel()
// - fonts, terrain textures, HUD textures, background,
// - UI buttons, pause system, and win screen.
// - Called once when leaving the Level state.
//
// =========================================================
void unloadLevel() {
    // Systems
    mossSystem.unload();
    Terrain::freeMeshLibrary();

    // Unload fonts
    if (titleFont) {
        AEGfxDestroyFont(titleFont);
        titleFont = 0;
    }
    if (font) {
        AEGfxDestroyFont(font);
        font = 0;
    }
    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }

    // unload terrain textures
    if (pTerrainDirtTex) {
        AEGfxTextureUnload(pTerrainDirtTex);
        pTerrainDirtTex = nullptr;
    }
    if (pTerrainStoneTex) {
        AEGfxTextureUnload(pTerrainStoneTex);
        pTerrainStoneTex = nullptr;
    }
    if (pTerrainMagicTex) {
        AEGfxTextureUnload(pTerrainMagicTex);
        pTerrainMagicTex = nullptr;
    }
    // unload HUD textures
    if (pHudWaterIconTex) {
        AEGfxTextureUnload(pHudWaterIconTex);
        pHudWaterIconTex = nullptr;
    }
    if (pHudGoalIconTex) {
        AEGfxTextureUnload(pHudGoalIconTex);
        pHudGoalIconTex = nullptr;
    }
    if (pHudPortalIconTex) {
        AEGfxTextureUnload(pHudPortalIconTex);
        pHudPortalIconTex = nullptr;
    }
    // Background
    bg.unload();

    // Level Manager
    levelManager.freeLevelEditor();
    // NOTE: Do NOT reset currentLevel_ to 0 here.
    // WinScreen sets it to nextLevel_ before triggering StateId::Level.
    // Resetting it here would wipe that value and always reload Level 0.

    // UI buttons
    buttonPause.unload();
    buttonResume.unload();
    buttonRestart.unload();
    buttonQuit.unload();

    // Pause & Confirmation system
    pauseSystem.unload();
    confirmationSystem.unload();

    // Win Screen
    winScreen.unload();
}

// =============================================================================
// Static Function Definition
// =============================================================================

// =========================================================
//
// spawnWaterWithLimit(f32 deltaTime)
//
// - Spawns water particles from all active pipe-type start points
// - at a fixed rate, consuming water capacity per spawn.
// - Automatically stops a pipe when its water is depleted.
//
// =========================================================
static void spawnWaterWithLimit(f32 deltaTime) {
    // Use a static timer that persists between function calls
    static f32 globalSpawnTimer = 0.0f;

    // Decrement the global timer
    globalSpawnTimer -= deltaTime;

    // Only spawn if enough time has passed
    if (globalSpawnTimer <= 0.0f) {
        // Reset timer to control spawn rate across all start points
        globalSpawnTimer = 0.025f; // Same spawn rate as before

        // Loop through all start points
        for (auto& startPoint : startEndPointSystem.startPoints_) {
            // Only process active pipe-type start points that are releasing water
            if (startPoint.releaseWater_ && startPoint.type_ == StartEndType::Pipe) {

                // Check if there's water remaining (or infinite mode)
                if (startPoint.waterRemaining_ > 0.0f || startPoint.infiniteWater_) {

                    // Consume water (unless infinite)
                    if (!startPoint.infiniteWater_) {
                        startPoint.waterRemaining_ -= 0.5f; // Adjust consumption rate as needed
                        if (startPoint.waterRemaining_ < 0.0f) {
                            startPoint.waterRemaining_ = 0.0f;
                            startPoint.releaseWater_ = false; // Auto-stop when empty
                        }
                    }

                    // Only spawn particle if there's still water
                    if (startPoint.waterRemaining_ > 0.0f || startPoint.infiniteWater_) {
                        // Spawn particle at the start point position
                        f32 randRadius = 10.0f;

                        // Calculate random x offset within the start point's width
                        f32 xOffset = startPoint.transform_.pos_.x +
                                      AERandFloat() * startPoint.transform_.scale_.x -
                                      (startPoint.transform_.scale_.x / 2.f);
                        AEVec2 position = {xOffset, startPoint.transform_.pos_.y -
                                                        (startPoint.transform_.scale_.y / 2.f) -
                                                        (randRadius)};

                        // Spawn the water particle
                        fluidSystem.spawnParticle(position.x, position.y, randRadius,
                                                  FluidType::Water);
                    }
                }
            }
        }
    }
}

// =========================================================
//
// drawHudIcon(AEGfxTexture* tex, AEGfxVertexList* mesh, f32 worldX, f32 worldY,
//             f32 iconSize, f32 uvOffsetX = 0.0f)
//
// - Renders a single HUD icon quad at the given world position
// - using the provided texture, mesh, and optional UV x-offset.
//
// =========================================================
static void drawHudIcon(AEGfxTexture* tex, AEGfxVertexList* mesh, f32 worldX, f32 worldY,
                        f32 iconSize, f32 uvOffsetX) {
    if (!tex || !mesh)
        return;
    AEMtx33 S, T, W;
    AEMtx33Scale(&S, iconSize, iconSize);
    AEMtx33Trans(&T, worldX, worldY);
    AEMtx33Concat(&W, &T, &S);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransform(W.m);
    AEGfxTextureSet(tex, uvOffsetX, 0.0f);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// =========================================================
//
// drawPixelBar(f32 worldX, f32 worldY, f32 barWidth, f32 barHeight, f32 fillR, f32 fillG,
//              f32 fillB, f32 borderR, f32 borderG, f32 borderB, f32 percentage)
//
// - Renders a segmented pixel-art progress bar with a dark outer panel,
// - accent border, empty track, and gradient-shaded fill tiles.
// - Fill is split into kTiles segments with a left-to-right brightness ramp
// - and a shimmer highlight strip on each filled tile.
//
// =========================================================
static void drawPixelBar(f32 worldX, f32 worldY, f32 barWidth, f32 barHeight, f32 fillR, f32 fillG,
                         f32 fillB, f32 borderR, f32 borderG, f32 borderB, f32 percentage) {
    if (!g_barMesh)
        g_barMesh = createRectMesh();
    if (percentage < 0.0f)
        percentage = 0.0f;
    if (percentage > 1.0f)
        percentage = 1.0f;

    AEMtx33 T, S, W;
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Outer dark panel
    AEMtx33Trans(&T, worldX, worldY);
    AEMtx33Scale(&S, barWidth + 6.0f, barHeight + 6.0f);
    AEMtx33Concat(&W, &T, &S);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.08f, 0.05f, 0.02f, 1.0f);
    AEGfxSetTransform(W.m);
    AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

    // Accent border
    AEMtx33Trans(&T, worldX, worldY);
    AEMtx33Scale(&S, barWidth + 4.0f, barHeight + 4.0f);
    AEMtx33Concat(&W, &T, &S);
    AEGfxSetColorToMultiply(borderR, borderG, borderB, 1.0f);
    AEGfxSetTransform(W.m);
    AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

    // Empty track
    AEMtx33Trans(&T, worldX, worldY);
    AEMtx33Scale(&S, barWidth, barHeight);
    AEMtx33Concat(&W, &T, &S);
    AEGfxSetColorToMultiply(0.04f, 0.06f, 0.02f, 1.0f);
    AEGfxSetTransform(W.m);
    AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

    if (percentage > 0.0f) {
        constexpr int kTiles = 7;
        f32 fillW = barWidth * percentage;
        f32 tileW = barWidth / static_cast<f32>(kTiles);
        f32 startX = worldX - barWidth * 0.5f;

        for (int i = 0; i < kTiles; ++i) {
            f32 tileLeft = startX + i * tileW;
            if (tileLeft >= startX + fillW)
                break;
            f32 actualW = tileW - 1.0f;
            if (tileLeft + actualW > startX + fillW)
                actualW = startX + fillW - tileLeft;
            if (actualW <= 0.0f)
                break;
            f32 tileCX = tileLeft + actualW * 0.5f;
            f32 t = static_cast<f32>(i) / static_cast<f32>(kTiles - 1);
            f32 bright = 0.55f + t * 0.45f;

            AEMtx33Trans(&T, tileCX, worldY);
            AEMtx33Scale(&S, actualW, barHeight - 2.0f);
            AEMtx33Concat(&W, &T, &S);
            AEGfxSetTransparency(0.9f);
            AEGfxSetColorToMultiply(fillR * bright, fillG * bright, fillB * bright, 1.0f);
            AEGfxSetTransform(W.m);
            AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

            // Shimmer
            AEMtx33Trans(&T, tileCX, worldY + (barHeight - 2.0f) * 0.5f - 1.5f);
            AEMtx33Scale(&S, actualW, 2.0f);
            AEMtx33Concat(&W, &T, &S);
            AEGfxSetTransparency(0.22f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetTransform(W.m);
            AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
}

// =========================================================
//
// drawPortalLimitUI()
//
// - Renders the portal icon to the left of the portal limit text anchor.
//
// =========================================================
static void drawPortalLimitUI(f32 x, f32 y) {
    f32 worldX = x * 710.0f;
    f32 worldY = y * 510.0f;
    f32 iconSize = 32.0f;

    // Icon sits just to the left of where the text anchor is
    drawHudIcon(pHudPortalIconTex, g_hudIconMesh,
                worldX - iconSize, // nudge left of text
                worldY, iconSize);
}

// =========================================================
//
// drawTotalWaterBar()
//
// - Renders the water icon and a progress bar showing
// - remaining water as a fraction of total capacity.
//
// =========================================================
static void drawTotalWaterBar(f32 x, f32 y, f32 remaining, f32 capacity) {
    if (capacity <= 0.0f)
        return;
    f32 worldX = x * 710.0f;
    f32 worldY = y * 510.0f;
    f32 barW = 200.0f, barH = 20.0f, iconSize = 32.0f;

    drawHudIcon(pHudWaterIconTex, g_hudIconMesh, worldX - barW * 0.5f - iconSize * 0.5f - 4.0f,
                worldY, iconSize);

    drawPixelBar(worldX, worldY, barW, barH, 0.18f, 0.57f, 1.0f, 0.0f, 0.0f, 0.45f,
                 remaining / capacity);
}

// =========================================================
//
// drawGoalBar()
//
// - Renders the goal flower icon and a progress bar showing
// - the current goal completion percentage (0-100).
// - Lazily creates the flower icon mesh with baked UVs on first call.
//
// =========================================================
static void drawGoalBar(f32 x, f32 y, f32 percentage) {
    if (percentage < 0.0f)
        percentage = 0.0f;
    if (percentage > 100.0f)
        percentage = 100.0f;
    f32 worldX = x * 710.0f;
    f32 worldY = y * 510.0f;
    f32 barW = 200.0f, barH = 20.0f, iconSize = 32.0f;

    // pink_flower_sprite_sheet.png is 4 frames wide.
    // We show frame 3 (the fully bloomed flower) as the goal icon.
    // A dedicated mesh with baked UVs avoids the stretch that would
    // occur if we used g_hudIconMesh (which spans U 0.0 -> 1.0).
    if (pHudGoalIconTex) {
        if (!s_flowerIconMesh) {
            constexpr float u0 = 3.0f / 4.0f, u1 = 1.0f;
            AEGfxMeshStart();
            AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, u0, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f,
                        -0.5f, 0.5f, 0xFFFFFFFF, u0, 0.0f);
            AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, u1, 0.0f, -0.5f,
                        0.5f, 0xFFFFFFFF, u0, 0.0f);
            s_flowerIconMesh = AEGfxMeshEnd();
        }
        f32 iconX = worldX - barW * 0.5f - iconSize * 0.5f - 4.0f;
        AEMtx33 S, T, W;
        AEMtx33Scale(&S, iconSize, iconSize);
        AEMtx33Trans(&T, iconX, worldY);
        AEMtx33Concat(&W, &T, &S);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransform(W.m);
        AEGfxTextureSet(pHudGoalIconTex, 0.0f, 0.0f);
        AEGfxMeshDraw(s_flowerIconMesh, AE_GFX_MDM_TRIANGLES);
    }

    drawPixelBar(worldX, worldY, barW, barH, 0.16f, 0.72f, 0.22f, 0.0f, 0.35f, 0.0f,
                 percentage / 100.0f);
}
