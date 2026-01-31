#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"

#include "Components.h"

#include "FluidSystem.h"

#include "StartEndPoint.h"

static FluidSystem fluidSystem;
static StartEndPoint startEndPointSystem;

void LoadLevel2() {
    // Todo
    std::cout << "Load level 2\n";
}

void InitializeLevel2() {
    // Todo
    std::cout << "Initialize level 2\n";
    fluidSystem.Initialize();
    startEndPointSystem.Initialize();

    startEndPointSystem.SetupStartPoint({-700.0f, 300.0f}, {100.0f, 100.0f}, StartEndType::Pipe,
                                        GoalDirection::Down);
    startEndPointSystem.SetupEndPoint({700.0f, -300.0f}, {100.0f, 100.0f}, StartEndType::Flower,
                                      GoalDirection::Up);
}

void UpdateLevel2(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    // std::cout << "Update level 2\n";

    f64 dt64 = AEFrameRateControllerGetFrameTime();
    f32 dt32 = (f32)dt64;

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
    if (AEInputCheckCurr(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {

        startEndPointSystem.CheckMouseClick();

    } else if (AEInputCheckReleased(AEVK_LBUTTON)) {

        startEndPointSystem.ResetIframe();
    }

    for (auto& startPoint : startEndPointSystem.startPoints_) {
        if (startPoint.release_water_) {
            static f32 spawn_timer = 0.0f;
            spawn_timer -= dt32;
            if (spawn_timer <= 0.0f) {

                // RESET TIMER: Set this to how fast you want water to flow
                // Original: 0.005f;
                spawn_timer = 0.025f;

                // the particle spawns at the values shown below, including its FluidType
                f32 noise = ((static_cast<int>(AERandFloat() * 12345) % 100)) * 0.001f - 0.1f;

                f32 randRadius = 13.0f - (noise * 100.0f);

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

    fluidSystem.UpdateMain(dt32);
    startEndPointSystem.Update(dt32, fluidSystem.GetParticlePool(FluidType::Water));

    if (startEndPointSystem.CheckWinCondition(fluidSystem.particleMaxCount)) {
        std::cout << "WIN\n ";
    }
}

void DrawLevel2() {
    // Todo
    // std::cout << "Draw level 2\n";
    fluidSystem.DrawColor();
    startEndPointSystem.DrawColor();
}

void FreeLevel2() {
    // Todo
    std::cout << "Free level 2\n";
    fluidSystem.Free();
    startEndPointSystem.Free();
}

void UnloadLevel2() {
    // Todo
    std::cout << "Unload level 2\n";
}