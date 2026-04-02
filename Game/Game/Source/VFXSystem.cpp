/*!
@file       VFXSystem.cpp
@author     Chia Hanxin/c.hanxin@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
           Reproduction or disclosure of this file or its contents
           without the prior written consent of DigiPen Institute of
           Technology is prohibited.
*//*______________________________________________________________________*/
#include "VFXSystem.h"

#include <iostream>

#include "ConfigManager.h"

// private
// only used when a game event requires a particle to be spawned
// =========================================================
//
//  ParticleEmitter's Main Initialize function
//
// Initializes a single ParticleEmitter with the inputted VFXType and position,
// assigns its configuration from the preloaded emitter config array, spawns
// its initial batch of particles, then immediately deactivates the emitter.
//
// =========================================================
void VFXSystem::InitializeEmitter(ParticleEmitter& emitter, VFXType type, AEVec2 pos) {

    emitter.active_ = true;
    emitter.type_ = type;
    emitter.pos_ = pos;
    emitter.emitterLifeTime_ = 0.0f;

    emitter.config_ = emitterConfigs_[static_cast<int>(type)];

    SpawnParticles(emitter);

    emitter.active_ = false;
}

// public
// =========================================================
//
//  VFXSystem's Main Initialize function
//
// Initializes VFXSystem by doing the following:
// - Preallocates all emitter and particle pool containers to their maximum capacities
// - Loads all graphics and emitter configurations for each VFXType from the ConfigManager.
//
// =========================================================
void VFXSystem::Initialize(u32 maxParticles, u32 maxEmitters) {
    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {
        graphicsConfigs_[i].mesh_ = nullptr;
        graphicsConfigs_[i].texture_ = nullptr;
    }

    // preallocate emitter container to avoid dynamic memory allocation during gameplay
    vfxEmitters_.resize(maxEmitters);

    // preallocate particle pools to avoid dynamic memory allocation during gameplay
    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {
        vfxParticlePool_[i].resize(maxParticles);
    }

    // All data below are readed from json file

    // DirtBurst
    AEGfxVertexList* particleMesh = CreateRectMesh();
    Graphics dirtGfx;
    dirtGfx.mesh_ = particleMesh;
    dirtGfx.texture_ = nullptr; // AEGfxTextureLoad("Assets/Textures/Dirt02.png");
    dirtGfx.layer_ = g_configManager.getInt("VFXSystem", "dirtGFX", "layer", 5);
    SetGraphicsConfig(VFXType::DirtBurst, dirtGfx);

    // PortalBurst
    Graphics portalGfx;
    portalGfx.mesh_ = particleMesh; // Reuse the exact same rect mesh
    portalGfx.texture_ = nullptr;
    portalGfx.layer_ = g_configManager.getInt("VFXSystem", "portalGFX", "layer", 5);
    SetGraphicsConfig(VFXType::PortalBurst, portalGfx);

    // PipeFlow
    Graphics pipeFlowGfx;
    pipeFlowGfx.mesh_ = particleMesh; // reuse the same rect mesh
    pipeFlowGfx.texture_ = nullptr;
    pipeFlowGfx.layer_ = g_configManager.getInt("VFXSystem", "pipeFlowGFX", "layer", 5);
    SetGraphicsConfig(VFXType::PipeFlow, pipeFlowGfx);

    // FlowerCollect
    Graphics flowerCollectGfx;
    flowerCollectGfx.mesh_ = particleMesh;
    flowerCollectGfx.texture_ = nullptr;
    flowerCollectGfx.layer_ = g_configManager.getInt("VFXSystem", "flowerCollectGFX", "layer", 5);
    SetGraphicsConfig(VFXType::FlowerCollect, flowerCollectGfx);

    // Starcollect
    Graphics starCollectGfx;
    starCollectGfx.mesh_ = particleMesh;
    starCollectGfx.texture_ = nullptr;
    starCollectGfx.layer_ = g_configManager.getInt("VFXSystem", "starCollectGFX", "layer", 5);
    SetGraphicsConfig(VFXType::StarCollect, starCollectGfx);

    // GemCollect — magenta burst
    Graphics gemCollectGfx;
    gemCollectGfx.mesh_ = particleMesh;
    gemCollectGfx.texture_ = nullptr;
    gemCollectGfx.layer_ = g_configManager.getInt("VFXSystem", "gemCollectGFX", "layer", 5);
    SetGraphicsConfig(VFXType::GemCollect, gemCollectGfx);

    // LeafCollect — green burst
    Graphics leafCollectGfx;
    leafCollectGfx.mesh_ = particleMesh;
    leafCollectGfx.texture_ = nullptr;
    leafCollectGfx.layer_ = g_configManager.getInt("VFXSystem", "leafCollectGFX", "layer", 5);
    SetGraphicsConfig(VFXType::LeafCollect, leafCollectGfx);

    // Emitter setup
    // DirtBurst
    EmitterConfig dirtConfig;
    dirtConfig.spawnCount_ = g_configManager.getInt("VFXSystem", "dirtConfig", "spawnCount_", 10);
    dirtConfig.minLife_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "minLife_", 0.3f);
    dirtConfig.maxLife_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "maxLife_", 0.6f);
    dirtConfig.minSpeed_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "minSpeed_", 50.0f);
    dirtConfig.maxSpeed_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "maxSpeed_", 150.0f);
    dirtConfig.minScale_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "minScale_", 10.0f);
    dirtConfig.maxScale_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "maxScale_", 11.5f);
    dirtConfig.r_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "r_", 0.3f);
    dirtConfig.g_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "g_", 0.2f);
    dirtConfig.b_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "b_", 0.1f);
    dirtConfig.a_ = g_configManager.getFloat("VFXSystem", "dirtConfig", "a_", 1.0f);
    SetEmitterConfig(VFXType::DirtBurst, dirtConfig);

    // PortalBurst
    EmitterConfig portalConfig;
    portalConfig.spawnCount_ =
        g_configManager.getInt("VFXSystem", "portalConfig", "spawnCount_", 15);
    portalConfig.minLife_ = g_configManager.getFloat("VFXSystem", "portalConfig", "minLife_", 0.2f);
    portalConfig.maxLife_ = g_configManager.getFloat("VFXSystem", "portalConfig", "maxLife_", 0.5f);
    portalConfig.minSpeed_ =
        g_configManager.getFloat("VFXSystem", "portalConfig", "minSpeed_", 40.0f);
    portalConfig.maxSpeed_ =
        g_configManager.getFloat("VFXSystem", "portalConfig", "maxSpeed_", 120.0f);
    portalConfig.minScale_ =
        g_configManager.getFloat("VFXSystem", "portalConfig", "minScale_", 3.0f);
    portalConfig.maxScale_ =
        g_configManager.getFloat("VFXSystem", "portalConfig", "maxScale_", 7.0f);
    portalConfig.r_ = g_configManager.getFloat("VFXSystem", "portalConfig", "r_", 0.8f);
    portalConfig.g_ = g_configManager.getFloat("VFXSystem", "portalConfig", "g_", 0.1f);
    portalConfig.b_ = g_configManager.getFloat("VFXSystem", "portalConfig", "b_", 1.0f);
    portalConfig.a_ = g_configManager.getFloat("VFXSystem", "portalConfig", "a_", 1.0f);
    SetEmitterConfig(VFXType::PortalBurst, portalConfig);

    // Pipe flow
    EmitterConfig pipeFlowConfig;
    pipeFlowConfig.spawnCount_ =
        g_configManager.getInt("VFXSystem", "pipeFlowConfig", "spawnCount_", 6);
    pipeFlowConfig.minLife_ =
        g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "minLife_", 0.2f);
    pipeFlowConfig.maxLife_ =
        g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "maxLife_", 0.45f);
    pipeFlowConfig.minSpeed_ =
        g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "minSpeed_", 20.0f);
    pipeFlowConfig.maxSpeed_ =
        g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "maxSpeed_", 60.0f);
    pipeFlowConfig.minScale_ =
        g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "minScale_", 3.0f);
    pipeFlowConfig.maxScale_ =
        g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "maxScale_", 6.0f);
    pipeFlowConfig.r_ = g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "r_", 0.4f);
    pipeFlowConfig.g_ = g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "g_", 0.7f);
    pipeFlowConfig.b_ = g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "b_", 1.0f);
    pipeFlowConfig.a_ = g_configManager.getFloat("VFXSystem", "pipeFlowConfig", "a_", 0.85f);
    SetEmitterConfig(VFXType::PipeFlow, pipeFlowConfig);

    // Flowercollect
    EmitterConfig flowerCollectConfig;
    flowerCollectConfig.spawnCount_ =
        g_configManager.getInt("VFXSystem", "flowerCollectConfig", "spawnCount_", 12);
    flowerCollectConfig.minLife_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "minLife_", 0.25f);
    flowerCollectConfig.maxLife_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "maxLife_", 0.5f);
    flowerCollectConfig.minSpeed_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "minSpeed_", 60.0f);
    flowerCollectConfig.maxSpeed_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "maxSpeed_", 160.0f);
    flowerCollectConfig.minScale_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "minScale_", 4.0f);
    flowerCollectConfig.maxScale_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "maxScale_", 8.0f);
    flowerCollectConfig.r_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "r_", 0.6f);
    flowerCollectConfig.g_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "g_", 1.0f);
    flowerCollectConfig.b_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "b_", 0.4f);
    flowerCollectConfig.a_ =
        g_configManager.getFloat("VFXSystem", "flowerCollectConfig", "a_", 1.0f);
    SetEmitterConfig(VFXType::FlowerCollect, flowerCollectConfig);

    // Starcollect
    EmitterConfig starCollectConfig;
    starCollectConfig.spawnCount_ =
        g_configManager.getInt("VFXSystem", "starCollectConfig", "spawnCount_", 15);
    starCollectConfig.minLife_ =
        g_configManager.getFloat("VFXSystem", "starCollectConfig", "minLife_", 0.3f);
    starCollectConfig.maxLife_ =
        g_configManager.getFloat("VFXSystem", "starCollectConfig", "maxLife_", 0.6f);
    starCollectConfig.minSpeed_ =
        g_configManager.getFloat("VFXSystem", "starCollectConfig", "minSpeed_", 80.0f);
    starCollectConfig.maxSpeed_ =
        g_configManager.getFloat("VFXSystem", "starCollectConfig", "maxSpeed_", 200.0f);
    starCollectConfig.minScale_ =
        g_configManager.getFloat("VFXSystem", "starCollectConfig", "minScale_", 4.0f);
    starCollectConfig.maxScale_ =
        g_configManager.getFloat("VFXSystem", "starCollectConfig", "maxScale_", 9.0f);
    starCollectConfig.r_ = g_configManager.getFloat("VFXSystem", "starCollectConfig", "r_", 1.0f);
    starCollectConfig.g_ = g_configManager.getFloat("VFXSystem", "starCollectConfig", "g_", 1.0f);
    starCollectConfig.b_ = g_configManager.getFloat("VFXSystem", "starCollectConfig", "b_", 0.0f);
    starCollectConfig.a_ = g_configManager.getFloat("VFXSystem", "starCollectConfig", "a_", 1.0f);
    SetEmitterConfig(VFXType::StarCollect, starCollectConfig);

    // gemCollect
    EmitterConfig gemCollectConfig;
    gemCollectConfig.spawnCount_ =
        g_configManager.getInt("VFXSystem", "gemCollectConfig", "spawnCount_", 15);
    gemCollectConfig.minLife_ =
        g_configManager.getFloat("VFXSystem", "gemCollectConfig", "minLife_", 0.3f);
    gemCollectConfig.maxLife_ =
        g_configManager.getFloat("VFXSystem", "gemCollectConfig", "maxLife_", 0.6f);
    gemCollectConfig.minSpeed_ =
        g_configManager.getFloat("VFXSystem", "gemCollectConfig", "minSpeed_", 80.0f);
    gemCollectConfig.maxSpeed_ =
        g_configManager.getFloat("VFXSystem", "gemCollectConfig", "maxSpeed_", 200.0f);
    gemCollectConfig.minScale_ =
        g_configManager.getFloat("VFXSystem", "gemCollectConfig", "minScale_", 4.0f);
    gemCollectConfig.maxScale_ =
        g_configManager.getFloat("VFXSystem", "gemCollectConfig", "maxScale_", 9.0f);
    gemCollectConfig.r_ = g_configManager.getFloat("VFXSystem", "gemCollectConfig", "r_", 1.0f);
    gemCollectConfig.g_ = g_configManager.getFloat("VFXSystem", "gemCollectConfig", "g_", 0.0f);
    gemCollectConfig.b_ = g_configManager.getFloat("VFXSystem", "gemCollectConfig", "b_", 1.0f);
    gemCollectConfig.a_ = g_configManager.getFloat("VFXSystem", "gemCollectConfig", "a_", 1.0f);
    SetEmitterConfig(VFXType::GemCollect, gemCollectConfig);

    // leafCollect
    EmitterConfig leafCollectConfig;
    leafCollectConfig.spawnCount_ =
        g_configManager.getInt("VFXSystem", "leafCollectConfig", "spawnCount_", 15);
    leafCollectConfig.minLife_ =
        g_configManager.getFloat("VFXSystem", "leafCollectConfig", "minLife_", 0.3f);
    leafCollectConfig.maxLife_ =
        g_configManager.getFloat("VFXSystem", "leafCollectConfig", "maxLife_", 0.6f);
    leafCollectConfig.minSpeed_ =
        g_configManager.getFloat("VFXSystem", "leafCollectConfig", "minSpeed_", 80.0f);
    leafCollectConfig.maxSpeed_ =
        g_configManager.getFloat("VFXSystem", "leafCollectConfig", "maxSpeed_", 200.0f);
    leafCollectConfig.minScale_ =
        g_configManager.getFloat("VFXSystem", "leafCollectConfig", "minScale_", 4.0f);
    leafCollectConfig.maxScale_ =
        g_configManager.getFloat("VFXSystem", "leafCollectConfig", "maxScale_", 9.0f);
    leafCollectConfig.r_ = g_configManager.getFloat("VFXSystem", "leafCollectConfig", "r_", 0.0f);
    leafCollectConfig.g_ = g_configManager.getFloat("VFXSystem", "leafCollectConfig", "g_", 1.0f);
    leafCollectConfig.b_ = g_configManager.getFloat("VFXSystem", "leafCollectConfig", "b_", 0.0f);
    leafCollectConfig.a_ = g_configManager.getFloat("VFXSystem", "leafCollectConfig", "a_", 1.0f);
    SetEmitterConfig(VFXType::LeafCollect, leafCollectConfig);
}

// =========================================================
//
// VFXSystem's main Update function
//
// - Iterates over every particle pool and advances all active particles by one timestep.
// - Handles particle aging and death
// - Applies downward gravity
// - Spins particles by their rotation speed
// - Fades alpha out smoothly in the latter half of each particle's lifetime.
//
// =========================================================
void VFXSystem::Update(f32 dt) {

    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {

        for (auto& p : vfxParticlePool_[i]) {

            // Skip inactive particles instantly
            if (!p.active_) {
                continue;
            }

            // Age the particle
            p.lifeTime_ += dt;

            // Check for particle death
            if (p.lifeTime_ >= p.maxLifeTime_) {
                p.active_ = false; // kill the particle
                continue;
            }

            // apply gravity
            p.vel_.y -= 500.0f * dt;
            p.pos_.x += p.vel_.x * dt;
            p.pos_.y += p.vel_.y * dt;

            // rotate the particle
            p.rotationRad_ += p.rotationSpeed_ * dt;

            // Fade out over time
            // Calculates a normalized ratio from 0.0 (birth) to 1.0 (death)
            f32 lifeRatio = p.lifeTime_ / p.maxLifeTime_;
            f32 fadeStartThreshold = 0.45f; // Wait until 50% dead before fading

            if (lifeRatio < fadeStartThreshold) {
                p.a_ = 1.0f; // Stay solid
            } else {
                // Remap the remaining life to a new 0.0 to 1.0 ratio
                f32 fadeRatio = (lifeRatio - fadeStartThreshold) / (1.0f - fadeStartThreshold);
                p.a_ = 1.0f - fadeRatio;
            }

            // scale the particle down as it dies
            // p.scale_ = p.scale_ * (1.0f - lifeRatio);
        }
    }
}
// =========================================================
//
//  VFXSystem's main Draw function
//
// - Renders all active particles across every VFXType pool.
// - For each pool, selects either texture or color render mode based on the configured graphics.
// - Constructs a world transform matrix from each particle's scale, rotation and position
// - Issues a mesh draw call with the particle's current color and transparency values.
//
// =========================================================
void VFXSystem::Draw() {

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {

        Graphics& gfx = graphicsConfigs_[i];

        // Safety check: If no mesh is configured, we dont draw this particle type
        if (gfx.mesh_ == nullptr) {
            continue;
        }

        // If texture exists, set to texture mode. Otherwise, color mode.
        if (gfx.texture_ != nullptr) {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxTextureSet(gfx.texture_, 0, 0);
        } else {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        }

        // Draw every active particle in this pool
        for (auto& p : vfxParticlePool_[i]) {

            if (!p.active_) {
                continue;
            }

            AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;

            AEMtx33Scale(&scaleMtx, p.scale_, p.scale_);
            AEMtx33Rot(&rotMtx, p.rotationRad_);
            AEMtx33Trans(&transMtx, p.pos_.x, p.pos_.y);
            AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
            AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

            AEGfxSetTransform(worldMtx.m);
            AEGfxSetColorToMultiply(p.r_, p.g_, p.b_, p.a_);
            AEGfxSetTransparency(p.a_);

            AEGfxMeshDraw(gfx.mesh_, AE_GFX_MDM_TRIANGLES);
        }
    }
}
// =========================================================
//
//  VFXSystem's main Free function
//
// - Releases all allocated GPU resources by safely freeing each unique
// mesh and texture pointer while guarding against double-frees caused
// by shared pointers across VFXType configs.
//
// -Clears all emitter and particle pool containers afterwards.
//
// =========================================================
void VFXSystem::Free() {
    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {
        AEGfxVertexList* currentMesh = graphicsConfigs_[i].mesh_;

        if (currentMesh != nullptr) {
            bool isDuplicate = false;

            // Look back at all previous configs to see if this pointer was already handled
            for (int j = 0; j < i; ++j) {
                if (graphicsConfigs_[j].mesh_ == currentMesh) {
                    isDuplicate = true;
                    break;
                }
            }

            // prevent double freeing
            if (isDuplicate) {
                AEGfxMeshFree(currentMesh);
            }

            graphicsConfigs_[i].mesh_ = nullptr;
        }
    }

    // free textures
    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {
        AEGfxTexture* currentTexture = graphicsConfigs_[i].texture_;

        if (currentTexture != nullptr) {
            bool isDuplicate = false;

            // Check previous indices for the same texture pointer
            for (int j = 0; j < i; ++j) {
                if (graphicsConfigs_[j].texture_ == currentTexture) {
                    isDuplicate = true;
                    break;
                }
            }

            if (!isDuplicate) {
                AEGfxTextureUnload(currentTexture);
            }

            graphicsConfigs_[i].texture_ = nullptr;
        }
    }

    vfxEmitters_.clear();
    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {
        vfxParticlePool_[i].clear();
    }
}
// =========================================================
//
//  VFXSystem's spawn vfx utility function
//
// - Public entry point for triggering a one-shot VFX event at the given
// world position.
// - Retrieves a free emitter from the pool and initializes it with the requested type
//
// - Logs a warning if no emitter is available.
//
// =========================================================
void VFXSystem::SpawnVFX(VFXType type, AEVec2 position) {
    ParticleEmitter* emitter = GetFreeEmitter();
    if (emitter != nullptr) {
        InitializeEmitter(*emitter, type, position);
    } else {
        std::cout << "Warning: No VFX Pools Available.";
    }
}
// =========================================================
//
// VFXSystem's free emitter getter function
//
// - Scans the emitter pool and returns a pointer to the first inactive
// ParticleEmitter available for reuse.
//
// - Returns nullptr if all emitters are currently active.
//
// =========================================================
ParticleEmitter* VFXSystem::GetFreeEmitter() {

    for (auto& p : vfxEmitters_) {
        if (!p.active_) {
            return &p;
        }
    }
    return nullptr;
}

// todo
std::vector<VFXParticle>& VFXSystem::GetParticlePool(VFXType type) {
    return vfxParticlePool_[static_cast<int>(type)];
}

// =========================================================
//
// VFXSystem's free particle getter function
//
// - Scans the particle pool for the specified VFXType and returns a pointer
// to the first inactive VFXParticle available for reuse.
//
// - Returns nullptr if the entire pool for that type is currently active.
//
// =========================================================
VFXParticle* VFXSystem::GetFreeParticle(VFXType type) {

    int typeIndex = static_cast<int>(type);

    // find an inactive particle pool to reuse
    for (auto& p : vfxParticlePool_[typeIndex]) {
        if (!p.active_) {
            return &p;
        }
    }
    return nullptr;
}

// private
// =========================================================
//
// VFXSystem's particle spawning utility function
//
// - Spawns a batch of particles for the given emitter up to its configured
// spawnCount.
//
// - Each particle is pulled from the inactive pool and assigned
// randomized lifetime, scale, radial burst velocity, rotation, and spin
// values interpolated between the emitter config's min and max bounds.
//
// =========================================================
void VFXSystem::SpawnParticles(ParticleEmitter& emitter) {

    // Loop exactly spawnCount_ times
    for (int i = 0; i < emitter.config_.spawnCount_; ++i) {

        // Grab a dead particle from the pool
        VFXParticle* p = GetFreeParticle(emitter.type_);

        // If the pool is completely full of active particles, stop trying to spawn
        if (p == nullptr) {
            break;
        }

        // Wake it up and set its starting position to match the emitter
        p->active_ = true;
        p->type_ = emitter.type_;
        p->pos_ = emitter.pos_;

        // Randomized stats based on the emitter's config
        p->lifeTime_ = 0.0f;
        p->maxLifeTime_ = emitter.config_.minLife_ +
                          AERandFloat() * (emitter.config_.maxLife_ - emitter.config_.minLife_);
        p->scale_ = emitter.config_.minScale_ +
                    AERandFloat() * (emitter.config_.maxScale_ - emitter.config_.minScale_);

        // Calculate the radial burst velocity
        // 6.2831853f is 2 * PI (a full circle in radians)
        f32 angle = 0.0f + AERandFloat() * (6.2831853f - 0.0f);
        f32 speed = emitter.config_.minSpeed_ +
                    AERandFloat() * (emitter.config_.maxSpeed_ - emitter.config_.minSpeed_);

        // Convert the angle and speed into X and Y velocity vectors
        p->vel_.x = std::cos(angle) * speed;
        p->vel_.y = std::sin(angle) * speed;

        // Reset colors based on the current emitter's values
        p->r_ = emitter.config_.r_;
        p->g_ = emitter.config_.g_;
        p->b_ = emitter.config_.b_;
        p->a_ = emitter.config_.a_;

        p->rotationRad_ = 0.0f + AERandFloat() * (6.2831853f - 0.0f);
        p->rotationSpeed_ =
            -2.0f + AERandFloat() * (2.0f - (-2.0f)); // Give it a slight random spin
    }
}

// public
// =========================================================
//
// VFXSystem's emitterConfigs_ setter function
//
// - Stores the provided EmitterConfig into the config array at the index
// corresponding to the given VFXType, making it the active configuration
// used when that type is next spawned.
//
// =========================================================
void VFXSystem::SetEmitterConfig(VFXType type, const EmitterConfig& config) {
    emitterConfigs_[static_cast<int>(type)] = config;
}

// public
// =========================================================
//
// VFXSystem's graphicConfigs_ setter function
//
// Stores the provided Graphics config into the graphics array at the index
// corresponding to the given VFXType, defining the mesh, texture and layer
// used when drawing particles of that type.
//
// =========================================================
void VFXSystem::SetGraphicsConfig(VFXType type, const Graphics& gfxConfig) {
    graphicsConfigs_[static_cast<int>(type)] = gfxConfig;
}

// public
// =========================================================
//
// VFXSystem's particle spawning utility function (continuous)
//
// - Drives a continuously repeating VFX effect by decrementing an internal
// spawn timer each frame.
//
// - When the timer expires, a new one-shot VFX is
// spawned at the given position and the timer is reset to the provided
// spawn rate interval.
//
// =========================================================
void VFXSystem::SpawnContinuous(VFXType type, AEVec2 position, f32 deltaTime, f32 spawnRate) {
    vfxSpawnTimer_ -= deltaTime;

    if (vfxSpawnTimer_ <= 0.0f) {
        SpawnVFX(type, position);
        vfxSpawnTimer_ = spawnRate; // Reset the timer using the inputted rate
    }
}

// =========================================================
//
// VFXSystem's particle timer reset function
//
// Immediately resets the internal spawn timer to zero, forcing
// SpawnContinuous to trigger a new spawn on its very next call.
//
// =========================================================
void VFXSystem::ResetSpawnTimer() { vfxSpawnTimer_ = 0.0f; }