#include "States/LevelSelector.h"

#include <iostream>

#include <AEEngine.h>

#include "Animations.h"
#include "Button.h"
#include "ConfigManager.h"
#include "GameStateManager.h"
#include "States/LevelManager.h"
#include "Utils.h"

// Destructible Background
#include "AudioSystem.h"
#include "Terrain.h"
#include "VFXSystem.h"

static s8 font;

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

    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pBgDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    // pBgStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
    // pBgMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");

    // Calculate window bounds
    f32 halfW = AEGfxGetWindowWidth() / 2.0f;
    f32 halfH = AEGfxGetWindowHeight() / 2.0f;

    // TILING LOGIC: Repeat the texture every 64 pixels
    f32 uRepeat = AEGfxGetWindowWidth() / 64.0f;
    f32 vRepeat = AEGfxGetWindowHeight() / 64.0f;

    previewMesh = CreateRectMesh();
    defaultPreviewTex = AEGfxTextureLoad("Assets/Textures/pink_button.png");
    // Preload all preview images
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        // Form the string needed for the file path
        std::string filePath = "Assets/Previews/Level_" + std::to_string(i + 1) + ".png";
        previewTextures.push_back(AEGfxTextureLoad(filePath.c_str()));
    }

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

    for (int i{}, x{}, y{}; i < static_cast<int>(Level::None); ++i, ++x) {
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
        tempButton.setTextFont(font);
        editorButtonPool_.push_back(tempButton);
    }

    titleText.font_ = font;
    inputPrompt.font_ = font;
    animManager.Clear();
    animManager.Add(&screenFader);
    animManager.Add(&someOtherCoolAnimation);
    animManager.InitializeAll();
}

void InitializeLevelSelector() {
    // Todo
    // std::cout << "Initialize Level Selector\n";
    levelManager.init();
    levelManager.checkLevelData();

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
}

void UpdateLevelSelector(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    // std::cout << "Update main menu\n";
    (void)deltaTime; // Unused parameter, but required by function signature

    hoveredLevelIndex = -1;
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        editorButtonPool_[i].updateTransform();
        bool isPlayable = (levelManager.playableLevels_[i]);
        // If the mouse is colliding with this specific button, save its index
        if (editorButtonPool_[i].isHovered() && isPlayable) {
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
    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    if ((AEInputCheckTriggered(AEVK_E) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        if (levelManager.getLevelEditorMode() == EditorMode::Edit) {
            levelManager.setLevelEditorMode(EditorMode::None);
            std::cout << "Level editor mode disabled\n";
            titleText.content_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(EditorMode::Edit);
            std::cout << "Level editor mode enabled\n";
            titleText.content_ = "LEVEL EDITOR MODE";
        }
    }

    if ((AEInputCheckTriggered(AEVK_C) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        if (levelManager.getLevelEditorMode() == EditorMode::Create) {
            levelManager.setLevelEditorMode(EditorMode::None);
            std::cout << "Create level mode disabled\n";
            titleText.content_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(EditorMode::Create);
            std::cout << "Create level mode enabled\n";
            titleText.content_ = "CREATE LEVEL";
        }
    }
    if ((AEInputCheckTriggered(AEVK_D) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        // Add level deletion logic here
        if (levelManager.getLevelEditorMode() == EditorMode::Delete) {
            levelManager.setLevelEditorMode(EditorMode::None);
            std::cout << "Delete level mode disabled\n";
            titleText.content_ = "SELECT LEVEL";
        } else {
            levelManager.setLevelEditorMode(EditorMode::Delete);
            std::cout << "Delete level mode enabled\n";
            titleText.content_ = "DELETE LEVEL";
        }
    }
    if ((AEInputCheckTriggered(AEVK_Q) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        std::cout << "Q triggered\n";
        GSM.nextState_ = StateId::MainMenu;
    }

    if ((AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) && !creatingLevel) {
        for (int i = 0; i < static_cast<int>(Level::None); ++i) {
            // Clicks for level selection and editor
            if (editorButtonPool_[i].checkMouseClick()) {
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
                    levelManager.SetCurrentLevel(i + 1);
                    GSM.nextState_ = StateId::Level;
                    break;
                case EditorMode::Create:
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
                g_audioSystem.playSound("dirt_break", "sfx", 0.25f, 1.0f);
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
                levelManager.setLevelEditorMode(EditorMode::None);
                titleText.content_ = "SELECT LEVEL";
                levelManager.checkLevelData();
            }
            break;
        }
    }

    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        editorButtonPool_[i].updateTransform();
    }
    animManager.UpdateAll(deltaTime);
}

void DrawLevelSelector() {
    // Todo
    // std::cout << "Draw Level Selector\n";
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(1.0f);

    bgDirt->renderTerrain();
    bgVfxSystem.Draw();

    // Draw button with different color base on level editor mode
    for (int i = 0; i < static_cast<int>(Level::None); ++i) {
        if (levelManager.playableLevels_[i]) {
            editorButtonPool_[i].setRGBA(1.0f, 1.0f, 1.f, 1.f); // Pale blue for playable levels
            editorButtonPool_[i].draw();
        } else {
            editorButtonPool_[i].setRGBA(0.5f, 0.5f, 0.5f, 1.f); // Grey for non-playable levels
            editorButtonPool_[i].draw();
        }
    }

    titleText.draw();

    if (creatingLevel) {
        inputPrompt.draw();
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
    animManager.DrawAll();
}

void FreeLevelSelector() {
    // Todo
    // std::cout << "Free main menu\n";
    bgVfxSystem.Free();

    delete bgDirt;
    bgDirt = nullptr;
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

    // Unload destructible background
    Terrain::freeMeshLibrary();
    if (pBgDirtTex) {
        AEGfxTextureUnload(pBgDirtTex);
        pBgDirtTex = nullptr;
    }

    animManager.FreeAll();
}