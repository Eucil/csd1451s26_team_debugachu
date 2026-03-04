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

void LoadLevel() {
    // std::cout << "Load level 3\n";
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pTerrainDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pTerrainStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");

    // Setup texts
    rotationText = Text(0.7f, 0.9f, "");
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 12);

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

void UpdateLevel(GameStateManager& GSM, f32 deltaTime) {
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

    for (auto& startPoint : startEndPointSystem.startPoints_) {
        if (startPoint.release_water_) {
            static f32 spawn_timer = 0.0f;
            spawn_timer -= deltaTime;
            if (spawn_timer <= 0.0f) {

                // RESET TIMER: Set this to how fast you want water to flow
                // Original: 0.005f;
                spawn_timer = 0.025f;

                // the particle spawns at the values shown below, including its FluidType
                f32 noise = ((static_cast<int>(AERandFloat() * 12345) % 100)) * 0.001f - 0.1f;

                // f32 randRadius = 13.0f - (noise * 100.0f);
                f32 randRadius = 5.0f;

                f32 x_offset = startPoint.transform_.pos_.x +
                               AERandFloat() * startPoint.transform_.scale_.x -
                               (startPoint.transform_.scale_.x / 2.f);
                AEVec2 position = {x_offset, startPoint.transform_.pos_.y -
                                                 (startPoint.transform_.scale_.y / 2.f) -
                                                 (randRadius)};

                fluidSystem.SpawnParticle(position.x, position.y, randRadius, FluidType::Water);
                s32 size = fluidSystem.GetParticleCount(FluidType::Water);
            }
        }
    }

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

void DrawLevel() {
    // std::cout << "Draw level 2\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    fluidSystem.DrawColor();
    startEndPointSystem.DrawColor();

    dirt->renderTerrain();
    stone->renderTerrain();
    portalSystem.DrawColor();
    vfxSystem.Draw();

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