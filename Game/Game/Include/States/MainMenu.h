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

// Third-party
#include <AEEngine.h>

class GameStateManager;

void loadMainMenu();

void initializeMainMenu();

void updateMainMenu(GameStateManager& GSM, f32 deltaTime);

void drawMainMenu();

void freeMainMenu();

void unloadMainMenu();