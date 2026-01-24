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
    if (AEInputCheckTriggered(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);
        // Spawn WATER at mouse position
        fluidSystem.SpawnParticle_d((f32)mouseX, (f32)mouseY, FluidType::Water);
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
}

void UnloadMainMenu() {
    // Todo
    std::cout << "Unload main menu\n";
}