#include "WinScreen.h"
#include "Utils.h"
#include <States/LevelManager.h>
#include <cstdio>

void WinScreen::Load(s8 font) {
    font_ = font;

    // Create background panel (semi-transparent black)
    graphics_.mesh_ = CreateRectMesh();
    graphics_.red_ = 0.0f;
    graphics_.green_ = 0.0f;
    graphics_.blue_ = 0.0f;
    graphics_.alpha_ = 0.7f;

    // Create buttons
    nextLevelButton_.loadMesh();
    nextLevelButton_.loadTexture("Assets/Textures/brown_button.png");

    restartButton_.loadMesh();
    restartButton_.loadTexture("Assets/Textures/brown_button.png");

    mainMenuButton_.loadMesh();
    mainMenuButton_.loadTexture("Assets/Textures/brown_button.png");

    // Initialize text
    titleText_ = TextData{"VICTORY!", -0.17f, 0.3f, 1.5f, 1.0f, 1.0f, 0.0f, 1.0f};
    collectiblesText_ = TextData{"", -0.21f, 0.1f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f};
    statsText_ = TextData{"", -0.34f, -0.1f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f};

    isVisible_ = false;
}

void WinScreen::Show(int collected, int total, int currentLevel) {
    collectiblesCollected_ = collected;
    totalCollectibles_ = total;
    currentLevel_ = currentLevel;
    nextLevel_ = currentLevel + 1;

    // Update text
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Collectibles: %d/%d", collected, total);
    collectiblesText_.content_ = buffer;

    // Position buttons

    restartButton_.setTransform({-200.0f, -140.0f}, {160.0f, 70.0f});
    restartButton_.setText("Restart", -0.31f, -0.32f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

    nextLevelButton_.setTransform({0.0f, -140.0f}, {160.0f, 70.0f});
    nextLevelButton_.setText("Next Level", -0.09f, -0.32f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

    mainMenuButton_.setTransform({200.0f, -140.0f}, {160.0f, 70.0f});
    mainMenuButton_.setText("Main Menu", 0.17f, -0.32f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Update transforms
    nextLevelButton_.updateTransform();
    restartButton_.updateTransform();
    mainMenuButton_.updateTransform();

    isVisible_ = true;
}

void WinScreen::Hide() { isVisible_ = false; }

void WinScreen::Update(GameStateManager& GSM) {
    if (!isVisible_)
        return;

    s32 windowHeight{AEGfxGetWindowHeight()};
    s32 windowWidth{AEGfxGetWindowWidth()};
    f32 worldMinX{AEGfxGetWinMinX()};
    f32 worldMinY{AEGfxGetWinMinY()};

    // Position at center of screen
    transform_.pos_ = {worldMinX + (static_cast<f32>(windowWidth) / 2.0f),
                       worldMinY + (static_cast<f32>(windowHeight) / 2.0f)};

    // Scale to fit screen
    transform_.scale_ = {static_cast<f32>(windowWidth), static_cast<f32>(windowHeight)};

    // No rotation
    transform_.rotationRad_ = 0.0f;

    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);

    // Check button clicks
    if (nextLevelButton_.checkMouseClick()) {
        // Check if there is a next level
        if (nextLevel_ <= 8) { // Assuming you have 8 levels
            levelManager.SetCurrentLevel(nextLevel_);
            GSM.nextState_ = StateId::Level;
        } else {
            // No more levels, go to main menu
            GSM.nextState_ = StateId::MainMenu;
        }
        Hide();
    }

    if (restartButton_.checkMouseClick()) {
        GSM.nextState_ = StateId::Restart;
        Hide();
    }

    if (mainMenuButton_.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
        Hide();
    }
}

void WinScreen::Draw() {
    if (!isVisible_)
        return;

    // Draw black-transparent background overlay
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(graphics_.red_, graphics_.green_, graphics_.blue_, graphics_.alpha_);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);

    // Draw text
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    titleText_.draw(font_);
    collectiblesText_.draw(font_);

    // Draw stats based on collectibles
    if (collectiblesCollected_ == totalCollectibles_) {
        statsText_.content_ = "Perfect! All collectibles found!";
        statsText_.scale_ = 0.7f;
    } else {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "You found %d of %d collectibles", collectiblesCollected_,
                 totalCollectibles_);
        statsText_.content_ = buffer;
        statsText_.scale_ = 0.8f;
    }
    statsText_.draw(font_);

    // Draw buttons
    nextLevelButton_.draw(font_);
    restartButton_.draw(font_);
    mainMenuButton_.draw(font_);
}

void WinScreen::Free() {
    // Don't unload textures here - they might still be needed
    // Just clear the visible state
    isVisible_ = false;

    // Clear any dynamic data if needed
    collectiblesCollected_ = 0;
    totalCollectibles_ = 0;

    printf("WinScreen: Freed\n");
}

void WinScreen::Unload() {
    // Properly unload all button resources
    nextLevelButton_.unload();
    restartButton_.unload();
    mainMenuButton_.unload();

    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }
    // Hide the screen
    isVisible_ = false;
    AEGfxDestroyFont(font_);

    printf("WinScreen: unloaded\n");
}