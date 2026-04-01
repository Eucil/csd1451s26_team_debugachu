/*!
@file       Button.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#include "Button.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <AEEngine.h>
#include <json/json.h>

#include "AudioSystem.h"
#include "Components.h"
#include "ConfigManager.h"
#include "Utils.h"

void Button::loadTexture(const char* path) { graphics_.texture_ = AEGfxTextureLoad(path); }

void Button::loadMesh() { graphics_.mesh_ = CreateRectMesh(); }

// Usage example:
// obj.initFromJson("level_buttons", "buttons", "Restart");
void Button::initFromJson(const std::string& file, const std::string& section) {
    const Json::Value& buttonSection = g_configManager.getSection(file, section);
    transform_.pos_.x = buttonSection["position"]["x"].asFloat();
    transform_.pos_.y = buttonSection["position"]["y"].asFloat();
    transform_.scale_.x = buttonSection["scale"]["x"].asFloat();
    transform_.scale_.y = buttonSection["scale"]["y"].asFloat();

    const Json::Value& t = buttonSection["text"];
    text_.content_ = t["content"].asString();
    text_.scale_ = t["scale"].asFloat();
    text_.r_ = t["red"].asFloat();
    text_.g_ = t["green"].asFloat();
    text_.b_ = t["blue"].asFloat();
    text_.a_ = t["alpha"].asFloat();
}

void Button::setTransform(const AEVec2& pos, const AEVec2& scale, f32 rotationRad) {
    transform_.pos_ = pos;
    transform_.scale_ = scale;
    transform_.rotationRad_ = rotationRad;
}

void Button::setTextFont(s8 font) { text_.font_ = font; }

Transform Button::getTransform() const { return transform_; }

void Button::setText(const std::string& content, const f32& x, const f32& y, const f32& scale,
                     const f32& r, const f32& g, const f32& b, const f32& a) {
    text_.content_ = content;
    text_.x_ = x;
    text_.y_ = y;
    text_.scale_ = scale;
    text_.r_ = r;
    text_.g_ = g;
    text_.b_ = b;
    text_.a_ = a;
}

void Button::setRGBA(const f32& r, const f32& g, const f32& b, const f32& a) {
    graphics_.red_ = r;
    graphics_.green_ = g;
    graphics_.blue_ = b;
    graphics_.alpha_ = a;
}

void Button::updateTransform() {
    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);

    // Center text within the button (text coords are normalized -1 to +1, bottom-left anchored).
    if (text_.font_ != 0) {
        f32 centerX = transform_.pos_.x / (AEGfxGetWindowWidth() / 2.0f);
        f32 centerY = transform_.pos_.y / (AEGfxGetWindowHeight() / 2.0f);
        f32 textWidth, textHeight;
        AEGfxGetPrintSize(text_.font_, text_.content_.c_str(), text_.scale_, &textWidth,
                          &textHeight);
        text_.x_ = centerX - textWidth / 2.0f;
        text_.y_ = centerY - textHeight / 2.0f;
    }
}

void Button::draw(bool hoverEffect) {
    // SAFETY CHECK: Make sure mesh exists
    if (graphics_.mesh_ == nullptr) {
        printf("ERROR: Button mesh is null! Content: %s\n", text_.content_.c_str());
        return;
    }

    f32 drawR = graphics_.red_;
    f32 drawG = graphics_.green_;
    f32 drawB = graphics_.blue_;
    if (hoverEffect) {
        const f32 kDimmed = 1.30f;
        bool hovered = isHovered();
        if (hovered && !wasHovered_) {
            g_audioSystem.playSound("hover", "sfx", 0.3f, 1.0f);
        }
        wasHovered_ = hovered;
        f32 tint = hovered ? 1.0f : kDimmed;
        drawR *= tint;
        drawG *= tint;
        drawB *= tint;
    }

    if (graphics_.texture_ == nullptr) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(drawR, drawG, drawB, graphics_.alpha_);
        AEGfxSetTransform(transform_.worldMtx_.m);
        AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
    } else {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(drawR, drawG, drawB, graphics_.alpha_);
        AEGfxSetTransform(transform_.worldMtx_.m);
        AEGfxTextureSet(graphics_.texture_, 0.0f, 0.0f);
        AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
    }

    // Render text at full brightness regardless of hover state
    if (text_.font_ != 0) {
        text_.draw();
    }
}

void Button::unload() {
    if (graphics_.texture_ != nullptr) {
        AEGfxTextureUnload(graphics_.texture_);
        graphics_.texture_ = nullptr;
    }
    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }
}

bool Button::checkMouseClick() const {
    // Get mouse position
    s32 mouseX{0};
    s32 mouseY{0};

    AEInputGetCursorPosition(&mouseX, &mouseY);
    mouseX -= AEGfxGetWindowWidth() / 2;
    mouseY = (AEGfxGetWindowHeight() / 2) - mouseY;

    // Check if left mouse click
    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        // Check by checking if mouse pos falls within the button's collider box
        f32 rectHalfWidth{transform_.scale_.x / 2.0f};
        f32 rectHalfHeight{transform_.scale_.y / 2.0f};
        if (mouseX >= (transform_.pos_.x - rectHalfWidth) &&
            mouseX <= (transform_.pos_.x + rectHalfWidth) &&
            mouseY >= (transform_.pos_.y - rectHalfHeight) &&
            mouseY <= (transform_.pos_.y + rectHalfHeight)) {

            g_audioSystem.playSound("click", "sfx", 0.4f, 1.0f);
            return true;
        }
    }

    return false;
}

void TextData::draw(bool center) {
    if (font_ == 0) {
        printf("ERROR: Attempting to draw text with null font!\n");
        return;
    }

    // Splits content on '\n'
    std::vector<std::string> lines;
    std::istringstream ss(content_);
    std::string token;
    while (std::getline(ss, token, '\n')) {
        lines.push_back(token);
    }
    if (lines.empty()) {
        lines.push_back(content_);
    }

    f32 lineW{0.0f};
    f32 lineH{0.0f};
    AEGfxGetPrintSize(font_, lines[0].empty() ? " " : lines[0].c_str(), scale_, &lineW, &lineH);
    f32 step = lineH * lineSpacing_;
    f32 totalHeight = step * static_cast<f32>(lines.size());

    // For centered mode:
    // start at the top of the vertically centered block
    f32 startY = center ? (y_ + totalHeight / 2.0f - step) : y_;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        f32 drawX = x_;
        f32 drawY = startY - i * step;
        if (center) {
            f32 w, h;
            AEGfxGetPrintSize(font_, lines[i].empty() ? " " : lines[i].c_str(), scale_, &w, &h);
            drawX = x_ - w / 2.0f;
        }
        AEGfxPrint(font_, lines[i].c_str(), drawX, drawY, scale_, r_, g_, b_, a_);
    }
}

void TextData::initFromJson(const std::string& file, const std::string& section) {
    const Json::Value& textSection = g_configManager.getSection(file, section);
    content_ = textSection["content"].asString();
    x_ = textSection["x"].asFloat();
    y_ = textSection["y"].asFloat();
    scale_ = textSection["scale"].asFloat();

    // lineSpacing is optional, not all JSON file will have it thus let it have a default
    lineSpacing_ = textSection.get("lineSpacing", 1.0f).asFloat();

    r_ = textSection["red"].asFloat();
    g_ = textSection["green"].asFloat();
    b_ = textSection["blue"].asFloat();
    a_ = textSection["alpha"].asFloat();
}

void TextData::setTransform(const f32& x, const f32& y, const f32& scale, const f32& r,
                            const f32& g, const f32& b, const f32& a) {
    x_ = x;
    y_ = y;
    scale_ = scale;
    r_ = r;
    g_ = g;
    b_ = b;
    a_ = a;
}

bool Button::isHovered() const {
    // Get mouse position
    s32 mouseX{0};
    s32 mouseY{0};

    AEInputGetCursorPosition(&mouseX, &mouseY);
    mouseX -= AEGfxGetWindowWidth() / 2;
    mouseY = (AEGfxGetWindowHeight() / 2) - mouseY;

    // Check if mouse pos falls within the button's collider box
    f32 rect_half_width{transform_.scale_.x / 2.0f};
    f32 rect_half_height{transform_.scale_.y / 2.0f};

    if (mouseX >= (transform_.pos_.x - rect_half_width) &&
        mouseX <= (transform_.pos_.x + rect_half_width) &&
        mouseY >= (transform_.pos_.y - rect_half_height) &&
        mouseY <= (transform_.pos_.y + rect_half_height)) {
        return true;
    }

    return false;
}