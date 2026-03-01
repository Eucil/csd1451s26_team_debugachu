#include "States/LevelSelector.h"
#include "States/LevelManager.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"
#include "UISystem.h"

static Button level1Button;
static Button level2Button;
static Button level3Button;
static Text level1Text;
static Text level2Text;
static Text level3Text;
static s8 font;

void LoadLevelSelector() {
    // Todo
    // std::cout << "Load main menu\n";

    // Setup buttons
    level1Button = Button(AEVec2{0.0f, 200.0f}, AEVec2{400.0f, 200.0f});
    level2Button = Button(AEVec2{0.0f, -100.0f}, AEVec2{400.0f, 200.0f});
    level3Button = Button(AEVec2{0.0f, -300.0f}, AEVec2{400.0f, 200.0f});

    level1Button.SetupMesh();
    level2Button.SetupMesh();
    level3Button.SetupMesh();

    // Setup texts
    level1Text = Text(-0.2f, 0.4f, "Level 1");
    level2Text = Text(-0.2f, -0.3f, "Level 2");
    level3Text = Text(-0.2f, -0.7f, "Level 3");

    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
}

void InitializeLevelSelector() {
    // Todo
    // std::cout << "Initialize main menu\n";
    levelManager.init();
    std::cout << "Level Editor Mode: " << (levelManager.getLevelEditorMode() ? "ON" : "OFF")
              << "\n";
}

void UpdateLevelSelector(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    // std::cout << "Update main menu\n";

    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    if (AEInputCheckTriggered(AEVK_E) || 0 == AESysDoesWindowExist()) {
        levelManager.setLevelEditorMode();
        if (levelManager.getLevelEditorMode()) {
            std::cout << "Level editor mode enabled\n";
        } else {
            std::cout << "Level editor mode disabled\n";
        }
    }

    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        if (level1Button.OnClick()) {
            std::cout << "Level1 button clicked\n";
            GSM.nextState_ = StateId::Level1;
        }
        if (level2Button.OnClick()) {
            std::cout << "Level2 button clicked\n";
            GSM.nextState_ = StateId::Level2;
        }
        if (level3Button.OnClick()) {
            std::cout << "Level3 button clicked\n";
            GSM.nextState_ = StateId::Level3;
        }
    }
}

void DrawLevelSelector() {
    // Todo
    // std::cout << "Draw main menu\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    level1Button.DrawColor(0.0f, 1.0f, 0.0f); // Green
    level2Button.DrawColor(1.0f, 0.0f, 0.0f); // Red
    level3Button.DrawColor(0.0f, 0.0f, 1.0f); // Blue

    const char* level1Str = level1Text.text_.c_str();
    AEGfxPrint(font, level1Str, level1Text.pos_x_, level1Text.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
    const char* level2Str = level2Text.text_.c_str();
    AEGfxPrint(font, level2Str, level2Text.pos_x_, level2Text.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
    const char* level3Str = level3Text.text_.c_str();
    AEGfxPrint(font, level3Str, level3Text.pos_x_, level3Text.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
}

void FreeLevelSelector() {
    // Todo
    // std::cout << "Free main menu\n";
}

void UnloadLevelSelector() {
    // Todo
    // std::cout << "Unload main menu\n";
    level1Button.UnloadMesh();
    level2Button.UnloadMesh();
    level3Button.UnloadMesh();
    AEGfxDestroyFont(font);
}