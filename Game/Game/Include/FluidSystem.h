#pragma once
#include "AEEngine.h"
#include "Components.h"
#include <vector>

enum class FluidType { Water, Lava, Count };

struct FluidParticle {
    Transform transform_;
    RigidBody2D physics_;
    Collider2D collider_;

    FluidType
        type_; //  <--- water, lava, etc
               // currently only being used for identifying the particle type for setters/getters

    bool portal_iframe_{false};           // <--- to prevent immediate re-teleportation
    f32 portal_iframe_timer_{0.2f};       // <--- timer for portal iframe
    f32 portal_iframe_maxduration_{0.2f}; // <--- duration of portal iframe in seconds

    // --------------------- Constructors / Destructors --------------------- //
    FluidParticle(f32 posX, f32 posY, f32 radius, FluidType type);
};

class FluidSystem {
public:
    u32 particleMaxCount{300};
    // --------------------- Constructors / Destructors --------------------- //

    // ------------------------- Basic Methods --------------------------- //
    void Initialize();

    void UpdateMain(f32 dt);

    void DrawColor();

    void DrawTexture();

    void Free();

    // ------------------------- Utility Methods --------------------------- //
    void SpawnParticle(f32 posX, f32 posY, f32 radius, FluidType type);

    int GetParticleCount(FluidType type);

    std::vector<FluidParticle>& GetParticlePool(FluidType type);

private:
    // ----------------------------- Components ----------------------------- //

    // particles[0] holds Water, particles[1] holds Lava, etc.
    // Stores live particles
    std::vector<FluidParticle> particlePools_[static_cast<int>(FluidType::Count)];

    f32 colorConfigs_[static_cast<int>(FluidType::Count)][4]; // [Type][RGBA]

    Graphics graphicsConfigs_[static_cast<int>(FluidType::Count)];

    void InitializeMesh();

    void UpdateTransforms(std::vector<FluidParticle>& particlePool);

    void UpdateCollision(std::vector<FluidParticle>& particlePool, f32 dt);

    void UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt);

    void UpdatePortalIframes(f32 dt, std::vector<FluidParticle>& particlePool);

    void SetTypeColor(f32 r, f32 g, f32 b, f32 a, FluidType type);

    void SetTypeGraphics(AEGfxVertexList* mesh_, AEGfxTexture* texture_, u32 layer_,
                         FluidType type);
};
