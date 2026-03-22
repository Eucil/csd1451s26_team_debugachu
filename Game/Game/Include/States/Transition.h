#pragma once
#include "AEEngine.h"
#include "FluidSystem.h"
#include "GameStateManager.h"

class TransitionManager {
private:
    bool isTransitioning_ = false;
    f32 tsunamiTimer_ = 0.0f;
    StateId targetState_ = StateId::MainMenu;

    // It needs pointers to the active GSM and FluidSystem to do its job
    GameStateManager* gsm_ = nullptr;
    FluidSystem* fluidSystem_ = nullptr;

public:
    // Initialize the manager with the systems it needs to talk to
    void Initialize(FluidSystem* fluidSystem) {
        fluidSystem_ = fluidSystem;
        isTransitioning_ = false;
        tsunamiTimer_ = 0.0f;
    }

    // 2. Pass the GSM in here when you actually click the button!
    void StartTsunami(GameStateManager* gsm, StateId target) {
        if (!isTransitioning_) {
            gsm_ = gsm; // Store the pointer now
            targetState_ = target;
            isTransitioning_ = true;
            tsunamiTimer_ = 0.0f;
        }
    }

    // Self-contained update logic
    void Update(f32 dt) {
        if (!isTransitioning_ || !fluidSystem_ || !gsm_)
            return;

        tsunamiTimer_ += dt;
        f32 windowW = static_cast<f32>(AEGfxGetWindowWidth());
        f32 windowH = static_cast<f32>(AEGfxGetWindowHeight());

        // Phase 1: Spawn the massive wave
        if (tsunamiTimer_ < 1.2f) {
            for (int i = 0; i < 15; ++i) {
                f32 randomX = (AERandFloat() * windowW) - (windowW / 2.0f);
                f32 randomRadius = 40.0f + (AERandFloat() * 30.0f);
                fluidSystem_->SpawnParticle(randomX, (windowH / 2.0f) + 80.0f, randomRadius,
                                            FluidType::Water);
            }
        }

        // Phase 2: Trigger the state change once the screen is covered
        if (tsunamiTimer_ >= 6.0) {
            gsm_->nextState_ = targetState_;

            // Reset itself so it's ready for the next time this state is loaded
            isTransitioning_ = false;
            tsunamiTimer_ = 0.0f;
        }
    }

    // Getters
    bool IsTransitioning() const { return isTransitioning_; }
};  


/*
#include "AEEngine.h"
class GameStateManager;
enum class StateId;


// Helper function to set where the transition should go
void SetTargetState(StateId target);

// Standard state lifecycle functions
void LoadTransition();
void InitializeTransition();
void UpdateTransition(GameStateManager& GSM, f32 deltaTime);
void DrawTransition();
void FreeTransition();
void UnloadTransition();

*/
