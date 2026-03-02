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
        radius * 0.7f; // * 0.7f so that collider is smaller than mesh

    type_ = type;
}

// ==========================================
// FluidSystem
// ==========================================

void FluidSystem::Initialize() {
    InitializeMesh();

    // Reduces memory reallocation
    int typeCount{static_cast<int>(FluidType::Count)};
    for (int i{0}; i < typeCount; i++) {
        particlePools_[i].reserve(1000);
    }

    SetTypeColor(0.0f, 0.5f, 1.0f, 1.0f, FluidType ::Water);
    SetTypeColor(1.0f, 0.2f, 0.0f, 1.0f, FluidType::Lava);
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
        // set colour
        AEGfxSetColorToMultiply(colorConfigs_[i][0],  //  <-- r
                                colorConfigs_[i][1],  //  <-- g
                                colorConfigs_[i][2],  //  <-- b
                                colorConfigs_[i][3]); //  <-- alpha

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(colorConfigs_[i][3]);

        // draw according to the particles' transform matrix
        for (auto& p : particlePools_[i]) { // <-- p = current particle being looped

            AEGfxSetTransform(p.transform_.worldMtx_.m);
            AEGfxMeshDraw(graphicsConfigs_[i].mesh_, AE_GFX_MDM_TRIANGLES);
        }
    }
}

void FluidSystem::DrawTexture() {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // Loops through (0) Water, (1) Lava, ...
    for (int i = 0; i < (int)FluidType::Count; ++i) {

        // if particle pool is empty, completely skip this pool
        if (particlePools_[i].empty()) {
            continue;
        }

        AEGfxTextureSet(graphicsConfigs_[i].texture_, 0, 0);

        AEGfxSetColorToMultiply(colorConfigs_[i][0],  //  <-- r
                                colorConfigs_[i][1],  //  <-- g
                                colorConfigs_[i][2],  //  <-- b
                                colorConfigs_[i][3]); //  <-- alpha

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(colorConfigs_[i][3]);

        // Loop through each particle
        for (auto& p : particlePools_[i]) {
            AEGfxSetTransform(p.transform_.worldMtx_.m);
            AEGfxMeshDraw(graphicsConfigs_[i].mesh_, AE_GFX_MDM_TRIANGLES);
        }
    }
}

void FluidSystem::Free() {
    // Free the mesh once and then set the rest
    if (graphicsConfigs_[0].mesh_ != nullptr) {
        AEGfxMeshFree(graphicsConfigs_[0].mesh_);
    }

    // Nullify all mesh pointers so we don't accidentally use dead memory
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        graphicsConfigs_[i].mesh_ = nullptr;
    }

    // Free textures
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        if (graphicsConfigs_[i].texture_ != nullptr) {
            AEGfxTextureUnload(
                graphicsConfigs_[i].texture_); // Or AEGfxTextureFree depending on version
            graphicsConfigs_[i].texture_ = nullptr;
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
    particlePools_[i].push_back(newParticle);
}

int FluidSystem::GetParticleCount(FluidType type) { return particlePools_[(int)type].size(); }

std::vector<FluidParticle>& FluidSystem::GetParticlePool(FluidType type) {
    return particlePools_[(int)type];
}

// UPDATE (MAYBE BROKEN)
void FluidSystem::InitializeMesh() {
    AEGfxVertexList* circleMesh = CreateCircleMesh(20);

    // Assign circle mesh to each particle type
    for (int i{0}; i < static_cast<int>(FluidType::Count); ++i) {
        if (circleMesh != nullptr) {
            graphicsConfigs_[i].mesh_ = circleMesh;
        }
    }
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

void FluidSystem::SetTypeColor(f32 r, f32 g, f32 b, f32 a, FluidType type) {
    int i = (int)type;

    colorConfigs_[i][0] = r;
    colorConfigs_[i][1] = g;
    colorConfigs_[i][2] = b;
    colorConfigs_[i][3] = a;
}

void FluidSystem::SetTypeGraphics(AEGfxVertexList* mesh, AEGfxTexture* texture, u32 layer,
                                  FluidType type) {
    int i = (int)type;

    graphicsConfigs_[i].mesh_ = mesh;
    graphicsConfigs_[i].texture_ = texture;
    graphicsConfigs_[i].layer_ = layer;
}

void FluidSystem::UpdateMain(f32 dt, Terrain& terrain) {
    // DT clamp
    if (dt > 0.016f) {
        dt = 0.016f;
    }

    // Substeps
    const int subSteps = 4;
    const f32 subDt = dt / (f32)subSteps;

    for (int s = 0; s < subSteps; s++) {

        CollisionSystem::terrainToFluidCollision(terrain, *this);

        for (int i = 0; i < (int)FluidType::Count; i++) {
            if (particlePools_[i].empty())
                continue;

            // 1) integrate
            UpdatePhysics(particlePools_[i], subDt);
        }
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