/*!
@file       Confirmation.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the definition of functions that make
            ConfirmationSystem, a Yes/No dialog used to confirm destructive
            actions such as restart, quit, and delete.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Confirmation.h"

// ==========================================
// Standard library
// ==========================================
#include <fstream>
#include <iostream>
#include <string>

// ==========================================
// Third-party
// ==========================================
#include <AEEngine.h>
#include <json/json.h>

// ==========================================
// Project
// ==========================================
#include "Components.h"
#include "ConfigManager.h"
#include "MeshUtils.h"

// =========================================================
//
// ConfirmationSystem::show()
//
// - Makes the dialog visible.
// - Sets justShown_ so input is ignored for one frame,
// - preventing the click that opened the dialog from taking input
// - on the Yes/No buttons.
//
// =========================================================
void ConfirmationSystem::show() {
    show_ = true;
    justShown_ = true;
}

// =========================================================
//
// ConfirmationSystem::hide()
//
// - Hides the dialog.
//
// =========================================================
void ConfirmationSystem::hide() { show_ = false; }

// =========================================================
//
// ConfirmationSystem::isShowing()
//
// - Returns the visibility state of the confirmation dialog.
//
// =========================================================
bool ConfirmationSystem::isShowing() const { return show_; }

// =========================================================
//
// ConfirmationSystem::load()
//
// - Loads all GPU assets needed by the dialog:
// - the background quad mesh and button meshes/textures.
// - Called once per session (not on restart).
//
// =========================================================
void ConfirmationSystem::load() {
    graphics_.mesh_ = createRectMesh();

    buttonYes_.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    buttonYes_.loadMesh();
    buttonNo_.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    buttonNo_.loadMesh();
}

// =========================================================
//
// ConfirmationSystem::init()
//
// - Reads background color from JSON, fills the screen with the background,
// - initializes button and text layouts from JSON, and hides the dialog.
// - Called on both first load and every restart.
//
// =========================================================
void ConfirmationSystem::init(s8& buttonFont) {
    const Json::Value& confirmationSection =
        g_configManager.getSection("confirmation_system", "Background");
    graphics_.red_ = confirmationSection["graphics"]["red"].asFloat();
    graphics_.green_ = confirmationSection["graphics"]["green"].asFloat();
    graphics_.blue_ = confirmationSection["graphics"]["blue"].asFloat();
    graphics_.alpha_ = confirmationSection["graphics"]["alpha"].asFloat();

    setTransformFillScreen();
    updateTransform();

    // Initialize buttons and text from JSON
    buttonYes_.initFromJson("confirmation_system", "ButtonYes");
    buttonYes_.setTextFont(buttonFont);
    buttonNo_.initFromJson("confirmation_system", "ButtonNo");
    buttonNo_.setTextFont(buttonFont);
    confirmationText_.initFromJson("confirmation_system", "ConfirmationText");
    confirmationText_.font_ = buttonFont;

    buttonYes_.updateTransform();
    buttonNo_.updateTransform();

    // make sure to hide confirmation system at start
    hide();
}

// =========================================================
//
// ConfirmationSystem::update()
//
// - Clears the justShown_ flag after one frame so clicks become valid.
// - Also refreshes the world transform in case the window resized.
//
// =========================================================
void ConfirmationSystem::update() {
    if (justShown_) {
        justShown_ = false;
    }
    updateTransform();
}

// =========================================================
//
// ConfirmationSystem::draw()
//
// - Renders the background, confirmation text, and Yes/No buttons.
// - Early-returns if the dialog is not currently showing.
//
// =========================================================
void ConfirmationSystem::draw() {
    if (!show_)
        return;

    renderBackground();
    confirmationText_.draw(true);
    buttonYes_.draw();
    buttonNo_.draw();
}

// =========================================================
//
// ConfirmationSystem::renderBackground()
//
// - Draws the solid-color background quad using the current
// - transform and RGBA color values from graphics_.
//
// =========================================================
void ConfirmationSystem::renderBackground() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(graphics_.red_, graphics_.green_, graphics_.blue_, graphics_.alpha_);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
}

// =========================================================
//
// ConfirmationSystem::setTransformFillScreen()
//
// - Positions and scales the background quad to cover the entire screen.
// - Uses the AEEngine window size and world-space min coordinates.
//
// =========================================================
void ConfirmationSystem::setTransformFillScreen() {
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
}

// =========================================================
//
// ConfirmationSystem::updateTransform()
//
// - Rebuilds the world matrix from the current scale, rotation, and position.
// - Must be called after any transform change for it to take effect in draw.
//
// =========================================================

void ConfirmationSystem::updateTransform() {
    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);
}

// =========================================================
//
// ConfirmationSystem::initFromJson()
//
// - Reads the background RGBA color from file -> section in the config JSON.
// - Used to recolor the background for different contexts if needed.
//
// =========================================================
void ConfirmationSystem::initFromJson(const std::string& file, const std::string& section) {
    const Json::Value& pauseSection = g_configManager.getSection(file, section);
    graphics_.red_ = pauseSection["graphics"]["red"].asFloat();
    graphics_.green_ = pauseSection["graphics"]["green"].asFloat();
    graphics_.blue_ = pauseSection["graphics"]["blue"].asFloat();
    graphics_.alpha_ = pauseSection["graphics"]["alpha"].asFloat();
}

// =========================================================
//
// ConfirmationSystem::unload()
//
// - Frees the background quad mesh and unloads button GPU assets.
// - Called once when leaving the state.
//
// =========================================================
void ConfirmationSystem::unload() {
    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }

    buttonYes_.unload();
    buttonNo_.unload();
}

// =========================================================
//
// ConfirmationSystem::confirmationYesClicked()
//
// - Returns true if Yes was clicked this frame.
// - Returns false if the dialog is hidden or was just shown this frame,
// - preventing the opening click from being treated as a confirmation.
//
// =========================================================
bool ConfirmationSystem::confirmationYesClicked() {
    if (!show_ || justShown_)
        return false;
    return buttonYes_.checkMouseClick();
}

// =========================================================
//
// ConfirmationSystem::confirmationNoClicked()
//
// - Returns true if No was clicked this frame.
// - Same one-frame guard as confirmationYesClicked.
//
// =========================================================
bool ConfirmationSystem::confirmationNoClicked() {
    if (!show_ || justShown_)
        return false;
    return buttonNo_.checkMouseClick();
}

// =========================================================
//
// ConfirmationSystem::setTask()
//
// - Sets the action this dialog is currently guarding.
// - Call this before show() so the caller knows what to do on Yes.
//
// =========================================================
void ConfirmationSystem::setTask(ConfirmationTask task) { task_ = task; }

// =========================================================
//
// ConfirmationSystem::getTask()
//
// - Returns the currently assigned task.
//
// =========================================================
ConfirmationTask ConfirmationSystem::getTask() const { return task_; }
