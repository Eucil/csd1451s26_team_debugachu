/*!
@file       MenuBackground.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date       March, 31, 2026

@brief      This header file contains the declarations of functions in the
            MenuBackground namespace, which provides a shared animated
            background used by the MainMenu, Credits, and Controls states.
            It encapsulates terrain, fluid, portal, and VFX simulations so
            that multiple states can render the same live background without
            duplicating code or assets.

            A reference counter ensures assets are loaded exactly once
            regardless of how many states call load() and unload().

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Third-party
#include <AEEngine.h>

// ==========================================
// MENU BACKGROUND
// ==========================================
namespace MenuBackground {

// Load assets (textures, meshes). Pass the level number to use as background.
// Guarded by a reference counter; safe to call from multiple states.
void load(int backgroundLevel = 99);

// Initialize/reset the simulation (terrain, fluid, portals, VFX).
// Call each time a state is entered.
void initialize();

// Update terrain destruction, fluid, portals, and VFX each frame.
void update(f32 deltaTime);

// Render the tiled background, terrain, portals, VFX, and fluid.
void draw();

// Attempt to destroy dirt at the current mouse world position.
// Returns true if dirt was removed so the caller can trigger audio.
bool destroyDirtAtMouse(f32 radius);

// Free runtime objects (terrain heap allocations, particle pools).
// Call in each state's free() function.
void free();

// Unload GPU assets (textures, meshes).
// Guarded by a reference counter; assets are freed only when all
// states that called load() have called unload().
void unload();

} // namespace MenuBackground