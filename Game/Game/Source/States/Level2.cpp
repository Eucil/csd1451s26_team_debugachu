#include "States/MainMenu.h"

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
#include "Terrain.h"
#include "UISystem.h"

static Terrain* dirt = nullptr;
static Terrain* stone = nullptr;

static FluidSystem fluidSystem;
static StartEndPoint startEndPointSystem;
static PortalSystem portalSystem;

static Text rotationText;
static s8 font;

void LoadLevel2() {
    // std::cout << "Load level 2\n";
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    // Setup texts
    rotationText = Text(0.7f, 0.9f, "");
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 12);
}

void InitializeLevel2() {
    // std::cout << "Initialize level 2\n";
    dirt = Terrain::Level2Dirt(TerrainMaterial::Dirt, {0.0f, 0.0f}, 45, 80, 20);
    stone = Terrain::Level2Stone(TerrainMaterial::Stone, {0.0f, 0.0f}, 45, 80, 20);

    dirt->initCellsTransform();
    dirt->initCellsGraphics();
    dirt->initCellsCollider();
    dirt->updateTerrain();

    stone->initCellsTransform();
    stone->initCellsGraphics();
    stone->initCellsCollider();
    stone->updateTerrain();

    fluidSystem.Initialize();
    startEndPointSystem.Initialize();
    portalSystem.Initialize();

    startEndPointSystem.SetupStartPoint({-650.0f, 400.0f}, {50.0f, 50.0f}, StartEndType::Pipe,
                                        GoalDirection::Down);
    startEndPointSystem.SetupEndPoint({650.0f, -400.0f}, {50.0f, 50.0f}, StartEndType::Flower,
                                      GoalDirection::Up);
}

void UpdateLevel2(GameStateManager& GSM, f32 deltaTime) {
    // std::cout << "Update level 2\n";

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

    if (AEInputCheckCurr(AEVK_LBUTTON)) {
        dirt->destroyAtMouse(20.0f);
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
    fluidSystem.UpdateMain(deltaTime, *dirt);
    fluidSystem.UpdateMain(deltaTime, *stone);
    startEndPointSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));
    portalSystem.Update(deltaTime, fluidSystem.GetParticlePool(FluidType::Water));

    if (startEndPointSystem.CheckWinCondition(fluidSystem.particleMaxCount)) {
        std::cout << "WIN\n ";
    }
}

void DrawLevel2() {
    // std::cout << "Draw level 2\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    fluidSystem.DrawColor();
    startEndPointSystem.DrawColor();

    dirt->renderTerrain();
    stone->renderTerrain();
    portalSystem.DrawColor();

    rotationText.text_ =
        "Portal Rotation:" + std::to_string(static_cast<s32>(portalSystem.GetRotationValue()));
    const char* rotationStr = rotationText.text_.c_str();
    AEGfxPrint(font, rotationStr, rotationText.pos_x_, rotationText.pos_y_, 1.f, 1.f, 1.f, 1.f,
               1.f);
}

void FreeLevel2() {
    // std::cout << "Free level 2\n";
    fluidSystem.Free();
    startEndPointSystem.Free();
    portalSystem.Free();

    delete dirt;
    dirt = nullptr;
    delete stone;
    stone = nullptr;
}

void UnloadLevel2() {
    // std::cout << "Unload level 2\n";
    Terrain::freeMeshLibrary();
    AEGfxDestroyFont(font);
}