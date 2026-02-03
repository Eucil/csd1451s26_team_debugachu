#pragma once

#include <AEEngine.h>
#include <string>

#include "Components.h"
#include "Utils.h"

class Button {
public:
    Transform transform_;
    Graphics graphics_;

    Button() {}

    Button(AEVec2 pos, AEVec2 scale) {
        transform_.pos_ = pos;
        transform_.scale_ = scale;
        transform_.rotationRad_ = 0.0f;

        // Set up world matrix
        AEMtx33 scale_mtx, rot_mtx, trans_mtx;

        AEMtx33Scale(&scale_mtx, transform_.scale_.x, transform_.scale_.y);
        AEMtx33Rot(&rot_mtx, transform_.rotationRad_);
        AEMtx33Trans(&trans_mtx, transform_.pos_.x, transform_.pos_.y);

        AEMtx33Concat(&transform_.worldMtx_, &rot_mtx, &scale_mtx);
        AEMtx33Concat(&transform_.worldMtx_, &trans_mtx, &transform_.worldMtx_);
    }

    void SetupMesh() { graphics_.mesh_ = CreateRectMesh(); }

    void DrawColor(f32 r, f32 g, f32 b) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        AEGfxSetTransform(transform_.worldMtx_.m);
        AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
    }

    void UnloadMesh() {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }

    bool OnClick() {
        // Get mouse position
        s32 mouse_x = 0, mouse_y = 0;
        AEInputGetCursorPosition(&mouse_x, &mouse_y);
        mouse_x -= AEGfxGetWindowWidth() / 2;
        mouse_y = (AEGfxGetWindowHeight() / 2) - mouse_y;

        // Check by checking if mouse pos falls within the button's collider box
        f32 rect_half_width = transform_.scale_.x / 2.0f;
        f32 rect_half_height = transform_.scale_.y / 2.0f;
        if (mouse_x >= (transform_.pos_.x - rect_half_width) &&
            mouse_x <= (transform_.pos_.x + rect_half_width) &&
            mouse_y >= (transform_.pos_.y - rect_half_height) &&
            mouse_y <= (transform_.pos_.y + rect_half_height)) {
            return true;
        }
        return false;
    }
};

class Text {
public:
    f32 pos_x_{0.0f};
    f32 pos_y_{0.0f};
    std::string text_{};

    Text() {}
    Text(f32 pos_x, f32 pos_y, std::string text) {
        pos_x_ = pos_x;
        pos_y_ = pos_y;
        text_ = text;
    }
};