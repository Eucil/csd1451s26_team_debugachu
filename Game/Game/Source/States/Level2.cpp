#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"

#include "Components.h"

#include "FluidSystem.h"

static FluidSystem fluidSystem;

void LoadLevel2() {
    // Todo
    std::cout << "Load level 2\n";
}

void InitializeLevel2() {
    // Todo
    std::cout << "Initialize level 2\n";
    fluidSystem.Initialize();
}

void UpdateLevel2(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    std::cout << "Update level 2\n";

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
        // STATIC VARIABLE: Keeps its value between function calls
        // We use this to count down time
        static f32 spawn_timer = 0.0f;
        spawn_timer -= dt32;
        if (spawn_timer <= 0.0f) {

            // RESET TIMER: Set this to how fast you want water to flow
            // Original: 0.005f;
            spawn_timer = 0.005f;
            // Spawn WATER at mouse position
            s32 mouseX, mouseY;
            AEInputGetCursorPosition(&mouseX, &mouseY);

            // convert mouse coordinates to world coordinates.
            // (mouse coords start at top left while world coords start in the center)
            f32 worldX = (f32)mouseX - (1600.0f / 2.0f);
            f32 worldY = (900.0f / 2.0f) - (f32)mouseY;

            // the particle spawns at the values shown below, including its FluidType
            f32 noise = ((static_cast<int>(AERandFloat() * 12345) % 100)) * 0.001f - 0.1f;

            f32 randRadius = 13.0f - (noise * 100.0f);

            fluidSystem.SpawnParticle(worldX, worldY, randRadius, FluidType::Water);
            s32 size = fluidSystem.GetParticleCount(FluidType::Water);
            std::cout << size << '\n';
        }
    }
    fluidSystem.UpdateMain(dt32);
}

void DrawLevel2() {
    // Todo
    std::cout << "Draw level 2\n";
    fluidSystem.DrawColor();
}

void FreeLevel2() {
    // Todo
    std::cout << "Free level 2\n";
    fluidSystem.Free();
}

void UnloadLevel2() {
    // Todo
    std::cout << "Unload level 2\n";
}