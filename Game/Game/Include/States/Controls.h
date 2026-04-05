/*!
@file       Controls.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu

@date       March, 31, 2026

@brief      This header file contains the declarations of functions that
            implement the Controls screen state, which displays a multi-page
            how-to-play guide with sprite illustrations, animated collectible
            icons, and navigation buttons. The state shares the live menu
            background with MainMenu and Credits.

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
// CONTROLS STATE
// ==========================================
void loadControls();
void initializeControls();
void updateControls(GameStateManager& GSM, f32 deltaTime);
void drawControls();
void freeControls();
void unloadControls();