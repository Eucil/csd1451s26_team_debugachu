/*!
@file       LevelSelector.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This header file declares the game state functions
            for the LevelSelector state, such as
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
void loadLevelSelector();
void initializeLevelSelector();
void updateLevelSelector(GameStateManager& GSM, f32 deltaTime);
void drawLevelSelector();
void freeLevelSelector();
void unloadLevelSelector();