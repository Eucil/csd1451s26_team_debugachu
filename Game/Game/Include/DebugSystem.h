/*!
@file       DebugSystem.h
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

#include <string>
#include <unordered_map>
#include <vector>

#include <AEEngine.h>
#include <json/json.h>

#include "Button.h"
#include "Components.h"
#include "GameStateManager.h"

class CollectibleSystem;
class FluidSystem;
class PortalSystem;
class StartEndPoint;
class Terrain;
class VFXSystem;

struct DebugToggle {
    std::string key_;
    std::string hudFormat_;
    Button checkbox_;
    TextData label_;
};

class DebugSystem {
public:
    void load(s8 font);
    void initFromJson(const std::string& file, const std::string& section);
    void unload();

    void open();
    void close();
    void toggle();

    bool isOpen() const;

    std::unordered_map<std::string, bool> options_;
    std::unordered_map<std::string, float> hudValues_;

    void update();
    void draw();
    void drawHUD();
    void drawAll(); // draws everything registered via setScene()

    void setScene(Terrain* dirt, Terrain* stone, Terrain* magic, FluidSystem* fluidSystem,
                  CollectibleSystem* collectibles, PortalSystem* portals, StartEndPoint* startEnd,
                  VFXSystem* vfx = nullptr);
    void clearScene();

private:
    void updateTransform();
    void renderBackground();
    void drawSingleCollider(const Transform& transform, const Collider2D& col);
    void drawTerrainColliders(Terrain& terrain);
    void drawFluidColliders(FluidSystem& fluidSystem);
    void drawCollectibleColliders(CollectibleSystem& system);
    void drawPortalColliders(PortalSystem& system);
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
