/*!
@file       Pause.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Pause.h"

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
#include "MeshUtils.h"

void PauseSystem::pause() { pause_ = true; }

void PauseSystem::resume() { pause_ = false; }

bool PauseSystem::isPaused() const { return pause_; }

void PauseSystem::renderBackground() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(graphics_.red_, graphics_.green_, graphics_.blue_, graphics_.alpha_);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
}

void PauseSystem::setTransformFillScreen() {
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

void PauseSystem::updateTransform() {
    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);
}

void PauseSystem::loadMesh() { graphics_.mesh_ = createRectMesh(); }

void PauseSystem::initFromJson(const std::string& file, const std::string& section) {
    const Json::Value& pauseSection = g_configManager.getSection(file, section);
    graphics_.red_ = pauseSection["graphics"]["red"].asFloat();
    graphics_.green_ = pauseSection["graphics"]["green"].asFloat();
    graphics_.blue_ = pauseSection["graphics"]["blue"].asFloat();
    graphics_.alpha_ = pauseSection["graphics"]["alpha"].asFloat();
}

void PauseSystem::unload() {
    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }
}
