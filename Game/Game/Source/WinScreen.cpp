#include "WinScreen.h"

#include <cstdio>

#include "States/LevelManager.h"
#include "Utils.h"

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
void WinScreen::Load(s8 font) {
    font_ = font;

    // Background panel mesh (full-screen semi-transparent black)
    graphics_.mesh_ = CreateRectMesh();
    graphics_.red_ = 0.0f;
    graphics_.green_ = 0.0f;
    graphics_.blue_ = 0.0f;
    graphics_.alpha_ = 0.7f;

    // Load button GPU resources
    nextLevelButton_.loadMesh();
    nextLevelButton_.loadTexture("Assets/Textures/brown_button.png");

    restartButton_.loadMesh();
    restartButton_.loadTexture("Assets/Textures/brown_button.png");

    mainMenuButton_.loadMesh();
    mainMenuButton_.loadTexture("Assets/Textures/brown_button.png");

    // Static text -- x/y are 0 because updateTransform() will auto-center them
    titleText_ = TextData{"VICTORY!", 0.f, 0.3f, 1.5f, 1.0f, 1.0f, 0.0f, 1.0f};
    titleText_.font_ = font_;
    collectiblesText_ = TextData{"", 0.f, 0.1f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f};
    collectiblesText_.font_ = font_;
    statsText_ = TextData{"", 0.f, -0.1f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f};
    statsText_.font_ = font_;

    isVisible_ = false;
}

// ----------------------------------------------------------------------------
// WinScreen::Show
// Call when the player completes a level.
//
// currentLevel -- the 1-based level number that was just completed.
//                 Pass levelManager.getCurrentLevel() from the Level state.
// ----------------------------------------------------------------------------
void WinScreen::Show(int collected, int total, int currentLevel) {
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
    snprintf(buffer, sizeof(buffer), "Collectibles: %d/%d", collected, total);
    collectiblesText_.content_ = buffer;

    // ------------------------------------------------------------------
    // Button setup
    // IMPORTANT: always call setTextFont BEFORE setText.
    // updateTransform() only auto-centers text when font_ != 0.
    // If setText is called first, font_ is still 0 and centering is skipped.
    // ------------------------------------------------------------------

    restartButton_.setTransform({-200.0f, -140.0f}, {160.0f, 70.0f});
    restartButton_.setTextFont(font_);
    restartButton_.setText("Restart", 0.f, 0.f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

    mainMenuButton_.setTransform({200.0f, -140.0f}, {160.0f, 70.0f});
    mainMenuButton_.setTextFont(font_);
    mainMenuButton_.setText("Main Menu", 0.f, 0.f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

    nextLevelButton_.setTransform({0.0f, -140.0f}, {160.0f, 70.0f});
    nextLevelButton_.setTextFont(font_);

    if (hasNextLevel_) {
        // Active state: normal button color, white text
        nextLevelButton_.setRGBA(1.0f, 1.0f, 1.0f, 1.0f);
        nextLevelButton_.setText("Next Level", 0.f, 0.f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        // Greyed-out state: dark tint on button, grey text
        nextLevelButton_.setRGBA(0.4f, 0.4f, 0.4f, 1.0f);
        nextLevelButton_.setText("Next Level", 0.f, 0.f, 0.6f, 0.4f, 0.4f, 0.4f, 1.0f);
    }

    // Recalculate world matrices -- this is where text gets auto-centered
    nextLevelButton_.updateTransform();
    restartButton_.updateTransform();
    mainMenuButton_.updateTransform();

    isVisible_ = true;
}

// ----------------------------------------------------------------------------
// WinScreen::Hide
// ----------------------------------------------------------------------------
void WinScreen::Hide() { isVisible_ = false; }

// ----------------------------------------------------------------------------
// WinScreen::Update
// Call every frame while the level state is active.
// ----------------------------------------------------------------------------
void WinScreen::Update(GameStateManager& GSM) {
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
            levelManager.SetCurrentLevel(nextLevel_); // tell the manager which level to load
            GSM.nextState_ = StateId::NextLevel;      // NextLevel != Level so the loop exits
            Hide();
        }
        // else: click silently ignored -- button is visually greyed out
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

// ----------------------------------------------------------------------------
// WinScreen::Draw
// ----------------------------------------------------------------------------
void WinScreen::Draw() {
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
        statsText_.content_ = "Perfect! All collectibles found!";
        statsText_.scale_ = 0.7f;
    } else {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "You found %d of %d collectibles", collectiblesCollected_,
                 totalCollectibles_);
        statsText_.content_ = buffer;
        statsText_.scale_ = 0.8f;
    }
    {
        f32 tw = 0.f, th = 0.f;
        AEGfxGetPrintSize(font_, statsText_.content_.c_str(), statsText_.scale_, &tw, &th);
        statsText_.x_ = -tw / 2.0f;
    }
    statsText_.draw();

    // Buttons
    nextLevelButton_.draw();
    restartButton_.draw();
    mainMenuButton_.draw();

    // Small hint below the Next Level button when it is greyed out
    if (!hasNextLevel_) {
        const char* hint = "No more levels!";
        f32 tw = 0.0f, th = 0.0f;
        AEGfxGetPrintSize(font_, hint, 0.45f, &tw, &th);
        AEGfxPrint(font_, hint, -tw / 2.0f, -0.47f, 0.45f, 0.5f, 0.5f, 0.5f, 1.0f);
    }
}

// ----------------------------------------------------------------------------
// WinScreen::Free  -- call in Level::Free()
// ----------------------------------------------------------------------------
void WinScreen::Free() {
    isVisible_ = false;
    collectiblesCollected_ = 0;
    totalCollectibles_ = 0;
    printf("WinScreen: Freed\n");
}

// ----------------------------------------------------------------------------
// WinScreen::Unload  -- call in Level::Unload()
// ----------------------------------------------------------------------------
void WinScreen::Unload() {
    nextLevelButton_.unload();
    restartButton_.unload();
    mainMenuButton_.unload();

    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }

    isVisible_ = false;
    AEGfxDestroyFont(font_);
    printf("WinScreen: Unloaded\n");
}