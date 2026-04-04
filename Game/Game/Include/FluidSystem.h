/*!
@file       FluidSystem.h
@author     Chia Hanxin/c.hanxin@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declarations of functions and classes
            for the fluid simulation system which includes the following:

                - FluidType, an enumeration for identifying different fluid
                  materials such as Water and Lava.
                - FluidParticle, a struct representing an individual particle
                  with transform, physics, and collider components.
                - FluidSystem, a manager class that handles the initialization,
                  spawning, physics updates, and rendering for all active
                  fluid particle pools.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <initializer_list>
#include <vector>

// Third-party
#include <AEEngine.h>

// Project
#include "Components.h"
#include "Terrain.h"

enum class FluidType { Water, Lava, Count };

struct FluidParticle {
    Transform transform_;
    RigidBody2D physics_;
    Collider2D collider_;

    FluidType
        type_; //  <--- water, lava, etc
               // currently only being used for identifying the particle type for setters/getters

    bool portalIframe_{false};           // <--- to prevent immediate re-teleportation
    f32 portalIframeTimer_{0.15f};       // <--- timer for portal iframe
    f32 portalIframeMaxduration_{0.15f}; // <--- duration of portal iframe in seconds

    FluidParticle(f32 posX, f32 posY, f32 radius, FluidType type);
};

class FluidSystem {
public:
    u32 particleMaxCount_{300};

    void initialize();

    void update(f32 dt, std::initializer_list<Terrain*> terrains);

    void drawColor();

    void drawTexture();

    void free();

    void spawnParticle(f32 posX, f32 posY, f32 radius, FluidType type);

    u32 getParticleCount(FluidType type);

    std::vector<FluidParticle>& getParticlePool(FluidType type);

private:
    // particles[0] holds Water, particles[1] holds Lava, etc.
    // Stores live particles
    std::vector<FluidParticle> particlePools_[static_cast<int>(FluidType::Count)];

    // Each fluid will have 3 graphics components
    Graphics graphicsConfigs_[static_cast<int>(FluidType::Count)][3];

    RigidBody2D physicsConfigs_[static_cast<int>(FluidType::Count)];

    void initializeGraphics(AEGfxVertexList* mesh_, AEGfxTexture* texture_, u32 layer_, f32 red,
                            f32 green, f32 blue, f32 alpha, FluidType type, u32 graphicsIndex);

    void initializePhysics(f32 mass, f32 gravity, AEVec2 velocity, FluidType type);

    void updateTransforms(std::vector<FluidParticle>& particlePool);

    void updatePhysics(std::vector<FluidParticle>& particlePool, f32 dt);

    void updatePortalIframes(f32 dt, std::vector<FluidParticle>& particlePool);
};
