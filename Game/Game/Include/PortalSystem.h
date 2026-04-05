/*!
@file       PortalSystem.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu

@date		March, 31, 2026

@brief      This header file declares the Portal struct and PortalSystem class,
            which manage placed portal pairs, particle teleportation,
            and portal rendering and placement in the level.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// =============================
// Standard library
// =============================
#include <vector>

// =============================
// Third-party
// =============================
#include <AEEngine.h>

// =============================
// Project
// =============================
#include "Components.h"
#include "FluidSystem.h"
#include "VFXSystem.h"

// ==========================================
// Portal Struct
// ==========================================
struct Portal {

    Transform transform_;

    Collider2D collider_;

    Portal* linkedPortal_{nullptr};

    f32 red_{};
    f32 green_{};
    f32 blue_{};

    Portal();
    Portal(AEVec2 pos, AEVec2 scale, f32 rotationDeg);
};

// ==========================================
// Portal System
// ==========================================
class PortalSystem {
private:
    AEGfxVertexList* rectMesh_ = nullptr;

    // Can have multiple portals
    std::vector<Portal*> portalVec_;
    Portal* currentPortal_{nullptr};

    // Graphic configs for each portal
    Graphics portalGraphicsConfigs_;
    Graphics arrowGraphicsConfigs_;

    f32 nextRed_{};
    f32 nextGreen_{};
    f32 nextBlue_{};

    AEVec2 portalScale_{};
    bool clickIframe_{false};
    f32 rotationValue_{};
    int portalLimit_{};
    f32 portalVfxCooldown_{0.0f};

public:
    // ==========================================
    // Lifecycle
    // ==========================================
    void initialize(int const& portalMax = 0);
    void update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfx);
    void draw();
    void free();

    void drawPreview();

    // ==========================================
    // Portal Placement
    // ==========================================
    bool setupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg);
    void rotatePortal();
    void checkMouseClick();
    void resetIframe();

    // ==========================================
    // Simulation
    // ==========================================
    bool collisionCheckWithWater(Portal portal, FluidParticle particle);

    // ==========================================
    // Getters
    // ==========================================
    f32 getRotationValue() const;
    const std::vector<Portal*>& getPortals() const;
    int getPortalLimit() const;
    int getPortalCount() const;
};
