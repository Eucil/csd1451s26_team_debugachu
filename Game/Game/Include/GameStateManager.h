#pragma once

#include <AEEngine.h>

#include "States/Credits.h"
#include "States/HowToPlay.h"
#include "States/Level.h"
#include "States/LevelSelector.h"
#include "States/MainMenu.h"
#include "States/Settings.h"

enum class StateId {
    Quit,
    Next,
    Restart,
    NextLevel,
    MainMenu,
    LevelSelector,
    Level,
    Credits,
    Settings,
    HowToPlay
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
        case StateId::MainMenu:
            fpLoad_ = LoadMainMenu;
            fpInitialize_ = InitializeMainMenu;
            fpUpdate_ = UpdateMainMenu;
            fpDraw_ = DrawMainMenu;
            fpFree_ = FreeMainMenu;
            fpUnload_ = UnloadMainMenu;
            break;
        case StateId::LevelSelector:
            fpLoad_ = LoadLevelSelector;
            fpInitialize_ = InitializeLevelSelector;
            fpUpdate_ = UpdateLevelSelector;
            fpDraw_ = DrawLevelSelector;
            fpFree_ = FreeLevelSelector;
            fpUnload_ = UnloadLevelSelector;
            break;
        case StateId::Level:
            fpLoad_ = LoadLevel;
            fpInitialize_ = InitializeLevel;
            fpUpdate_ = UpdateLevel;
            fpDraw_ = DrawLevel;
            fpFree_ = FreeLevel;
            fpUnload_ = UnloadLevel;
            break;
        case StateId::Credits:
            fpLoad_ = LoadCredits;
            fpInitialize_ = InitializeCredits;
            fpUpdate_ = UpdateCredits;
            fpDraw_ = DrawCredits;
            fpFree_ = FreeCredits;
            fpUnload_ = UnloadCredits;
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
            fpLoad_ = LoadLevel;
            fpInitialize_ = InitializeLevel;
            fpUpdate_ = UpdateLevel;
            fpDraw_ = DrawLevel;
            fpFree_ = FreeLevel;
            fpUnload_ = UnloadLevel;
            break;
        case StateId::HowToPlay:
            fpLoad_ = LoadHowToPlay;
            fpInitialize_ = InitializeHowToPlay;
            fpUpdate_ = UpdateHowToPlay;
            fpDraw_ = DrawHowToPlay;
            fpFree_ = FreeHowToPlay;
            fpUnload_ = UnloadHowToPlay;
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