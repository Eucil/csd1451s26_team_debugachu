#include "AEEngine.h"

#include "CollisionSystem.h"
#include "FluidSystem.h"
#include "Utils.h"

#include <cmath>
#include <iostream>

// ==========================================
// FluidParticle
// ==========================================
// @todo incomplete, should set physics as well
FluidParticle::FluidParticle(f32 posX, f32 posY, f32 radius, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {radius * 2.0f,
                         radius * 2.0f}; // Multiply by 2.0f as scale represents diameter
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius =
        radius * 0.6f; // * 0.6f so that collider is smaller than mesh

    type_ = type;
}

// ==========================================
// FluidSystem
// ==========================================

void FluidSystem::InitializeGraphics(AEGfxVertexList* mesh, AEGfxTexture* texture, u32 layer,
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

void FluidSystem::InitializePhysics(f32 mass, f32 gravity, AEVec2 velocity, FluidType type) {
    size_t fluidIndex = static_cast<size_t>(type);

    physicsConfigs_[fluidIndex].mass_ = mass;
    physicsConfigs_[fluidIndex].gravity_ = gravity;
    physicsConfigs_[fluidIndex].velocity_ = velocity;
}

void FluidSystem::Initialize() {
    // Reduces memory reallocation
    int typeCount{static_cast<int>(FluidType::Count)};
    for (int i{0}; i < typeCount; i++) {
        particlePools_[i].reserve(1000);
    }

    // Initialize physics for each fluid type
    InitializePhysics(1.0f, -500.0f, {0.0f, 0.0f}, FluidType::Water);
    InitializePhysics(1.0f, -200.0f, {0.0f, 0.0f}, FluidType::Lava);

    // Initialize graphics for each fluid type
    InitializeGraphics(CreateCircleMesh(10, 0.5f), nullptr, 2, 1.0f, 1.0f, 1.0f, 1.0f,
                       FluidType::Water, 0);
    InitializeGraphics(CreateCircleMesh(10, 0.47f), nullptr, 2, 0.4f, 0.7f, 1.0f, 1.0f,
                       FluidType::Water, 1);
    InitializeGraphics(CreateCircleMesh(10, 0.4f), nullptr, 2, 0.0f, 0.5f, 1.0f, 1.0f,
                       FluidType::Water, 2);

    InitializeGraphics(CreateCircleMesh(10, 0.5f), nullptr, 2, 1.0f, 0.2f, 0.0f, 1.0f,
                       FluidType::Lava, 0);
    InitializeGraphics(CreateCircleMesh(10, 0.45f), nullptr, 2, 1.0f, 0.2f, 0.0f, 1.0f,
                       FluidType::Lava, 1);
    InitializeGraphics(CreateCircleMesh(10, 0.4f), nullptr, 2, 1.0f, 0.2f, 0.0f, 1.0f,
                       FluidType::Lava, 2);
}

void FluidSystem::UpdateTransforms(std::vector<FluidParticle>& particlePool) {

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

void FluidSystem::UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt) {

    //  load new constants with config values
    f32 mass = particlePool[0].physics_.mass_;
    f32 gravity = particlePool[0].physics_.gravity_;

    for (auto& p : particlePool) {

        // ================================================ //
        // EFFECT 1: Gravity
        // ================================================ //
        //
        // Applies gravity to the particle's velocity.
        // We multiply by dt to make the simulation frame rate independent, then update position
        p.physics_.velocity_.y += gravity * dt;

        // Add a tiny random kick to every particle.
        // This prevents them from ever stacking perfectly still.
        f32 noiseX = ((rand() % 100) / 50.0f) - 1.0f; // Range -1.0 to 1.0
        f32 noiseY = ((rand() % 100) / 50.0f) - 1.0f; // Range -1.0 to 1.0

        p.physics_.velocity_.x += noiseX * dt * 3.0f;
        p.physics_.velocity_.y += noiseY * dt * 3.0f;

        // GLOBAL FUNCTION
        // Updates Position after UpdateCollision and main physics calculations within UpdatePhysics
        // has been done.
        p.transform_.pos_.x += p.physics_.velocity_.x * dt;
        p.transform_.pos_.y += p.physics_.velocity_.y * dt;

        // ================================================ //
        // OPTIMISATION: STOPS VERY SLOW PARTICLES
        // ================================================ //
        //
        // NOTE: 1.99f is a MAGIC NUMBER, it was chosen coz if the particle is moving at less than
        // (1 pixel, 1 pixel) velocity, it is considered miniscule.
        //
        // (sqrt(1.0f + 1.0f) ) ^ 2
        f32 thresholdVel = 1.41f * 1.41f;

        if ((p.physics_.velocity_.x * p.physics_.velocity_.x) +
                (p.physics_.velocity_.y * p.physics_.velocity_.y) <
            1.99f) {
            p.physics_.velocity_.x = 0.0f;
            p.physics_.velocity_.y = 0.0f;
        }
    }
}

void FluidSystem::UpdatePortalIframes(f32 dt, std::vector<FluidParticle>& particlePool) {
    // Loop through all particles in the current pool
    for (auto& p : particlePool) {
        // If the particle is in iframe, reduce the iframe timer
        if (p.portal_iframe_) {
            p.portal_iframe_timer_ -= dt;
            // If the timer reaches zero, disable iframe
            if (p.portal_iframe_timer_ <= 0.0f) {
                p.portal_iframe_ = false;
                p.portal_iframe_timer_ = p.portal_iframe_maxduration_;
            }
        }
    }
}

void FluidSystem::Update(f32 dt, Terrain& terrain) {
    // DT clamp
    if (dt > 0.016f) {
        dt = 0.016f;
    }

    // Substeps
    const int subSteps = 4;
    const f32 subDt = dt / (f32)subSteps;

    for (int s = 0; s < subSteps; s++) {

        for (int i = 0; i < (int)FluidType::Count; i++) {
            if (particlePools_[i].empty())
                continue;

            // Physicss
            UpdatePhysics(particlePools_[i], subDt);
        }

        // Collision
        CollisionSystem::terrainToFluidCollision(terrain, *this);
    }

    // Final per-frame updates
    for (int i = 0; i < (int)FluidType::Count; i++) {
        if (particlePools_[i].empty()) {
            continue;
        }
        UpdateTransforms(particlePools_[i]);
        UpdatePortalIframes(dt, particlePools_[i]);
    }
}

void FluidSystem::DrawColor() {

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

void FluidSystem::DrawTexture() {
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

void FluidSystem::Free() {
    // Free all fluid meshes
    for (size_t fluidIndex{0}; fluidIndex < static_cast<size_t>(FluidType::Count); ++fluidIndex) {
        for (size_t i{0}; i < 3; ++i) {
            std::cout << graphicsConfigs_[fluidIndex][i].mesh_ << std::endl;
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

void FluidSystem::SpawnParticle(f32 posX, f32 posY, f32 radius, FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, radius, type);
    newParticle.physics_ = physicsConfigs_[i];
    particlePools_[i].push_back(newParticle);
}

u32 FluidSystem::GetParticleCount(FluidType type) { return particlePools_[(u32)type].size(); }

std::vector<FluidParticle>& FluidSystem::GetParticlePool(FluidType type) {
    return particlePools_[(int)type];
}
