/*!
@file       PlayerLevel.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file implements the PlayerLevel game state,
            handling player-created level selection, map preview, level creation UI,
            collectible display, and the destructible background.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/PlayerLevel.h"

// ==========================================
// Standard library
// ==========================================
#include <iostream>

// ==========================================
// Third-party
// ==========================================
#include <AEEngine.h>

// ==========================================
// Project
// ==========================================
#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "Collectible.h"
#include "ConfigManager.h"
#include "Confirmation.h"
#include "DebugSystem.h"
#include "FluidSystem.h"
#include "GameStateManager.h"
#include "LevelManager.h"
#include "MenuBackground.h"
#include "Terrain.h"
#include "VFXSystem.h"

// ==========================================
// Fonts
// ==========================================
static s8 titleFont = 0;
static s8 buttonFont = 0;

// ==========================================
// Game Systems
// ==========================================
static VFXSystem lsVfxSystem;
static CollectibleSystem lsCollectibleSystem;

// ==========================================
// Map Preview
// ==========================================
static std::vector<AEGfxTexture*> previewTextures;
static AEGfxTexture* defaultPreviewTex = nullptr;
static AEGfxVertexList* previewMesh = nullptr;
static int hoveredLevelIndex = -1; // -1 means the mouse is not in a button
static UIFader previewFader(8.0f); // Fast fade in/out, 8.0f refers to speed of fade out
static int displayLevelIndex = -1;

// ==========================================
// Animations
// ==========================================
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

// ==========================================
// Level Buttons & Text
// ==========================================
static std::vector<Button> levelButtonPool_;
static TextData titleText{};
static std::string titleBaseText;

// ==========================================
// Level Creation UI
// ==========================================
static bool creatingLevel = false;
static int inputWidthMin{}, inputWidthMax{}, inputHeightMin{}, inputHeightMax{},
    inputPortalLimitMin{}, inputPortalLimitMax{};
static int inputWidth{}, inputHeight{}, inputPortalLimit{};
static int levelInput{};
static std::string widthBaseText, heightBaseText, portalLimitBaseText;
static TextData inputWidthPrompt;
static TextData inputHeightPrompt;
static TextData inputPortalLimitPrompt;
static Button buttonIncreaseWidth;
static Button buttonDecreaseWidth;
static Button buttonIncreaseHeight;
static Button buttonDecreaseHeight;
static Button buttonIncreasePortalLimit;
static Button buttonDecreasePortalLimit;
static Button buttonConfirmCreation;
static Button buttonCancelCreation;
static Button creationBackground;

// ==========================================
// Action Buttons
// ==========================================
static Button buttonSelect;
static Button buttonEdit;
static Button buttonCreate;
static Button buttonDelete;
static Button buttonBack;
static Button buttonToLevelSelector;

// ==========================================
// Level Index Range
// ==========================================
static int startLevelIndex = static_cast<int>(Level::PlayerLevels);
static int numPlayerLevels = static_cast<int>(Level::None) - startLevelIndex;

static ConfirmationSystem confirmationSystem;
static int levelToDeleteIndex = -1;

// ==========================================
// Static Functions
// ==========================================
static void mapPreviewLoad();
static void mapPreviewUpdate(f32 deltaTime);
static void mapPreviewDraw();
static void drawPlaceholderSlots(AEVec2 buttonPos, int collectedCount, AEGfxVertexList* mesh);

// =========================================================
//
// loadPlayerLevel()
//
// - Loads fonts, background, map preview textures,
// - the player level button pool, creation UI assets, and action buttons.
// - Called once per session (not on restart).
//
// =========================================================
void loadPlayerLevel() {
    // Font
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // Background & map preview
    MenuBackground::load(100);
    mapPreviewLoad();

    // Level button config
    f32 buttonStartposX =
        g_configManager.getFloat("PlayerLevel", "default", "buttonStartposX", -600.f);
    f32 buttonStartposY =
        g_configManager.getFloat("PlayerLevel", "default", "buttonStartposY", 200.f);
    f32 buttonXOffset = g_configManager.getFloat("PlayerLevel", "default", "buttonXOffset", 400.f);
    f32 buttonYOffset = g_configManager.getFloat("PlayerLevel", "default", "buttonYOffset", 200.f);
    AEVec2 buttonScale = {};
    buttonScale.x = g_configManager.getFloat("PlayerLevel", "default", "buttonScale_x", 200.f);
    buttonScale.y = g_configManager.getFloat("PlayerLevel", "default", "buttonScale_y", 150.f);
    f32 textPosDivisorX =
        g_configManager.getFloat("PlayerLevel", "default", "textPosDivisorX", 800.f);
    f32 textPosDivisorY =
        g_configManager.getFloat("PlayerLevel", "default", "textPosDivisorY", 450.f);
    f32 textXOffset = g_configManager.getFloat("PlayerLevel", "default", "textXOffset", 0.025f);
    f32 textYOffset = g_configManager.getFloat("PlayerLevel", "default", "textYOffset", 0.05f);
    f32 textScale = g_configManager.getFloat("PlayerLevel", "default", "textScale", 1.f);
    f32 textR = g_configManager.getFloat("PlayerLevel", "default", "textR", 1.f);
    f32 textG = g_configManager.getFloat("PlayerLevel", "default", "textG", 1.f);
    f32 textB = g_configManager.getFloat("PlayerLevel", "default", "textB", 1.f);
    f32 textA = g_configManager.getFloat("PlayerLevel", "default", "textA", 1.f);
    f32 extraOffsetX = 0.f;

    // Level creation config
    inputWidthMin = g_configManager.getInt("PlayerLevel", "levelCreation", "inputWidthMin", 5);
    inputHeightMin = g_configManager.getInt("PlayerLevel", "levelCreation", "inputHeightMin", 5);
    inputPortalLimitMin =
        g_configManager.getInt("PlayerLevel", "levelCreation", "inputPortalLimitMin", 1);
    inputWidthMax = g_configManager.getInt("PlayerLevel", "levelCreation", "inputWidthMax", 85);
    inputHeightMax = g_configManager.getInt("PlayerLevel", "levelCreation", "inputHeightMax", 40);
    inputPortalLimitMax =
        g_configManager.getInt("PlayerLevel", "levelCreation", "inputPortalLimitMax", 10);

    // Level buttons
    // i = button pool index, x/y = grid position
    for (int i{}, x{}, y{}; i < numPlayerLevels; ++i, ++x) {
        // Push back button and text
        if (i % 4 == 0 && i != 0) {
            y++;
            x = 0;
        }
        if (i % 9 == 0 && i != 0) {
            extraOffsetX += 0.03f;
        }
        Button tempButton;
        AEVec2 buttonPos = {buttonStartposX + (buttonXOffset * x),
                            buttonStartposY - (buttonYOffset * y)};
        tempButton.setTransform(buttonPos, buttonScale);
        tempButton.loadMesh();
        tempButton.loadTexture("Assets/Textures/brown_square_50_50.png");

        tempButton.setText(
            std::to_string(i + 1), buttonPos.x / textPosDivisorX - textXOffset - extraOffsetX,
            buttonPos.y / textPosDivisorY - textYOffset, textScale, textR, textG, textB, textA);
        tempButton.setTextFont(titleFont);
        levelButtonPool_.push_back(tempButton);
    }

    // Animations
    animManager.clear();
    animManager.add(&screenFader);
    animManager.add(&someOtherCoolAnimation);
    animManager.initializeAll();

    // Creation UI buttons
    buttonIncreaseWidth.loadMesh();
    buttonIncreaseWidth.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseWidth.loadMesh();
    buttonDecreaseWidth.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonIncreaseHeight.loadMesh();
    buttonIncreaseHeight.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseHeight.loadMesh();
    buttonDecreaseHeight.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonIncreasePortalLimit.loadMesh();
    buttonIncreasePortalLimit.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreasePortalLimit.loadMesh();
    buttonDecreasePortalLimit.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonConfirmCreation.loadMesh();
    buttonConfirmCreation.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    buttonCancelCreation.loadMesh();
    buttonCancelCreation.loadTexture("Assets/Textures/brown_rectangle_40_24.png");

    creationBackground.loadMesh();
    creationBackground.setRGBA(0.f, 0.f, 0.f, 0.8f);
    creationBackground.setTransform({0.f, 0.f}, {static_cast<float>(AEGfxGetWindowWidth()),
                                                 static_cast<float>(AEGfxGetWindowHeight())});
    // Action buttons
    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    buttonSelect.loadMesh();
    buttonSelect.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    buttonEdit.loadMesh();
    buttonEdit.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    buttonCreate.loadMesh();
    buttonCreate.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    buttonDelete.loadMesh();
    buttonDelete.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    buttonToLevelSelector.loadMesh();
    buttonToLevelSelector.loadTexture("Assets/Textures/brown_rectangle_40_24.png");

    // Confirmation
    confirmationSystem.load();
    confirmationSystem.hide();
}

// =========================================================
//
// initializePlayerLevel()
//
// - Initializes background, VFX, level manager, collectible icons,
// - UI text, creation UI buttons, and action buttons.
// - Called on both first load and every restart.
//
// =========================================================
void initializePlayerLevel() {

    // Background & VFX
    MenuBackground::initialize();
    lsVfxSystem.initialize(800, 20);

    // Level manager
    levelManager.init();
    levelManager.checkLevelData();

    // Collectibles
    // i is the button pool index; the actual level score is fetched at i + 1 + startLevelIndex
    lsCollectibleSystem.initialize();
    for (int i{}; i < numPlayerLevels; ++i) {
        int count = levelManager.getHighScore(i + 1 + startLevelIndex);
        std::cout << count << '\n';
        // Safety cap assuming a max of 3 items per level
        if (count > 3)
            count = 3;

        // Fetch the world position of the specific button
        AEVec2 buttonPos = levelButtonPool_[i].getTransform().pos_;

        f32 spacing = 40.0f; // Gap between each collectible

        // Center the spawn points based on how many items we are drawing
        f32 startX = buttonPos.x - (spacing * (3 - 1)) / 2.0f;
        f32 posY = buttonPos.y - 95.0f; // Shift down to be below the button edge

        for (int j = 0; j < count; ++j) {
            AEVec2 pos = {startX + j * spacing, posY};
            // Type 0 = Star, Type 1 = Gem, Type 2 = Leaf
            CollectibleType type = static_cast<CollectibleType>(j);
            lsCollectibleSystem.loadLevelCollectibles(pos, type);
        }
    }

    // UI text
    titleText.initFromJson("player_level_texts", "Header");
    titleText.font_ = titleFont;
    titleBaseText = titleText.content_;

    // Text and Buttons for level creation UI
    inputWidthPrompt.initFromJson("player_level_texts", "inputWidthPrompt");
    inputWidthPrompt.font_ = buttonFont;
    widthBaseText = inputWidthPrompt.content_;
    inputHeightPrompt.initFromJson("player_level_texts", "inputHeightPrompt");
    inputHeightPrompt.font_ = buttonFont;
    heightBaseText = inputHeightPrompt.content_;
    inputPortalLimitPrompt.initFromJson("player_level_texts", "inputPortalLimitPrompt");
    inputPortalLimitPrompt.font_ = buttonFont;
    portalLimitBaseText = inputPortalLimitPrompt.content_;
    buttonIncreaseWidth.initFromJson("player_level_buttons", "IncreaseWidth");
    buttonIncreaseWidth.setTextFont(buttonFont);
    buttonDecreaseWidth.initFromJson("player_level_buttons", "DecreaseWidth");
    buttonDecreaseWidth.setTextFont(buttonFont);
    buttonIncreaseHeight.initFromJson("player_level_buttons", "IncreaseHeight");
    buttonIncreaseHeight.setTextFont(buttonFont);
    buttonDecreaseHeight.initFromJson("player_level_buttons", "DecreaseHeight");
    buttonDecreaseHeight.setTextFont(buttonFont);
    buttonIncreasePortalLimit.initFromJson("player_level_buttons", "IncreasePortalLimit");
    buttonIncreasePortalLimit.setTextFont(buttonFont);
    buttonDecreasePortalLimit.initFromJson("player_level_buttons", "DecreasePortalLimit");
    buttonDecreasePortalLimit.setTextFont(buttonFont);
    buttonConfirmCreation.initFromJson("player_level_buttons", "ConfirmCreate");
    buttonConfirmCreation.setTextFont(buttonFont);
    buttonCancelCreation.initFromJson("player_level_buttons", "CancelCreate");
    buttonCancelCreation.setTextFont(buttonFont);

    buttonIncreaseWidth.updateTransform();
    buttonDecreaseWidth.updateTransform();
    buttonIncreaseHeight.updateTransform();
    buttonDecreaseHeight.updateTransform();
    buttonIncreasePortalLimit.updateTransform();
    buttonDecreasePortalLimit.updateTransform();
    buttonConfirmCreation.updateTransform();
    buttonCancelCreation.updateTransform();

    // Action buttons for selecting/editing/creating/deleting levels
    buttonBack.initFromJson("player_level_buttons", "Back");
    buttonBack.setTextFont(buttonFont);
    buttonSelect.initFromJson("player_level_buttons", "Select");
    buttonSelect.setTextFont(buttonFont);
    buttonEdit.initFromJson("player_level_buttons", "Edit");
    buttonEdit.setTextFont(buttonFont);
    buttonCreate.initFromJson("player_level_buttons", "Create");
    buttonCreate.setTextFont(buttonFont);
    buttonDelete.initFromJson("player_level_buttons", "Delete");
    buttonDelete.setTextFont(buttonFont);
    buttonToLevelSelector.initFromJson("player_level_buttons", "ToLevelSelector");
    buttonToLevelSelector.setTextFont(buttonFont);

    buttonBack.updateTransform();
    buttonSelect.updateTransform();
    buttonEdit.updateTransform();
    buttonCreate.updateTransform();
    buttonDelete.updateTransform();
    creationBackground.updateTransform();
    buttonToLevelSelector.updateTransform();

    // Confirmation System
    confirmationSystem.init(buttonFont);
}

// =========================================================
//
// updatePlayerLevel(GameStateManager& GSM, f32 deltaTime)
//
// - Handles per-frame input for level selection, editor mode switching,
// - level creation UI, confirmation dialog, and background digging.
// - Uses startLevelIndex to offset button indices into the player level range.
//
// =========================================================
void updatePlayerLevel(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        (void)deltaTime; // Unused parameter, but required by function signature

        // Map preview
        mapPreviewUpdate(deltaTime);

        // Confirmation system not displayed inputs
        if (!confirmationSystem.isShowing()) {

            // Select Button
            if ((buttonSelect.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                if (levelManager.getLevelEditorMode() != EditorMode::None) {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // Edit Button
            if ((buttonEdit.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                if (levelManager.getLevelEditorMode() != EditorMode::Edit) {
                    levelManager.setLevelEditorMode(EditorMode::Edit);
                    titleText.content_ = "EDIT LEVEL";
                } else {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // Create Button
            if ((buttonCreate.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                if (levelManager.getLevelEditorMode() != EditorMode::Create) {
                    levelManager.setLevelEditorMode(EditorMode::Create);
                    titleText.content_ = "CREATE LEVEL";
                } else {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // Delete Button
            if ((buttonDelete.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                // Add level deletion logic here
                if (levelManager.getLevelEditorMode() != EditorMode::Delete) {
                    levelManager.setLevelEditorMode(EditorMode::Delete);
                    titleText.content_ = "DELETE LEVEL";
                } else {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // To Level Selector Button
            if ((buttonToLevelSelector.checkMouseClick() || 0 == AESysDoesWindowExist()) &&
                !creatingLevel) {
                screenFader.startFadeOut(&GSM, StateId::LevelSelector);
            }

            // Back Button
            if ((buttonBack.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                screenFader.startFadeOut(&GSM, StateId::MainMenu);
            }

            // Level Selection Buttons
            if ((AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) &&
                !creatingLevel) {
                for (int i = 0, j = startLevelIndex; i < numPlayerLevels; ++i, ++j) {
                    // Clicks for level selection and editor
                    if (levelButtonPool_[i].checkMouseClick()) {
                        std::cout << "Level " << (i + 1) << " button clicked\n";

                        // Handle level selection based on editor mode
                        switch (levelManager.getLevelEditorMode()) {
                        case EditorMode::None:
                            // If none, just play the level if it's playable
                            if (levelManager.playableLevels_[j]) {
                                levelManager.setCurrentLevel(j + 1);
                                screenFader.startFadeOut(&GSM, StateId::Level);
                            }
                            break;
                        case EditorMode::Edit:
                            // If edit, go to level editor with selected level
                            if (levelManager.playableLevels_[j]) {
                                levelManager.setCurrentLevel(j + 1);
                                GSM.nextState_ = StateId::Level;
                            }
                            break;
                        case EditorMode::Create:
                            // If create, allow user to input width, height and tilesize before
                            // creating Only can create levels in empty slots, which means if
                            // playableLevels_[i] is false. This is to prevent accidental
                            // overwriting of existing levels, since creating a level will overwrite
                            if (!levelManager.playableLevels_[j]) {
                                std::cout << "Level editor mode: Create\n";
                                creatingLevel = true;
                                inputWidth = inputWidthMax;
                                inputHeight = inputHeightMax;
                                inputPortalLimit = inputPortalLimitMin;
                                levelInput = j + 1;
                            }
                            break;
                        case EditorMode::Delete:
                            // If delete, delete the level data and update playable levels
                            std::cout << "Level editor mode: Delete\n";
                            levelToDeleteIndex = j + 1;
                            confirmationSystem.show();
                            confirmationSystem.setTask(ConfirmationTask::Delete);
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }

        // Confirmation system inputs
        if (confirmationSystem.confirmationYesClicked()) {
            if (confirmationSystem.getTask() == ConfirmationTask::Delete) {
                levelManager.deleteLevelData(levelToDeleteIndex);
                levelManager.checkLevelData();
                levelToDeleteIndex = -1;
                confirmationSystem.hide();
            }
        }
        if (confirmationSystem.confirmationNoClicked()) {
            confirmationSystem.hide();
            confirmationSystem.setTask(ConfirmationTask::No);
            levelToDeleteIndex = -1;
        }

        // Digging for destructible background
        if (!creatingLevel) {
            if (AEInputCheckCurr(AEVK_LBUTTON)) {
                bool hitDirt = MenuBackground::destroyDirtAtMouse(20.0f);
                if (hitDirt)
                    g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
            }
        }
        MenuBackground::update(deltaTime);
        lsVfxSystem.update(deltaTime);

        // Level creation UI
        if (creatingLevel) {
            inputWidthPrompt.content_ = widthBaseText + std::to_string(inputWidth);
            inputHeightPrompt.content_ = heightBaseText + std::to_string(inputHeight);
            inputPortalLimitPrompt.content_ =
                portalLimitBaseText + std::to_string(inputPortalLimit);

            // Width buttons
            if (buttonIncreaseWidth.checkMouseClick()) {
                inputWidth += 5;
                if (inputWidth > inputWidthMax)
                    inputWidth = inputWidthMax;
            }
            if (buttonDecreaseWidth.checkMouseClick()) {
                inputWidth -= 5;
                if (inputWidth < inputWidthMin)
                    inputWidth = inputWidthMin;
            }
            // Height buttons
            if (buttonIncreaseHeight.checkMouseClick()) {
                inputHeight += 5;
                if (inputHeight > inputHeightMax)
                    inputHeight = inputHeightMax;
            }
            if (buttonDecreaseHeight.checkMouseClick()) {
                inputHeight -= 5;
                if (inputHeight < inputHeightMin)
                    inputHeight = inputHeightMin;
            }
            // Portal buttons
            if (buttonIncreasePortalLimit.checkMouseClick()) {
                inputPortalLimit += 2;
                if (inputPortalLimit > inputPortalLimitMax)
                    inputPortalLimit = inputPortalLimitMax;
            }
            if (buttonDecreasePortalLimit.checkMouseClick()) {
                inputPortalLimit -= 2;
                if (inputPortalLimit < inputPortalLimitMin)
                    inputPortalLimit = inputPortalLimitMin;
            }
            // Create/Cancel buttons
            if (buttonConfirmCreation.checkMouseClick()) {
                // Create level with input parameters
                levelManager.createLevelData(levelInput, inputWidth, inputHeight, 20,
                                             inputPortalLimit);
                creatingLevel = false;
                levelManager.setLevelEditorMode(EditorMode::None);
                titleText.content_ = titleBaseText;
                levelManager.checkLevelData();
            }
            if (buttonCancelCreation.checkMouseClick()) {
                creatingLevel = false;
                levelManager.setCurrentLevel(0);
                std::cout << "Cancelled level creation\n";
            }

            for (int i = 0; i < numPlayerLevels; ++i) {
                levelButtonPool_[i].updateTransform();
            }
        }
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }
    // Always update
    std::vector<FluidParticle> dummyPool;
    lsCollectibleSystem.update(deltaTime, dummyPool, lsVfxSystem);

    animManager.updateAll(deltaTime);
    confirmationSystem.update();
}

// =========================================================
//
// drawPlayerLevel()
//
// - Renders background, player level buttons, collectible icons,
// - placeholder slots, title text, creation overlay,
// - action buttons, map preview, and confirmation dialog.
//
// =========================================================
void drawPlayerLevel() {

    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(1.0f);

    // Background & VFX
    MenuBackground::draw();
    lsVfxSystem.draw();

    // Draw button with different color base on level editor mode
    for (int i = 0; i < numPlayerLevels; ++i) {
        if (levelManager.playableLevels_[i + startLevelIndex]) {
            levelButtonPool_[i].setRGBA(1.0f, 1.0f, 1.f, 1.f); // Base color for playable levels
        } else {
            levelButtonPool_[i].setRGBA(0.5f, 0.5f, 0.5f, 1.f); // Grey for non-playable levels
        }

        // Disable hover effect when creating level or confirmation
        if (creatingLevel || confirmationSystem.isShowing()) {
            levelButtonPool_[i].draw(false);
        } else {
            levelButtonPool_[i].draw();
        }
    }

    // Draw collectibles
    lsCollectibleSystem.draw();

    // Collectible placeholders
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 1.0f); // Dark Grey / Almost Black

    for (int i = 0; i < numPlayerLevels; ++i) {
        int count = levelManager.getHighScore(i + startLevelIndex + 1);
        if (count > 3)
            count = 3;
        drawPlaceholderSlots(levelButtonPool_[i].getTransform().pos_, count, previewMesh);
    }

    // UI text & buttons
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    titleText.draw(true);

    // Level creation overlay
    if (creatingLevel) {
        creationBackground.setRGBA(0.f, 0.f, 0.f, 0.8f);
        creationBackground.draw(false);
        inputWidthPrompt.draw(true);
        inputHeightPrompt.draw(true);
        inputPortalLimitPrompt.draw(true);

        buttonIncreaseWidth.draw();
        buttonDecreaseWidth.draw();
        buttonIncreaseHeight.draw();
        buttonDecreaseHeight.draw();
        buttonIncreasePortalLimit.draw();
        buttonDecreasePortalLimit.draw();
        buttonConfirmCreation.draw();
        buttonCancelCreation.draw();
    }

    // Action buttons & map preview
    if (!creatingLevel && !confirmationSystem.isShowing()) {
        buttonBack.draw();
        buttonSelect.draw();
        buttonEdit.draw();
        buttonCreate.draw();
        buttonDelete.draw();
        buttonToLevelSelector.draw();
        mapPreviewDraw();
    }

    // Confirmation, animations & debug
    confirmationSystem.draw();
    animManager.drawAll();
    g_debugSystem.drawAll();
}

// =========================================================
//
// freePlayerLevel()
//
// - Frees per-session runtime resources:
// - background, VFX, animations, and collectibles.
// - Called on every restart and on state exit.
//
// =========================================================
void freePlayerLevel() {
    MenuBackground::free();
    lsVfxSystem.free();

    g_debugSystem.clearScene();
    animManager.freeAll();

    lsCollectibleSystem.free();
}

// =========================================================
//
// unloadPlayerLevel()
//
// - Unloads all persistent assets loaded in loadPlayerLevel():
// - level buttons, creation UI buttons, action buttons,
// - fonts, preview mesh, preview textures, background, and confirmation.
// - Called once when leaving the PlayerLevel state.
//
// =========================================================

void unloadPlayerLevel() {

    // Level Buttons
    for (int i = 0; i < numPlayerLevels; ++i) {
        levelButtonPool_[i].unload();
    }

    // Unload Creation UI Buttons
    buttonIncreaseWidth.unload();
    buttonDecreaseWidth.unload();
    buttonIncreaseHeight.unload();
    buttonDecreaseHeight.unload();
    buttonIncreasePortalLimit.unload();
    buttonDecreasePortalLimit.unload();
    buttonConfirmCreation.unload();
    buttonCancelCreation.unload();

    // Unload Action Buttons
    buttonBack.unload();
    buttonSelect.unload();
    buttonEdit.unload();
    buttonCreate.unload();
    buttonDelete.unload();
    buttonToLevelSelector.unload();
    creationBackground.unload();

    levelButtonPool_.clear();

    // Unload fonts
    if (titleFont) {
        AEGfxDestroyFont(titleFont);
        titleFont = 0;
    }
    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }

    // Preview Mesh and Textures
    if (previewMesh) {
        AEGfxMeshFree(previewMesh);
        previewMesh = nullptr;
    }
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

    // Unload destructible background
    MenuBackground::unload();

    // Unload remaining System
    confirmationSystem.unload();
    animManager.freeAll();
}

// =========================================================
//
//  Static Functions Definition
//
// =========================================================

// =========================================================
//
// mapPreviewLoad()
//
// - Creates the shared preview quad mesh and preloads
// - all level preview textures from Assets/Previews/.
//
// =========================================================
static void mapPreviewLoad() {
    previewMesh = createRectMesh();
    defaultPreviewTex = AEGfxTextureLoad("Assets/Previews/Empty.png");
    // Preload all preview images
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        // Form the string needed for the file path
        std::string filePath = "Assets/Previews/Level_" + std::to_string(i + 1) + ".png";
        previewTextures.push_back(AEGfxTextureLoad(filePath.c_str()));
    }
}

// =========================================================
//
// mapPreviewUpdate(f32 deltaTime)
//
// - Tracks which level button the mouse is hovering over
// - and drives the preview image fade-in/fade-out animation.
//
// =========================================================
static void mapPreviewUpdate(f32 deltaTime) {
    hoveredLevelIndex = -1;
    for (int i = 0; i < levelButtonPool_.size(); ++i) {
        levelButtonPool_[i].updateTransform();
        bool isPlayable = (levelManager.playableLevels_[i + static_cast<int>(Level::PlayerLevels)]);
        // If the mouse is colliding with this specific button, save its index
        if (levelButtonPool_[i].isHovered() && isPlayable) {
            hoveredLevelIndex = i + static_cast<int>(Level::PlayerLevels);
        }
    }
    if (hoveredLevelIndex != -1) {
        displayLevelIndex = hoveredLevelIndex; // Remember what we are looking at
        previewFader.fadeIn();                 // Tell it to animate in
    } else {
        previewFader.fadeOut(); // Tell it to animate out
    }

    // Process the math for this frame
    previewFader.update(deltaTime);
}

// =========================================================
//
// mapPreviewDraw()
//
// - Renders the hovered level's preview image near the cursor,
// - clamping position to stay within screen boundaries.
// - Uses UIFader alpha for smooth fade transitions.
//
// =========================================================
static void mapPreviewDraw() {
    // If mouse is within a button, hoveredLevelIndex = button level.
    // Ensure that hLI is < number of elements within the vector container
    if (previewFader.isVisible() && displayLevelIndex != -1 &&
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
            AEGfxSetTransparency(previewFader.getAlpha());

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

// =========================================================
//
// drawPlaceholderSlots()
//
// - Draws dark placeholder icons below a level button
// - for each collectible slot the player has not yet filled.
//
// =========================================================
static void drawPlaceholderSlots(AEVec2 buttonPos, int collectedCount, AEGfxVertexList* mesh) {
    // Only need to draw if they haven't collected everything
    if (collectedCount >= 3)
        return;

    constexpr f32 spacing = 40.0f;
    constexpr f32 slotScale = 18.0f;
    f32 startX = buttonPos.x - (spacing * (3 - 1)) / 2.0f;
    f32 posY = buttonPos.y - 95.0f;

    for (int j = collectedCount; j < 3; ++j) {
        AEVec2 pos = {startX + j * spacing, posY};

        AEMtx33 scale, trans, world;
        AEMtx33Scale(&scale, slotScale, slotScale);
        AEMtx33Trans(&trans, pos.x, pos.y);
        AEMtx33Concat(&world, &trans, &scale);

        AEGfxSetTransform(world.m);
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }
}