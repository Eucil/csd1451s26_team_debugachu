/*!
@file       MouseUtils.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "MouseUtils.h"

// Standard library
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "ConfigManager.h"

AEVec2 getMouseWorldPos() {
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    AEVec2 worldPos{};
    worldPos.x = (f32)mouseX - (AEGfxGetWindowWidth() / 2.0f);
    worldPos.y = (AEGfxGetWindowHeight() / 2.0f) - (f32)mouseY;

    return worldPos;
}