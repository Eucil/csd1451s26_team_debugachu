/*!
@file       Credits.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once
#include <AEEngine.h>

class GameStateManager;

void LoadCredits();
void InitializeCredits();
void UpdateCredits(GameStateManager& GSM, f32 deltaTime);
void DrawCredits();
void FreeCredits();
void UnloadCredits();