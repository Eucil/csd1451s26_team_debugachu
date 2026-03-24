#pragma once

#include <AEEngine.h>

// ----------------------------------------------------------------------------
// MenuBackground
//
// Shared animated background used by both MainMenu and Credits screens.
// Encapsulates the terrain + fluid simulation so both states can render
// the same live background without duplicating code or assets.
//
// Usage:
//   Call MenuBackground::Load()             once during the state's Load()
//   Call MenuBackground::Initialize()       each time the state is entered
//   Call MenuBackground::Update()           every frame in Update()
//   Call MenuBackground::Draw()             every frame in Draw()
//   Call MenuBackground::DestroyDirtAtMouse() in Update() when left mouse held
//   Call MenuBackground::Free()             in Free() when leaving the state
//   Call MenuBackground::Unload()           in Unload() when done with menus
// ----------------------------------------------------------------------------

namespace MenuBackground {

// Load assets (textures, fonts, meshes).
void Load();

// Initialize/reset the simulation (terrain, fluid, portals, etc.)
void Initialize();

// Update physics, fluid, portals each frame.
void Update(f32 deltaTime);

// Render terrain, fluid, portals, VFX.
void Draw();

// Attempt to destroy dirt at the current mouse world position.
// Returns true if dirt was actually destroyed so the caller can
// trigger VFX and audio on the same frame.
bool DestroyDirtAtMouse(f32 radius);

// Free runtime objects (terrain heap allocs, particle pools).
// Call in each state's Free() function.
void Free();

// Unload GPU assets (textures, meshes, fonts).
// Call in Unload() when transitioning away from all menu states.
void Unload();

} // namespace MenuBackground