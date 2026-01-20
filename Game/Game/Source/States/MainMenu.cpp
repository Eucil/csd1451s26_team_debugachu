#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"

void LoadMainMenu() {
    // Todo
    std::cout << "Load main menu\n";
}

void InitializeMainMenu() {
    // Todo
    std::cout << "Initialize main menu\n";
}

void UpdateMainMenu(GameStateManager& GSM, float deltaTime) {
    // Todo
    std::cout << "Update main menu\n";

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
}

void DrawMainMenu() {
    // Todo
    std::cout << "Draw main menu\n";
}

void FreeMainMenu() {
    // Todo
    std::cout << "Free main menu\n";
}

void UnloadMainMenu() {
    // Todo
    std::cout << "Unload main menu\n";
}