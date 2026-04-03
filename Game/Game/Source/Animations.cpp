/*!
@file       Animations.cpp
@author     Chia Hanxin/c.hanxin@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Animations.h"

// ==========================================
//              IAnimation
// ==========================================

// =========================================================
//
// IAnimation::Initialize()
//
// - Provides a default implementation of the Initialize interface.
//
// =========================================================
void IAnimation::Initialize() {}

// =========================================================
//
// IAnimation::Update()
//
// - Provides a default implementation of the Update interface.
//
// =========================================================
void IAnimation::Update(f32 /*dt*/) {}

// =========================================================
//
// IAnimation::Draw()
//
// - Provides a default implementation of the Draw interface.
//
// =========================================================
void IAnimation::Draw() {}

// =========================================================
//
// IAnimation::Free()
//
// - Provides a default implementation of the Free interface.
//
// =========================================================
void IAnimation::Free() {}

// =========================================================
//
// AnimationManager::Add()
//
// - Validates that the incoming animation pointer is non-null.
// - Appends the animation pointer to the internal animations list.
//
// =========================================================
void AnimationManager::Add(IAnimation* anim) {
    if (anim) {
        animations_.push_back(anim);
    }
}

// =========================================================
//
// AnimationManager::InitializeAll()
//
// - Iterates over every registered animation pointer.
// - Calls Initialize() on each animation in order of registration.
//
// =========================================================
void AnimationManager::InitializeAll() {
    for (auto* anim : animations_) {
        anim->Initialize();
    }
}

// =========================================================
//
// AnimationManager::UpdateAll()
//
// - Iterates over every registered animation pointer.
// - Calls Update() with the given delta time on each animation.
//
// =========================================================
void AnimationManager::UpdateAll(f32 dt) {
    for (auto* anim : animations_) {
        anim->Update(dt);
    }
}

// =========================================================
//
// AnimationManager::DrawAll()
//
// - Iterates over every registered animation pointer.
// - Calls Draw() on each animation in order of registration.
//
// =========================================================
void AnimationManager::DrawAll() {
    for (auto* anim : animations_) {
        anim->Draw();
    }
}

// =========================================================
//
// AnimationManager::FreeAll()
//
// - Iterates over every registered animation pointer.
// - Calls Free() on each animation to release its resources.
//
// =========================================================
void AnimationManager::FreeAll() {
    for (auto* anim : animations_) {
        anim->Free();
    }
}

// =========================================================
//
// AnimationManager::Clear()
//
// - Empties the internal animations list, removing all registered pointers.
//
// =========================================================
void AnimationManager::Clear() { animations_.clear(); }

// ==========================================
//                  UIFader
// ==========================================

// =========================================================
//
// UIFader::UIFader()
//
// - Constructs the UIFader and stores the provided fade speed.
//
// =========================================================
UIFader::UIFader(f32 speed) : fadeSpeed_(speed) {}

// =========================================================
//
// UIFader::Update()
//
// - Checks whether the current alpha is below the target alpha.
// - If below, increments current alpha by fade speed scaled to delta time.
// - Clamps current alpha to the target if it overshoots upward.
// - Checks whether the current alpha is above the target alpha.
// - If above, decrements current alpha by fade speed scaled to delta time.
// - Clamps current alpha to the target if it overshoots downward.
//
// =========================================================
void UIFader::Update(f32 dt) {
    if (currentAlpha_ < targetAlpha_) {
        currentAlpha_ += fadeSpeed_ * dt;
        if (currentAlpha_ > targetAlpha_) {
            currentAlpha_ = targetAlpha_;
        }
    } else if (currentAlpha_ > targetAlpha_) {
        currentAlpha_ -= fadeSpeed_ * dt;
        if (currentAlpha_ < targetAlpha_) {
            currentAlpha_ = targetAlpha_;
        }
    }
}

// =========================================================
//
// UIFader::FadeIn()
//
// - Sets the target alpha to 1.0, causing the fader to animate toward full opacity.
//
// =========================================================
void UIFader::FadeIn() { targetAlpha_ = 1.0f; }

// =========================================================
//
// UIFader::FadeOut()
//
// - Sets the target alpha to 0.0, causing the fader to animate toward full transparency.
//
// =========================================================
void UIFader::FadeOut() { targetAlpha_ = 0.0f; }


// =========================================================
//
// UIFader::SetAlpha()
//
// - Assigns the provided alpha value to both current and target alpha simultaneously.
// - Bypasses any interpolation, snapping opacity instantly to the given value.
//
// =========================================================
void UIFader::SetAlpha(f32 alpha) { currentAlpha_ = targetAlpha_ = alpha; }

// =========================================================
//
// UIFader::GetAlpha()
//
// - Returns the current interpolated alpha value of the fader.
//
// =========================================================
f32 UIFader::GetAlpha() const { return currentAlpha_; }

// =========================================================
//
// UIFader::IsVisible()
//
// - Returns true if current alpha is greater than zero, false otherwise.
//
// =========================================================
bool UIFader::IsVisible() const { return currentAlpha_ > 0.0f; }

// ==========================================
//              Screen Fader
// ==========================================

// =========================================================
//
// ScreenFaderManager::ScreenFaderManager()
//
// - Constructs the ScreenFaderManager and forwards the given speed to the internal UIFader.
//
// =========================================================
ScreenFaderManager::ScreenFaderManager(f32 speed) : fader_(speed) {}

// =========================================================
//
// ScreenFaderManager::Initialize()
//
// - Creates the fullscreen black mesh if one does not already exist.
// - Snaps the fader's alpha immediately to fully opaque.
// - Begins a fade-out so the screen transitions from black to visible on entry.
// - Resets the isFadingOut_ flag to false.
//
// =========================================================
void ScreenFaderManager::Initialize() {
    if (!blackMesh_) {
        blackMesh_ = CreateRectMesh();
    }
    fader_.SetAlpha(1.0f);
    fader_.FadeOut();
    isFadingOut_ = false;
}

// =========================================================
//
// ScreenFaderManager::StartFadeOut()
//
// - Checks that a fade-out is not already in progress before proceeding.
// - Stores the provided GameStateManager pointer for use upon fade completion.
// - Stores the target state to transition to once the fade completes.
// - Sets the isFadingOut_ flag to true.
// - Begins fading the screen to fully opaque black.
//
// =========================================================
void ScreenFaderManager::StartFadeOut(GameStateManager* gsm, StateId target) {
    if (!isFadingOut_) {
        gsm_ = gsm;
        targetState_ = target;
        isFadingOut_ = true;
        fader_.FadeIn();
    }
}

// =========================================================
//
// ScreenFaderManager::Update()
//
// - Advances the internal UIFader by one timestep using the given delta time.
// - Checks if a fade-out is in progress and the alpha has reached full opacity.
// - If both conditions are met, assigns the target state to the GameStateManager.
//
// =========================================================
void ScreenFaderManager::Update(f32 dt) {
    fader_.Update(dt);
    if (isFadingOut_ && fader_.GetAlpha() >= 1.0f) {
        if (gsm_) {
            gsm_->nextState_ = targetState_;
        }
    }
}

// =========================================================
//
// ScreenFaderManager::Draw()
//
// - Checks that the fader is visible and a valid mesh exists before drawing.
// - Sets the render mode to color and blend mode to alpha blend.
// - Applies the current fader alpha as the transparency value.
// - Sets the multiply color to pure black.
// - Retrieves the current window width and height.
// - Constructs a scale matrix matching the full window dimensions.
// - Concatenates the scale into a world transform matrix.
// - Applies the world transform and draws the fullscreen black mesh.
//
// =========================================================
void ScreenFaderManager::Draw() {
    if (fader_.IsVisible() && blackMesh_) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(fader_.GetAlpha());
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

        f32 w = static_cast<f32>(AEGfxGetWindowWidth());
        f32 h = static_cast<f32>(AEGfxGetWindowHeight());

        AEMtx33 scale, world;
        AEMtx33Scale(&scale, w, h);
        AEMtx33Identity(&world);
        AEMtx33Concat(&world, &world, &scale);

        AEGfxSetTransform(world.m);
        AEGfxMeshDraw(blackMesh_, AE_GFX_MDM_TRIANGLES);
    }
}

// =========================================================
//
// ScreenFaderManager::Free()
//
// - Checks that the black mesh pointer is valid before attempting to free it.
// - Frees the GPU mesh resource.
// - Nulls out the mesh pointer to prevent dangling references.
//
// =========================================================
void ScreenFaderManager::Free() {
    if (blackMesh_) {
        AEGfxMeshFree(blackMesh_);
        blackMesh_ = nullptr;
    }
}

// =========================================================
//
// ScreenFaderManager::IsFadingOut()
//
// - Returns the current state of the isFadingOut_ flag.
//
// =========================================================
bool ScreenFaderManager::IsFadingOut() const { return isFadingOut_; }