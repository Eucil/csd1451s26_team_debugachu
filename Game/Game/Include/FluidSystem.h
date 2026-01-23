#pragma once
#include "AEEngine.h"
#include "Components.h"
#include <vector>
enum class FluidType { Water, Lava };

struct FluidParticle {

    // ----------------------------- Components ----------------------------- //

    Transform transform_; //  <--- posX, posY, scaleX, scaleY, rotA, worldMtx

    RigidBody2D rgBody2D_; //  <--- mass, velocity, acceleration, forces, gravityScale

    FluidType type_; //  <--- can be water, lava, wtv

    // --------------------- Constructors / Destructors --------------------- //

    //* 1.   Default (Water)
    FluidParticle();

    //* 2.   Custom Particle
    FluidParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, FluidType type);
};

class FluidSystem {
private:
    // ----------------------------- Components ----------------------------- //

    std::vector<FluidParticle> particles; //  <--- stores all live particles

    Graphics waterGraphics_; //  <--- mesh, texture, layer
    f32 waterColor_[4];

    // Graphics lavaGraphics_;
    // f32 lavaColor_[4];

public:
    // --------------------- Constructors / Destructors --------------------- //

    // ------------------------- Basic Methods --------------------------- //

    void Initialize();

    void Update();

    void ApplyGraphics();

    void DrawColor(); //  <---    draws the mesh using the mesh's color

    void DrawTexture(); //  <---    draws the mesh using a loaded texture

    // ------------------------- Utility Methods --------------------------- //
    void SetMesh(AEGfxVertexList* pMesh);

    void SetWaterColor(f32 r, f32 g, f32 b, f32 a);

    void SetTexture() {}
    // ------------------------- Game Methods --------------------------- //
    void SpawnParticle();
};
