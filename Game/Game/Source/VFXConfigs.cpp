#include "VFXConfigs.h"

void LoadGlobalVFXConfigs(VFXSystem& vfxSystem) {

    // Graphics setup
    AEGfxVertexList* circleMesh = CreateCircleMesh(20, 0.5f);

    Graphics dirtGfx;
    dirtGfx.mesh_ = circleMesh;
    dirtGfx.texture_ = nullptr; // AEGfxTextureLoad("Assets/dirt_particle.png");
    dirtGfx.layer_ = 5;
    vfxSystem.SetGraphicsConfig(VFXType::DirtBurst, dirtGfx);

    // Emitter setup
    EmitterConfig dirtConfig;
    dirtConfig.spawnCount_ = 20;
    dirtConfig.minLife_ = 0.3f;
    dirtConfig.maxLife_ = 0.6f;
    dirtConfig.minSpeed_ = 50.0f;
    dirtConfig.maxSpeed_ = 150.0f;
    dirtConfig.minScale_ = 10.0f;
    dirtConfig.maxScale_ = 11.5f;
    dirtConfig.r_ = 0.3f;
    dirtConfig.g_ = 0.2f;
    dirtConfig.b_ = 0.1f;
    dirtConfig.a_ = 1.0f;
    vfxSystem.SetEmitterConfig(VFXType::DirtBurst, dirtConfig);

    // EmitterConfig portalConfig;
    //  ... setup portal config ...
    // vfxSystem.SetEmitterConfig(VFXType::Portal, portalConfig);
}