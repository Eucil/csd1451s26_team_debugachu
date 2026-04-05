/*!
@file       Level.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declarations of
            the game state functions for the Level state, such as
            load, initialize, update, draw, free, and unload.

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
// GAME STATE FUNCTIONS
// ==========================================
void loadLevel();
void initializeLevel();
void updateLevel(GameStateManager& GSM, f32 deltaTime);
void drawLevel();
void freeLevel();
void unloadLevel();