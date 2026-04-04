/*!
@file       Confirmation.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Confirmation.h"

// Standard library
#include <fstream>
#include <iostream>
#include <string>

// Third-party
#include <AEEngine.h>
#include <json/json.h>

// Project
#include "Components.h"
#include "ConfigManager.h"
#include "Utils.h"

void ConfirmationSystem::show() {
    show_ = true;
    justShown_ = true;
}

void ConfirmationSystem::hide() { show_ = false; }

bool ConfirmationSystem::isShowing() const { return show_; }

void ConfirmationSystem::load() {
    graphics_.mesh_ = createRectMesh();

    buttonYes_.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    buttonYes_.loadMesh();
    buttonNo_.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    buttonNo_.loadMesh();
}

void ConfirmationSystem::init(s8& buttonFont) {
    const Json::Value& confirmationSection =
        g_configManager.getSection("confirmation_system", "Background");
    graphics_.red_ = confirmationSection["graphics"]["red"].asFloat();
    graphics_.green_ = confirmationSection["graphics"]["green"].asFloat();
    graphics_.blue_ = confirmationSection["graphics"]["blue"].asFloat();
    graphics_.alpha_ = confirmationSection["graphics"]["alpha"].asFloat();

    setTransformFillScreen();
    updateTransform();

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

void ConfirmationSystem::update() {
    if (justShown_) {
        justShown_ = false;
    }
    updateTransform();
}

void ConfirmationSystem::draw() {
    if (!show_)
        return;

    renderBackground();
    confirmationText_.draw(true);
    buttonYes_.draw();
    buttonNo_.draw();
}

void ConfirmationSystem::renderBackground() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(graphics_.red_, graphics_.green_, graphics_.blue_, graphics_.alpha_);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
}

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

void ConfirmationSystem::updateTransform() {
    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);
}

void ConfirmationSystem::initFromJson(const std::string& file, const std::string& section) {
    const Json::Value& pauseSection = g_configManager.getSection(file, section);
    graphics_.red_ = pauseSection["graphics"]["red"].asFloat();
    graphics_.green_ = pauseSection["graphics"]["green"].asFloat();
    graphics_.blue_ = pauseSection["graphics"]["blue"].asFloat();
    graphics_.alpha_ = pauseSection["graphics"]["alpha"].asFloat();
}

void ConfirmationSystem::unload() {
    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }

    buttonYes_.unload();
    buttonNo_.unload();
}

bool ConfirmationSystem::confirmationYesClicked() {
    if (!show_ || justShown_)
        return false;
    return buttonYes_.checkMouseClick();
}

bool ConfirmationSystem::confirmationNoClicked() {
    if (!show_ || justShown_)
        return false;
    return buttonNo_.checkMouseClick();
}

void ConfirmationSystem::setTask(ConfirmationTask task) { task_ = task; }

ConfirmationTask ConfirmationSystem::getTask() const { return task_; }
