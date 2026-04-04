/*!
@file       MenuBackground.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

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

// ----------------------------------------------------------------------------
// MenuBackground
//
// Shared animated background used by both MainMenu and Credits screens.
// Encapsulates the terrain + fluid simulation so both states can render
// the same live background without duplicating code or assets.
//
// Usage:
//   Call MenuBackground::load()             once during the state's load()
//   Call MenuBackground::initialize()       each time the state is entered
//   Call MenuBackground::update()           every frame in update()
//   Call MenuBackground::draw()             every frame in draw()
//   Call MenuBackground::destroyDirtAtMouse() in update() when left mouse held
//   Call MenuBackground::free()             in free() when leaving the state
//   Call MenuBackground::unload()           in unload() when done with menus
// ----------------------------------------------------------------------------

namespace MenuBackground {

// Load assets (textures, fonts, meshes).
void load(int backgroundLevel = 99);

// Initialize/reset the simulation (terrain, fluid, portals, etc.)
void initialize();

// Update physics, fluid, portals each frame.
void update(f32 deltaTime);

// Render terrain, fluid, portals, VFX.
void draw();

// Attempt to destroy dirt at the current mouse world position.
// Returns true if dirt was actually destroyed so the caller can
// trigger VFX and audio on the same frame.
bool destroyDirtAtMouse(f32 radius);

// Free runtime objects (terrain heap allocs, particle pools).
// Call in each state's free() function.
void free();

// Unload GPU assets (textures, meshes, fonts).
// Call in unload() when transitioning away from all menu states.
void unload();

} // namespace MenuBackground