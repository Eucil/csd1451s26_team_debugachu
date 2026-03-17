#pragma once
#include "AEEngine.h"
#include "Components.h"
#include "Terrain.h"
#include <initializer_list>
#include <vector>

enum class FluidType { Water, Lava, Count };

struct FluidParticle {
    Transform transform_;
    RigidBody2D physics_;
    Collider2D collider_;

    FluidType
        type_; //  <--- water, lava, etc
               // currently only being used for identifying the particle type for setters/getters

    bool portal_iframe_{false};            // <--- to prevent immediate re-teleportation
    f32 portal_iframe_timer_{0.15f};       // <--- timer for portal iframe
    f32 portal_iframe_maxduration_{0.15f}; // <--- duration of portal iframe in seconds

    FluidParticle(f32 posX, f32 posY, f32 radius, FluidType type);
};

class FluidSystem {
public:
    u32 particleMaxCount{300};

    void Initialize();

    void Update(f32 dt, Terrain& terrain);

    void Update(f32 dt, std::initializer_list<Terrain*> terrains);

    void DrawColor();

    void DrawTexture();

    void Free();

    void SpawnParticle(f32 posX, f32 posY, f32 radius, FluidType type);

    u32 GetParticleCount(FluidType type);

    std::vector<FluidParticle>& GetParticlePool(FluidType type);

private:
    // particles[0] holds Water, particles[1] holds Lava, etc.
    // Stores live particles
    std::vector<FluidParticle> particlePools_[static_cast<int>(FluidType::Count)];

    // Each fluid will have 3 graphics components
    Graphics graphicsConfigs_[static_cast<int>(FluidType::Count)][3];

    RigidBody2D physicsConfigs_[static_cast<int>(FluidType::Count)];

    void InitializeGraphics(AEGfxVertexList* mesh_, AEGfxTexture* texture_, u32 layer_, f32 red,
                            f32 green, f32 blue, f32 alpha, FluidType type, u32 graphicsIndex);

    void InitializePhysics(f32 mass, f32 gravity, AEVec2 velocity, FluidType type);

    void UpdateTransforms(std::vector<FluidParticle>& particlePool);

    void UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt);

    void UpdatePortalIframes(f32 dt, std::vector<FluidParticle>& particlePool);
};
