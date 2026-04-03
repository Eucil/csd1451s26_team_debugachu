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

#include <vector>

#include <AEEngine.h>

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
    Portal* current_portal_{nullptr};

    // graphic configs for each portal
    Graphics portal_graphicsConfigs_;
    Graphics arrow_graphicsConfigs_;

    f32 nextRed_{};
    f32 nextGreen_{};
    f32 nextBlue_{};

    AEVec2 portalScale_{};
    bool clickIframe_{false};
    f32 rotationValue_{};
    int portalLimit_{};
    f32 portalVfxCooldown_{0.0f};

public:
    void Initialize(int const& portalMax = 0);

    bool SetupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg);

    bool CollisionCheckWithWater(Portal portal, FluidParticle particle);

    void Update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfx);

    void Draw();

    void DrawPreview();

    void Free();

    void CheckMouseClick();

    void ResetIframe();

    void RotatePortal();

    f32 GetRotationValue() const;

    const std::vector<Portal*>& GetPortals() const;

    int GetPortalLimit() const;

    int GetPortalCount() const;
};
