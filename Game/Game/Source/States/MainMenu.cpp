#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"

#include "FluidSystem.h"

static FluidSystem fluidSystem;

void LoadMainMenu() {
    // Todo
    std::cout << "Load main menu\n";
}

void InitializeMainMenu() {
    // Todo

    fluidSystem.Initialize();

    std::cout << "Initialize main menu\n";
}

void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    std::cout << "Update main menu\n";

    f64 dt64 = AEFrameRateControllerGetFrameTime();
    f32 dt32 = (f32)dt64;

    // Press 1 to go to level 1
    if (AEInputCheckTriggered(AEVK_1) || 0 == AESysDoesWindowExist()) {
        std::cout << "1 triggered\n";
        GSM.nextState_ = StateId::Level1;
    }

    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    // Press M1 to spawn particles
    if (AEInputCheckCurr(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        // STATIC VARIABLE: Keeps its value between function calls
        // We use this to count down time
        static f32 spawn_timer = 0.0f;
        spawn_timer -= dt32;
        if (spawn_timer <= 0.0f) {

            // RESET TIMER: Set this to how fast you want water to flow
            // Original: 0.005f;
        	spawn_timer = 0.05f; 
            // Spawn WATER at mouse position
            s32 mouseX, mouseY;
            AEInputGetCursorPosition(&mouseX, &mouseY);

            // convert mouse coordinates to world coordinates.
            // (mouse coords start at top left while world coords start in the center)
            f32 worldX = (f32)mouseX - (1600.0f / 2.0f);
            f32 worldY = (900.0f / 2.0f) - (f32)mouseY;

            // the particle spawns at the values shown below, including its FluidType
            fluidSystem.SpawnParticle(worldX, worldY, FluidType::Water);

		}



    }

    // Update functions
    fluidSystem.UpdateMain(dt32);
}

void DrawMainMenu() {
    // Todo
    std::cout << "Draw main menu\n";

    fluidSystem.DrawColor();
}

void FreeMainMenu() {
    // Todo
    std::cout << "Free main menu\n";

    fluidSystem.Free();
}

void UnloadMainMenu() {
    // Todo
    std::cout << "Unload main menu\n";
}