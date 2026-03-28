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

static TextData rotationText;
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
static Button buttonBack;
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
// tc added end

// Animations
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

void LoadLevel() {
    // std::cout << "Load level 3\n";
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pTerrainDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pTerrainStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
    pTerrainMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");

    // Setup texts
    rotationText = TextData{"", 0.7f, 0.92f, 0.5f};
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // tc added start
    collectibleSystem.Load(font);
    winScreen.Load(font);
    mossSystem.Load(font);

    startEndPointSystem.InitializeUI(font);

    totalWaterText.x_ = -0.35f;
    totalWaterText.y_ = 0.92f;
    totalWaterText.scale_ = 0.5f;
    totalWaterText.content_ = "Water: 0/0";
    totalWaterText.font_ = font;

    goalText.x_ = 0.2f;
    goalText.y_ = 0.92f;
    goalText.scale_ = 0.5f;
    goalText.content_ = "Goal: 0%";
    goalText.font_ = font;

    rotationText.font_ = font;

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
    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    buttonRestart.loadMesh();
    buttonRestart.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    buttonQuit.loadMesh();
    buttonQuit.loadTexture("Assets/Textures/brown_rectangle_80_24.png");

    pauseSystem.loadMesh();
    // Once level is loaded, make sure it is not paused
    pauseSystem.resume();

    // Debug system
    g_debugSystem.load(font);
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

    // UI buttons
    buttonPause.initFromJson("level_buttons", "Pause");
    buttonPause.setTextFont(buttonFont);
    buttonBack.initFromJson("level_buttons", "Back");
    buttonBack.setTextFont(buttonFont);
    buttonRestart.initFromJson("level_buttons", "Restart");
    buttonRestart.setTextFont(buttonFont);
    buttonQuit.initFromJson("level_buttons", "Quit");
    buttonQuit.setTextFont(buttonFont);

    // Pause system
    pauseSystem.initFromJson("pause_system", "Background");
    pauseHeaderText.initFromJson("pause_system", "Header");
    pauseHeaderText.font_ = titleFont;

    // Debug system
    g_debugSystem.initFromJson("debug_system", "DebugOverlay");

    // Animations
    animManager.Clear();
    animManager.Add(&screenFader);
    animManager.Add(&someOtherCoolAnimation);
    animManager.InitializeAll();
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

            if (buttonBack.checkMouseClick()) {
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
            buttonBack.updateTransform();
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
                        g_audioSystem.playSound("dirt_break", "sfx", 0.25f, 1.0f);
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

                // Calculate goal percentage based on particles collected vs max particles
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

                if (startEndPointSystem.CheckWinCondition(fluidSystem.particleMaxCount_) &&
                    !winTriggered) {
                    winTriggered = true;
                    std::cout << "WIN - Showing win screen\n";
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

// tc added start - Enhanced total water counter with a progress bar and dark blue border
static void DrawTotalWaterBar(f32 x, f32 y, f32 remaining, f32 capacity) {
    if (capacity <= 0.0f)
        return;

    f32 percentage = remaining / capacity;
    if (percentage < 0.0f)
        percentage = 0.0f;

    // Bar dimensions (in screen coordinates)
    f32 barWidth = 200.0f;
    f32 barHeight = 20.0f;
    f32 borderThickness = 2.0f;

    // Convert from screen coordinates (-0.9 to 0.9 range) to world coordinates
    f32 worldX = x * 710.0f; // Convert from -0.9 to world X
    f32 worldY = y * 510.0f; // Convert from 0.9 to world Y

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Create a global mesh for the bar that can be cleaned up
    if (!g_barMesh) {
        g_barMesh = CreateRectMesh();
    }

    // Draw dark blue border (slightly larger rectangle)
    AEMtx33 trans_mtx, scale_mtx, world_mtx;

    // Border - slightly larger than the bar
    AEMtx33Trans(&trans_mtx, worldX, worldY);
    AEMtx33Scale(&scale_mtx, barWidth + borderThickness * 2, barHeight + borderThickness * 2);
    AEMtx33Concat(&world_mtx, &trans_mtx, &scale_mtx);

    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.5f, 1.0f); // Dark blue border
    AEGfxSetTransform(world_mtx.m);
    AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

    // Draw background bar (gray)
    AEMtx33Trans(&trans_mtx, worldX, worldY);
    AEMtx33Scale(&scale_mtx, barWidth, barHeight);
    AEMtx33Concat(&world_mtx, &trans_mtx, &scale_mtx);

    AEGfxSetTransparency(0.3f);
    AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
    AEGfxSetTransform(world_mtx.m);
    AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

    // Draw fill bar (blue)
    if (percentage > 0.0f) {
        f32 fillWidth = barWidth * percentage;
        f32 fillX = worldX - (barWidth - fillWidth) / 2.0f;

        AEMtx33Trans(&trans_mtx, fillX, worldY);
        AEMtx33Scale(&scale_mtx, fillWidth, barHeight);
        AEMtx33Concat(&world_mtx, &trans_mtx, &scale_mtx);

        AEGfxSetTransparency(0.8f);
        AEGfxSetColorToMultiply(0.0f, 0.5f, 1.0f, 1.0f); // Blue fill
        AEGfxSetTransform(world_mtx.m);
        AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);
    }
}

// Enhanced goal progress bar with dark green border
static void DrawGoalBar(f32 x, f32 y, f32 percentage) {
    if (percentage < 0.0f)
        percentage = 0.0f;
    if (percentage > 100.0f)
        percentage = 100.0f;

    // Bar dimensions (in screen coordinates)
    f32 barWidth = 200.0f;
    f32 barHeight = 20.0f;
    f32 borderThickness = 2.0f;

    // Convert from screen coordinates (-0.9 to 0.9 range) to world coordinates
    f32 worldX = x * 710.0f; // Convert from -0.9 to world X
    f32 worldY = y * 510.0f; // Convert from 0.9 to world Y

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    if (!g_barMesh) {
        g_barMesh = CreateRectMesh();
    }

    // Draw dark green border (slightly larger rectangle)
    AEMtx33 trans_mtx, scale_mtx, world_mtx;

    // Border - slightly larger than the bar
    AEMtx33Trans(&trans_mtx, worldX, worldY);
    AEMtx33Scale(&scale_mtx, barWidth + borderThickness * 2, barHeight + borderThickness * 2);
    AEMtx33Concat(&world_mtx, &trans_mtx, &scale_mtx);

    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 0.4f, 0.0f, 1.0f); // Dark green border
    AEGfxSetTransform(world_mtx.m);
    AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

    // Draw background bar (gray)
    AEMtx33Trans(&trans_mtx, worldX, worldY);
    AEMtx33Scale(&scale_mtx, barWidth, barHeight);
    AEMtx33Concat(&world_mtx, &trans_mtx, &scale_mtx);

    AEGfxSetTransparency(0.3f);
    AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
    AEGfxSetTransform(world_mtx.m);
    AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);

    // Draw fill bar (green)
    if (percentage > 0.0f) {
        f32 fillWidth = barWidth * (percentage / 100.0f);
        f32 fillX = worldX - (barWidth - fillWidth) / 2.0f;

        AEMtx33Trans(&trans_mtx, fillX, worldY);
        AEMtx33Scale(&scale_mtx, fillWidth, barHeight);
        AEMtx33Concat(&world_mtx, &trans_mtx, &scale_mtx);

        AEGfxSetTransparency(0.8f);
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f); // Green fill
        AEGfxSetTransform(world_mtx.m);
        AEGfxMeshDraw(g_barMesh, AE_GFX_MDM_TRIANGLES);
    }
}
// tc added end

void DrawLevel() {
    // std::cout << "Draw level 2\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

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
            startEndPointSystem.DrawColorPreview(StartEndType::Pipe);
            break;
        case GameBlock::EndPoint:
            startEndPointSystem.DrawColorPreview(StartEndType::Flower);
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

    // Water progress bar below the water text with dark blue border
    DrawTotalWaterBar(totalWaterText.x_ + 0.35f, totalWaterText.y_ - 0.09f, totalWaterRemaining,
                      totalWaterCapacity);

    // Goal progress bar below the goal text with dark green border
    DrawGoalBar(goalText.x_ + 0.33f, goalText.y_ - 0.09f, goalPercentage);
    // tc added end

    rotationText.content_ =
        "Portal Rotation:" + std::to_string(static_cast<s32>(portalSystem.GetRotationValue()));
    rotationText.draw();
    // Buttons
    buttonPause.draw();

    winScreen.Draw();

    if (pauseSystem.isPaused()) { // Game is paused
        // Background
        pauseSystem.renderBackground();

        pauseHeaderText.draw(true);

        // UI buttons
        buttonBack.draw();
        buttonRestart.draw();
        buttonQuit.draw();
    } else { // Game is not paused
    }

    if (g_debugSystem.isOpen()) { // Debug system is open
        g_debugSystem.draw();
    } else { // Debug system is not open
    }
    // Animations
    animManager.DrawAll();
}

void FreeLevel() {
    // std::cout << "Free level 2\n";
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
    AEGfxDestroyFont(font);
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

    levelManager.freeLevelEditor();
    // NOTE: Do NOT reset currentLevel_ to 0 here.
    // WinScreen sets it to nextLevel_ before triggering StateId::Level.
    // Resetting it here would wipe that value and always reload Level 0.

    // UI buttons
    buttonPause.unload();
    buttonBack.unload();
    buttonRestart.unload();
    buttonQuit.unload();

    // Pause system
    pauseSystem.unload();

    // Debug system
    g_debugSystem.unload();

    winScreen.Unload();
}