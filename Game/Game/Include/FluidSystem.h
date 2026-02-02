#pragma once
#include "AEEngine.h"
#include "Components.h"
#include <vector>

enum class FluidType {
    // Allignment: 4 bytes
    // Size: 4 bytes
    Water,
    Lava,
    Count
};

struct FluidParticle {

    // ----------------------------- Components ----------------------------- //
    // Allignment: 4 bytes
    // Size: 80 bytes

    Transform transform_; //  <--- posX, posY, scaleX, scaleY, rotA, worldMtx

    RigidBody2D physics_; //  <--- mass, gravity, drag, veloX, veloY

    Collider2D collider_; //  <--- radius

    FluidType
        type_; //  <--- water, lava, etc
               // currently only being used for identifying the particle type for setters/getters

    bool portal_iframe_{false};            // <--- to prevent immediate re-teleportation
    f32 portal_iframe_timer_{0.15f};       // <--- timer for portal iframe
    f32 portal_iframe_maxduration_{0.15f}; // <--- duration of portal iframe in seconds

    // --------------------- Constructors / Destructors --------------------- //

    // NOTE: There is NO WAY to change any of the values in the components
    // so set it properly within the constructor!!!!

    //* 1.   Lava Particle
    //  make a constructor that takes in new velocity,mass etc for lava particles

    //* 2.   Custom Particle
    FluidParticle(f32 posX, f32 posY, f32 radius, FluidType type);
};

class FluidSystem {
private:
    // ----------------------------- Components ----------------------------- //

    // particles[0] holds Water, particles[1] holds Lava, etc.
    std::vector<FluidParticle>
        particlePools_[static_cast<int>(FluidType::Count)]; //  <--- stores all live particles

    // graphic configs for each particle type
    Graphics graphicsConfigs_[static_cast<int>(FluidType::Count)]; // <--- mesh, texture, layer

    // colour configs (r,g,b,alpha)
    f32 colorConfigs_[static_cast<int>(FluidType::Count)][4]; // [Type][RGBA]

    // physics configs for mass, gravity, drag, velocityX, velocityY

    //** CONVERT ALL TS TO READ CONFIGS FROM TEXT FILES

public:
    s32 particleMaxCount{300};
    // --------------------- Constructors / Destructors --------------------- //

    // ------------------------- Basic Methods --------------------------- //

    void InitializeMesh();

    void Initialize();

    void UpdateTransforms(std::vector<FluidParticle>& particlePool);

    void UpdateCollision(std::vector<FluidParticle>& particlePool, f32 dt);

    void UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt);

    void UpdatePortalIframes(f32 dt, std::vector<FluidParticle>& particlePool);

    void UpdateMain(f32 dt);

    void DrawColor(); //  <---    draws the mesh using the mesh's color

    void DrawTexture(); //  <---    draws the mesh using a loaded texture

    void Free();

    // ------------------------- Setter / Getter Methods --------------------------- //

    // usage: fluidSys1.SetTypeColor(FluidType::Water, 0.0f, 0.0f, 1.0f, 1.0f);
    void SetTypeColor(f32 r, f32 g, f32 b, f32 a, FluidType type);

    void SetTypeGraphics(AEGfxVertexList* mesh_, AEGfxTexture* texture_, u32 layer_,
                         FluidType type);

    // ------------------------- Utility Methods --------------------------- //
    void SpawnParticle(f32 posX, f32 posY, f32 radius, FluidType type);

    int GetParticleCount(FluidType type);

    std::vector<FluidParticle>& GetParticlePool(FluidType type);
};
