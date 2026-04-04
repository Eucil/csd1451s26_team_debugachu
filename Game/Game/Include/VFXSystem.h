/*!
@file       VFXSystem.h
@author     Chia Hanxin/c.hanxin@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declarations of functions and classes
            for the visual effects system which includes the following:
                - VFXType, an enumeration for identifying various visual effects
                  such as bursts for dirt, portals, and collectible items.
                - VFXParticle, a struct representing an individual effect unit
                  with properties for life, transform, velocity, and color.
                - ParticleEmitter, a structure used to trigger and initialize
                  batches of particles at specific world positions.
                - VFXSystem, a manager class that handles memory preallocation,
                  resource loading, particle lifecycle updates (aging/fading),
                  and batch rendering.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <vector>

// Third-party
#include <AEEngine.h>

// Project
#include "Components.h"
#include "Utils.h"
enum class VFXType {
    DirtBurst,
    PortalBurst,
    PipeFlow,
    FlowerCollect,
    StarCollect, // yellow burst when a Star is collected
    GemCollect,  // magenta burst when a Gem is collected
    LeafCollect, // green burst when a Leaf is collected
    Count
};

struct VFXParticle {

    VFXType type_{VFXType::DirtBurst};

    // since vfxparticles dont collide with one another, have no change in physics, short
    // lifetime, they should be lightweight and thus we shouldnt use Components.h components.
    AEVec2 pos_{0.0f, 0.0f};
    AEVec2 vel_{0.0f, 0.0f};

    f32 scale_{1.0f};
    f32 rotationRad_{0.0f};
    f32 rotationSpeed_{0.0f};

    f32 r_{1.0f}, g_{1.0f}, b_{1.0f}, a_{1.0f};

    f32 lifeTime_{0.0f};
    f32 maxLifeTime_{1.0f};

    bool active_{false}; // Used for object pooling
    char padding_[4]{0};
};

struct EmitterConfig {
    int spawnCount_{0}; // How many particles to spawn per burst
    f32 minLife_{0.0f}, maxLife_{0.0f};
    f32 minSpeed_{0.0f}, maxSpeed_{0.0f};
    f32 minScale_{1.0f}, maxScale_{1.0f};
    f32 r_{1.0f}, g_{1.0f}, b_{1.0f}, a_{1.0f};
};

struct ParticleEmitter {
    bool active_{false};
    VFXType type_{VFXType::DirtBurst};
    AEVec2 pos_{0.0f, 0.0f};
    f32 emitterLifeTime_{0.0f}; // Timer for how long the emitter stays alive.
    f32 angleRad_ = 0.0f;

    EmitterConfig config_;
};

class VFXSystem {
public:
    f32 vfxSpawnTimer_{0.0f};

    void initialize(u32 maxParticles = 800, u32 maxEmitters = 20);

    void update(f32 dt);

    void draw();

    void free();

    std::vector<VFXParticle>& getParticlePool(VFXType type);

    void setEmitterConfig(VFXType type, const EmitterConfig& config);

    void setGraphicsConfig(VFXType type, const Graphics& gfxConfig);

    void spawnVFX(VFXType type, AEVec2 position, f32 angleRad = 0.0f);

    // Used together with ResetSpawnTimer
    void spawnContinuous(VFXType type, AEVec2 position, f32 deltaTime, f32 spawnRate = 0.1f);

    void resetSpawnTimer();

    u32 getActiveParticleCount() const {
        u32 count = 0;
        for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {
            for (const auto& p : vfxParticlePool_[i]) {
                if (p.active_)
                    count++;
            }
        }
        return count;
    }

private:
    std::vector<VFXParticle> vfxParticlePool_[static_cast<int>(VFXType::Count)];

    std::vector<ParticleEmitter> vfxEmitters_;

    Graphics graphicsConfigs_[static_cast<int>(VFXType::Count)];

    EmitterConfig emitterConfigs_[static_cast<int>(VFXType::Count)];

    void initializeEmitter(ParticleEmitter& emitter, VFXType type, AEVec2 pos, f32 angleRad);

    ParticleEmitter* getFreeEmitter();

    VFXParticle* getFreeParticle(VFXType type);

    void spawnParticles(ParticleEmitter& emitter);
};