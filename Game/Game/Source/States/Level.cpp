#include "States/MainMenu.h"

#include <fstream>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <AEEngine.h>

#include "CollisionSystem.h"
#include "Components.h"
#include "FluidSystem.h"
#include "GameStateManager.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "States/LevelManager.h"
#include "Terrain.h"
#include "UISystem.h"
#include "VFXConfigs.h"
#include "VFXSystem.h"

static Terrain* dirt = nullptr;
static Terrain* stone = nullptr;
static AEGfxTexture* pTerrainDirtTex{nullptr};
static AEGfxTexture* pTerrainStoneTex{nullptr};

static FluidSystem fluidSystem;
static StartEndPoint startEndPointSystem;
static PortalSystem portalSystem;

static Text rotationText;
static s8 font;

static int height, width, tileSize;
static bool fileExist;

static VFXSystem vfxSystem;

// tc added start
static Text totalWaterText;
static Text goalText;
static f32 totalWaterRemaining = 0.0f;
static f32 totalWaterCapacity = 0.0f;
static f32 goalPercentage = 0.0f;
static AEGfxVertexList* g_barMesh = nullptr; // Global bar mesh for cleanup
// tc added end

void LoadLevel() {
    // std::cout << "Load level 3\n";
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pTerrainDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pTerrainStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");

    // Setup texts
    rotationText = Text(0.7f, 0.9f, "");
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 12);

    // tc added start
    startEndPointSystem.InitializeUI(font);
    totalWaterText = Text(-0.6f, 0.92f, "Water: 0/0");
    goalText = Text(0.1f, 0.92f, "Goal: 0%");
    // tc added end

    levelManager.initEditorUI();

    if (levelManager.getLevelData(levelManager.getCurrentLevel())) {
        levelManager.parseMapInfo(width, height, tileSize);
        fileExist = true;
    } else {
        std::cout << "Failed to load level data\n";
        std::cout << "Using default values\n";
        width = 80;
        height = 45;
        tileSize = 20;
        fileExist = false;
    }
}

void InitializeLevel() {
    // std::cout << "Initialize level 3\n";
    fluidSystem.Initialize();
    startEndPointSystem.Initialize();
    if (fileExist) {
        levelManager.parseStartEndInfo(startEndPointSystem);
    }
    portalSystem.Initialize();

    dirt =
        new Terrain(TerrainMaterial::Dirt, pTerrainDirtTex, {0.0f, 0.0f}, height, width, tileSize);
    stone = new Terrain(TerrainMaterial::Stone, pTerrainStoneTex, {0.0f, 0.0f}, height, width,
                        tileSize);
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

    vfxSystem.Initialize(800, 20);
    LoadGlobalVFXConfigs(vfxSystem); // <-- change this to read from a json instead
}

// tc added start - Function to handle water spawning with limit
static void SpawnWaterWithLimit(f32 deltaTime) {
    // Use a static timer that persists between function calls
    static f32 global_spawn_timer = 0.0f;

    // Decrement the global timer
    global_spawn_timer -= deltaTime;

    // Only spawn if enough time has passed
    if (global_spawn_timer <= 0.0f) {
        // Reset timer to control spawn rate across all start points
        global_spawn_timer = 0.025f; // Same spawn rate as before

        // Loop through all start points
        for (auto& startPoint : startEndPointSystem.startPoints_) {
            // Only process active pipe-type start points that are releasing water
            if (startPoint.release_water_ && startPoint.type_ == StartEndType::Pipe) {

                // Check if there's water remaining (or infinite mode)
                if (startPoint.water_remaining_ > 0.0f || startPoint.infinite_water_) {

                    // Consume water (unless infinite)
                    if (!startPoint.infinite_water_) {
                        startPoint.water_remaining_ -= 0.5f; // Adjust consumption rate as needed
                        if (startPoint.water_remaining_ < 0.0f) {
                            startPoint.water_remaining_ = 0.0f;
                            startPoint.release_water_ = false; // Auto-stop when empty
                        }
                    }

                    // Only spawn particle if there's still water
                    if (startPoint.water_remaining_ > 0.0f || startPoint.infinite_water_) {
                        // Spawn particle at the start point position
                        f32 randRadius = 5.0f;

                        // Calculate random x offset within the start point's width
                        f32 x_offset = startPoint.transform_.pos_.x +
                                       AERandFloat() * startPoint.transform_.scale_.x -
                                       (startPoint.transform_.scale_.x / 2.f);
                        AEVec2 position = {x_offset, startPoint.transform_.pos_.y -
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
    // tc added start
    totalWaterRemaining = 0.0f;
    totalWaterCapacity = 0.0f;
    for (const auto& startPoint : startEndPointSystem.startPoints_) {
        if (startPoint.active_ && startPoint.type_ == StartEndType::Pipe) {
            totalWaterRemaining += startPoint.water_remaining_;
            totalWaterCapacity += startPoint.water_capacity_;
        }
    }
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Water: %.0f/%.0f", totalWaterRemaining, totalWaterCapacity);
    totalWaterText.text_ = buffer;

    // Calculate goal percentage based on particles collected vs max particles
    goalPercentage = (static_cast<f32>(startEndPointSystem.particlesCollected_) /
                      static_cast<f32>(fluidSystem.particleMaxCount / 3)) *
                     100.0f;
    if (goalPercentage > 100.0f)
        goalPercentage = 100.0f;

    char goalBuffer[32];
    snprintf(goalBuffer, sizeof(goalBuffer), "Goal: %.0f%%", goalPercentage);
    goalText.text_ = goalBuffer;

    if (AEInputCheckTriggered(AEVK_F)) {
        // Refill all start points
        startEndPointSystem.RefillAllWater();
    }

    if (AEInputCheckTriggered(AEVK_G)) {
        // Toggle infinite water for all start points
        startEndPointSystem.ToggleInfiniteWater();
    }
    // tc added end

    // std::cout << "Update level 3\n";

    // Press Q to go to main menu
    if (AEInputCheckTriggered(AEVK_Q) || 0 == AESysDoesWindowExist()) {
        std::cout << "Q triggered\n";
        GSM.nextState_ = StateId::MainMenu;
    }

    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    // Keyboard/Mouse inputs for level editor and gameplay
    // If in editor mode, edit level
    if (levelManager.getLevelEditorMode() == editorMode::Edit) {
        // Inputs to build level
        if (!levelManager.getDisplayBuilderContainer()) {
            f32 brush_size = levelManager.brush_radius_;
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
            case GameBlock::StartPoint:
                if (AEInputCheckReleased(AEVK_LBUTTON)) {
                    startEndPointSystem.SpawnAtMousePos(StartEndType::Pipe, GoalDirection::Down);
                } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                    startEndPointSystem.DeleteAtMousePos();
                }
                break;
            case GameBlock::EndPoint:
                if (AEInputCheckReleased(AEVK_LBUTTON)) {
                    startEndPointSystem.SpawnAtMousePos(StartEndType::Flower, GoalDirection::Up);
                } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
                    startEndPointSystem.DeleteAtMousePos();
                }
                break;
            default:
                break;
            }
        }
        // Inputs to save level
        if (AEInputCheckReleased(AEVK_S)) {
            levelManager.saveMapInfo(width, height, tileSize);
            levelManager.saveTerrainInfo(dirt->getNodes(), "Dirt");
            levelManager.saveTerrainInfo(stone->getNodes(), "Stone");
            levelManager.saveStartEndInfo(startEndPointSystem.startPoints_,
                                          startEndPointSystem.endPoint_);
            levelManager.writeToFile(levelManager.getCurrentLevel());
        }

    } else {
        // Else do inputs for gameplay instead
        if (AEInputCheckCurr(AEVK_LBUTTON)) {
            dirt->destroyAtMouse(20.0f);

            vfxSystem.vfxSpawnTimer_ -= deltaTime;
            if (vfxSystem.vfxSpawnTimer_ <= 0.0f) {

                vfxSystem.SpawnVFX(VFXType::DirtBurst, GetMouseWorldPos());

                vfxSystem.vfxSpawnTimer_ = 0.1f;
            }
        } else {
            vfxSystem.vfxSpawnTimer_ = 0.0f;
        }

        if (AEInputCheckTriggered(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {

            startEndPointSystem.CheckMouseClick();

        } else if (AEInputCheckReleased(AEVK_LBUTTON)) {

            startEndPointSystem.ResetIframe();
        }

        if (AEInputCheckTriggered(AEVK_RBUTTON) || 0 == AESysDoesWindowExist()) {
            portalSystem.CheckMouseClick();
        } else if (AEInputCheckReleased(AEVK_RBUTTON)) {
            portalSystem.ResetIframe();
        }
        if (AEInputCheckTriggered(AEVK_MBUTTON)) {
            portalSystem.RotatePortal();
        }
    }

    // tc added start - Call the water spawn function
    SpawnWaterWithLimit(deltaTime);
    // tc added end

    // fluidSystem.UpdateMain(deltaTime);
    fluidSystem.Update(deltaTime, *dirt);
    fluidSystem.Update(deltaTime, *stone);
    startEndPointSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));
    portalSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));
    vfxSystem.Update(deltaTime);

    levelManager.updateLevelEditor();

    if (startEndPointSystem.CheckWinCondition(fluidSystem.particleMaxCount)) {
        std::cout << "WIN\n ";
    }
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

    fluidSystem.DrawColor();
    startEndPointSystem.DrawColor();

    dirt->renderTerrain();
    stone->renderTerrain();
    portalSystem.DrawColor();
    vfxSystem.Draw();

    // tc added start - Show total water counter with progress bar and goal progress bar
    const char* totalWaterStr = totalWaterText.text_.c_str();
    AEGfxPrint(font, totalWaterStr, totalWaterText.pos_x_, totalWaterText.pos_y_, 1.f, 1.f, 1.f,
               1.f, 1.f);

    const char* goalStr = goalText.text_.c_str();
    AEGfxPrint(font, goalStr, goalText.pos_x_, goalText.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);

    // Water progress bar below the water text with dark blue border
    DrawTotalWaterBar(totalWaterText.pos_x_ + 0.33f, totalWaterText.pos_y_ - 0.09f,
                      totalWaterRemaining, totalWaterCapacity);

    // Goal progress bar below the goal text with dark green border
    DrawGoalBar(goalText.pos_x_ + 0.33f, goalText.pos_y_ - 0.09f, goalPercentage);
    // tc added end

    if (levelManager.getLevelEditorMode() == editorMode::Edit) {
        levelManager.renderLevelEditorUI();
        switch (levelManager.getCurrentGameBlock()) {
        case GameBlock::Dirt:;
            levelManager.DrawBrushPreview(TerrainMaterial::Dirt);
            break;
        case GameBlock::Stone:
            levelManager.DrawBrushPreview(TerrainMaterial::Stone);
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

    rotationText.text_ =
        "Portal Rotation:" + std::to_string(static_cast<s32>(portalSystem.GetRotationValue()));
    const char* rotationStr = rotationText.text_.c_str();
    AEGfxPrint(font, rotationStr, rotationText.pos_x_, rotationText.pos_y_, 1.f, 1.f, 1.f, 1.f,
               1.f);
}

void FreeLevel() {
    // std::cout << "Free level 2\n";
    fluidSystem.Free();
    startEndPointSystem.Free();
    portalSystem.Free();
    vfxSystem.Free();

    // tc added start
    if (g_barMesh) {
        AEGfxMeshFree(g_barMesh);
        g_barMesh = nullptr;
    }
    // tc added end

    delete dirt;
    dirt = nullptr;
    delete stone;
    stone = nullptr;
}

void UnloadLevel() {
    // std::cout << "Unload level 2\n";
    Terrain::freeMeshLibrary();
    AEGfxDestroyFont(font);

    AEGfxTextureUnload(pTerrainDirtTex);
    AEGfxTextureUnload(pTerrainStoneTex);

    levelManager.freeLevelEditor();
    levelManager.SetCurrentLevel(0);
}