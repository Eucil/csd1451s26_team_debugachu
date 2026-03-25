#pragma once

#include <AEEngine.h>

class GameStateManager;

// ----------------------------------------------------------------------------
// HowToPlay State
//
// A multi-page "How to Play" screen navigated with Next/Back buttons
// and keyboard (left/right arrows or escape to exit).
//
// Page structure:
//   Page 1 - Controls  (movement, shooting)
//   Page 2 - Objective (what to do)
//   Page 3 - Enemies   (what to avoid)
//   Page 4 - Tips      (power-ups, survival)
//
// Usage: wire into GameStateManager just like Credits.
// ----------------------------------------------------------------------------

void LoadHowToPlay();
void InitializeHowToPlay();
void UpdateHowToPlay(GameStateManager& GSM, f32 deltaTime);
void DrawHowToPlay();
void FreeHowToPlay();
void UnloadHowToPlay();