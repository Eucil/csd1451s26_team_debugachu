/*!
@file       Level.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/Level.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <AEEngine.h>

#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "Collectible.h"
#include "CollisionSystem.h"
#include "Components.h"
#include "ConfigManager.h"
#include "DebugSystem.h"
#include "FluidSystem.h"
#include "GameStateManager.h"
#include "Moss.h"
#include "Pause.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "States/LevelManager.h"
#include "States/MainMenu.h"
#include "Terrain.h"
#include "VFXSystem.h"
#include "WinScreen.h"

static Terrain* dirt = nullptr;
static Terrain* stone = nullptr;
static Terrain* magic = nullptr;

static AEGfxTexture* pTerrainDirtTex{nullptr};
static AEGfxTexture* pTerrainStoneTex{nullptr};
static AEGfxTexture* pTerrainMagicTex{nullptr};

static FluidSystem fluidSystem;
static StartEndPoint startEndPointSystem;
static PortalSystem portalSystem;
// tc added start
static MossSystem mossSystem;

static WinScreen winScreen;

// tc added end

static TextData pauseHeaderText;

// Fonts
static s8 titleFont = 0;
static s8 font = 0;
static s8 buttonFont = 0;

static int height, width, tileSize, portalLimit;
static bool fileExist;

static VFXSystem vfxSystem;

// Buttons
static Button buttonPause;
static Button buttonResume;
static Button buttonRestart;
static Button buttonQuit;

static PauseSystem pauseSystem;
static bool winTriggered = false; // latches true once win fires this session

// tc added start
static CollectibleSystem collectibleSystem;
static TextData totalWaterText;
static TextData goalText;
static f32 totalWaterRemaining = 0.0f;
static f32 totalWaterCapacity = 0.0f;
static f32 goalPercentage = 0.0f;
static AEGfxVertexList* g_barMesh = nullptr; // Global bar mesh for cleanup

// Background
static TiledBackground bg;

// HUD icon textures -- loaded in LoadLevel, freed in FreeLevel+UnloadLevel
static AEGfxTexture* pHudWaterIconTex = nullptr;
static AEGfxTexture* pHudGoalIconTex = nullptr;
static AEGfxVertexList* g_hudIconMesh = nullptr;

// Goal icon animation
static f32 goalFlowerFrameTimer_ = 0.0f;
static int goalFlowerFrame_ = 0;
static constexpr int kGoalFlowerFrames = 4;
static constexpr f32 kGoalFlowerFrameTime = 0.25f;
// tc added end

// Animations
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

// Static Functions
static void SpawnWaterWithLimit(f32 deltaTime);
static void DrawTotalWaterBar(f32 x, f32 y, f32 remaining, f32 capacity);
static void DrawGoalBar(f32 x, f32 y, f32 percentage);

void LoadLevel() {
    // std::cout << "Load level 3\n";
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pTerrainDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pTerrainStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
    pTerrainMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");
    bg.loadFromJson("background", "Background");

    // Setup texts
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // tc added start
    collectibleSystem.Load(font);
    winScreen.Load(font);
    mossSystem.Load(font);

    startEndPointSystem.InitializeUI(font);

    totalWaterText.x_ = -0.39f;
    totalWaterText.y_ = 0.92f;
    totalWaterText.scale_ = 0.5f;
    totalWaterText.content_ = "Water: 0/0";
    totalWaterText.font_ = font;

    goalText.x_ = 0.2f;
    goalText.y_ = 0.92f;
    goalText.scale_ = 0.5f;
    goalText.content_ = "Goal: 0%";
    goalText.font_ = font;

    // tc added end

    levelManager.initEditorUI(font);

    if (levelManager.getLevelData(levelManager.getCurrentLevel())) {
        levelManager.parseMapInfo(width, height, tileSize, portalLimit);
        fileExist = true;
    } else {
        std::cout << "Failed to load level data\n";
        std::cout << "Using default values\n";
        width = 80;
        height = 45;
        tileSize = 20;
        portalLimit = 0;
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

    pauseSystem.loadMesh();
    // Once level is loaded, make sure it is not paused
    pauseSystem.resume();
}

void InitializeLevel() {
    // std::cout << "Initialize level 3\n";
    fluidSystem.Initialize();
    portalSystem.Initialize(portalLimit);
    mossSystem.Initialize();

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

    startEndPointSystem.Initialize();
    if (fileExist) {
        levelManager.parseStartEndInfo(startEndPointSystem);
    }

    printf("=== WATER DEBUG ===\n");
    printf("Number of start points: %zu\n", startEndPointSystem.startPoints_.size());
    float totalWater = 0.0f;
    for (const auto& sp : startEndPointSystem.startPoints_) {
        if (sp.active_ && sp.type_ == StartEndType::Pipe) {
            printf("  Start point at (%.1f, %.1f): water = %.1f/%.1f\n", sp.transform_.pos_.x,
                   sp.transform_.pos_.y, sp.waterRemaining_, sp.waterCapacity_);
            totalWater += sp.waterRemaining_;
        }
    }
    printf("Total water: %.1f\n", totalWater);
    printf("===================\n");

    collectibleSystem.Initialize();
    if (fileExist) {
        levelManager.parseCollectibleInfo(collectibleSystem);
    }
    if (fileExist) {
        levelManager.parseMossInfo(mossSystem);
    }
    if (fileExist) {
        levelManager.parsePortalInfo(portalSystem);
    }

    vfxSystem.Initialize(800, 20);
    winTriggered = false; // reset win latch for this level

    // Reset HUD animation state every level start (including Restart)
    goalFlowerFrameTimer_ = 0.0f;
    goalFlowerFrame_ = 0;

    // HUD icon textures and mesh -- created here so Restart (Free+Initialize)
    // always recreates them. LoadLevel/UnloadLevel are NOT called on Restart.
    if (pHudWaterIconTex) {
        AEGfxTextureUnload(pHudWaterIconTex);
        pHudWaterIconTex = nullptr;
    }
    if (pHudGoalIconTex) {
        AEGfxTextureUnload(pHudGoalIconTex);
        pHudGoalIconTex = nullptr;
    }
    if (g_hudIconMesh) {
        AEGfxMeshFree(g_hudIconMesh);
        g_hudIconMesh = nullptr;
    }

    pHudWaterIconTex = AEGfxTextureLoad("Assets/Textures/hud_water_icon.png");
    pHudGoalIconTex = AEGfxTextureLoad("Assets/Textures/pink_flower_sprite_sheet.png");

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
    buttonQuit.initFromJson("level_buttons", "Quit");
    buttonQuit.setTextFont(buttonFont);

    // Pause system
    pauseSystem.initFromJson("pause_system", "Background");
    pauseHeaderText.initFromJson("pause_system", "Header");
    pauseHeaderText.font_ = titleFont;

    // Animations
    animManager.Clear();
    animManager.Add(&screenFader);
    animManager.Add(&someOtherCoolAnimation);
    animManager.InitializeAll();

    g_debugSystem.setScene(dirt, stone, magic, &fluidSystem, &collectibleSystem, &portalSystem,
                           &startEndPointSystem, &vfxSystem);
}

// tc added start - Function to handle water spawning with limit
static void SpawnWaterWithLimit(f32 deltaTime) {
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
                        fluidSystem.SpawnParticle(position.x, position.y, randRadius,
                                                  FluidType::Water);
                    }
                }
            }
        }
    }
}
// tc added end

void UpdateLevel(GameStateManager& GSM, f32 deltaTime) {
    // std::cout << "Update level 3\n";

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

            if (buttonResume.checkMouseClick()) {
                pauseSystem.resume();
            }
            if (buttonRestart.checkMouseClick()) {
                screenFader.StartFadeOut(&GSM, StateId::Restart);
                // GSM.nextState_ = StateId::Restart;
                pauseSystem.resume();
            }
            if (buttonQuit.checkMouseClick()) {
                screenFader.StartFadeOut(&GSM, StateId::MainMenu);
                // GSM.nextState_ = StateId::MainMenu;
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

            // Press Q to go to main menu
            if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist()) {
                std::cout << "Q triggered\n";
                GSM.nextState_ = StateId::MainMenu;
            }

            // Press R to restart
            if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
                std::cout << "R triggered\n";
                GSM.nextState_ = StateId::Restart;
            }

            // Keyboard/Mouse inputs for level editor and gameplay
            if (levelManager.getLevelEditorMode() == EditorMode::Edit) {
                // ====================
                // Level editor mode
                // ====================
                levelManager.updateLevelEditor();
                collectibleSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));
                mossSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water),
                                  startEndPointSystem);
                portalSystem.RotatePortal();

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
                            startEndPointSystem.SpawnAtMousePos(StartEndType::Pipe,
                                                                GoalDirection::Down);
                        } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                            startEndPointSystem.DeleteAtMousePos();
                        }
                        break;
                    case GameBlock::EndPoint:
                        if (AEInputCheckReleased(AEVK_LBUTTON)) {
                            startEndPointSystem.SpawnAtMousePos(StartEndType::Flower,
                                                                GoalDirection::Up);
                        } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                            startEndPointSystem.DeleteAtMousePos();
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
                                portalSystem.CheckMouseClick();
                            } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                                portalSystem.ResetIframe();
                            }
                        }
                        if (AEInputCheckTriggered(AEVK_MBUTTON)) {
                            portalSystem.RotatePortal();
                        }
                        break;
                    default:
                        break;
                    }
                }
                // Inputs to save level
                if (AEInputCheckReleased(AEVK_S)) {
                    levelManager.saveMapInfo(width, height, tileSize,
                                             portalSystem.GetPortalLimit());
                    levelManager.saveTerrainInfo(dirt->getNodes(), "Dirt");
                    levelManager.saveTerrainInfo(stone->getNodes(), "Stone");
                    levelManager.saveTerrainInfo(magic->getNodes(), "Magic");
                    levelManager.saveStartEndInfo(startEndPointSystem.startPoints_,
                                                  startEndPointSystem.endPoint_);
                    levelManager.saveCollectibleInfo(collectibleSystem.GetCollectibles());
                    levelManager.saveMossInfo(mossSystem.GetMosses());
                    levelManager.savePortalInfo(portalSystem);
                    levelManager.writeToFile(levelManager.getCurrentLevel());
                }
            } else {
                // ====================
                // Gameplay mode
                // ====================

                if (AEInputCheckCurr(AEVK_LBUTTON)) {
                    bool hitDirt = dirt->destroyAtMouse(20.0f);
                    // Only run the VFX timer if we actually dug through dirt
                    if (hitDirt) {
                        vfxSystem.SpawnContinuous(VFXType::DirtBurst, GetMouseWorldPos(), deltaTime,
                                                  0.1f);
                        g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
                    } else {
                        vfxSystem.ResetSpawnTimer();
                    }
                } else {
                    vfxSystem.ResetSpawnTimer();
                }

                if (AEInputCheckTriggered(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {

                    startEndPointSystem.CheckMouseClick();

                } else if (AEInputCheckReleased(AEVK_LBUTTON)) {

                    startEndPointSystem.ResetIframe();
                }

                // If magic terrain is near mouse click, place portal
                if (magic->isNearestNodeToMouseAtThreshold() == true) {
                    if (AEInputCheckTriggered(AEVK_RBUTTON) || 0 == AESysDoesWindowExist()) {
                        portalSystem.CheckMouseClick();
                    } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                        portalSystem.ResetIframe();
                    }
                }
                portalSystem.RotatePortal();

                // tc added start
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
                goalPercentage = (static_cast<f32>(startEndPointSystem.particlesCollected_) /
                                  static_cast<f32>(fluidSystem.particleMaxCount_ / 4)) *
                                 100.0f;
                if (goalPercentage > 100.0f)
                    goalPercentage = 100.0f;

                char goalBuffer[32];
                snprintf(goalBuffer, sizeof(goalBuffer), "Goal: %.0f%%", goalPercentage);
                goalText.content_ = goalBuffer;

                if (AEInputCheckTriggered(AEVK_F)) {
                    // Refill all start points
                    startEndPointSystem.RefillAllWater();
                }

                if (AEInputCheckTriggered(AEVK_G)) {
                    // Toggle infinite water for all start points
                    startEndPointSystem.ToggleInfiniteWater();
                }
                // tc added end

                for (auto& startPoint : startEndPointSystem.startPoints_) {
                    if (startPoint.releaseWater_) {
                        static f32 spawnTimer = 0.0f;
                        spawnTimer -= deltaTime;
                        if (spawnTimer <= 0.0f) {

                            // RESET TIMER: Set this to how fast you want water to flow
                            // Original: 0.005f;
                            spawnTimer = 0.025f;

                            // f32 randRadius = 13.0f - (noise * 100.0f);
                            f32 randRadius = 7.0f;

                            f32 xOffset = startPoint.transform_.pos_.x +
                                          AERandFloat() * startPoint.transform_.scale_.x -
                                          (startPoint.transform_.scale_.x / 2.f);
                            AEVec2 position = {xOffset, startPoint.transform_.pos_.y -
                                                            (startPoint.transform_.scale_.y / 2.f) -
                                                            (randRadius)};

                            // Call the water spawn function
                            // tc added end
                            // fluidSystem.SpawnParticle(position.x, position.y, randRadius,
                            // FluidType::Water);
                        }
                        SpawnWaterWithLimit(deltaTime);
                    }
                }

                // tc added start - Update collectibles
                collectibleSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));
                mossSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water),
                                  startEndPointSystem);

                // Check if all items collected
                if (collectibleSystem.CheckAllCollected()) {
                    std::cout << "All items collected!\n";
                    // You can trigger level complete here
                }

                fluidSystem.Update(deltaTime, {dirt, stone});
                startEndPointSystem.Update(deltaTime,
                                           fluidSystem.GetParticlePool(FluidType::Water));
                portalSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));
                vfxSystem.Update(deltaTime);

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
                    std::cout << "WIN - Showing win screen\n";

                    // Save highscore
                    levelManager.saveLevelProgress(levelManager.getCurrentLevel(),
                                                   collectibleSystem.GetCollectedCount());
                    winScreen.Show(collectibleSystem.GetCollectedCount(),
                                   collectibleSystem.GetTotalCount(),
                                   levelManager.getCurrentLevel());
                }

                winScreen.Update(GSM);
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
    animManager.UpdateAll(deltaTime);
}

// =============================================================================
// HUD helpers
// =============================================================================

static void DrawHudIcon(AEGfxTexture* tex, AEGfxVertexList* mesh, f32 worldX, f32 worldY,
                        f32 iconSize, f32 uvOffsetX = 0.0f) {
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

static void DrawPixelBar(f32 worldX, f32 worldY, f32 barWidth, f32 barHeight, f32 fillR, f32 fillG,
                         f32 fillB, f32 borderR, f32 borderG, f32 borderB, f32 percentage) {
    if (!g_barMesh)
        g_barMesh = CreateRectMesh();
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

static void DrawTotalWaterBar(f32 x, f32 y, f32 remaining, f32 capacity) {
    if (capacity <= 0.0f)
        return;
    f32 worldX = x * 710.0f;
    f32 worldY = y * 510.0f;
    f32 barW = 200.0f, barH = 20.0f, iconSize = 32.0f;

    DrawHudIcon(pHudWaterIconTex, g_hudIconMesh, worldX - barW * 0.5f - iconSize * 0.5f - 4.0f,
                worldY, iconSize);

    DrawPixelBar(worldX, worldY, barW, barH, 0.18f, 0.57f, 1.0f, 0.0f, 0.0f, 0.45f,
                 remaining / capacity);
}

static AEGfxVertexList* s_flowerIconMesh = nullptr;

static void DrawGoalBar(f32 x, f32 y, f32 percentage) {
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

    DrawPixelBar(worldX, worldY, barW, barH, 0.16f, 0.72f, 0.22f, 0.0f, 0.35f, 0.0f,
                 percentage / 100.0f);
}
// tc added end

void DrawLevel() {
    // std::cout << "Draw level 2\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    bg.draw();

    // ====================
    // Gameplay mode
    // ====================
    fluidSystem.DrawColor();
    startEndPointSystem.DrawTexture(fluidSystem.particleMaxCount_);

    dirt->renderTerrain();
    stone->renderTerrain();
    magic->renderTerrain();
    portalSystem.Draw();
    vfxSystem.Draw();

    // tc added start
    // Draw collectibles
    collectibleSystem.Draw();
    // Draw moss
    mossSystem.Draw();

    // Add preview render in gameplay
    if (levelManager.getLevelEditorMode() == EditorMode::None) {
        // Only show portal preview if mouse is near magic terrain, otherwise always show dirt
        // preview
        if (magic->isNearestNodeToMouseAtThreshold() == true) {
            // Draw portal preview if mouse is near magic terrain
            portalSystem.DrawPreview();
        } else {
            levelManager.drawBrushPreview(TerrainMaterial::Dirt, 20.f);
        }
    }

    // ====================
    // Editor Mode mode
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
            startEndPointSystem.DrawPreview(StartEndType::Pipe);
            break;
        case GameBlock::EndPoint:
            startEndPointSystem.DrawPreview(StartEndType::Flower);
            break;
        case GameBlock::Collectible:
            collectibleSystem.DrawPreview();
            break;
        case GameBlock::Moss:
            mossSystem.DrawPreview();
            break;
        case GameBlock::Portal:
            portalSystem.DrawPreview();
            break;
        default:
            break;
        }
    }

    // ====================
    // UI Elements
    // ====================
    collectibleSystem.DrawUI();
    // Show total water counter with progress bar and goal progress bar
    totalWaterText.draw();
    goalText.draw();

    DrawTotalWaterBar(totalWaterText.x_ + 0.39f, totalWaterText.y_ - 0.09f, totalWaterRemaining,
                      totalWaterCapacity);
    DrawGoalBar(goalText.x_ + 0.39f, goalText.y_ - 0.09f, goalPercentage);
    // tc added end

    // Buttons
    buttonPause.draw();

    winScreen.Draw();

    if (pauseSystem.isPaused()) { // Game is paused
        // Background
        pauseSystem.renderBackground();

        pauseHeaderText.draw(true);

        // UI buttons
        buttonResume.draw();
        buttonRestart.draw();
        buttonQuit.draw();
    } else { // Game is not paused
    }

    g_debugSystem.drawAll();

    // Animations
    animManager.DrawAll();
}

void FreeLevel() {
    // std::cout << "Free level 2\n";
    g_debugSystem.clearScene();
    winScreen.Free();
    fluidSystem.Free();
    startEndPointSystem.Free();
    portalSystem.Free();
    vfxSystem.Free();
    animManager.FreeAll();
    // tc added start
    mossSystem.Free();
    collectibleSystem.Free();

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
    // tc added end

    delete dirt;
    dirt = nullptr;
    delete stone;
    stone = nullptr;
    delete magic;
    magic = nullptr;
}

void UnloadLevel() {
    // std::cout << "Unload level 2\n";
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
    if (pHudWaterIconTex) {
        AEGfxTextureUnload(pHudWaterIconTex);
        pHudWaterIconTex = nullptr;
    }
    if (pHudGoalIconTex) {
        AEGfxTextureUnload(pHudGoalIconTex);
        pHudGoalIconTex = nullptr;
    }
    bg.unload();

    levelManager.freeLevelEditor();
    // NOTE: Do NOT reset currentLevel_ to 0 here.
    // WinScreen sets it to nextLevel_ before triggering StateId::Level.
    // Resetting it here would wipe that value and always reload Level 0.

    // UI buttons
    buttonPause.unload();
    buttonResume.unload();
    buttonRestart.unload();
    buttonQuit.unload();

    // Pause system
    pauseSystem.unload();

    winScreen.Unload();
}