#include "Animations.h"

// ==========================================
// 1. THE INTERFACE (The Contract)
// ==========================================
void IAnimation::Initialize() {}
void IAnimation::Update(f32 /*dt*/) {}
void IAnimation::Draw() {}
void IAnimation::Free() {}

// ==========================================
// 2. THE ANIMATION MANAGER
// ==========================================
void AnimationManager::Add(IAnimation* anim) {
    if (anim) {
        animations_.push_back(anim);
    }
}

void AnimationManager::InitializeAll() {
    for (auto* anim : animations_) {
        anim->Initialize();
    }
}

void AnimationManager::UpdateAll(f32 dt) {
    for (auto* anim : animations_) {
        anim->Update(dt);
    }
}

void AnimationManager::DrawAll() {
    for (auto* anim : animations_) {
        anim->Draw();
    }
}

void AnimationManager::FreeAll() {
    for (auto* anim : animations_) {
        anim->Free();
    }
}

void AnimationManager::Clear() { animations_.clear(); }

// ==========================================
// 3. UIFADER
// ==========================================
UIFader::UIFader(f32 speed) : fadeSpeed_(speed) {}

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

void UIFader::FadeIn() { targetAlpha_ = 1.0f; }

void UIFader::FadeOut() { targetAlpha_ = 0.0f; }

void UIFader::SetAlpha(f32 alpha) { currentAlpha_ = targetAlpha_ = alpha; }

f32 UIFader::GetAlpha() const { return currentAlpha_; }

bool UIFader::IsVisible() const { return currentAlpha_ > 0.0f; }

// ==========================================
// 4. SCREEN FADER
// ==========================================
ScreenFaderManager::ScreenFaderManager(f32 speed) : fader_(speed) {}

void ScreenFaderManager::Initialize() {
    if (!blackMesh_) {
        blackMesh_ = CreateRectMesh();
    }
    fader_.SetAlpha(1.0f);
    fader_.FadeOut();
    isFadingOut_ = false;
}

void ScreenFaderManager::StartFadeOut(GameStateManager* gsm, StateId target) {
    if (!isFadingOut_) {
        gsm_ = gsm;
        targetState_ = target;
        isFadingOut_ = true;
        fader_.FadeIn();
    }
}

void ScreenFaderManager::Update(f32 dt) {
    fader_.Update(dt);
    if (isFadingOut_ && fader_.GetAlpha() >= 1.0f) {
        if (gsm_) {
            gsm_->nextState_ = targetState_;
        }
    }
}

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

void ScreenFaderManager::Free() {
    if (blackMesh_) {
        AEGfxMeshFree(blackMesh_);
        blackMesh_ = nullptr;
    }
}

bool ScreenFaderManager::IsFadingOut() const { return isFadingOut_; }