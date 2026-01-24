#include "AEEngine.h"

#include "FluidSystem.h"
#include "Utils.h"

#include <cmath>
#include <iostream>

// ==========================================
// FluidParticle
// ==========================================

FluidParticle::FluidParticle(f32 posX, f32 posY, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {100.0f, 100.0f};
    transform_.rotationRad_ = 0.0;
    switch (type) {
    case FluidType::Water:
        type_ = FluidType::Water;
    case FluidType::Lava:
        type_ = FluidType::Lava;
    default:
        type_ = FluidType::Water;
    }
}

FluidParticle::FluidParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {scaleX, scaleY};
    transform_.rotationRad_ = rot;
    type_ = type;
}

// ==========================================
// FluidSystem
// ==========================================

void FluidSystem::Initialize() {

    // CreateCircleMesh(number of slices);
    AEGfxVertexList* circleMesh = CreateCircleMesh(4);

    // Assign circle mesh to all particle types
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        if (circleMesh != nullptr) {
            graphicsConfigs_[i].mesh_ = circleMesh;
        }
    }

    //  Set Default Colors
    SetTypeColor(FluidType::Water, 0.0f, 0.5f, 1.0f, 0.8f);
    SetTypeColor(FluidType::Lava, 1.0f, 0.2f, 0.0f, 1.0f);
}

// Sets the matrices for every single INDIVIDUAL particle in the
// specified particle pool.
void FluidSystem::UpdateTransforms(std::vector<FluidParticle>& particlePool) {

    for (auto& p : particlePool) {

        AEMtx33 scale, rot, trans;

        AEMtx33Scale(&scale, p.transform_.scale_.x, p.transform_.scale_.y);
        AEMtx33Rot(&rot, p.transform_.rotationRad_);
        AEMtx33Trans(&trans, p.transform_.pos_.x, p.transform_.pos_.y);

        // Concatenate 1: Scale -> Rotate -> Store
        // Concatenate 2: Store -> Translate -> Store
        AEMtx33Concat(&p.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&p.transform_.worldMtx_, &trans, &p.transform_.worldMtx_);
    }
}

void FluidSystem::UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt) {}

// This function affects ALL particles (used after all other sub-Update functions)
void FluidSystem::UpdateMain(f32 dt) {
    for (int i = 0; i < (int)FluidType::Count; i++) {
        // Skip empty pools to save time
        if (particlePools_[i].empty())
            continue;

        // updates ALL particles within this pool
        UpdatePhysics(particlePools_[i], dt);
        UpdateTransforms(particlePools_[i]);
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

    // texture mode
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // Loops through (0) Water, (1) Lava, ...
    for (int i = 0; i < (int)FluidType::Count; ++i) {

        // if particle pool is empty, completely skip this pool
        if (particlePools_[i].empty()) {
            continue;
        }

        // get texture from current fluidsystem texture ptr
        AEGfxTextureSet(graphicsConfigs_[i].texture_, 0, 0);

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

void FluidSystem::Free() {}

void FluidSystem::SetTypeColor(FluidType type, f32 r, f32 g, f32 b, f32 a) {
    // 1. Convert Enum to Integer index (Water -> 0, Lava -> 1)
    int i = (int)type;

    // 2. Write to the config array
    colorConfigs_[i][0] = r;
    colorConfigs_[i][1] = g;
    colorConfigs_[i][2] = b;
    colorConfigs_[i][3] = a;
}

int FluidSystem::GetParticleCount(FluidType type) { return particlePools_[(int)type].size(); }

void FluidSystem::SpawnParticle_d(f32 posX, f32 posY, FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, type);
    particlePools_[i].push_back(newParticle);
}

void FluidSystem::SpawnParticle_c(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot,
                                  FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, scaleX, scaleY, rot, type);
    particlePools_[i].push_back(newParticle);
}
