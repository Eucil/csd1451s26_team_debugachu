/*!
@file       PortalSystem.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <vector>

// Third-party
#include <AEEngine.h>

// Project
#include "Components.h"
#include "FluidSystem.h"
#include "VFXSystem.h"

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

class PortalSystem {
private:
    AEGfxVertexList* rectMesh_ = nullptr;

    // Can have multiple portals
    std::vector<Portal*> portalVec_;
    Portal* currentPortal_{nullptr};

    // graphic configs for each portal
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
    void initialize(int const& portalMax = 0);

    bool setupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg);

    bool collisionCheckWithWater(Portal portal, FluidParticle particle);

    void update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfx);

    void draw();

    void drawPreview();

    void free();

    void checkMouseClick();

    void resetIframe();

    void rotatePortal();

    f32 getRotationValue() const;

    const std::vector<Portal*>& getPortals() const;

    int getPortalLimit() const;

    int getPortalCount() const;
};
