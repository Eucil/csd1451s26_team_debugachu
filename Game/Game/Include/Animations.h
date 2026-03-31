/*!
@file       Animations.h
@author     Chia Hanxin/c.hanxin@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

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
    virtual ~IAnimation() = default;

    // Empty virtual functions declared
    virtual void Initialize();
    virtual void Update(f32 dt);
    virtual void Draw();
    virtual void Free();
};

// ==========================================
// 2. THE ANIMATION MANAGER
// ==========================================
class AnimationManager {
private:
    std::vector<IAnimation*> animations_;

public:
    void Add(IAnimation* anim);
    void InitializeAll();
    void UpdateAll(f32 dt);
    void DrawAll();
    void FreeAll();
    void Clear();
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
    UIFader(f32 speed = 5.0f);

    void Update(f32 dt) override;
    void FadeIn();
    void FadeOut();
    void SetAlpha(f32 alpha);

    f32 GetAlpha() const;
    bool IsVisible() const;
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
    ScreenFaderManager(f32 speed = 2.0f);

    void Initialize() override;
    void StartFadeOut(GameStateManager* gsm, StateId target);
    void Update(f32 dt) override;
    void Draw() override;
    void Free() override;
    bool IsFadingOut() const;
};