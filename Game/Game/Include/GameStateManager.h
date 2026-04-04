/*!
@file       GameStateManager.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Third-party
#include <AEEngine.h>

// Project
#include "States/Controls.h"
#include "States/Credits.h"
#include "States/Level.h"
#include "States/LevelSelector.h"
#include "States/LogoScreen.h"
#include "States/MainMenu.h"
#include "States/PlayerLevel.h"
#include "States/Settings.h"

enum class StateId {
    Quit,
    Next,
    Restart,
    NextLevel,
    LogoScreen,
    MainMenu,
    LevelSelector,
    PlayerLevel,
    Level,
    Credits,
    Settings,
    Controls
};

class GameState {
public:
    void (*init_)(void);
    void (*load_)(void);
    void (*update_)(f32);
    void (*free_)(void);

private:
};

class GameStateManager {
public:
    StateId currentState_;
    StateId previousState_;
    StateId nextState_;

    void init(StateId startingStateId) {
        currentState_ = previousState_ = nextState_ = startingStateId;
    }

    void update(StateId updateStateId) {
        switch (updateStateId) {
        case StateId::LogoScreen:
            fpLoad_ = loadLogoScreen;
            fpInitialize_ = initializeLogoScreen;
            fpUpdate_ = updateLogoScreen;
            fpDraw_ = drawLogoScreen;
            fpFree_ = freeLogoScreen;
            fpUnload_ = unloadLogoScreen;
            break;
        case StateId::MainMenu:
            fpLoad_ = loadMainMenu;
            fpInitialize_ = initializeMainMenu;
            fpUpdate_ = updateMainMenu;
            fpDraw_ = drawMainMenu;
            fpFree_ = freeMainMenu;
            fpUnload_ = unloadMainMenu;
            break;
        case StateId::LevelSelector:
            fpLoad_ = loadLevelSelector;
            fpInitialize_ = initializeLevelSelector;
            fpUpdate_ = updateLevelSelector;
            fpDraw_ = drawLevelSelector;
            fpFree_ = freeLevelSelector;
            fpUnload_ = unloadLevelSelector;
            break;
        case StateId::PlayerLevel:
            fpLoad_ = loadPlayerLevel;
            fpInitialize_ = initializePlayerLevel;
            fpUpdate_ = updatePlayerLevel;
            fpDraw_ = drawPlayerLevel;
            fpFree_ = freePlayerLevel;
            fpUnload_ = unloadPlayerLevel;
            break;
        case StateId::Level:
            fpLoad_ = loadLevel;
            fpInitialize_ = initializeLevel;
            fpUpdate_ = updateLevel;
            fpDraw_ = drawLevel;
            fpFree_ = freeLevel;
            fpUnload_ = unloadLevel;
            break;
        case StateId::Credits:
            fpLoad_ = loadCredits;
            fpInitialize_ = initializeCredits;
            fpUpdate_ = updateCredits;
            fpDraw_ = drawCredits;
            fpFree_ = freeCredits;
            fpUnload_ = unloadCredits;
            break;
        case StateId::Settings:
            fpLoad_ = loadSettings;
            fpInitialize_ = initializeSettings;
            fpUpdate_ = updateSettings;
            fpDraw_ = drawSettings;
            fpFree_ = freeSettings;
            fpUnload_ = unloadSettings;
            break;
        case StateId::NextLevel:
            // NextLevel is a distinct value from Level so the main loop's
            // while (currentState_ == nextState_) condition breaks and
            // triggers a full Free/Unload/Load/Initialize cycle.
            // The Level functions are reused -- only the state ID differs.
            fpLoad_ = loadLevel;
            fpInitialize_ = initializeLevel;
            fpUpdate_ = updateLevel;
            fpDraw_ = drawLevel;
            fpFree_ = freeLevel;
            fpUnload_ = unloadLevel;
            break;
        case StateId::Controls:
            fpLoad_ = loadControls;
            fpInitialize_ = initializeControls;
            fpUpdate_ = updateControls;
            fpDraw_ = drawControls;
            fpFree_ = freeControls;
            fpUnload_ = unloadControls;
            break;
        }
    }

    void callLoad() {
        if (fpLoad_) {
            fpLoad_();
        }
    }

    void callInitialize() {
        if (fpInitialize_) {
            fpInitialize_();
        }
    }

    void callUpdate(f32 deltaTime) {
        if (fpUpdate_) {
            fpUpdate_(*this, deltaTime);
        }
    }

    void callDraw() {
        if (fpDraw_) {
            fpDraw_();
        }
    }

    void callFree() {
        if (fpFree_) {
            fpFree_();
        }
    }

    void callUnload() {
        if (fpUnload_) {
            fpUnload_();
        }
    }

private:
    void (*fpLoad_)();
    void (*fpInitialize_)();
    void (*fpUpdate_)(GameStateManager&, f32);
    void (*fpDraw_)();
    void (*fpFree_)();
    void (*fpUnload_)();
};