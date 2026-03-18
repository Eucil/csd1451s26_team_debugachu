#pragma once
#include "AEEngine.h"
#include "Components.h"
#include "Utils.h"
#include <vector>
enum class VFXType {
    DirtBurst,
    // Add magic particles, portal particles wtv
    Count
};

struct VFXParticle {

    VFXType type_{VFXType::DirtBurst};

    // since vfxparticles dont collide with one another, have no change in physics, short lifetime,
    // they should be lightweight and thus we shouldnt use Components.h components.
    AEVec2 pos_{0.0f, 0.0f};
    AEVec2 vel_{0.0f, 0.0f};

    f32 scale_{1.0f};
    f32 rotationRad_{0.0f};
    f32 rotationSpeed_{0.0f};

    f32 r_{1.0f}, g_{1.0f}, b_{1.0f}, a_{1.0f};

    f32 lifeTime_{0.0f};
    f32 maxLifeTime_{1.0f};

    bool active_{false}; // Used for object pooling
    char padding[4];
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

    EmitterConfig config_;
};

class VFXSystem {
public:
    f32 vfxSpawnTimer_{0.0f};

    void Initialize(u32 maxParticles = 800, u32 maxEmitters = 20);

    void Update(f32 dt);

    void Draw();

    void Free();

    void SetEmitterConfig(VFXType type, const EmitterConfig& config);

    void SetGraphicsConfig(VFXType type, const Graphics& gfxConfig);

    void SpawnVFX(VFXType type, AEVec2 position);

    // Used together with ResetSpawnTimer
    void SpawnContinuous(VFXType type, AEVec2 position, f32 deltaTime, f32 spawnRate = 0.1f);

    void ResetSpawnTimer();

private:
    std::vector<VFXParticle> vfxParticlePool_[static_cast<int>(VFXType::Count)];

    std::vector<ParticleEmitter> vfxEmitters_;

    Graphics graphicsConfigs_[static_cast<int>(VFXType::Count)];

    EmitterConfig emitterConfigs_[static_cast<int>(VFXType::Count)];

    void InitializeEmitter(ParticleEmitter& emitter, VFXType type, AEVec2 pos);

    ParticleEmitter* GetFreeEmitter();

    VFXParticle* GetFreeParticle(VFXType type);

    void SpawnParticles(ParticleEmitter& emitter);
};