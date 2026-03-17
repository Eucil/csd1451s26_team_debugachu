#include "States/LevelSelector.h"
#include "States/LevelManager.h"

#include <iostream>

#include <AEEngine.h>

#include "Animations.h"
#include "Button.h"
#include "ConfigManager.h"
#include "GameStateManager.h"
#include "Utils.h"

static s8 font;

// Map Preview Variables
static std::vector<AEGfxTexture*> previewTextures;
static AEGfxTexture* defaultPreviewTex = nullptr;
static AEGfxVertexList* previewMesh = nullptr;
static int hoveredLevelIndex = -1; // -1 means the mouse is not in a button
static UIFader previewFader(8.0f); // Fast fade in/out, 8.0f refers to speed of fade out
static int displayLevelIndex = -1;

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

    previewMesh = CreateRectMesh();
    defaultPreviewTex = AEGfxTextureLoad("Assets/Textures/pink_button.png");
    // Preload all preview images
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        // Form the string needed for the file path
        std::string filePath = "Assets/Previews/Level_" + std::to_string(i + 1) + ".png";
        previewTextures.push_back(AEGfxTextureLoad(filePath.c_str()));
    }

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

    hoveredLevelIndex = -1;
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        editorButtonPool_[i].updateTransform();

        // If the mouse is colliding with this specific button, save its index!
        if (editorButtonPool_[i].isHovered()) {
            hoveredLevelIndex = i;
        }
    }
    if (hoveredLevelIndex != -1) {
        displayLevelIndex = hoveredLevelIndex; // Remember what we are looking at!
        previewFader.FadeIn();                 // Tell it to animate in
    } else {
        previewFader.FadeOut(); // Tell it to animate out
    }

    // Process the math for this frame
    previewFader.Update(deltaTime);
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

    // If mouse is within a button, hoveredLevelIndex = button level.
    // Ensure that hLI is < number of elements within the vector container
    if (previewFader.IsVisible() && displayLevelIndex != -1 &&
        displayLevelIndex < previewTextures.size()) {

        // Set current texture pointer to point to the current hovered level's image preview
        AEGfxTexture* texToDraw = previewTextures[displayLevelIndex];
        if (texToDraw == nullptr) {
            texToDraw = defaultPreviewTex;
        }
        // Safety check to ensure the texture actually loaded
        if (texToDraw != nullptr) {

            // Get current mouse position and convert to world space
            s32 screenX, screenY;
            AEInputGetCursorPosition(&screenX, &screenY);

            f32 windowWidth = static_cast<f32>(AEGfxGetWindowWidth());
            f32 windowHeight = static_cast<f32>(AEGfxGetWindowHeight());

            f32 mouseX = static_cast<f32>(screenX) - (windowWidth / 2.0f);
            f32 mouseY = (windowHeight / 2.0f) - static_cast<f32>(screenY);

            // Set preview size and padding
            f32 previewW = 300.0f;
            f32 previewH = 200.0f;
            f32 padding = 10.0f; // Distance from the cursor so it doesn't cover the mouse

            // Default Position: Top-Right of the cursor
            f32 drawX = mouseX + (previewW / 2.0f) + padding;
            f32 drawY = mouseY + (previewH / 2.0f) + padding;

            // Screen boundaries
            f32 rightScreenEdge = windowWidth / 2.0f;
            f32 topScreenEdge = windowHeight / 2.0f;

            // If drawing it Top-Right pushes it off the right edge, flip to Top-Left
            if (drawX + (previewW / 2.0f) > rightScreenEdge) {
                drawX = mouseX - (previewW / 2.0f) - padding;
            }

            // If drawing it Top-Right pushes it off the top edge, flip to Bottom-Right
            if (drawY + (previewH / 2.0f) > topScreenEdge) {
                drawY = mouseY - (previewH / 2.0f) - padding;
            }

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxTextureSet(texToDraw, 0, 0);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            // Set alpha based on UIFader value
            AEGfxSetTransparency(previewFader.GetAlpha());

            // Transform matrix for the image
            AEMtx33 scale, trans, world;
            AEMtx33Scale(&scale, previewW, previewH);
            AEMtx33Trans(&trans, drawX, drawY);
            AEMtx33Concat(&world, &trans, &scale);

            AEGfxSetTransform(world.m);
            AEGfxMeshDraw(previewMesh, AE_GFX_MDM_TRIANGLES);
        }
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

    if (previewMesh) {
        AEGfxMeshFree(previewMesh);
        previewMesh = nullptr;
    }

    // Unload all texture pointers in the vector container
    for (AEGfxTexture* tex : previewTextures) {
        if (tex != nullptr) {
            AEGfxTextureUnload(tex);
        }
    }
    previewTextures.clear();

    if (defaultPreviewTex != nullptr) {
        AEGfxTextureUnload(defaultPreviewTex);
        defaultPreviewTex = nullptr;
    }
}