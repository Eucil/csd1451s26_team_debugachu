/*!
@file       Animations.h
@author     Chia Hanxin/c.hanxin@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declarations of functions and classes
            for the animation system which includes the following classes:

                - IAnimation, a base-class which acts as an interface for
                  modular animations
                - AnimationManager, a manager class for all sub-animation classes
                - UIFader, an animation sub-class which acts as a reusable
                  component for interpolating alpha transparency on UI elements.
                - ScreenFaderManager, an animation sub-class which coordinates
                  with the GameStateManager for state-transition overlays

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#pragma once

// ==========================================
// Includes
// ==========================================

// Standard library
#include <vector>

// Third-party
#include <AEEngine.h>

// Project
#include "GameStateManager.h"
#include "MeshUtils.h"

// ==========================================
// Interface
// ==========================================
class IAnimation {
public:
    virtual ~IAnimation() = default;

    // Empty virtual functions declared
    virtual void initialize();
    virtual void update(f32 dt);
    virtual void draw();
    virtual void free();
};

// ==========================================
// Animation Manager
// ==========================================
class AnimationManager {
private:
    std::vector<IAnimation*> animations_;

public:
    void add(IAnimation* anim);
    void initializeAll();
    void updateAll(f32 dt);
    void drawAll();
    void freeAll();
    void clear();
};

// ==========================================
// UIFader (Inherits from IAnimation)
// ==========================================
class UIFader : public IAnimation {
private:
    f32 currentAlpha_ = 0.0f;
    f32 targetAlpha_ = 0.0f;
    f32 fadeSpeed_ = 5.0f;

public:
    UIFader(f32 speed = 5.0f);

    void update(f32 dt) override;
    void fadeIn();
    void fadeOut();
    void setAlpha(f32 alpha);

    f32 getAlpha() const;
    bool isVisible() const;
};

// ==========================================
// Screen Fader (Inherits from IAnimation)
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

    void initialize() override;
    void startFadeOut(GameStateManager* gsm, StateId target);
    void update(f32 dt) override;
    void draw() override;
    void free() override;
    bool isFadingOut() const;
};