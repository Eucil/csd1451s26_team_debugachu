#pragma once
#include "AEEngine.h"
#include "Components.h"
#include <vector>

// Enum class for fluid types (1 byte is enough for 256 types)
enum class FluidType : uint8_t 
{ 
    Water,
    Lava,
    Count 
}; 


//** figure out how to make this 64 bytes idk for fun so that it fits in cache lines better
struct FluidParticle {

    // ----------------------------- Components ----------------------------- //

    Transform transform_; //  <--- posX, posY, scaleX, scaleY, rotA, worldMtx

    RigidBody2D physics_; //  <--- mass, gravity, drag, veloX, veloY

    FluidType type_; //  <--- water, lava, etc

    // --------------------- Constructors / Destructors --------------------- //

    //* 1.   Default Particle (Water)
    FluidParticle(f32 posX, f32 posY, FluidType type);

    //* 2.   Lava Particle
    //  make a constructor that takes in new velocity,mass etc for lava particles
   
    //* 3.   Custom Particle
    FluidParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, FluidType type);
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

    //  config values for  posX, posY, scaleX, scaleY, velocityX, velocityY, rotRad
    //  f32 transformConfigs_[(int)FluidType::Count][7]; <---- DO I EVEN NEED THIS

    // physics configs for mass, gravity, drag, velocityX, velocityY

    //** CONVERT ALL TS TO READ CONFIGS FROM TEXT FILES

public:
    // --------------------- Constructors / Destructors --------------------- //

    // ------------------------- Basic Methods --------------------------- //

    void InitializeMesh();

    void Initialize();

    void UpdateTransforms(std::vector<FluidParticle>& particlePool);

    void UpdateCollision();

    void UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt, FluidType type);

    void UpdateMain(f32 dt);

    void DrawColor(); //  <---    draws the mesh using the mesh's color

    void DrawTexture(); //  <---    draws the mesh using a loaded texture

    void Free();

    // ------------------------- Setter / Getter Methods --------------------------- //

    // usage: fluidSys1.SetTypeColor(FluidType::Water, 0.0f, 0.0f, 1.0f, 1.0f);
    void SetTypeColor(f32 r, f32 g, f32 b, f32 a, FluidType type);

    //
    void SetTypePhysics(f32 mass, f32 gravity, FluidType type);

    // ------------------------- Utility Methods --------------------------- //

    void SpawnParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot,
                         FluidType type);

    int GetParticleCount(FluidType type);
};
