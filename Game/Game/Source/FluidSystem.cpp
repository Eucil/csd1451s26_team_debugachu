#include "AEEngine.h"

#include "FluidSystem.h"
#include "Utils.h"

#include <cmath>
#include <iostream>

// ==========================================
// FluidParticle
// ==========================================

// @todo incomplete, should set physics as well
FluidParticle::FluidParticle(f32 posX, f32 posY, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {1.0f, 1.0f};
    transform_.rotationRad_ = 0.0f;


    switch (type) {
    case FluidType::Water:
        type_ = FluidType::Water;
        transform_.radius_ = 20.0f;
    case FluidType::Lava:
        type_ = FluidType::Lava;
        transform_.radius_ = 10.0f;
    default:
        type_ = FluidType::Water;
        transform_.radius_ = 20.0f;
    }
    transform_.scale_ = {transform_.radius_, transform_.radius_};
}

// @todo incomplete, should set physics as well
FluidParticle::FluidParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, f32 rad, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {scaleX, scaleY};
    transform_.rotationRad_ = rot;
    transform_.radius_ = rad;

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

// REPLACE this with new collision system later on
void FluidSystem::UpdateCollision(std::vector<FluidParticle>& particlePool, f32 dt) { 



    // PSEUDO PHYSICS 1. Simple Wall Collisions 
    for (auto& p : particlePool) {

        // If particle goes beyond left wall, set its position back to the wall
        if (p.transform_.pos_.x > 700.0f) {
			p.transform_.pos_.x = 700.0f;
		}
        // If particle goes beyond right wall, set its position back to the wall
        if (p.transform_.pos_.x < -700.0f) {
            p.transform_.pos_.x = -700.0f;
            
        }
        // If particle goes beyond the floor, set its position back to the floor
        if (p.transform_.pos_.y < -400.0f) {
            p.transform_.pos_.y = -400.0f;

            // Simple bounce/friction
            p.physics_.velocity_.y *= -0.5f; 
        }



    }

    // COLLISION: Particle vs Particle
    size_t count = particlePool.size();

    // Currently, we are checking one particle against every other particle (bad and we should replace this later!!!!!)
    // For example, particle in particlePool[0] is checked against all other particles in the pool
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            FluidParticle& p1 = particlePool[i];
            FluidParticle& p2 = particlePool[j];

            // Calculate distance squared between p1 and p2
            f32 dx = p1.transform_.pos_.x - p2.transform_.pos_.x;
            f32 dy = p1.transform_.pos_.y - p2.transform_.pos_.y;
            f32 distSq = dx * dx + dy * dy;

            f32 radius = p1.transform_.radius_; // Replace with p1.collider.radius if you have it
            f32 minDist = radius * 2.0f;
        }
    }



    // OPTIMISATION: STOPS VERY SLOW PARTICLES
    // If the particle is moving very slowly, we just stop it completely to save calculations 
    // while also preventing "ghost jittering" on the floor.
    for (auto& p : particlePool) 
    {
        if ((p.physics_.velocity_.x * p.physics_.velocity_.x) +
            (p.physics_.velocity_.y * p.physics_.velocity_.y) <  1.99f) {

            p.physics_.velocity_.x = 0.0f;
            p.physics_.velocity_.y = 0.0f;
        }
    }

    
    // TRASH but maybe useful CODE:
    /*
    * size_t count = particlePool.size();
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            FluidParticle& p1 = particlePool[i];
            FluidParticle& p2 = particlePool[j];

            // Calculate distance squared (avoid sqrt for performance checks)
            f32 dx = p1.transform_.pos_.x - p2.transform_.pos_.x;
            f32 dy = p1.transform_.pos_.y - p2.transform_.pos_.y;
            f32 distSq = dx*dx + dy*dy;

            // Assuming all particles have the same radius for now
            f32 radius = 10.0f; // Replace with p1.collider.radius if you have it
            f32 minDist = radius * 2.0f;

            if (distSq < minDist * minDist) {
                // COLLISION DETECTED
                f32 dist = sqrtf(distSq);
                
                // Prevent division by zero
                if (dist < 0.0001f) dist = 0.0001f; 

                // Calculate how much they overlap
                f32 overlap = minDist - dist;

                // Normalized vector pointing from p2 to p1
                f32 nx = dx / dist;
                f32 ny = dy / dist;

                // Resolve: Push them apart (half the overlap each)
                f32 moveX = nx * (overlap * 0.5f);
                f32 moveY = ny * (overlap * 0.5f);

                p1.transform_.pos_.x += moveX;
                p1.transform_.pos_.y += moveY;
                p2.transform_.pos_.x -= moveX;
                p2.transform_.pos_.y -= moveY;
            }
        }
    }
    * 
    * 
    * 
    * 
    * 
    // DAMPING VALUE:
    // mimics air resistance, slows down particles over time
    p.vx *= 0.999f;
    p.vy *= 0.999f;

    */

}   


// Sub-function, called under UpdateMain(), AFTER UpdateCollision
void FluidSystem::UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt) {

    //  load new constants with config values
    f32 mass = particlePool[0].physics_.mass_;
    f32 gravity = particlePool[0].physics_.gravity_ ;

    for (auto& p : particlePool) {

        // PSEUDO-PHYSICS:
        // velocity accumulates over time under constant acceleration which in this case is gravity,
        // multiply by dt to make the simulation frame rate independent, then update position based
        // on velocity
        p.physics_.velocity_.y += gravity * dt;









        // GLOBAL FUNCTION
        // Updates Position after UpdateCollision and main physics calculations within UpdatePhysics has been done.
        p.transform_.pos_.y += p.physics_.velocity_.y * dt;

    }
    
}

// This function affects ALL particles (used after all other sub-Update functions)
void FluidSystem::UpdateMain(f32 dt) {
    for (int i = 0; i < (int)FluidType::Count; i++) {

        // Skip empty pools to save time
        if (particlePools_[i].empty())  continue;


        // 1. Moves the particles (Apply Gravity/Velocity)
        UpdatePhysics(particlePools_[i], dt);


        // 2. Apply collision (Push them out of walls/each other)
        UpdateCollision(particlePools_[i], dt);


        // 3. Update the graphics matrix
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

    // The configs here refers to colorConfigs_ stored in FluidSystem
    colorConfigs_[i][0] = r; 
    colorConfigs_[i][1] = g;
    colorConfigs_[i][2] = b;
    colorConfigs_[i][3] = a;
}

void FluidSystem::SetTypeGraphics(AEGfxVertexList* mesh, AEGfxTexture* texture, u32 layer,
    FluidType type) {
    
    int i = (int)type;

    // The configs here refers to graphicsConfigs_ stored in FluidSystem
    // mesh, texture, layer refer to pointers declared within the file that calls this function
	graphicsConfigs_[i].mesh_ = mesh;  
	graphicsConfigs_[i].texture_ = texture;
	graphicsConfigs_[i].layer_ = layer;

}

void FluidSystem::SetTypeTransform(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rotRad, f32 radius, FluidType type) {
	
	int i = (int)type;

	// The configs here refers to transformConfigs_ stored in FluidSystem
    transformConfigs_[i][0] = posX;
    transformConfigs_[i][1] = posY;
    transformConfigs_[i][2] = scaleX;
    transformConfigs_[i][3] = scaleY;
    transformConfigs_[i][4] = rotRad;
    transformConfigs_[i][5] = radius;
}

int FluidSystem::GetParticleCount(FluidType type) { return particlePools_[(int)type].size(); }

void FluidSystem::SpawnParticle(f32 posX, f32 posY, FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, type);
    particlePools_[i].push_back(newParticle);
}


void FluidSystem::SpawnParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, f32 rad, FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, scaleX, scaleY, rot, rad, type);
    particlePools_[i].push_back(newParticle);
}
