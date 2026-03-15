#include "States/LevelSelector.h"
#include "States/LevelManager.h"

#include <iostream>

#include <AEEngine.h>

#include "Button.h"
#include "ConfigManager.h"
#include "GameStateManager.h"
#include "Utils.h"

static s8 font;

static std::vector<Button> editorButtonPool_;
static TextData titleText{
    "SELECT LEVEL",
    -0.4f,
    0.8f,
};

static bool creatingLevel = false;
static f32 width{}, height{}, tileSize{};
static int confirmInput{}, levelInput{};
static std::string inputStr;
static TextData inputPrompt{
    "Width",
    -0.4f,
    -0.8f,
};

void LoadLevelSelector() {
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);

    // Setup texts
    f32 button_startpos_x =
        configManager.getFloat("LevelSelector", "default", "button_startpos_x", -600.f);
    f32 button_startpos_y =
        configManager.getFloat("LevelSelector", "default", "button_startpos_y", 200.f);
    f32 button_x_offset =
        configManager.getFloat("LevelSelector", "default", "button_x_offset", 400.f);
    f32 button_y_offset =
        configManager.getFloat("LevelSelector", "default", "button_y_offset", 200.f);
    AEVec2 button_scale = {};
    button_scale.x = configManager.getFloat("LevelSelector", "default", "button_scale_x", 200.f);
    button_scale.y = configManager.getFloat("LevelSelector", "default", "button_scale_y", 150.f);
    f32 text_pos_divisor_x =
        configManager.getFloat("LevelSelector", "default", "text_pos_divisor_x", 800.f);
    f32 text_pos_divisor_y =
        configManager.getFloat("LevelSelector", "default", "text_pos_divisor_y", 450.f);
    f32 text_x_offset = configManager.getFloat("LevelSelector", "default", "text_x_offset", 0.025f);
    f32 text_y_offset = configManager.getFloat("LevelSelector", "default", "text_y_offset", 0.05f);
    f32 text_scale = configManager.getFloat("LevelSelector", "default", "text_scale", 1.f);
    f32 text_r = configManager.getFloat("LevelSelector", "default", "text_r", 1.f);
    f32 text_g = configManager.getFloat("LevelSelector", "default", "text_g", 1.f);
    f32 text_b = configManager.getFloat("LevelSelector", "default", "text_b", 1.f);
    f32 text_a = configManager.getFloat("LevelSelector", "default", "text_a", 1.f);

    for (int i{}, x{}, y{}; i < static_cast<int>(Level::None); ++i, ++x) {
        // Push back button and text
        if (i % 4 == 0 && i != 0) {
            y++;
            x = 0;
        }
        Button tempButton;
        AEVec2 buttonPos = {button_startpos_x + (button_x_offset * x),
                            button_startpos_y - (button_y_offset * y)};
        tempButton.setTransform(buttonPos, button_scale);
        tempButton.loadMesh();
        tempButton.loadTexture("Assets/Textures/pale_blue_button.png");

        tempButton.setText(std::to_string(i + 1), buttonPos.x / text_pos_divisor_x - text_x_offset,
                           buttonPos.y / text_pos_divisor_y - text_y_offset, text_scale, text_r,
                           text_g, text_b, text_a);
        editorButtonPool_.push_back(tempButton);
    }
}

void InitializeLevelSelector() {
    // Todo
    // std::cout << "Initialize Level Selector\n";
    levelManager.init();

    levelManager.checkLevelData();

    titleText.content_ = "SELECT LEVEL";
}

void UpdateLevelSelector(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    // std::cout << "Update main menu\n";
    (void)deltaTime; // Unused parameter, but required by function signature

    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    if ((AEInputCheckTriggered(AEVK_E) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        if (levelManager.getLevelEditorMode() == editorMode::Edit) {
            levelManager.setLevelEditorMode(editorMode::None);
            std::cout << "Level editor mode disabled\n";
            titleText.content_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(editorMode::Edit);
            std::cout << "Level editor mode enabled\n";
            titleText.content_ = "LEVEL EDITOR MODE";
        }
    }

    if ((AEInputCheckTriggered(AEVK_C) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        if (levelManager.getLevelEditorMode() == editorMode::Create) {
            levelManager.setLevelEditorMode(editorMode::None);
            std::cout << "Create level mode disabled\n";
            titleText.content_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(editorMode::Create);
            std::cout << "Create level mode enabled\n";
            titleText.content_ = "CREATE LEVEL";
        }
    }
    if ((AEInputCheckTriggered(AEVK_D) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        // Add level deletion logic here
        if (levelManager.getLevelEditorMode() == editorMode::Delete) {
            levelManager.setLevelEditorMode(editorMode::None);
            std::cout << "Delete level mode disabled\n";
            titleText.content_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(editorMode::Delete);
            std::cout << "Delete level mode enabled\n";
            titleText.content_ = "DELETE LEVEL";
        }
    }

    if ((AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        for (int i = 0; i < static_cast<int>(Level::None); ++i) {
            // Clicks for level selection and editor
            if (editorButtonPool_[i].checkMouseClick()) {
                std::cout << "Level " << (i + 1) << " button clicked\n";

                // Handle level selection based on editor mode
                switch (levelManager.getLevelEditorMode()) {
                case editorMode::None:
                    // If none, just play the level if it's playable
                    if (levelManager.playableLevels_[i]) {
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
            inputPrompt.content_ = "Width: " + inputStr;
            if (AEInputCheckReleased(AEVK_RETURN)) {
                confirmInput++;
                width = std::stof(inputStr);
                inputStr = "";
            }
            break;
        case 1:
            inputNumbers(inputStr);
            inputPrompt.content_ = "Height: " + inputStr;
            if (AEInputCheckReleased(AEVK_RETURN)) {
                confirmInput++;
                height = std::stof(inputStr);
                inputStr = "";
            }
            break;
        case 2:
            inputNumbers(inputStr);
            inputPrompt.content_ = "Tile Size: " + inputStr;
            if (AEInputCheckReleased(AEVK_RETURN)) {
                tileSize = std::stof(inputStr);
                levelManager.createLevelData(levelInput, static_cast<int>(width),
                                             static_cast<int>(height), static_cast<int>(tileSize));
                creatingLevel = false;
                levelManager.setLevelEditorMode(editorMode::None);
                titleText.content_ = "SELECT LEVEL";
                levelManager.checkLevelData();
            }
            break;
        }
    }

    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        editorButtonPool_[i].updateTransform();
    }
}

void DrawLevelSelector() {
    // Todo
    // std::cout << "Draw Level Selector\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Draw button with different color base on level editor mode
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        if (levelManager.playableLevels_[i]) {
            editorButtonPool_[i].draw(font);
        } else {
            editorButtonPool_[i].draw(font);
        }
    }

    titleText.draw(font);

    if (creatingLevel) {
        inputPrompt.draw(font);
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
        editorButtonPool_[i].unload();
    }

    editorButtonPool_.clear();
    AEGfxDestroyFont(font);
}