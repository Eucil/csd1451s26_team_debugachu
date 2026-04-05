/*!
@file       PlayerLevel.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declarations of
            the game state functions for the PlayerLevel state, such as
            load, initialize, update, draw, free, and unload.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// ==========================================
// Third-party
// ==========================================
#include <AEEngine.h>

class GameStateManager;

// ==========================================
// GAME STATE FUNCTIONS
// ==========================================
void loadPlayerLevel();
void initializePlayerLevel();
void updatePlayerLevel(GameStateManager& GSM, f32 deltaTime);
void drawPlayerLevel();
void freePlayerLevel();
void unloadPlayerLevel();