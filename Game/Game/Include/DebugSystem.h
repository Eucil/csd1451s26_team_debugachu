/*!
@file       DebugSystem.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file declares the DebugSystem class, which provides
            an in-game overlay for toggling debug options, rendering colliders
            and velocity vectors, and displaying HUD values.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <string>
#include <unordered_map>
#include <vector>

// Third-party
#include <AEEngine.h>
#include <json/json.h>

// Project
#include "Button.h"
#include "Components.h"
#include "GameStateManager.h"

class CollectibleSystem;
class FluidSystem;
class PortalSystem;
class StartEndPoint;
class Terrain;
class VFXSystem;

// Represents a single toggleable debug option with a checkbox button and label.
struct DebugToggle {
    std::string key_;       // Key used to look up the toggle in options_
    std::string hudFormat_; // Optional printf-style format string for HUD display
    Button checkbox_;
    TextData label_;
};

class DebugSystem {
public:
    // Loads meshes and fonts needed to render the debug overlay.
    void load(s8 font);

    // Initialises toggle buttons and layout from the given JSON file and section.
    void initFromJson(const std::string& file, const std::string& section);

    // Frees all meshes owned by the debug system.
    void unload();

    // Opens the debug overlay.
    void open();

    // Closes the debug overlay.
    void close();

    // Toggles the debug overlay open or closed.
    void toggle();

    // Returns true if the debug overlay is currently open.
    bool isOpen() const;

    // Toggle states keyed by name — read directly by game systems each frame.
    std::unordered_map<std::string, bool> options_;

    // Numeric values displayed on the HUD, keyed by toggle name.
    std::unordered_map<std::string, float> hudValues_;

    // Handles checkbox click input and updates toggle states.
    void update();

    // Draws the debug overlay panel, toggles, and close button.
    void draw();

    // Draws HUD values (FPS, particle counts, etc.) as screen-space text.
    void drawHUD();

    // Runs all debug visualisations and HUD updates for the registered scene.
    void drawAll();

    // Registers scene system pointers so drawAll() can iterate them each frame.
    void setScene(Terrain* dirt, Terrain* stone, Terrain* magic, FluidSystem* fluidSystem,
                  CollectibleSystem* collectibles, PortalSystem* portals, StartEndPoint* startEnd,
                  VFXSystem* vfx = nullptr);

    // Clears all registered scene pointers.
    void clearScene();

private:
    // Rebuilds the world matrix for the overlay background panel.
    void updateTransform();

    // Draws the semi-transparent background quad for the overlay.
    void renderBackground();

    // Draws the collider wireframe for a single object.
    void drawSingleCollider(const Transform& transform, const Collider2D& col);

    // Draws a velocity vector line for a single object when ShowVelocity is on.
    void drawVelocity(const Transform& transform, const RigidBody2D& rb);

    // Draws velocity vectors for all fluid particles when ShowVelocity is on.
    void drawFluidVelocities(FluidSystem& fluidSystem);

    // Draws collider wireframes for all active cells in a terrain layer.
    void drawTerrainColliders(Terrain& terrain);

    // Draws collider wireframes for all fluid particles.
    void drawFluidColliders(FluidSystem& fluidSystem);

    // Draws collider wireframes for all active collectibles.
    void drawCollectibleColliders(CollectibleSystem& system);

    // Draws collider wireframes for all portals.
    void drawPortalColliders(PortalSystem& system);

    // Draws collider wireframes for start and end point objects.
    void drawStartEndColliders(StartEndPoint& system);

    bool open_{false};
    s8 font_{0};

    // Scene objects registered by the current game state
    Terrain* dirt_{nullptr};
    Terrain* stone_{nullptr};
    Terrain* magic_{nullptr};
    FluidSystem* fluidSystem_{nullptr};
    CollectibleSystem* collectibles_{nullptr};
    PortalSystem* portals_{nullptr};
    StartEndPoint* startEnd_{nullptr};
    VFXSystem* vfx_{nullptr};

    Transform transform_;
    Graphics graphics_;
    AEGfxVertexList* hudMesh_{nullptr};
    AEGfxVertexList* wireRectMesh_{nullptr};
    AEGfxVertexList* wireCircleMesh_{nullptr};
    AEGfxVertexList* wireLineMesh_{nullptr};
    TextData headerText_;

    Button buttonClose_;
    std::vector<DebugToggle> toggles_;
};

extern DebugSystem g_debugSystem;
