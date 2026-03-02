#include "VFXSystem.h"
#include "Utils.h"

// public
void VFXSystem::Initialize(u32 maxParticles, u32 maxEmitters) {
    InitializeGFXConfigs();

    // preallocate emitter container to avoid dynamic memory allocation during gameplay
    vfxEmitters_.resize(maxEmitters);

    // preallocate particle pools to avoid dynamic memory allocation during gameplay
    for (int i = 0; i < static_cast<int>(VFXType::Count); ++i) {
        vfxParticlePool_[i].resize(maxParticles);
    }
}

void VFXSystem::Update(f32 dt) {}

void VFXSystem::Draw() {}

void VFXSystem::SpawnVFX(AEVec2 position, int particleCount) {
    for (int i = 0; i < particleCount; i++) {
    }
}
// private
void VFXSystem::InitializeGFXConfigs() {

    AEGfxVertexList* circleMesh = CreateCircleMesh(20);

    // Initialize graphics configurations for each VFX type

    // Dirt:
    graphicsConfigs_[static_cast<int>(VFXType::DirtBurst)].mesh_ = circleMesh;
    graphicsConfigs_[static_cast<int>(VFXType::DirtBurst)].texture_ =
        AEGfxTextureLoad("Assets/dirt_particle.png");
    graphicsConfigs_[static_cast<int>(VFXType::DirtBurst)].layer_ =
        5; // Render above player and terrain
}

ParticleEmitter* VFXSystem::GetFreeEmitter() {

    for (auto& p : vfxEmitters_) {
        if (!p.active_) {
            return &p;
        }
    }
    return nullptr;
}

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