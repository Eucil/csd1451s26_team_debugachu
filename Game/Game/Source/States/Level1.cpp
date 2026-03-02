#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "CollisionSystem.h"
#include "Components.h"
#include "FluidSystem.h"
#include "GameStateManager.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "Terrain.h"
#include "VFXSystem.h"

#include "VFXConfigs.h" // <--- temporary

static Terrain* dirt = nullptr;
static Terrain* stone = nullptr;

static FluidSystem fluidSystem;
static StartEndPoint startEndPointSystem;
static PortalSystem portalSystem;
static VFXSystem vfxSystem;

static bool debug_mode_level1 = false;

static void UpdateInputPolling(GameStateManager& GSM, f32 deltaTime);

void LoadLevel1() {
    // std::cout << "Load level 1\n";

    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();
}

void InitializeLevel1() {
    // std::cout << "Initialize level 1\n";
    dirt = Terrain::Level1Dirt(TerrainMaterial::Dirt, {0.0f, 0.0f}, 45, 80, 20);
    stone = Terrain::Level1Stone(TerrainMaterial::Stone, {0.0f, 0.0f}, 45, 80, 20);

    dirt->initCellsTransform();
    dirt->initCellsGraphics();
    dirt->initCellsCollider();
    dirt->updateTerrain();

    stone->initCellsTransform();
    stone->initCellsGraphics();
    stone->initCellsCollider();
    stone->updateTerrain();

    // Fluid, start end point, portals
    fluidSystem.Initialize();
    startEndPointSystem.Initialize();
    portalSystem.Initialize();

    startEndPointSystem.SetupStartPoint({-650.0f, 400.0f}, {50.0f, 50.0f}, StartEndType::Pipe,
                                        GoalDirection::Down);
    startEndPointSystem.SetupEndPoint({650.0f, -400.0f}, {50.0f, 50.0f}, StartEndType::Flower,
                                      GoalDirection::Up);

    vfxSystem.Initialize(800, 20);
    LoadGlobalVFXConfigs(vfxSystem); // <-- change this to read from a json instead
}

void UpdateLevel1(GameStateManager& GSM, f32 deltaTime) {
    // std::cout << "Update level 1\n";

    UpdateInputPolling(GSM, deltaTime);

    for (auto& startPoint : startEndPointSystem.startPoints_) {
        if (startPoint.release_water_) {
            static f32 spawn_timer = 0.0f;
            spawn_timer -= deltaTime;
            if (spawn_timer <= 0.0f) {

                // RESET TIMER: Set this to how fast you want water to spawn
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

    fluidSystem.Update(deltaTime, *dirt);
    fluidSystem.Update(deltaTime, *stone);
    startEndPointSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));
    portalSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));

    vfxSystem.Update(deltaTime);

    // Terrain to fluid
    //
    //
    // CollisionSystem::terrainToFluidCollision(dirt, fluidSystem);

    if (startEndPointSystem.CheckWinCondition(fluidSystem.particleMaxCount)) {
        std::cout << "WIN\n ";
    }
}

void DrawLevel1() {
    // std::cout << "Draw level 1\n";

    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    fluidSystem.DrawColor();
    startEndPointSystem.DrawColor();
    portalSystem.DrawColor();

    dirt->renderTerrain();
    if (debug_mode_level1)
        dirt->renderCollidersDebug();
    stone->renderTerrain();

    vfxSystem.Draw();
}

void FreeLevel1() {
    // std::cout << "Free level 1\n";

    fluidSystem.Free();
    startEndPointSystem.Free();
    portalSystem.Free();
    vfxSystem.Free();

    delete dirt;
    dirt = nullptr;
    delete stone;
    stone = nullptr;
}

void UnloadLevel1() {
    // std::cout << "Unload level 1\n";

    Terrain::freeMeshLibrary();
}

void UpdateInputPolling(GameStateManager& GSM, f32 deltaTime) {
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

    if (AEInputCheckTriggered(AEVK_D) || 0 == AESysDoesWindowExist()) {
        debug_mode_level1 = !debug_mode_level1;
    }

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
}