/*!
@file       WinScreen.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "WinScreen.h"

// Standard library
#include <cstdio>

// Project
#include "AudioSystem.h"
#include "LevelManager.h"

// ----------------------------------------------------------------------------
// MAX_LEVELS
// Level::None is the sentinel that ends the regular level list.
// static_cast<int>(Level::None) gives the total count (12).
// playableLevels_[i] corresponds to level number (i + 1).
//   e.g. playableLevels_[0] = Level 1
//        playableLevels_[1] = Level 2  ...
// ----------------------------------------------------------------------------
static constexpr int MAX_LEVELS = static_cast<int>(Level::None);

// ----------------------------------------------------------------------------
// WinScreen::Load
// Call once when the level state loads.
// ----------------------------------------------------------------------------
void WinScreen::load(s8 font) {
    font_ = font;

    // Background panel mesh (full-screen semi-transparent black)
    graphics_.mesh_ = createRectMesh();
    graphics_.red_ = 0.0f;
    graphics_.green_ = 0.0f;
    graphics_.blue_ = 0.0f;
    graphics_.alpha_ = 0.7f;

    // Load button GPU resources
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

// ----------------------------------------------------------------------------
// WinScreen::Show
// Call when the player completes a level.
//
// currentLevel -- the 1-based level number that was just completed.
//                 Pass levelManager.getCurrentLevel() from the Level state.
// ----------------------------------------------------------------------------
void WinScreen::show(int collected, int total, int currentLevel) {
    collectiblesCollected_ = collected;
    totalCollectibles_ = total;

    // If currentLevel is 0 (uninitialised), fall back to the manager's value
    currentLevel_ = (currentLevel > 0) ? currentLevel : levelManager.getCurrentLevel();
    nextLevel_ = currentLevel_ + 1;

    // ------------------------------------------------------------------
    // Determine whether a playable next level exists.
    //
    // playableLevels_ is 0-indexed: playableLevels_[0] = Level 1, etc.
    // nextLevel_ is 1-based, so the array index is (nextLevel_ - 1).
    //
    // Examples:
    //   currentLevel_ = 1  ->  nextLevel_ = 2  ->  index 1  ->  Level 2
    //   currentLevel_ = 12 ->  nextLevel_ = 13 ->  index 12 ->  out of range -> false
    // ------------------------------------------------------------------
    int nextIdx = nextLevel_ - 1;
    hasNextLevel_ = (nextIdx >= 0 && nextIdx < MAX_LEVELS && levelManager.playableLevels_[nextIdx]);

    printf("WinScreen: currentLevel=%d  nextLevel=%d  hasNextLevel=%s\n", currentLevel_, nextLevel_,
           hasNextLevel_ ? "true" : "false");

    // Update dynamic text
    char buffer[64];
    snprintf(buffer, sizeof(buffer), collectiblesFormat_.c_str(), collected, total);
    collectiblesText_.content_ = buffer;

    // ------------------------------------------------------------------
    // Button setup
    // IMPORTANT: always call setTextFont BEFORE setText.
    // updateTransform() only auto-centers text when font_ != 0.
    // If setText is called first, font_ is still 0 and centering is skipped.
    // ------------------------------------------------------------------

    restartButton_.initFromJson("win_screen_buttons", "Restart");
    restartButton_.setTextFont(font_);

    mainMenuButton_.initFromJson("win_screen_buttons", "MainMenu");
    mainMenuButton_.setTextFont(font_);

    nextLevelButton_.initFromJson("win_screen_buttons", "Next");
    nextLevelButton_.setTextFont(font_);

    if (hasNextLevel_) {
        // Active state: normal button color
        nextLevelButton_.setRGBA(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        // Greyed-out state: dark tint on button, grey text
        nextLevelButton_.setRGBA(0.4f, 0.4f, 0.4f, 1.0f);
    }

    // Recalculate world matrices -- this is where text gets auto-centered
    nextLevelButton_.updateTransform();
    restartButton_.updateTransform();
    mainMenuButton_.updateTransform();

    isVisible_ = true;
    g_audioSystem.playSound("magic", "sfx", 0.35f, 1.0f);
}

// ----------------------------------------------------------------------------
// WinScreen::Hide
// ----------------------------------------------------------------------------
void WinScreen::hide() { isVisible_ = false; }

// ----------------------------------------------------------------------------
// WinScreen::Update
// Call every frame while the level state is active.
// ----------------------------------------------------------------------------
void WinScreen::update(GameStateManager& GSM) {
    if (!isVisible_)
        return;

    // Keep the overlay anchored to the full screen even if the camera moves
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

    // ------------------------------------------------------------------
    // Next Level button
    // Only functional when hasNextLevel_ is true.
    // When greyed out, clicks are registered by checkMouseClick() but
    // we deliberately ignore them so nothing happens.
    // ------------------------------------------------------------------
    if (nextLevelButton_.checkMouseClick()) {
        if (hasNextLevel_) {
            printf("WinScreen: Loading level %d\n", nextLevel_);
            levelManager.setCurrentLevel(nextLevel_);
            // The GSM loop only triggers a full reload when nextState_ != currentState_.
            // If we always use NextLevel, the second next-level click finds
            // currentState_ == NextLevel == nextState_ and does nothing.
            // If we always use Level, same problem coming from StateId::Level.
            // Solution: alternate - pick whichever ID differs from currentState_.
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

// ----------------------------------------------------------------------------
// WinScreen::Draw
// ----------------------------------------------------------------------------
void WinScreen::draw() {
    if (!isVisible_)
        return;

    // Semi-transparent black overlay covering the whole screen
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(graphics_.red_, graphics_.green_, graphics_.blue_, graphics_.alpha_);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);

    // ----------------------------------------------------------------
    // Text -- auto-centered using AEGfxGetPrintSize.
    // AEGfxPrint draws from the bottom-left corner, so x must be
    // set to -(textWidth / 2) to visually center on screen.
    // ----------------------------------------------------------------
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // Center VICTORY! title
    {
        f32 tw = 0.f, th = 0.f;
        AEGfxGetPrintSize(font_, titleText_.content_.c_str(), titleText_.scale_, &tw, &th);
        titleText_.x_ = -tw / 2.0f;
    }
    titleText_.draw();

    // Center collectibles text
    {
        f32 tw = 0.f, th = 0.f;
        AEGfxGetPrintSize(font_, collectiblesText_.content_.c_str(), collectiblesText_.scale_, &tw,
                          &th);
        collectiblesText_.x_ = -tw / 2.0f;
    }
    collectiblesText_.draw();

    // Build and center stats text
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

    // Buttons
    nextLevelButton_.draw();
    restartButton_.draw();
    mainMenuButton_.draw();

    // Small hint below the Next Level button when it is greyed out
    if (!hasNextLevel_) {
        noNextLevelText_.draw(true);
    }
}

// ----------------------------------------------------------------------------
// WinScreen::Free  -- call in Level::free()
// ----------------------------------------------------------------------------
void WinScreen::free() {
    isVisible_ = false;
    collectiblesCollected_ = 0;
    totalCollectibles_ = 0;
    printf("WinScreen: Freed\n");
}

// ----------------------------------------------------------------------------
// WinScreen::Unload  -- call in Level::unload()
// ----------------------------------------------------------------------------
void WinScreen::unload() {
    nextLevelButton_.unload();
    restartButton_.unload();
    mainMenuButton_.unload();

    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }

    isVisible_ = false;
    // NOTE: Do NOT destroy font_ here. The font is owned by Level.cpp and
    // destroyed in UnloadLevel(). Destroying it here causes a double-free
    // on the second level load, breaking the win screen on subsequent use.
    font_ = 0;
    printf("WinScreen: Unloaded\n");
}