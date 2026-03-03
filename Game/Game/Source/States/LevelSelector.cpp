#include "States/LevelSelector.h"
#include "States/LevelManager.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"
#include "UISystem.h"

static s8 font;

static std::vector<Button> buttonPool;
static std::vector<Text> textPool;
static Text titleText = Text(-0.4f, 0.8f, "SELECT LEVEL");

static bool creatingLevel = false;
static f32 width{}, height{}, tileSize{};
static int confirmInput{}, levelInput{};
static std::string inputStr;
static Text inputPrompt = Text(-0.4f, -0.8f, "Width:");

void LoadLevelSelector() {
    // Todo
    // std::cout << "Load Level Selector\n";
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);

    // Setup texts
    f32 x_offset = 0.025f;
    f32 y_offset = 0.05f;

    for (int i{}, x{}, y{}; i < static_cast<int>(Level::None); ++i, ++x) {
        // Push back button and text
        if (i % 4 == 0 && i != 0) {
            y++;
            x = 0;
        }
        Button tempButton =
            Button(AEVec2{-600.f + (400.f * x), 200.0f - (200.f * y)}, AEVec2{200.0f, 150.0f});
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

    levelManager.checkLevelData();

    titleText.text_ = "SELECT LEVEL";
}

void UpdateLevelSelector(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    // std::cout << "Update main menu\n";

    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    if ((AEInputCheckTriggered(AEVK_E) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        if (levelManager.getLevelEditorMode() == editorMode::Edit) {
            levelManager.setLevelEditorMode(editorMode::None);
            std::cout << "Level editor mode disabled\n";
            titleText.text_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(editorMode::Edit);
            std::cout << "Level editor mode enabled\n";
            titleText.text_ = "LEVEL EDITOR MODE";
        }
    }

    if ((AEInputCheckTriggered(AEVK_C) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        if (levelManager.getLevelEditorMode() == editorMode::Create) {
            levelManager.setLevelEditorMode(editorMode::None);
            std::cout << "Create level mode disabled\n";
            titleText.text_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(editorMode::Create);
            std::cout << "Create level mode enabled\n";
            titleText.text_ = "CREATE LEVEL";
        }
    }
    if ((AEInputCheckTriggered(AEVK_D) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        // Add level deletion logic here
        if (levelManager.getLevelEditorMode() == editorMode::Delete) {
            levelManager.setLevelEditorMode(editorMode::None);
            std::cout << "Delete level mode disabled\n";
            titleText.text_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(editorMode::Delete);
            std::cout << "Delete level mode enabled\n";
            titleText.text_ = "DELETE LEVEL";
        }
    }

    if ((AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        for (int i = 0; i < static_cast<int>(Level::None); ++i) {
            // Clicks for level selection and editor
            if (buttonPool[i].OnClick()) {
                std::cout << "Level " << (i + 1) << " button clicked\n";

                // Handle level selection based on editor mode
                switch (levelManager.getLevelEditorMode()) {
                case editorMode::None:
                    // If none, just play the level if it's playable
                    if (levelManager.playableLevels[i]) {
                        levelManager.SetCurrentLevel(i + 1);
                        GSM.nextState_ = StateId::Level;
                    }
                    break;
                case editorMode::Edit:
                    // If edit, go to level editor with selected level
                    levelManager.SetCurrentLevel(i + 1);
                    GSM.nextState_ = StateId::Level;
                    break;
                case editorMode::Create:
                    // If create, allow user to input width, height and tilesize before creating
                    std::cout << "Level editor mode: Create\n";
                    creatingLevel = true;
                    width = 0.f;
                    height = 0.f;
                    tileSize = 0.f;
                    confirmInput = 0;
                    inputStr = "";
                    levelInput = i + 1;
                    break;
                case editorMode::Delete:
                    // If delete, delete the level data and update playable levels
                    std::cout << "Level editor mode: Delete\n";
                    levelManager.deleteLevelData(i + 1);
                    levelManager.checkLevelData();
                    break;
                default:
                    break;
                }
            }
        }
    }

    if (creatingLevel) {
        if (AEInputCheckReleased(AEVK_Q) || 0 == AESysDoesWindowExist()) {
            creatingLevel = false;
            levelManager.SetCurrentLevel(0);
            std::cout << "Cancelled level creation\n";
        }
        switch (confirmInput) {
        case 0:
            inputNumbers(inputStr);
            inputPrompt.text_ = "Width: " + inputStr;
            if (AEInputCheckReleased(AEVK_RETURN)) {
                confirmInput++;
                width = std::stof(inputStr);
                inputStr = "";
            }
            break;
        case 1:
            inputNumbers(inputStr);
            inputPrompt.text_ = "Height: " + inputStr;
            if (AEInputCheckReleased(AEVK_RETURN)) {
                confirmInput++;
                height = std::stof(inputStr);
                inputStr = "";
            }
            break;
        case 2:
            inputNumbers(inputStr);
            inputPrompt.text_ = "Tile Size: " + inputStr;
            if (AEInputCheckReleased(AEVK_RETURN)) {
                tileSize = std::stof(inputStr);
                levelManager.createLevelData(levelInput, width, height, tileSize);
                creatingLevel = false;
                levelManager.setLevelEditorMode(editorMode::None);
                titleText.text_ = "SELECT LEVEL";
                levelManager.checkLevelData();
            }
            break;
        }
    }
}

void DrawLevelSelector() {
    // Todo
    // std::cout << "Draw Level Selector\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Draw button with different color base on level editor mode
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

    const char* titleStr = titleText.text_.c_str();
    AEGfxPrint(font, titleStr, titleText.pos_x_, titleText.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);

    if (creatingLevel) {
        const char* inputPromptStr = inputPrompt.text_.c_str();
        AEGfxPrint(font, inputPromptStr, inputPrompt.pos_x_, inputPrompt.pos_y_, 1.f, 1.f, 1.f, 1.f,
                   1.f);
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