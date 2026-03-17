#include "WinScreen.h"
#include "Utils.h"
#include <States/LevelManager.h>
#include <cstdio>

void WinScreen::Load(s8 font) {
    font_ = font;

    // Create background panel (semi-transparent black)
    backgroundPanel_.loadMesh();

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

    // Draw semi-transparent background overlay
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.7f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    // Full screen dark overlay
    AEMtx33 scale, trans, world;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&world, &trans, &scale);
    AEGfxSetTransform(world.m);

    // Use a simple quad mesh for overlay
    static AEGfxVertexList* quadMesh = nullptr;
    if (!quadMesh) {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
                    -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f,
                    0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
        quadMesh = AEGfxMeshEnd();
    }
    AEGfxMeshDraw(quadMesh, AE_GFX_MDM_TRIANGLES);

    // Draw white panel in center
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    AEMtx33Scale(&scale, 600.0f, 400.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&world, &trans, &scale);
    AEGfxSetTransform(world.m);
    AEGfxMeshDraw(quadMesh, AE_GFX_MDM_TRIANGLES);

    // Draw border (darker rectangle)
    AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 1.0f);
    AEMtx33Scale(&scale, 610.0f, 410.0f);
    AEMtx33Concat(&world, &trans, &scale);
    AEGfxSetTransform(world.m);
    AEGfxMeshDraw(quadMesh, AE_GFX_MDM_TRIANGLES);

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

void WinScreen::Shutdown() {
    // Properly unload all button resources
    backgroundPanel_.unload();
    nextLevelButton_.unload();
    restartButton_.unload();
    mainMenuButton_.unload();

    // Hide the screen
    isVisible_ = false;

    printf("WinScreen: Shutdown complete\n");
}