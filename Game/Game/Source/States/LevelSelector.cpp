/*!
@file       LevelSelector.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
                        Chia Hanxin/c.hanxin@digipen.edu,
                        Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
                        Reproduction or disclosure of this file or its contents
                        without the prior written consent of DigiPen Institute of
                        Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/LevelSelector.h"

#include <AEEngine.h>
#include <iostream>

#include "Animations.h"
#include "Button.h"
#include "Collectible.h"
#include "ConfigManager.h"
#include "FluidSystem.h"
#include "GameStateManager.h"
#include "States/LevelManager.h"
#include "Utils.h"

// Destructible Background
#include "AudioSystem.h"
#include "DebugSystem.h"
#include "Terrain.h"
#include "VFXSystem.h"

static s8 titleFont = 0;
static s8 buttonFont = 0;

// Destructible Background Variables
static int bgHeight, bgWidth, bgTileSize, bgPortalLimit;
static bool bgFileExist;

static Terrain* bgDirt = nullptr;
static AEGfxTexture* pBgDirtTex{nullptr};
static VFXSystem bgVfxSystem;

// Map Preview Variables
static std::vector<AEGfxTexture*> previewTextures;
static AEGfxTexture* defaultPreviewTex = nullptr;
static AEGfxVertexList* previewMesh = nullptr;
static int hoveredLevelIndex = -1; // -1 means the mouse is not in a button
static UIFader previewFader(8.0f); // Fast fade in/out, 8.0f refers to speed of fade out
static int displayLevelIndex = -1;

// Animations
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

// Buttons and Text
static std::vector<Button> levelButtonPool_;
static TextData titleText{"SELECT LEVEL", 0.0f, 0.8f};
static CollectibleSystem lsCollectibleSystem;

// Level Creation UI
static bool creatingLevel = false;
static int inputWidth{}, inputHeight{}, inputPortalLimit{};
static int confirmInput{}, levelInput{};
static std::string inputStr;
static TextData inputPrompt{"Width", -0.4f, 0.6f};
static TextData recommendedPrompt{"Input between: 20-50", 0.0f, 0.0f};
static TextData quitCreatingPrompt{"Press Q to quit creating", 0.f, -0.3f};
static TextData enterCreatingPrompt{"Press Enter to confirm", 0.f, -0.6f};

static Button creationBackground;
static Button buttonSelect;
static Button buttonEdit;
static Button buttonCreate;
static Button buttonDelete;
static Button buttonBack;
static Button buttonToPlayerLevel;

// Static functions
static void MapPreviewLoad();
static void MapPreviewUpdate(f32 deltaTime);
static void MapPreviewDraw();
static void DrawPlaceholderSlots(AEVec2 buttonPos, int collectedCount, AEGfxVertexList* mesh);

void LoadLevelSelector() {
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pBgDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");

    MapPreviewLoad();

    // Setup texts
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

    titleText.font_ = titleFont;
    inputPrompt.font_ = titleFont;
    recommendedPrompt.font_ = titleFont;
    quitCreatingPrompt.font_ = titleFont;
    enterCreatingPrompt.font_ = titleFont;

    animManager.Clear();
    animManager.Add(&screenFader);
    animManager.Add(&someOtherCoolAnimation);
    animManager.InitializeAll();

    creationBackground.loadMesh();
    creationBackground.setRGBA(0.f, 0.f, 0.f, 0.8f);
    creationBackground.setTransform({0.f, 0.f}, {static_cast<float>(AEGfxGetWindowWidth()),
                                                 static_cast<float>(AEGfxGetWindowHeight())});
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
}

void InitializeLevelSelector() {

    levelManager.init();
    levelManager.checkLevelData();

    lsCollectibleSystem.Initialize(); // Creates the meshes and clears the list
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
            lsCollectibleSystem.LoadLevelCollectibles(pos, type);
        }
    }

    titleText.content_ = "SELECT LEVEL";
    // Initialize destructible Background
    bgVfxSystem.Initialize(800, 20);
    if (levelManager.getLevelData(100)) {
        levelManager.parseMapInfo(bgWidth, bgHeight, bgTileSize, bgPortalLimit);
        bgFileExist = true;
    } else {
        bgWidth = 80;
        bgHeight = 45;
        bgTileSize = 20;
        bgFileExist = false;
    }
    bgDirt = new Terrain(TerrainMaterial::Dirt, pBgDirtTex, {0.0f, 0.0f}, bgHeight, bgWidth,
                         bgTileSize, true);

    if (bgFileExist) {
        levelManager.parseTerrainInfo(bgDirt->getNodes(), "Dirt");
    }
    bgDirt->initCellsTransform();
    bgDirt->initCellsGraphics();
    bgDirt->initCellsCollider();
    bgDirt->updateTerrain();

    g_debugSystem.setScene(bgDirt, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                           &bgVfxSystem);

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
}

void UpdateLevelSelector(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z))
            g_debugSystem.open();

        (void)deltaTime; // Unused parameter, but required by function signature

        MapPreviewUpdate(deltaTime);

        // Press R to restart
        if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
            std::cout << "R triggered\n";
            GSM.nextState_ = StateId::Restart;
        }

        if ((buttonSelect.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
            if (levelManager.getLevelEditorMode() != EditorMode::None) {
                levelManager.setLevelEditorMode(EditorMode::None);
                titleText.content_ = "SELECT LEVEL";
            }
        }

        if ((buttonEdit.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
            if (levelManager.getLevelEditorMode() != EditorMode::Edit) {
                levelManager.setLevelEditorMode(EditorMode::Edit);
                titleText.content_ = "EDIT LEVEL";
            } else {
                levelManager.setLevelEditorMode(EditorMode::None);
                titleText.content_ = "SELECT LEVEL";
            }
        }

        if ((buttonCreate.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
            if (levelManager.getLevelEditorMode() != EditorMode::Create) {
                levelManager.setLevelEditorMode(EditorMode::Create);
                titleText.content_ = "CREATE LEVEL";
            } else {
                levelManager.setLevelEditorMode(EditorMode::None);
                titleText.content_ = "SELECT LEVEL";
            }
        }
        if ((buttonDelete.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
            // Add level deletion logic here
            if (levelManager.getLevelEditorMode() != EditorMode::Delete) {
                levelManager.setLevelEditorMode(EditorMode::Delete);
                titleText.content_ = "DELETE LEVEL";
            } else {
                levelManager.setLevelEditorMode(EditorMode::None);
                titleText.content_ = "SELECT LEVEL";
            }
        }

        if ((buttonToPlayerLevel.checkMouseClick() || 0 == AESysDoesWindowExist()) &&
            !creatingLevel) {          
            GSM.nextState_ = StateId::PlayerLevel;
            screenFader.StartFadeOut(&GSM, StateId::PlayerLevel);
        }

        if ((buttonBack.checkMouseClick() || 0 == AESysDoesWindowExist()) && !creatingLevel) {
            GSM.nextState_ = StateId::MainMenu;
            screenFader.StartFadeOut(&GSM, StateId::MainMenu);
        }

        if ((AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
            for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
                // Clicks for level selection and editor
                if (levelButtonPool_[i].checkMouseClick()) {
                    std::cout << "Level " << (i + 1) << " button clicked\n";

                    // Handle level selection based on editor mode
                    switch (levelManager.getLevelEditorMode()) {
                    case EditorMode::None:
                        // If none, just play the level if it's playable
                        if (levelManager.playableLevels_[i]) {
                            levelManager.SetCurrentLevel(i + 1);
                            screenFader.StartFadeOut(&GSM, StateId::Level);
                            // GSM.nextState_ = StateId::Level;
                        }
                        break;
                    case EditorMode::Edit:
                        // If edit, go to level editor with selected level
                        if (levelManager.playableLevels_[i]) {
                            levelManager.SetCurrentLevel(i + 1);
                            GSM.nextState_ = StateId::Level;
                        }
                        break;
                    case EditorMode::Create:
                        // If create, allow user to input width, height and tilesize before creating
                        // Only can create levels in empty slots, which means if playableLevels_[i]
                        // is false. This is to prevent accidental overwriting of existing levels,
                        // since creating a level will overwrite
                        if (!levelManager.playableLevels_[i]) {
                            std::cout << "Level editor mode: Create\n";
                            creatingLevel = true;
                            inputWidth = 0;
                            inputHeight = 0;
                            inputPortalLimit = 0;
                            confirmInput = 0;
                            inputStr = "";
                            levelInput = i + 1;
                        }
                        break;
                    case EditorMode::Delete:
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
        // Digging for destructible background
        if (!creatingLevel) {
            if (AEInputCheckCurr(AEVK_LBUTTON)) {
                bool hitDirt = bgDirt->destroyAtMouse(20.0f);
                if (hitDirt) {
                    bgVfxSystem.SpawnContinuous(VFXType::DirtBurst, GetMouseWorldPos(), deltaTime,
                                                0.1f);
                    g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
                } else {
                    bgVfxSystem.ResetSpawnTimer();
                }
            } else {
                bgVfxSystem.ResetSpawnTimer();
            }
        }
        bgVfxSystem.Update(deltaTime);

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
                recommendedPrompt.content_ = "Input between: 1 - 80";
                if (AEInputCheckReleased(AEVK_RETURN)) {
                    inputWidth = std::stoi(inputStr);
                    if (inputWidth <= 0 || inputWidth > 80) {
                        // Set back to 0 and prompt user again if input is invalid
                        inputWidth = 0;
                    } else {
                        confirmInput++;
                        inputStr = "";
                    }
                }
                break;
            case 1:
                inputNumbers(inputStr);
                inputPrompt.content_ = "Height: " + inputStr;
                recommendedPrompt.content_ = "Input between: 1 - 45";
                if (AEInputCheckReleased(AEVK_RETURN)) {
                    inputHeight = std::stoi(inputStr);
                    if (inputHeight <= 0 || inputHeight > 45) {
                        // Set back to 0 and prompt user again if input is invalid
                        inputHeight = 0;
                    } else {
                        confirmInput++;
                        inputStr = "";
                    }
                }
                break;
            case 2:
                inputNumbers(inputStr);
                inputPrompt.content_ = "Portal Limit: " + inputStr;
                recommendedPrompt.content_ = "Input between: 0 - 10";
                if (AEInputCheckReleased(AEVK_RETURN)) {
                    inputPortalLimit = std::stoi(inputStr);
                    if (inputPortalLimit < 0 || inputPortalLimit > 10) {
                        // Set back to 0 and prompt user again if input is invalid
                        inputPortalLimit = 0;
                    } else {
                        levelManager.createLevelData(levelInput, static_cast<int>(inputWidth),
                                                     static_cast<int>(inputHeight), 20,
                                                     static_cast<int>(inputPortalLimit));
                        creatingLevel = false;
                        levelManager.setLevelEditorMode(EditorMode::None);
                        titleText.content_ = "SELECT LEVEL";
                        levelManager.checkLevelData();
                    }
                }
                break;
            }
        }

        for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
            levelButtonPool_[i].updateTransform();
        }
        std::vector<FluidParticle> dummyPool;
        lsCollectibleSystem.Update(deltaTime, dummyPool, bgVfxSystem);
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }

    animManager.UpdateAll(deltaTime);
}

void DrawLevelSelector() {

    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(1.0f);

    bgDirt->renderTerrain();
    bgVfxSystem.Draw();

    // Draw button with different color base on level editor mode
    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        if (levelManager.playableLevels_[i]) {
            levelButtonPool_[i].setRGBA(1.0f, 1.0f, 1.f, 1.f); // Pale blue for playable levels
            levelButtonPool_[i].draw();
        } else {
            levelButtonPool_[i].setRGBA(0.5f, 0.5f, 0.5f, 1.f); // Grey for non-playable levels
            levelButtonPool_[i].draw();
        }
    }

    lsCollectibleSystem.Draw();

    // --- DRAW BLACK PLACEHOLDERS FOR UNCOLLECTED ITEMS (Local to LevelSelector) ---
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 1.0f); // Dark Grey / Almost Black

    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        int count = levelManager.getHighScore(i + 1);
        if (count > 3)
            count = 3;
        DrawPlaceholderSlots(levelButtonPool_[i].getTransform().pos_, count, previewMesh);
    }

    // --- RESET ENGINE GRAPHICS STATE FOR THE REST OF THE UI ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    buttonBack.draw();
    buttonSelect.draw();
    buttonEdit.draw();
    buttonCreate.draw();
    buttonDelete.draw();
    buttonToPlayerLevel.draw();

    titleText.draw(true);

    if (creatingLevel) {
        creationBackground.setRGBA(0.f, 0.f, 0.f, 0.8f);
        creationBackground.draw();
        inputPrompt.draw();
        recommendedPrompt.draw(true);
        quitCreatingPrompt.draw(true);
        enterCreatingPrompt.draw(true);
    }

    MapPreviewDraw();

    animManager.DrawAll();
    g_debugSystem.drawAll();
}

void FreeLevelSelector() {
    g_debugSystem.clearScene();
    bgVfxSystem.Free();
    animManager.FreeAll();

    lsCollectibleSystem.Free();

    delete bgDirt;
    bgDirt = nullptr;
}

void UnloadLevelSelector() {

    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        levelButtonPool_[i].unload();
    }

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

    // Unload destructible background
    Terrain::freeMeshLibrary();
    if (pBgDirtTex) {
        AEGfxTextureUnload(pBgDirtTex);
        pBgDirtTex = nullptr;
    }

    animManager.FreeAll();
}

// =========================================================
//
//              Map Preview Functions
//
// =========================================================
static void MapPreviewLoad() {
    previewMesh = CreateRectMesh();
    defaultPreviewTex = AEGfxTextureLoad("Assets/Previews/Empty.png");
    // Preload all preview images
    for (int i = 0; i < static_cast<int>(Level::PlayerLevels); ++i) {
        // Form the string needed for the file path
        std::string filePath = "Assets/Previews/Level_" + std::to_string(i + 1) + ".png";
        previewTextures.push_back(AEGfxTextureLoad(filePath.c_str()));
    }
}
static void MapPreviewUpdate(f32 deltaTime) {
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
        previewFader.FadeIn();                 // Tell it to animate in
    } else {
        previewFader.FadeOut(); // Tell it to animate out
    }

    // Process the math for this frame
    previewFader.Update(deltaTime);
}

static void MapPreviewDraw() {
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

// =========================================================
//
//              Collectibles Preview Functions
//
// =========================================================
// Static helper to draw placeholder collectible slots below a level button
static void DrawPlaceholderSlots(AEVec2 buttonPos, int collectedCount, AEGfxVertexList* mesh) {
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