/*!
@file       LevelSelector.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
                        Chia Hanxin/c.hanxin@digipen.edu,
                        Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This source file implements the LevelSelector game state,
            handling level button layout, map preview, level creation UI,
            collectible display, and the destructible background.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
                        Reproduction or disclosure of this file or its contents
                        without the prior written consent of DigiPen Institute of
                        Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/LevelSelector.h"

// Standard library
#include <iostream>

// Third-party
#include <AEEngine.h>

// Project
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
static Button buttonToPlayerLevel;

// ==========================================
// Confirmation
// ==========================================
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
// loadLevelSelector()
//
// - Loads fonts, background, map preview textures,
// - level button pool, creation UI assets, and action buttons.
// - Called once per session (not on restart).
//
// =========================================================
void loadLevelSelector() {
    // Fonts
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // Background & map preview
    MenuBackground::load(100);
    mapPreviewLoad();

    // Level button config
    f32 buttonStartposX =
        g_configManager.getFloat("LevelSelector", "default", "buttonStartposX", -600.f);
    f32 buttonStartposY =
        g_configManager.getFloat("LevelSelector", "default", "buttonStartposY", 200.f);
    f32 buttonXOffset =
        g_configManager.getFloat("LevelSelector", "default", "buttonXOffset", 400.f);
    f32 buttonYOffset =
        g_configManager.getFloat("LevelSelector", "default", "buttonYOffset", 200.f);
    AEVec2 buttonScale = {};
    buttonScale.x = g_configManager.getFloat("LevelSelector", "default", "buttonScale_x", 200.f);
    buttonScale.y = g_configManager.getFloat("LevelSelector", "default", "buttonScale_y", 150.f);
    f32 textPosDivisorX =
        g_configManager.getFloat("LevelSelector", "default", "textPosDivisorX", 800.f);
    f32 textPosDivisorY =
        g_configManager.getFloat("LevelSelector", "default", "textPosDivisorY", 450.f);
    f32 textXOffset = g_configManager.getFloat("LevelSelector", "default", "textXOffset", 0.025f);
    f32 textYOffset = g_configManager.getFloat("LevelSelector", "default", "textYOffset", 0.05f);
    f32 textScale = g_configManager.getFloat("LevelSelector", "default", "textScale", 1.f);
    f32 textR = g_configManager.getFloat("LevelSelector", "default", "textR", 1.f);
    f32 textG = g_configManager.getFloat("LevelSelector", "default", "textG", 1.f);
    f32 textB = g_configManager.getFloat("LevelSelector", "default", "textB", 1.f);
    f32 textA = g_configManager.getFloat("LevelSelector", "default", "textA", 1.f);
    f32 extraOffsetX = 0.f;

    // Level creation config
    inputWidthMin = g_configManager.getInt("LevelSelector", "levelCreation", "inputWidthMin", 5);
    inputHeightMin = g_configManager.getInt("LevelSelector", "levelCreation", "inputHeightMin", 5);
    inputPortalLimitMin =
        g_configManager.getInt("LevelSelector", "levelCreation", "inputPortalLimitMin", 1);
    inputWidthMax = g_configManager.getInt("LevelSelector", "levelCreation", "inputWidthMax", 85);
    inputHeightMax = g_configManager.getInt("LevelSelector", "levelCreation", "inputHeightMax", 40);
    inputPortalLimitMax =
        g_configManager.getInt("PlayerLevel", "levelCreation", "inputPortalLimitMax", 10);

    // Level buttons
    for (int i{}, x{}, y{}; i < static_cast<int>(Level::PlayerLevels); ++i, ++x) {
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

    // UI text & animations
    titleText.font_ = titleFont;
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
    buttonToPlayerLevel.loadMesh();
    buttonToPlayerLevel.loadTexture("Assets/Textures/brown_rectangle_40_24.png");

    // Confirmation
    confirmationSystem.load();
    confirmationSystem.hide();
}

// =========================================================
//
// initializeLevelSelector()
//
// - Initializes the background, VFX, level data, collectible icons,
// - UI text, creation UI buttons, and action buttons.
// - Called on both first load and every restart.
//
// =========================================================
void initializeLevelSelector() {

    // Background & VFX
    MenuBackground::initialize();
    lsVfxSystem.initialize(800, 20);

    // Level manager
    levelManager.init();
    levelManager.checkLevelData();

    // Collectibles
    lsCollectibleSystem.initialize();
    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        int count = levelManager.getHighScore(i + 1);
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
    titleText.initFromJson("level_selector_texts", "Header");
    titleText.font_ = titleFont;
    titleBaseText = titleText.content_;

    // Level creation UI
    inputWidthPrompt.initFromJson("level_selector_texts", "inputWidthPrompt");
    inputWidthPrompt.font_ = buttonFont;
    widthBaseText = inputWidthPrompt.content_;
    inputHeightPrompt.initFromJson("level_selector_texts", "inputHeightPrompt");
    inputHeightPrompt.font_ = buttonFont;
    heightBaseText = inputHeightPrompt.content_;
    inputPortalLimitPrompt.initFromJson("level_selector_texts", "inputPortalLimitPrompt");
    inputPortalLimitPrompt.font_ = buttonFont;
    portalLimitBaseText = inputPortalLimitPrompt.content_;
    buttonIncreaseWidth.initFromJson("level_selector_buttons", "IncreaseWidth");
    buttonIncreaseWidth.setTextFont(buttonFont);
    buttonDecreaseWidth.initFromJson("level_selector_buttons", "DecreaseWidth");
    buttonDecreaseWidth.setTextFont(buttonFont);
    buttonIncreaseHeight.initFromJson("level_selector_buttons", "IncreaseHeight");
    buttonIncreaseHeight.setTextFont(buttonFont);
    buttonDecreaseHeight.initFromJson("level_selector_buttons", "DecreaseHeight");
    buttonDecreaseHeight.setTextFont(buttonFont);
    buttonIncreasePortalLimit.initFromJson("level_selector_buttons", "IncreasePortalLimit");
    buttonIncreasePortalLimit.setTextFont(buttonFont);
    buttonDecreasePortalLimit.initFromJson("level_selector_buttons", "DecreasePortalLimit");
    buttonDecreasePortalLimit.setTextFont(buttonFont);
    buttonConfirmCreation.initFromJson("level_selector_buttons", "ConfirmCreate");
    buttonConfirmCreation.setTextFont(buttonFont);
    buttonCancelCreation.initFromJson("level_selector_buttons", "CancelCreate");
    buttonCancelCreation.setTextFont(buttonFont);

    buttonIncreaseWidth.updateTransform();
    buttonDecreaseWidth.updateTransform();
    buttonIncreaseHeight.updateTransform();
    buttonDecreaseHeight.updateTransform();
    buttonIncreasePortalLimit.updateTransform();
    buttonDecreasePortalLimit.updateTransform();
    buttonConfirmCreation.updateTransform();
    buttonCancelCreation.updateTransform();

    // Action buttons
    buttonBack.initFromJson("level_selector_buttons", "Back");
    buttonBack.setTextFont(buttonFont);
    buttonSelect.initFromJson("level_selector_buttons", "Select");
    buttonSelect.setTextFont(buttonFont);
    buttonEdit.initFromJson("level_selector_buttons", "Edit");
    buttonEdit.setTextFont(buttonFont);
    buttonCreate.initFromJson("level_selector_buttons", "Create");
    buttonCreate.setTextFont(buttonFont);
    buttonDelete.initFromJson("level_selector_buttons", "Delete");
    buttonDelete.setTextFont(buttonFont);
    buttonToPlayerLevel.initFromJson("level_selector_buttons", "ToPlayerLevel");
    buttonToPlayerLevel.setTextFont(buttonFont);

    buttonBack.updateTransform();
    buttonSelect.updateTransform();
    buttonEdit.updateTransform();
    buttonCreate.updateTransform();
    buttonDelete.updateTransform();
    buttonToPlayerLevel.updateTransform();
    creationBackground.updateTransform();

    // Confirmation System
    confirmationSystem.init(buttonFont);
}

// =========================================================
//
// updateLevelSelector(GameStateManager& GSM, f32 deltaTime)
//
// - Handles per-frame input for level selection, editor mode switching,
// - level creation UI, confirmation dialog, and background digging.
// - Also updates all systems and animations.
//
// =========================================================
void updateLevelSelector(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z))
            g_debugSystem.open();

        (void)deltaTime; // Unused parameter, but required by function signature

        // Map preview
        mapPreviewUpdate(deltaTime);

        //==========================
        // Update when confirmation dialog is not showing
        //==========================
        if (!confirmationSystem.isShowing()) {

            const bool levelEditorEnabled = g_debugSystem.options_.count("LevelEditorAccess") &&
                                            g_debugSystem.options_.at("LevelEditorAccess");

            // Reset editor mode if access was revoked
            if (!levelEditorEnabled && levelManager.getLevelEditorMode() != EditorMode::None) {
                levelManager.setLevelEditorMode(EditorMode::None);
                titleText.content_ = titleBaseText;
            }

            // Select Button
            if (levelEditorEnabled &&
                (buttonSelect.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                if (levelManager.getLevelEditorMode() != EditorMode::None) {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // Edit Button
            if (levelEditorEnabled &&
                (buttonEdit.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                if (levelManager.getLevelEditorMode() != EditorMode::Edit) {
                    levelManager.setLevelEditorMode(EditorMode::Edit);
                    titleText.content_ = "EDIT LEVEL";
                } else {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // Create Button
            if (levelEditorEnabled &&
                (buttonCreate.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                if (levelManager.getLevelEditorMode() != EditorMode::Create) {
                    levelManager.setLevelEditorMode(EditorMode::Create);
                    titleText.content_ = "CREATE LEVEL";
                } else {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // Delete Button
            if (levelEditorEnabled &&
                (buttonDelete.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                if (levelManager.getLevelEditorMode() != EditorMode::Delete) {
                    levelManager.setLevelEditorMode(EditorMode::Delete);
                    titleText.content_ = "DELETE LEVEL";
                } else {
                    levelManager.setLevelEditorMode(EditorMode::None);
                    titleText.content_ = titleBaseText;
                }
            }

            // To Player Level Button
            if ((buttonToPlayerLevel.checkMouseClick() || 0 == AESysDoesWindowExist()) &&
                !creatingLevel) {
                screenFader.startFadeOut(&GSM, StateId::PlayerLevel);
            }

            // Back Button
            if ((buttonBack.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
                screenFader.startFadeOut(&GSM, StateId::MainMenu);
            }

            // Level Selection Buttons
            if ((AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) &&
                !creatingLevel) {
                for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
                    // Clicks for level selection and editor
                    if (levelButtonPool_[i].checkMouseClick()) {
                        std::cout << "Level " << (i + 1) << " button clicked\n";

                        // Handle level selection based on editor mode
                        switch (levelManager.getLevelEditorMode()) {
                        case EditorMode::None:
                            // If none, just play the level if it's playable
                            if (levelManager.playableLevels_[i]) {
                                levelManager.setCurrentLevel(i + 1);
                                screenFader.startFadeOut(&GSM, StateId::Level);
                            }
                            break;
                        case EditorMode::Edit:
                            // If edit, go to level editor with selected level
                            if (levelManager.playableLevels_[i]) {
                                levelManager.setCurrentLevel(i + 1);
                                GSM.nextState_ = StateId::Level;
                            }
                            break;
                        case EditorMode::Create:
                            // If create, allow user to input width, height and tilesize before
                            // creating. Only can create levels in empty slots, which means if
                            // playableLevels_[i] is false. This is to prevent accidental
                            // overwriting of existing levels, since creating a level will overwrite
                            if (!levelManager.playableLevels_[i]) {
                                std::cout << "Level editor mode: Create\n";
                                creatingLevel = true;
                                inputWidth = inputWidthMax;
                                inputHeight = inputHeightMax;
                                inputPortalLimit = inputPortalLimitMin;
                                levelInput = i + 1;
                            }
                            break;
                        case EditorMode::Delete:
                            // If delete, open confirmation tab before delete
                            std::cout << "Level editor mode: Delete\n";
                            levelToDeleteIndex = i + 1;
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

        // Confirmation System Inputs
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

            // Width Buttons
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
            // Height Buttons
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
            // Portal Buttons
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
            // Confirm/Cancel Button
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

            for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
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
// drawLevelSelector()
//
// - Renders all visual layers each frame:
// - background, level buttons, collectible icons, placeholder slots,
// - title text, creation overlay, action buttons, and map preview.
//
// =========================================================
void drawLevelSelector() {

    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(1.0f);

    // Background & VFX
    MenuBackground::draw();
    lsVfxSystem.draw();

    // Level buttons
    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        if (levelManager.playableLevels_[i]) {
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

    // Collectibles
    lsCollectibleSystem.draw();

    // Collectible placeholders
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 1.0f); // Dark Grey / Almost Black

    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        int count = levelManager.getHighScore(i + 1);
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
        buttonToPlayerLevel.draw();

        if (g_debugSystem.options_.count("LevelEditorAccess") &&
            g_debugSystem.options_.at("LevelEditorAccess")) {
            buttonSelect.draw();
            buttonEdit.draw();
            buttonCreate.draw();
            buttonDelete.draw();
        }

        mapPreviewDraw();
    }

    // Confirmation, animations & debug
    confirmationSystem.draw();
    animManager.drawAll();
    g_debugSystem.drawAll();
}

// =========================================================
//
// freeLevelSelector()
//
// - Frees per-session runtime resources:
// - background, VFX, animations, and collectibles.
// - Called on every restart and on state exit.
//
// =========================================================
void freeLevelSelector() {
    MenuBackground::free();
    lsVfxSystem.free();
    g_debugSystem.clearScene();
    animManager.freeAll();
    lsCollectibleSystem.free();
}

// =========================================================
//
// unloadLevelSelector()
//
// - Unloads all persistent assets loaded in loadLevelSelector():
// - level buttons, creation UI buttons, action buttons,
// - fonts, preview mesh, preview textures, and background.
// - Called once when leaving the LevelSelector state.
//
// =========================================================
void unloadLevelSelector() {

    // Level buttons
    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        levelButtonPool_[i].unload();
    }

    // Creation UI buttons
    buttonIncreaseWidth.unload();
    buttonDecreaseWidth.unload();
    buttonIncreaseHeight.unload();
    buttonDecreaseHeight.unload();
    buttonIncreasePortalLimit.unload();
    buttonDecreasePortalLimit.unload();
    buttonConfirmCreation.unload();
    buttonCancelCreation.unload();

    // Action buttons
    buttonBack.unload();
    buttonSelect.unload();
    buttonEdit.unload();
    buttonCreate.unload();
    buttonDelete.unload();
    buttonToPlayerLevel.unload();
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

    // Unload preview mesh and textures
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

    // Unload background
    MenuBackground::unload();

    // Confirmation
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
    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
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
    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        levelButtonPool_[i].updateTransform();
        bool isPlayable = (levelManager.playableLevels_[i]);
        // If the mouse is colliding with this specific button, save its index
        if (levelButtonPool_[i].isHovered() && isPlayable) {
            hoveredLevelIndex = i;
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