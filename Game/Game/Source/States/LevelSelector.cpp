#include "States/LevelSelector.h"
#include "States/LevelManager.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"
#include "UISystem.h"

static s8 font;

static std::vector<Button> buttonPool;
static std::vector<Text> textPool;

void LoadLevelSelector() {
    // Todo
    // std::cout << "Load Level Selector\n";
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);

    // Setup texts
    f32 x_offset = 0.025f;
    f32 y_offset = 0.05f;

    for (int i{}; i < static_cast<int>(Level::None); ++i) {
        // Push back button and text
        Button tempButton = Button(AEVec2{-600.f + (300.f * i), 200.0f}, AEVec2{200.0f, 150.0f});
        tempButton.SetupMesh();
        buttonPool.push_back(tempButton);
        Text tempText =
            Text(tempButton.transform_.pos_.x / 800.f - x_offset,
                 tempButton.transform_.pos_.y / 450.f - y_offset, std::to_string(i + 1));
        textPool.push_back(tempText);
    }
}

void InitializeLevelSelector() {
    // Todo
    // std::cout << "Initialize Level Selector\n";
    levelManager.init();
    std::cout << "Level Editor Mode: " << (levelManager.getLevelEditorMode() ? "ON" : "OFF")
              << "\n";

    levelManager.checkLevelData();
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
        for (int i = 0; i < static_cast<int>(Level::None); ++i) {
            // Only allow level selection if the level is playable or if level editor mode is
            // enabled
            if ((buttonPool)[i].OnClick() &&
                (levelManager.playableLevels[i] || levelManager.getLevelEditorMode())) {
                std::cout << "Level " << (i + 1) << " button clicked\n";
                levelManager.SetCurrentLevel(i + 1);
                GSM.nextState_ = static_cast<StateId>(static_cast<int>(StateId::Level1) + i);
                break;
            }
        }
    }
}

void DrawLevelSelector() {
    // Todo
    // std::cout << "Draw Level Selector\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        if (levelManager.playableLevels[i]) {
            buttonPool[i].DrawColor(0.0f, 1.0f, 0.0f);
        } else {
            buttonPool[i].DrawColor(0.5f, 0.5f, 0.5f);
        }
    }

    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        const char* textStr = textPool[i].text_.c_str();
        AEGfxPrint(font, textStr, textPool[i].pos_x_, textPool[i].pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
    }
}

void FreeLevelSelector() {
    // Todo
    // std::cout << "Free main menu\n";
}

void UnloadLevelSelector() {
    // Todo
    // std::cout << "Unload main menu\n";
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        buttonPool[i].UnloadMesh();
    }

    buttonPool.clear();
    textPool.clear();
    AEGfxDestroyFont(font);
}