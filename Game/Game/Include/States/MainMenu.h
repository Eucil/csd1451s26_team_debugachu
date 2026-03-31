/*!
@file       MainMenu.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Chia Hanxin/c.hanxin@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

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

void LoadMainMenu();

void InitializeMainMenu();

void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime);

void DrawMainMenu();

void FreeMainMenu();

void UnloadMainMenu();