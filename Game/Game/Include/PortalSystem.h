#pragma once
#include "AEEngine.h"
#include "Components.h"
#include "FluidSystem.h"
#include <vector>

struct Portal {

    Transform transform_;

    Collider2D collider_;

    Portal* linked_portal_{nullptr};

    f32 red{};
    f32 green{};
    f32 blue{};

    Portal();
    Portal(AEVec2 pos, AEVec2 scale, f32 rotationDeg);
};

class PortalSystem {
private:
    AEGfxVertexList* rectMesh = nullptr;

    // Can have multiple portals
    std::vector<Portal*> portal_vec;
    Portal* current_portal_{nullptr};

    // graphic configs for each StartEnd type
    Graphics graphicsConfigs_;

    bool click_iframe{false};
    f32 rotation_value = 0.0f;

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
};
