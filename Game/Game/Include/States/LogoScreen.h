/*!
@file       LogoScreen.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

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

void loadLogoScreen();
void initializeLogoScreen();
void updateLogoScreen(GameStateManager& GSM, f32 deltaTime);
void drawLogoScreen();
void freeLogoScreen();
void unloadLogoScreen();