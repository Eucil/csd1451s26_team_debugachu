#pragma once
#include "AEEngine.h"
#include "Components.h"
#include <vector>
enum class FluidType { Water, Lava, Count };

struct FluidParticle {

    // ----------------------------- Components ----------------------------- //

    Transform transform_; //  <--- posX, posY, scaleX, scaleY, rotA, worldMtx

    RigidBody2D rgBody2D_; //  <--- mass, velocity, acceleration, forces, gravityScale

    FluidType type_; //  <--- can be water, lava, wtv

    // --------------------- Constructors / Destructors --------------------- //

    //* 1.   Default Particle
    FluidParticle(f32 posX, f32 posY, FluidType type);

    //* 2.   Custom Particle
    FluidParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, FluidType type);
};

class FluidSystem {
private:
    // ----------------------------- Components ----------------------------- //

    // particles[0] holds Water, particles[1] holds Lava, etc.
    std::vector<FluidParticle>
        particlePools_[(int)FluidType::Count]; //  <--- stores all live particles

    // graphic configs
    Graphics graphicsConfigs_[(int)FluidType::Count]; // <--- mesh, texture, layer

    // colour configs (r,g,b,alpha)
    f32 colorConfigs_[(int)FluidType::Count][4]; // [Type][RGBA]

public:
    // --------------------- Constructors / Destructors --------------------- //

    // ------------------------- Basic Methods --------------------------- //

    void Initialize();

    void UpdateTransforms(std::vector<FluidParticle>& particlePool);

    void UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt);

    void UpdateMain(f32 dt);

    void DrawColor(); //  <---    draws the mesh using the mesh's color

    void DrawTexture(); //  <---    draws the mesh using a loaded texture

    void Free();

    // ------------------------- Setter / Getter Methods --------------------------- //

    // usage: fluidSys1.SetTypeColor(FluidType::Water, 0.0f, 0.0f, 1.0f, 1.0f);
    void SetTypeColor(FluidType type, f32 r, f32 g, f32 b, f32 a);

    // ------------------------- Utility Methods --------------------------- //
    void SpawnParticle_d(f32 posX, f32 posY, FluidType type);

    void SpawnParticle_c(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, FluidType type);

    int GetParticleCount(FluidType type);
};
