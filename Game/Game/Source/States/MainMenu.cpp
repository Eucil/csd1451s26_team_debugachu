#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"
#include "UISystem.h"

static Button level1Button;
static Button level2Button;
static Text level1Text;
static Text level2Text;
static s8 font;

void LoadMainMenu() {
    // Todo
    // std::cout << "Load main menu\n";

    // Setup buttons
    level1Button = Button(AEVec2{0.0f, 200.0f}, AEVec2{400.0f, 200.0f});
    level2Button = Button(AEVec2{0.0f, -100.0f}, AEVec2{400.0f, 200.0f});

    level1Button.SetupMesh();
    level2Button.SetupMesh();

    // Setup texts
    level1Text = Text(-0.2f, 0.4f, "Level 1");
    level2Text = Text(-0.2f, -0.3f, "Level 2");

    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
}

void InitializeMainMenu() {
    // Todo
    // std::cout << "Initialize main menu\n";
}

void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    // std::cout << "Update main menu\n";

    // Press 1 to go to level 1
    if (AEInputCheckTriggered(AEVK_1) || 0 == AESysDoesWindowExist()) {
        std::cout << "1 triggered\n";
        GSM.nextState_ = StateId::Level1;
    }

    // Press 2 to go to level 2
    if (AEInputCheckTriggered(AEVK_2) || 0 == AESysDoesWindowExist()) {
        std::cout << "2 triggered\n";
        GSM.nextState_ = StateId::Level2;
    }

    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        if (level1Button.OnClick()) {
            std::cout << "Start button clicked\n";
            GSM.nextState_ = StateId::Level1;
        }
        if (level2Button.OnClick()) {
            std::cout << "Exit button clicked\n";
            GSM.nextState_ = StateId::Level2;
        }
    }
}

void DrawMainMenu() {
    // Todo
    // std::cout << "Draw main menu\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    level1Button.DrawColor(0.0f, 1.0f, 0.0f); // Green
    level2Button.DrawColor(1.0f, 0.0f, 0.0f); // Red

    const char* level1Str = level1Text.text_.c_str();
    AEGfxPrint(font, level1Str, level1Text.pos_x_, level1Text.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
    const char* level2Str = level2Text.text_.c_str();
    AEGfxPrint(font, level2Str, level2Text.pos_x_, level2Text.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
}

void FreeMainMenu() {
    // Todo
    // std::cout << "Free main menu\n";
}

void UnloadMainMenu() {
    // Todo
    // std::cout << "Unload main menu\n";
    level1Button.UnloadMesh();
    level2Button.UnloadMesh();
    AEGfxDestroyFont(font);
}