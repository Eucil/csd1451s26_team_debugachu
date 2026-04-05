/*!
@file       Credits.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu

@date       March, 31, 2026

@brief      This header file contains the declarations of functions that
            implement the Credits screen state, which displays a scrolling
            credits sequence loaded from a JSON config file. The state shares
            the live menu background with MainMenu and Controls.

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
// CREDITS STATE
// ==========================================
void loadCredits();
void initializeCredits();
void updateCredits(GameStateManager& GSM, f32 deltaTime);
void drawCredits();
void freeCredits();
void unloadCredits();