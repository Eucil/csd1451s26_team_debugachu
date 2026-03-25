#pragma once
#include "GameStateManager.h"
#include "Utils.h"
#include <AEEngine.h>
#include <vector>

// ==========================================
// 1. THE INTERFACE (The Contract)
// ==========================================
class IAnimation {
public:
    // Virtual destructor is required for C++ interfaces
    virtual ~IAnimation() = default;

    // Empty virtual functions.
    // If a specific animation doesn't need one of these, it safely does nothing
    virtual void Initialize() {}
    virtual void Update(f32 /*dt*/) {}
    virtual void Draw() {}
    virtual void Free() {}
};

// ==========================================
// 2. THE ANIMATION MANAGER
// ==========================================
class AnimationManager {
private:
    // A list of pointers to ANY class that inherits from IAnimation
    std::vector<IAnimation*> animations_;

public:
    void Add(IAnimation* anim) {
        if (anim)
            animations_.push_back(anim);
    }

    void InitializeAll() {
        for (auto* anim : animations_)
            anim->Initialize();
    }

    void UpdateAll(f32 dt) {
        for (auto* anim : animations_)
            anim->Update(dt);
    }

    void DrawAll() {
        for (auto* anim : animations_)
            anim->Draw();
    }

    void FreeAll() {
        for (auto* anim : animations_)
            anim->Free();
    }

    void Clear() { animations_.clear(); }
};

// ==========================================
// 3. UIFADER (Inherits from IAnimation)
// ==========================================
class UIFader : public IAnimation {
private:
    f32 currentAlpha_ = 0.0f;
    f32 targetAlpha_ = 0.0f;
    f32 fadeSpeed_ = 5.0f;

public:
    UIFader(f32 speed = 5.0f) : fadeSpeed_(speed) {}

    void Update(f32 dt) override {
        if (currentAlpha_ < targetAlpha_) {
            currentAlpha_ += fadeSpeed_ * dt;
            if (currentAlpha_ > targetAlpha_)
                currentAlpha_ = targetAlpha_;
        } else if (currentAlpha_ > targetAlpha_) {
            currentAlpha_ -= fadeSpeed_ * dt;
            if (currentAlpha_ < targetAlpha_)
                currentAlpha_ = targetAlpha_;
        }
    }

    void FadeIn() { targetAlpha_ = 1.0f; }
    void FadeOut() { targetAlpha_ = 0.0f; }
    void SetAlpha(f32 alpha) { currentAlpha_ = targetAlpha_ = alpha; }

    f32 GetAlpha() const { return currentAlpha_; }
    bool IsVisible() const { return currentAlpha_ > 0.0f; }
};

// ==========================================
// 4. SCREEN FADER (Inherits from IAnimation)
// ==========================================
class ScreenFaderManager : public IAnimation {
private:
    UIFader fader_;
    bool isFadingOut_ = false;
    StateId targetState_ = StateId::MainMenu;
    GameStateManager* gsm_ = nullptr;
    AEGfxVertexList* blackMesh_ = nullptr;

public:
    ScreenFaderManager(f32 speed = 2.0f) : fader_(speed) {}

    void Initialize() override {
        if (!blackMesh_)
            blackMesh_ = CreateRectMesh();
        fader_.SetAlpha(1.0f);
        fader_.FadeOut();
        isFadingOut_ = false;
    }

    void StartFadeOut(GameStateManager* gsm, StateId target) {
        if (!isFadingOut_) {
            gsm_ = gsm;
            targetState_ = target;
            isFadingOut_ = true;
            fader_.FadeIn();
        }
    }

    void Update(f32 dt) override {
        fader_.Update(dt);
        if (isFadingOut_ && fader_.GetAlpha() >= 1.0f) {
            if (gsm_)
                gsm_->nextState_ = targetState_;
        }
    }

    void Draw() override {
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

    void Free() override {
        if (blackMesh_) {
            AEGfxMeshFree(blackMesh_);
            blackMesh_ = nullptr;
        }
    }

    bool IsFadingOut() const { return isFadingOut_; }
};