#include "AEEngine.h"
#include "Components.h"
#include <vector>
enum class VFXType {
    DirtBurst,
    // Add magic particles, portal particles wtv
    Count
};

struct VFXParticle {

    VFXType type_{VFXType::DirtBurst};

    AEVec2 pos_{0.0f, 0.0f};
    AEVec2 vel_{0.0f, 0.0f};

    f32 scale_{1.0f};
    f32 rotationRad_{0.0f};
    f32 rotationSpeed_{0.0f}; 

    f32 r_{1.0f}, g_{1.0f}, b_{1.0f}, a_{1.0f};

    f32 lifeTime_{0.0f};
    f32 maxLifeTime_{1.0f};

    bool active_{false}; // Used for object pooling
};

struct ParticleEmitter {
    bool active_{false};
    VFXType type_;

    AEVec2 pos_;

    // Timer for how long the emitter stays alive.
    // (If 0, it's an instant "burst" like breaking dirt. If > 0, it's continuous like a torch).
    f32 emitterLifeTime_{0.0f};

    // Spawn rules
    int spawnCount_{0};       // How many particles to spawn per burst
    f32 minLife_, maxLife_;   // Randomize particle lifetimes
    f32 minSpeed_, maxSpeed_; // Randomize explosion force
    f32 minScale_, maxScale_;
};

class VFXSystem {
public:
    void Initialize(u32 maxParticles = 800, u32 maxEmitters = 20);

    void Update(f32 dt);

    void Draw();

    void SpawnExplosion(AEVec2 position, int particleCount);

private:

    std::vector<VFXParticle> vfxParticlePool_[static_cast<int>(VFXType::Count)];

    std::vector<ParticleEmitter> vfxEmitters_;

    Graphics graphicsConfigs_[static_cast<int>(VFXType::Count)];

    //u32 activeParticleCount_{0};

    void InitializeGFXConfigs();

    ParticleEmitter* GetFreeEmitter();

    VFXParticle* GetFreeParticle(VFXType type);

    void ConfigureEmitter(ParticleEmitter& emitter, VFXType type, AEVec2 pos);

    void SpawnParticles(ParticleEmitter& emitter);

};