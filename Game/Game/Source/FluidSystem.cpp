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
    transform_.scale_ = {1.0f, 1.0f};
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

    switch (type) {
    case FluidType::Water:
        type_ = FluidType::Water;
    case FluidType::Lava:
        type_ = FluidType::Lava;
    default:
        type_ = FluidType::Water;
    }
}

// ==========================================
// FluidSystem
// ==========================================


// UPDATE (MAYBE BROKEN)
void FluidSystem::InitializeMesh() {

    // CreateCircleMesh(number of slices);
    AEGfxVertexList* circleMesh = CreateCircleMesh(20);

    // Assign circle mesh to all particle types
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        if (circleMesh != nullptr) {
            graphicsConfigs_[i].mesh_ = circleMesh;
        }
    }
}

void FluidSystem::Initialize() {

    // Initialize particle mesh
    InitializeMesh();

    // Initialize matri
    //**InitializePhysics();

    // Set colors for each particle type
    SetTypeColor(0.0f, 0.5f, 1.0f, 0.8f, FluidType ::Water);
    SetTypeColor(1.0f, 0.2f, 0.0f, 1.0f, FluidType::Lava);
}

// Sets the mesh-matrices for every single INDIVIDUAL particle in the
// specified particle pool.

// Sub-function, called under UpdateMain()
void FluidSystem::UpdateTransforms(std::vector<FluidParticle>& particlePool) {

    for (auto& p : particlePool) {

        AEMtx33 scale, rot, trans;

        // Takes scale, rotation, position FROM THE PARTICLE ITSELF
        AEMtx33Scale(&scale, p.transform_.scale_.x, p.transform_.scale_.y);
        AEMtx33Rot(&rot, p.transform_.rotationRad_);
        AEMtx33Trans(&trans, p.transform_.pos_.x, p.transform_.pos_.y);

        // Concatenate 1: Scale -> Rotate -> Store
        // Concatenate 2: Store -> Translate -> Store
        AEMtx33Concat(&p.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&p.transform_.worldMtx_, &trans, &p.transform_.worldMtx_);
    }
}


void FluidSystem::UpdateCollision() {
    // bla bla  
}   



// Sets the pos.x and pos.y for every single INDIVIDUAL particle in the
// specified particle pool.

// Sub-function, called under UpdateMain(), AFTER UpdateCollision
void FluidSystem::UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt, FluidType type) {
    int typeIndex = (int)type;

    // load new constants with config values
    f32 mass = particlePool[0].physics_.mass_;
    f32 gravity = particlePool[0].physics_.gravity_ ;

    for (auto& p : particlePool) {

        // EFFECT 1: causes the particle to fall downwards
        //physicsConfigs_[typeIndex].velocityX_ += gravity * dt;
        //p.physics_.velocityX_ += gravity * dt;
        p.physics_.velocityY_ += gravity * dt;

        // Update Position
        //p.transform_.pos_.x += p.physics_.velocityX_ * dt;
        p.transform_.pos_.y += p.physics_.velocityY_ * dt;





        // FAKE WALLS (BOUNDARY)
        if (p.transform_.pos_.x < -800.0f) {
            p.transform_.pos_.x = -800.0f;
        }
        if (p.transform_.pos_.y < -450.0f) {
			p.transform_.pos_.y = -450.0f;
		}
    }
    
}

// This function affects ALL particles (used after all other sub-Update functions)
void FluidSystem::UpdateMain(f32 dt) {
    for (int i = 0; i < (int)FluidType::Count; i++) {
        // Skip empty pools to save time
        if (particlePools_[i].empty())
            continue;

        // updates ALL particles within this pool
        UpdateCollision();  
        UpdatePhysics(particlePools_[i], dt, (FluidType)i);
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
void FluidSystem::Free() {
    // free the mesh once and then set the rest
    if (graphicsConfigs_[0].mesh_ != nullptr) {
        AEGfxMeshFree(graphicsConfigs_[0].mesh_);
    }

    // nullify all mesh pointers so we don't accidentally use dead memory
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        graphicsConfigs_[i].mesh_ = nullptr;
    }

    // free textures
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        if (graphicsConfigs_[i].texture_ != nullptr) {
            AEGfxTextureUnload(
                graphicsConfigs_[i].texture_); // Or AEGfxTextureFree depending on version
            graphicsConfigs_[i].texture_ = nullptr;
        }
    }

    // empty the vectors
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        particlePools_[i].clear();
    }
}

void FluidSystem::SetTypeColor(f32 r, f32 g, f32 b, f32 a, FluidType type) {
    int i = (int)type;

    colorConfigs_[i][0] = r;
    colorConfigs_[i][1] = g;
    colorConfigs_[i][2] = b;
    colorConfigs_[i][3] = a;
}

void FluidSystem::SetTypePhysics(f32 mass, f32 gravity, FluidType type) {

}

int FluidSystem::GetParticleCount(FluidType type) { return particlePools_[(int)type].size(); }

void FluidSystem::SpawnParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, scaleX, scaleY, rot, type);
    particlePools_[i].push_back(newParticle);
}
