/*!
@file       Settings.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

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

void loadSettings();

void initializeSettings();

void updateSettings(GameStateManager& GSM, f32 deltaTime);

void drawSettings();

void freeSettings();

void unloadSettings();