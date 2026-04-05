/*!
@file       LogoScreen.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date       March, 31, 2026

@brief      This header file contains the declarations of functions that
            implement the LogoScreen state, which displays the DigiPen logo
            with a fade-in, hold, and fade-out sequence before automatically
            transitioning to the MainMenu state. Pressing any key or clicking
            skips directly to the MainMenu.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Third-party
#include <AEEngine.h>

class GameStateManager;

// ==========================================
// LOGO SCREEN STATE
// ==========================================
void loadLogoScreen();
void initializeLogoScreen();
void updateLogoScreen(GameStateManager& GSM, f32 deltaTime);
void drawLogoScreen();
void freeLogoScreen();
void unloadLogoScreen();