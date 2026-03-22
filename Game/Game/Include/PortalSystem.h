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

    // graphic configs for each StartEnd type
    Graphics graphicsConfigs_;

    bool clickIframe_{false};
    f32 rotationValue_ = 0.0f;

public:
    void Initialize();

    void SetupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg);

    bool CollisionCheckWithWater(Portal portal, FluidParticle particle);

    void Update(f32 dt, std::vector<FluidParticle>& particlePool);

    void DrawColor();

    void DrawTexture();

    void Free();

    void CheckMouseClick();

    void ResetIframe();

    void RotatePortal();

    f32 GetRotationValue();

    const std::vector<Portal*>& GetPortals() const;
};
