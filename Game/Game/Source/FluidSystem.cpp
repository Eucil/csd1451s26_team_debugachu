/*!
@file       FluidSystem.cpp
@author     Chia Hanxin/c.hanxin@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the definitions of functions and classes
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

// ==========================================
//               Includes
// ==========================================

// FluidSystem.h
#include "FluidSystem.h"

// Standard library
#include <cmath>
#include <iostream>

// Third-party
#include <AEEngine.h>

// Project
#include "CollisionSystem.h"
#include "ConfigManager.h"
#include "MeshUtils.h"

// ==========================================
//               FluidParticle
// ==========================================

// =========================================================
//
// Constructor for a single instance of a FluidParticle
// - Sets pos,scale,rot,collidershape,data
//
// - physics and graphics ccomponents are set by FluidSystem
//
// =========================================================
FluidParticle::FluidParticle(f32 posX, f32 posY, f32 radius, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {radius * 2.0f,
                         radius * 2.0f}; // Multiply by 2.0f as scale represents diameter
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius_ =
        radius * 0.6f; // * 0.6f so that collider is smaller than mesh

    type_ = type;
}

// ==========================================
//                 FluidSystem
// ==========================================

// =================================================================
//
//  FluidSystem's graphicConfigs_ Initialization function
//
//  Initializes the graphicConfigs_ data member which contains
//  graphic configs for every particle type within
//  FluidSystem
//
// =================================================================
void FluidSystem::initializeGraphics(AEGfxVertexList* mesh, AEGfxTexture* texture, u32 layer,
                                     f32 red, f32 green, f32 blue, f32 alpha, FluidType type,
                                     u32 graphicsIndex) {
    size_t fluidIndex = static_cast<size_t>(type);

    graphicsConfigs_[fluidIndex][graphicsIndex].mesh_ = mesh;
    graphicsConfigs_[fluidIndex][graphicsIndex].texture_ = texture;
    graphicsConfigs_[fluidIndex][graphicsIndex].layer_ = layer;
    graphicsConfigs_[fluidIndex][graphicsIndex].red_ = red;
    graphicsConfigs_[fluidIndex][graphicsIndex].blue_ = blue;
    graphicsConfigs_[fluidIndex][graphicsIndex].green_ = green;
    graphicsConfigs_[fluidIndex][graphicsIndex].alpha_ = alpha;
}

// =================================================================
//
//  FluidSystem's physicsConfigs_ Initialization function
//
//  Initializes the physicsConfigs_ data member which contains
//  physics configs for every particle type within
//  FluidSystem
//
// =================================================================
void FluidSystem::initializePhysics(f32 mass, f32 gravity, AEVec2 velocity, FluidType type) {
    size_t fluidIndex = static_cast<size_t>(type);

    physicsConfigs_[fluidIndex].mass_ = mass;
    physicsConfigs_[fluidIndex].gravity_ = gravity;
    physicsConfigs_[fluidIndex].velocity_ = velocity;
}

// =================================================================
//
//  FluidSystem's Main Initialization function
//
// Initializes a single instance of a FluidSystem
// - Initializes both graphics and physics of every FluidParticle type
//
// =================================================================
void FluidSystem::initialize() {
    // Reduces memory reallocation
    int typeCount{static_cast<int>(FluidType::Count)};
    for (int i{0}; i < typeCount; i++) {
        particlePools_[i].reserve(1000);
    }

    initializePhysics(
        g_configManager.getFloat("FluidSystem", "Water", "mass", 1.0f),
        g_configManager.getFloat("FluidSystem", "Water", "gravity", -500.0f),
        g_configManager.getAEVec2("FluidSystem", "Water", "velocity", AEVec2{0.0f, 0.0f}),
        FluidType::Water);
    initializePhysics(
        g_configManager.getFloat("FluidSystem", "Lava", "mass", 1.0f),
        g_configManager.getFloat("FluidSystem", "Lava", "gravity", -200.0f),
        g_configManager.getAEVec2("FluidSystem", "Lava", "velocity", AEVec2{0.0f, 0.0f}),
        FluidType::Lava);

    // Initialize graphics for each fluid type
    // 3 Layers per particle to make our particles look more like water visually. (white, light
    // blue, dark blue)
    initializeGraphics(createCircleMesh(10, 0.5f), nullptr, 2, 1.0f, 1.0f, 1.0f, 1.0f,
                       FluidType::Water, 0);
    initializeGraphics(createCircleMesh(10, 0.47f), nullptr, 2, 0.4f, 0.7f, 1.0f, 1.0f,
                       FluidType::Water, 1);
    initializeGraphics(createCircleMesh(10, 0.4f), nullptr, 2, 0.0f, 0.5f, 1.0f, 1.0f,
                       FluidType::Water, 2);

    initializeGraphics(createCircleMesh(10, 0.5f), nullptr, 2, 1.0f, 0.2f, 0.0f, 1.0f,
                       FluidType::Lava, 0);
    initializeGraphics(createCircleMesh(10, 0.45f), nullptr, 2, 1.0f, 0.2f, 0.0f, 1.0f,
                       FluidType::Lava, 1);
    initializeGraphics(createCircleMesh(10, 0.4f), nullptr, 2, 1.0f, 0.2f, 0.0f, 1.0f,
                       FluidType::Lava, 2);
}

// =================================================================
//
//  FluidSystem's Transform matrices Update function
//
// - Updates the transform matrices for every particle in an inputted
//   particle pool.
//
// =================================================================
void FluidSystem::updateTransforms(std::vector<FluidParticle>& particlePool) {

    for (auto& p : particlePool) {

        AEMtx33 scale, rot, trans;

        AEMtx33Scale(&scale, p.transform_.scale_.x, p.transform_.scale_.y);
        AEMtx33Rot(&rot, p.transform_.rotationRad_);
        AEMtx33Trans(&trans, p.transform_.pos_.x, p.transform_.pos_.y);

        // worldMtx = trans * rot * scale
        AEMtx33Concat(&p.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&p.transform_.worldMtx_, &trans, &p.transform_.worldMtx_);
    }
}

// =========================================================
//
//  FluidSystem's physics Update function
//
// Updates physics parameters, velocity and position for an inputted particle pool,
// applies gravity to particles and contains various optimisations to maximise performance.
//
// The list of optimisations include:
// - Applies gravity and clamps delta time for consistency / prevent tunneling
// - Enforces terminal velocity and horizontal speed caps to prevent tunneling
// - Halts extremely slow-moving particles to optimize performance
// - Applies anti-oscillation noise to nearly-stopped particles
// - Updates final particle positions based on calculated velocities
//
// =========================================================
void FluidSystem::updatePhysics(std::vector<FluidParticle>& particlePool, f32 dt) {

    if (dt > 0.0166667f) {
        dt = 0.0166667f;
    }
    //  load new constants with the previously set config values
    f32 gravity = particlePool[0].physics_.gravity_;

    // Set maximum allowed speed
    const f32 kMaxSpeed = 800.0f;
    const f32 kMaxSpeedSq = kMaxSpeed * kMaxSpeed;

    for (auto& p : particlePool) {

        // ================================================ //
        // Gravity
        // ================================================ //
        //
        // Applies gravity to the particle's velocity.
        // We multiply by dt to make the simulation frame rate independent, then update position
        p.physics_.velocity_.y += gravity * dt;

        // ================================================ //
        // 1. Optimisation: CAP MAXIMUM SPEED (FOR FAST PARTICLES)
        // ================================================ //
        // Without this, particles falling from a height can reach very high speeds (negative
        // velocity), which can cause them to tunnel through terrain colliders.
        //
        // This caps negative velocity at -500.0f vertically and 300.0f horizontally
        const f32 kTerminalFallSpeed = -500.0f;
        if (p.physics_.velocity_.y < kTerminalFallSpeed) {
            p.physics_.velocity_.y = kTerminalFallSpeed;
        }

        // Same cap applied horizontally to prevent tunneling from high horizontal speeds (e.g. from
        // being pushed by a fast-moving floor or explosion).
        const f32 kMaxHorizontalSpeed = 300.0f;
        if (p.physics_.velocity_.x > kMaxHorizontalSpeed)
            p.physics_.velocity_.x = kMaxHorizontalSpeed;
        if (p.physics_.velocity_.x < -kMaxHorizontalSpeed)
            p.physics_.velocity_.x = -kMaxHorizontalSpeed;

        // ================================================ //
        // 2. Optimisation: STOPS VERY SLOW PARTICLES
        // ================================================ //
        // Lowered from 1.41*1.41 (~2.0) to 0.5 so slow-moving particles are not
        // immediately zeroed. The original threshold was killing horizontal flow
        // since particles sliding along terrain move slowly.
        f32 thresholdVel = 0.5f;
        f32 currentSpeedSq = (p.physics_.velocity_.x * p.physics_.velocity_.x) +
                             (p.physics_.velocity_.y * p.physics_.velocity_.y);

        // If currentSpeedSq is lower than 0.5f, we are moving very slowly and can stop to save
        // performance.
        if (currentSpeedSq < thresholdVel) {
            p.physics_.velocity_.x = 0.0f;
            p.physics_.velocity_.y = 0.0f;
        }

        // ANTI-OSCILLATION: Only apply noise to nearly-stopped particles.
        // Previously noise ran on every particle every substep: in dense settled
        // groups this caused all particles to randomly oscillate together, creating
        // the vigorous left-right swinging behaviour.
        // Now noise only fires when a particle is nearly still (speed < ~2.2 units/s)
        // to break perfect vertical stacking, leaving actively moving particles alone.
        if (currentSpeedSq < 5.0f) {
            f32 noiseX = ((rand() % 100) / 50.0f) - 1.0f;
            f32 noiseY = ((rand() % 100) / 50.0f) - 1.0f;
            p.physics_.velocity_.x += noiseX * dt * 3.0f;
            p.physics_.velocity_.y += noiseY * dt * 3.0f;
        }

        currentSpeedSq = (p.physics_.velocity_.x * p.physics_.velocity_.x) +
                         (p.physics_.velocity_.y * p.physics_.velocity_.y);

        // Prevents compounded velocity from UpdateCollision from pushing particles to extreme
        // speeds that can cause tunneling.
        // ================================================ //
        // 3. Optimisation: CAP MAXIMUM SPEED (again)
        // ================================================ //

        // Acts as a final emergency safety net to prevent particles from moving too fast after the
        // various optimisations above.
        if (currentSpeedSq > kMaxSpeedSq) {
            // If currentSpeedSq > kMaxSpeed, we are going too fast.
            // Calculate actual speed to normalize.
            f32 actualSpeed = std::sqrt(currentSpeedSq);

            // Normalize and multiply by our hard speed limit to get a normalized direction vector
            // with capped speed.
            p.physics_.velocity_.x = (p.physics_.velocity_.x / actualSpeed) * kMaxSpeed;
            p.physics_.velocity_.y = (p.physics_.velocity_.y / actualSpeed) * kMaxSpeed;
        }

        // ================================================ //
        // UPDATE POSITION (MUST BE LAST)
        // ================================================ /
        // Updates Position after UpdateCollision and main physics calculations within UpdatePhysics
        // has been done.
        p.transform_.pos_.x += p.physics_.velocity_.x * dt;
        p.transform_.pos_.y += p.physics_.velocity_.y * dt;
    }
}

// =========================================================
//
// FluidSystem's portal invincibility frames Update function
//
// Manages portal teleportation cooldowns for fluid particles
// - Decrements the iframe timer for particles that recently teleported
// - Re-enables portal interaction once the timer reaches zero
//
// =========================================================
void FluidSystem::updatePortalIframes(f32 dt, std::vector<FluidParticle>& particlePool) {
    // Loop through all particles in the current pool
    for (auto& p : particlePool) {
        // If the particle is in iframe, reduce the iframe timer
        if (p.portalIframe_) {
            p.portalIframeTimer_ -= dt;
            // If the timer reaches zero, disable iframe
            if (p.portalIframeTimer_ <= 0.0f) {
                p.portalIframe_ = false;
                p.portalIframeTimer_ = p.portalIframeMaxduration_;
            }
        }
    }
}

// =========================================================
//
//  FluidSystem's Main Update function
//
// Main update loop for the fluid simulation
// - Divides the frame delta time into smaller substeps for physics stability
// - Updates particle physics and processes terrain collisions per substep
// - Updates final graphical transforms and portal iframes once per frame
//
// =========================================================
void FluidSystem::update(f32 dt, std::initializer_list<Terrain*> terrains) {
    // DT clamp
    if (dt > 0.016f && dt < 0.016f * 5.0f) {
        dt = 0.016f;
    }

    // Substeps
    const int subSteps = 4;
    const f32 subDt = dt / (f32)subSteps;

    for (int s = 0; s < subSteps; s++) {

        for (int i = 0; i < (int)FluidType::Count; i++) {
            if (particlePools_[i].empty())
                continue;

            // Physics
            updatePhysics(particlePools_[i], subDt);
        }

        // Collision (there are more substeps within the function below)
        for (Terrain* terrain : terrains) {
            CollisionSystem::terrainToFluidCollision(*terrain, *this, subDt);
        }
    }

    // Final per-frame updates
    for (int i = 0; i < (int)FluidType::Count; i++) {
        if (particlePools_[i].empty()) {
            continue;
        }
        updateTransforms(particlePools_[i]);
        updatePortalIframes(dt, particlePools_[i]);
    }
}

// =========================================================
//
//  FluidSystem's draw color function
//
// Renders all active fluid particles using solid colors
// - Sets graphics engine to color render mode and enables blending
// - Iterates through fluid types and draws up to 3 layered meshes per particle
// - Applies specific RGBA multipliers to tint the fluid layers
//
// =========================================================
void FluidSystem::drawColor() {

    // color render mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Loops through (0) Water, (1) Lava, ...
    for (int i = 0; i < (int)FluidType::Count; ++i) {

        // if particle pool is empty, completely skip this pool
        if (particlePools_[i].empty()) {
            continue;
        }

        // Loop through each graphics component in each fluid type
        for (u32 j{0}; j < 3; ++j) {
            AEGfxSetColorToMultiply(graphicsConfigs_[i][j].red_, graphicsConfigs_[i][j].green_,
                                    graphicsConfigs_[i][j].blue_, graphicsConfigs_[i][j].alpha_);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);

            // draw according to the particles' transform matrix
            for (auto& p : particlePools_[i]) { // <-- p = current particle being rendered
                AEGfxSetTransform(p.transform_.worldMtx_.m);
                AEGfxMeshDraw(graphicsConfigs_[i][j].mesh_, AE_GFX_MDM_TRIANGLES);
            }
        }
    }
}

// =========================================================
//
//  FluidSystem's draw texture function
//
// Renders active fluid particles using textures
// - Sets graphics engine to texture render mode and enables blending
// - Binds the appropriate texture for each of the 3 graphical layers
// - Applies color tints and draws the textured meshes for each particle
//
// =========================================================
void FluidSystem::drawTexture() {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // Loops through (0) Water, (1) Lava, ...
    for (size_t fluidIndex{0}; fluidIndex < static_cast<size_t>(FluidType::Count); ++fluidIndex) {
        // if particle pool is empty, completely skip this pool
        if (particlePools_[fluidIndex].empty()) {
            continue;
        }

        // Loop through each graphics component in each fluid type
        for (u32 i{0}; i < 3; ++i) {
            AEGfxTextureSet(graphicsConfigs_[fluidIndex][i].texture_, 0, 0);

            AEGfxSetColorToMultiply(
                graphicsConfigs_[fluidIndex][i].red_, graphicsConfigs_[fluidIndex][i].green_,
                graphicsConfigs_[fluidIndex][i].blue_, graphicsConfigs_[fluidIndex][i].alpha_);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);

            // draw according to the particles' transform matrix
            for (auto& p : particlePools_[fluidIndex]) { // <-- p = current particle being rendered
                AEGfxSetTransform(p.transform_.worldMtx_.m);
                AEGfxMeshDraw(graphicsConfigs_[fluidIndex][i].mesh_, AE_GFX_MDM_TRIANGLES);
            }
        }
    }
}

// =========================================================
//
//  FluidSystem's free function
//
// Frees memory and resources utilized by the FluidSystem
// - Destroys allocated graphical meshes and nullifies pointers
// - Unloads textures from the graphics engine
// - Clears all fluid particle pools
//
// =========================================================
void FluidSystem::free() {
    // Free all fluid meshes
    for (size_t fluidIndex{0}; fluidIndex < static_cast<size_t>(FluidType::Count); ++fluidIndex) {
        for (size_t i{0}; i < 3; ++i) {
            if (graphicsConfigs_[fluidIndex][i].mesh_ != nullptr) {
                AEGfxMeshFree(graphicsConfigs_[fluidIndex][i].mesh_);

                // Nullify all mesh pointers so we don't accidentally use freed memory
                graphicsConfigs_[fluidIndex][i].mesh_ = nullptr;
            }
        }
    }

    // Free textures
    for (size_t fluidIndex{0}; fluidIndex < static_cast<size_t>(FluidType::Count); ++fluidIndex) {
        for (size_t i{0}; i < 3; ++i) {
            if (graphicsConfigs_[fluidIndex][i].texture_ != nullptr) {
                AEGfxTextureUnload(graphicsConfigs_[fluidIndex][i]
                                       .texture_); // Or AEGfxTextureFree depending on version
                graphicsConfigs_[fluidIndex][i].texture_ = nullptr;
            }
        }
    }

    // Empty the particle pool
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        particlePools_[i].clear();
    }
}

// =========================================================
//
//  FluidSystem's particle spawning utility function
//
// Instantiates and spawns a new fluid particle
// - Creates a particle with the specified position, size, and type
// - Assigns the appropriate pre-initialized physics configuration
// - Pushes the new particle into the corresponding fluid pool
//
// =========================================================
void FluidSystem::spawnParticle(f32 posX, f32 posY, f32 radius, FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, radius, type);
    newParticle.physics_ = physicsConfigs_[i];
    particlePools_[i].push_back(newParticle);
}

// =========================================================
//
// Fluidsystem's particle count getter function
//
// Retrieves the current number of active particles for a fluid type
// - Returns the size of the specified fluid's particle pool
//
// =========================================================
u32 FluidSystem::getParticleCount(FluidType type) {
    return static_cast<u32>(particlePools_[(u32)type].size());
}

// =========================================================
//
//  Fluidsystem's particle pool getter function
//
// Retrieves a reference to a specific fluid type's particle pool
// - Allows external systems to read or modify the active particles
//
// =========================================================
std::vector<FluidParticle>& FluidSystem::getParticlePool(FluidType type) {
    return particlePools_[(int)type];
}
