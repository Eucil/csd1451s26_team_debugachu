#pragma once

#include "States/Level1.h"
#include "States/MainMenu.h"

enum class StateId { Quit, Next, Restart, MainMenu, Level1 };

class GameState {
public:
    void (*init_)(void);
    void (*load_)(void);
    void (*update_)(float);
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
        case StateId::Level1:
            fpLoad_ = LoadLevel1;
            fpInitialize_ = InitializeLevel1;
            fpUpdate_ = UpdateLevel1;
            fpDraw_ = DrawLevel1;
            fpFree_ = FreeLevel1;
            fpUnload_ = UnloadLevel1;
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

    void callUpdate(float deltaTime) {
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
    void (*fpUpdate_)(GameStateManager&, float);
    void (*fpDraw_)();
    void (*fpFree_)();
    void (*fpUnload_)();
};
