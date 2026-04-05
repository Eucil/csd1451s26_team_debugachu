/*!
@file       MouseUtils.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the definitions of functions and classes
            for the mouse utility system which includes the following:

                - getMouseWorldPos, a coordinate transformation utility that
                  converts hardware-level window coordinates into center-aligned
                  world-space coordinates.
                - Helper logic for centering the game origin and inverting the
                  vertical axis to match Cartesian coordinate standards.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

// ==========================================
// Includes
// ==========================================
#include "MouseUtils.h"

// Standard library
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "ConfigManager.h"

// =========================================================
//
//  MouseUtility's getMouseWorldPos function
//
// Retrieves the current hardware cursor position and translates it into
// world-space coordinates relative to the center of the application window.
//
// The list of transformations include:
// - Polls the AlphaEngine for raw pixel coordinates from the top-left origin.
// - Re-centers the X-coordinate by offsetting it by half the window width.
// - Inverts the Y-axis and re-centers the origin to match world-space Cartesian
//   coordinates by subtracting the raw Y from half the window height.
// - Returns the final result as a floating-point AEVec2 for physics and gameplay use.
//
// =========================================================
AEVec2 getMouseWorldPos() {
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    AEVec2 worldPos{};
    worldPos.x = (f32)mouseX - (AEGfxGetWindowWidth() / 2.0f);
    worldPos.y = (AEGfxGetWindowHeight() / 2.0f) - (f32)mouseY;

    return worldPos;
}