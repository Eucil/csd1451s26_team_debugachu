#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"

void LoadLevel1() {
    // Todo
    std::cout << "Load level 1\n";
}

void InitializeLevel1() {
    // Todo
    std::cout << "Initialize level 1\n";
}

void UpdateLevel1(GameStateManager& GSM, float deltaTime) {
    // Todo
    std::cout << "Update level 1\n";

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
}

void DrawLevel1() {
    // Todo
    std::cout << "Draw level 1\n";
}

void FreeLevel1() {
    // Todo
    std::cout << "Free level 1\n";
}

void UnloadLevel1() {
    // Todo
    std::cout << "Unload level 1\n";
}