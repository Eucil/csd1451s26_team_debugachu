#pragma once

#include "Button.h"

#include <fstream>
#include <iostream>
#include <string>

#include <AEEngine.h>
#include <json/json.h>

#include "Components.h"
#include "ConfigManager.h"
#include "Utils.h"

void NewButton::loadTexture(const char* path) { graphics_.texture_ = AEGfxTextureLoad(path); }

void NewButton::loadMesh() { graphics_.mesh_ = CreateRectMesh(); }

// Usage example:
// obj.initFromJson("level_buttons", "buttons", "Restart");
void NewButton::initFromJson(const std::string& file, const std::string& section) {
    const Json::Value& buttonSection = configManager.getSection(file, section);
    transform_.pos_.x = buttonSection["position"]["x"].asFloat();
    transform_.pos_.y = buttonSection["position"]["y"].asFloat();
    transform_.scale_.x = buttonSection["scale"]["x"].asFloat();
    transform_.scale_.y = buttonSection["scale"]["y"].asFloat();

    const Json::Value& t = buttonSection["text"];
    text_.content_ = t["content"].asString();
    text_.x_ = t["x"].asFloat();
    text_.y_ = t["y"].asFloat();
    text_.scale_ = t["scale"].asFloat();
    text_.r_ = t["red"].asFloat();
    text_.g_ = t["green"].asFloat();
    text_.b_ = t["blue"].asFloat();
    text_.a_ = t["alpha"].asFloat();
}

void NewButton::updateTransform() {
    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);
}

void NewButton::draw(s8 font) {
    // Render button (no text)
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxTextureSet(graphics_.texture_, 0.0f, 0.0f);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);

    // Render text
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxPrint(font, text_.content_.c_str(), text_.x_, text_.y_, text_.scale_, text_.r_, text_.g_,
               text_.b_, text_.a_);
}

void NewButton::unload() {
    if (graphics_.texture_ != nullptr) {
        AEGfxTextureUnload(graphics_.texture_);
        graphics_.texture_ = nullptr;
    }
    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }
}

bool NewButton::checkMouseClick() const {
    // Get mouse position
    s32 mouseX{0};
    s32 mouseY{0};

    AEInputGetCursorPosition(&mouseX, &mouseY);
    mouseX -= AEGfxGetWindowWidth() / 2;
    mouseY = (AEGfxGetWindowHeight() / 2) - mouseY;

    // Check if left mouse click
    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        // Check by checking if mouse pos falls within the button's collider box
        f32 rect_half_width{transform_.scale_.x / 2.0f};
        f32 rect_half_height{transform_.scale_.y / 2.0f};
        if (mouseX >= (transform_.pos_.x - rect_half_width) &&
            mouseX <= (transform_.pos_.x + rect_half_width) &&
            mouseY >= (transform_.pos_.y - rect_half_height) &&
            mouseY <= (transform_.pos_.y + rect_half_height)) {
            return true;
        }
    }

    return false;
}