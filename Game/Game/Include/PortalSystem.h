#pragma once

#include <vector>

#include <AEEngine.h>

#include "Components.h"
#include "FluidSystem.h"

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

public:
    void Initialize(int const& portalMax = 0);

    bool SetupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg);

    bool CollisionCheckWithWater(Portal portal, FluidParticle particle);

    void Update(f32 dt, std::vector<FluidParticle>& particlePool);

    void Draw();

    void DrawPreview();

    void Free();

    void CheckMouseClick();

    void ResetIframe();

    void RotatePortal();

    f32 GetRotationValue() const;

    const std::vector<Portal*>& GetPortals() const;

    int GetPortalLimit() const;
};
