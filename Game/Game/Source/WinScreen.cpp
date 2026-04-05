/*!
@file       WinScreen.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date       March, 31, 2026

@brief      This source file contains the definitions of functions that
            implement the WinScreen overlay, which is shown when the player
            completes a level. It handles result display, collectible
            statistics, and navigation to the next level, restart, or
            main menu.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "WinScreen.h"

// Project
#include "AudioSystem.h"
#include "LevelManager.h"

// ==========================================
//            CONSTANTS
// ==========================================

// Level::None is the sentinel that ends the regular level list.
// static_cast<int>(Level::None) gives the total playable level count.
// playableLevels_[i] corresponds to level number (i + 1).
static constexpr int MAX_LEVELS = static_cast<int>(Level::None);

// ==========================================
//            WIN SCREEN
// ==========================================

// =========================================================
//
// WinScreen::load()
//
// - Stores the font handle.
// - Creates the full-screen semi-transparent black background mesh.
// - Loads GPU meshes and textures for all three navigation buttons.
// - Reads all text data from the win_screen_text JSON config.
// - Stores the collectibles format string for later dynamic substitution.
//
// =========================================================
void WinScreen::load(s8 font) {
    font_ = font;

    graphics_.mesh_ = createRectMesh();
    graphics_.red_ = 0.0f;
    graphics_.green_ = 0.0f;
    graphics_.blue_ = 0.0f;
    graphics_.alpha_ = 0.7f;

    nextLevelButton_.loadMesh();
    nextLevelButton_.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    restartButton_.loadMesh();
    restartButton_.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    mainMenuButton_.loadMesh();
    mainMenuButton_.loadTexture("Assets/Textures/brown_rectangle_80_24.png");

    titleText_.initFromJson("win_screen_text", "Title");
    titleText_.font_ = font_;

    collectiblesText_.initFromJson("win_screen_text", "Collectibles");
    collectiblesText_.font_ = font_;
    collectiblesFormat_ = collectiblesText_.content_;

    statsPerfectText_.initFromJson("win_screen_text", "StatsPerfect");
    statsPerfectText_.font_ = font_;

    statsPartialText_.initFromJson("win_screen_text", "StatsPartial");
    statsPartialText_.font_ = font_;

    noNextLevelText_.initFromJson("win_screen_text", "NoNextLevel");
    noNextLevelText_.font_ = font_;

    isVisible_ = false;
}

// =========================================================
//
// WinScreen::show()
//
// - Stores the collected and total collectible counts.
// - Determines the current level, falling back to the manager if zero.
// - Checks whether a playable next level exists by looking up
//   levelManager.playableLevels_ at index (nextLevel_ - 1).
// - Formats the collectibles text string with the actual counts.
// - Initialises button transforms from JSON; setTextFont must be called
//   before setText so that updateTransform() can centre the label.
// - Greys out the Next Level button when no next level is available.
// - Recalculates all button world matrices.
// - Marks the win screen as visible and plays the victory sound.
//
// =========================================================
void WinScreen::show(int collected, int total, int currentLevel) {
    collectiblesCollected_ = collected;
    totalCollectibles_ = total;

    currentLevel_ = (currentLevel > 0) ? currentLevel : levelManager.getCurrentLevel();
    nextLevel_ = currentLevel_ + 1;

    int nextIdx = nextLevel_ - 1;
    hasNextLevel_ = (nextIdx >= 0 && nextIdx < MAX_LEVELS && levelManager.playableLevels_[nextIdx]);

    char buffer[64];
    snprintf(buffer, sizeof(buffer), collectiblesFormat_.c_str(), collected, total);
    collectiblesText_.content_ = buffer;

    restartButton_.initFromJson("win_screen_buttons", "Restart");
    restartButton_.setTextFont(font_);
    mainMenuButton_.initFromJson("win_screen_buttons", "MainMenu");
    mainMenuButton_.setTextFont(font_);
    nextLevelButton_.initFromJson("win_screen_buttons", "Next");
    nextLevelButton_.setTextFont(font_);

    nextLevelButton_.setRGBA(hasNextLevel_ ? 1.0f : 0.4f, hasNextLevel_ ? 1.0f : 0.4f,
                             hasNextLevel_ ? 1.0f : 0.4f, 1.0f);

    nextLevelButton_.updateTransform();
    restartButton_.updateTransform();
    mainMenuButton_.updateTransform();

    isVisible_ = true;
    g_audioSystem.playSound("magic", "sfx", 0.35f, 1.0f);
}

// =========================================================
//
// WinScreen::hide()
//
// - Sets isVisible_ to false to stop drawing and updating.
//
// =========================================================
void WinScreen::hide() { isVisible_ = false; }

// =========================================================
//
// WinScreen::update()
//
// - Returns early if the win screen is not visible.
// - Keeps the overlay anchored to the full screen by computing the
//   world position from the current window origin and dimensions.
// - Rebuilds the background panel world matrix each frame.
// - Handles Next Level button click: sets the next level on the manager
//   and alternates the target state between Level and NextLevel so the
//   GSM always detects a state change and triggers a full reload.
// - Handles Restart button click: transitions to StateId::Restart.
// - Handles Main Menu button click: transitions to StateId::MainMenu.
//
// =========================================================
void WinScreen::update(GameStateManager& GSM) {
    if (!isVisible_)
        return;

    s32 windowWidth = AEGfxGetWindowWidth();
    s32 windowHeight = AEGfxGetWindowHeight();
    f32 worldMinX = AEGfxGetWinMinX();
    f32 worldMinY = AEGfxGetWinMinY();

    transform_.pos_ = {worldMinX + static_cast<f32>(windowWidth) / 2.0f,
                       worldMinY + static_cast<f32>(windowHeight) / 2.0f};
    transform_.scale_ = {static_cast<f32>(windowWidth), static_cast<f32>(windowHeight)};
    transform_.rotationRad_ = 0.0f;

    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);

    if (nextLevelButton_.checkMouseClick()) {
        if (hasNextLevel_) {
            levelManager.setCurrentLevel(nextLevel_);
            GSM.nextState_ =
                (GSM.currentState_ == StateId::NextLevel) ? StateId::Level : StateId::NextLevel;
            hide();
        }
    }

    if (restartButton_.checkMouseClick()) {
        GSM.nextState_ = StateId::Restart;
        hide();
    }

    if (mainMenuButton_.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
        hide();
    }
}

// =========================================================
//
// WinScreen::draw()
//
// - Returns early if the win screen is not visible.
// - Draws the semi-transparent black background panel.
// - Centers and draws the victory title text.
// - Centers and draws the collectibles count text.
// - Draws the perfect or partial stats text depending on the result.
// - Draws all three navigation buttons.
// - Draws the "no next level" hint text when the Next Level button
//   is greyed out.
//
// =========================================================
void WinScreen::draw() {
    if (!isVisible_)
        return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(graphics_.red_, graphics_.green_, graphics_.blue_, graphics_.alpha_);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    {
        f32 tw = 0.f, th = 0.f;
        AEGfxGetPrintSize(font_, titleText_.content_.c_str(), titleText_.scale_, &tw, &th);
        titleText_.x_ = -tw / 2.0f;
    }
    titleText_.draw();

    {
        f32 tw = 0.f, th = 0.f;
        AEGfxGetPrintSize(font_, collectiblesText_.content_.c_str(), collectiblesText_.scale_, &tw,
                          &th);
        collectiblesText_.x_ = -tw / 2.0f;
    }
    collectiblesText_.draw();

    if (collectiblesCollected_ == totalCollectibles_) {
        statsPerfectText_.draw(true);
    } else {
        TextData temp = statsPartialText_;
        char buffer[64];
        snprintf(buffer, sizeof(buffer), statsPartialText_.content_.c_str(), collectiblesCollected_,
                 totalCollectibles_);
        temp.content_ = buffer;
        temp.draw(true);
    }

    nextLevelButton_.draw();
    restartButton_.draw();
    mainMenuButton_.draw();

    if (!hasNextLevel_) {
        noNextLevelText_.draw(true);
    }
}

// =========================================================
//
// WinScreen::free()
//
// - Resets visibility and clears collectible counts.
// - Called in Level::free().
//
// =========================================================
void WinScreen::free() {
    isVisible_ = false;
    collectiblesCollected_ = 0;
    totalCollectibles_ = 0;
}

// =========================================================
//
// WinScreen::unload()
//
// - Unloads all three navigation button GPU resources.
// - Frees the background panel mesh.
// - Resets visibility and nulls the font handle.
// - Note: the font is owned by Level.cpp and must not be destroyed
//   here to avoid a double-free on subsequent level loads.
//
// =========================================================
void WinScreen::unload() {
    nextLevelButton_.unload();
    restartButton_.unload();
    mainMenuButton_.unload();

    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }

    isVisible_ = false;
    font_ = 0;
}