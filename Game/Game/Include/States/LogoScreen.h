#pragma once

#include <AEEngine.h>

class GameStateManager;

// ----------------------------------------------------------------------------
// LogoScreen State
//
// Displays the DigiPen logo with a fade-in, hold, fade-out sequence,
// then transitions automatically to the MainMenu state.
//
// Sequence:
//   1. Fade in  (alpha 0 -> 255 over fadeSpeed_ seconds)
//   2. Hold     (fully visible for holdDuration_ seconds)
//   3. Fade out (alpha 255 -> 0 over fadeSpeed_ seconds)
//   4. Transition to StateId::MainMenu
//
// Pressing any key or clicking skips directly to the MainMenu.
// ----------------------------------------------------------------------------

void LoadLogoScreen();
void InitializeLogoScreen();
void UpdateLogoScreen(GameStateManager& GSM, f32 deltaTime);
void DrawLogoScreen();
void FreeLogoScreen();
void UnloadLogoScreen();