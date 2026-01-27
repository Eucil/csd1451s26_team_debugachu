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
    // used for drawing
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {1.0f, 1.0f};
    transform_.rotationRad_ = 0.0f;

    // misc
    type_ = type;

    // uesd for physics
    switch (type) {
    case FluidType::Water:
        transform_.radius_ = 10.0f;
        break;
    case FluidType::Lava:
        transform_.radius_ = 10.0f;
        break;
    default:
        transform_.radius_ = 20.0f;
        break;
    }

    // Multiply scale by 2.0f coz radius is radius (not diameter)
    transform_.scale_ = {transform_.radius_ * 2.0f, transform_.radius_ * 2.0f};
}

// @todo incomplete, should set physics as well
FluidParticle::FluidParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, f32 rot, f32 rad, FluidType type) {

    // used for drawing
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {scaleX, scaleY};
    transform_.rotationRad_ = rot;

    // used for physics
    transform_.radius_ = rad;

    // misc
    type_ = type;
    switch (type) {
    case FluidType::Water:
        break;
    case FluidType::Lava:
        break;
    default:
        break;
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
    SetTypeColor(0.0f, 0.5f, 1.0f, 1.0f, FluidType ::Water);
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

    // EFFECT 1. Simple Wall Collisions 
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

            // EFFECT 2: Simple bounce/friction
            //
            // We half the velocity upon collision with the floor to simulate energy loss
            p.physics_.velocity_.y *= -0.5f; 
        }



    }
    // ---------------------------------------------------- //
    // PROBLEM 1: COLLISION (Particle vs Particle)
    // ---------------------------------------------------- //
    // 
    // Currently, the particles are able to fall straight to the floor and bounce back up just like in real life.
    // However, the particles lack collision and pass through one another. 
    // Hence, we need to implement particle-particle collision.

    size_t count = particlePool.size();

    // O(n^2) collision detection
    // Currently, we are checking one particle (p1) against every other particle     <---- (bad, slow and we should replace this later!!!!!)
    // For example, particle in particlePool[0] is checked against all other particles in the pool
    // 
    // This means that all the calculations below are done N * (N-1) / 2 times per frame, where N = number of particles.
    // So particleA checks against particleB, particleC, particleD, ...
    // then particleB checks against particleC, particleD, ...
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            FluidParticle& p1 = particlePool[i];
            FluidParticle& p2 = particlePool[j];

            // Calculate distance between p1 and p2 (this is our direction vector from p1 to p2)
            f32 dx = p1.transform_.pos_.x - p2.transform_.pos_.x;
            f32 dy = p1.transform_.pos_.y - p2.transform_.pos_.y;

            f32 radius = p1.transform_.radius_;  // <--- collider radius / 

            // minDist refers to the MINIMUM distance from center of p1 to center of p2 before collision occurs

            f32 minDist = radius * 2.0f;

            // distSq refers to the ACTUAL distance from center of p1 to center of p2 squared
            // (this is the magnitude of the direction vector from p1 to p2 and thus the length it)
            f32 distSq = dx * dx + dy * dy;

            // f32 dist = sqrtf(distSq); <-  // Minor OPTIMISATION: Only calculate square root if COLLISION DETECTED!!
          

            // If actual distance < minimum distance = COLLISION DETECTED
            if (distSq < minDist * minDist) {

                // Only calculate square root when collision is detected
                f32 dist = sqrtf(distSq);

                // Prevent division by zero if dist is extremely small
                if (dist < 0.0001f) {
                    dist = 0.0001f;
                }


                // Use the distance between p2 and p1 to calculate a unit vector (normalised vector)
                // this means that for every unit of distance, posx and posy changes by nx, ny.
                // So any calculations involving moving posX and posY should be multiplied by nx and ny.
                // 
                // (so that diagonal movement is the same as horizontal/vertical movement)
                // (this is equivalent to normal vector = (1/ magnitude) * direction vector)
                f32 nx = dx / dist;
                f32 ny = dy / dist;


                // We now calculate how much the two particles overlap when they collide
                //
                // REMEMBER: distSq = dx * dx + dy * dy, 
                //           minDist = radius * 2.0f,
                //           dist = sqrtf(distSq).
                f32 overlap = minDist - dist;


                // EFFECT 3. Simple Particle Repulsion
                // 
                // Resolve the collision by pushing them apart based on the amount of overlap calculated above
                // 
                // We multiply it by 0.4f to 0.5f since we want to move both particles equally.
                // For example, if 0.5f is used,
                // p1 gets half of moveX to the left, p2 gets half of moveX to the right
                // 
                // In this case, moveX and moveY acts just like velocity except it is calculated based on how much they overlap.
                // 
                // ADJUST the multiplier value to change how strongly they repel each other (original: 0.5f)
                // 
                // REMEMBER: nx,ny is a UNIT VECTOR of the DIRECTION VECTOR from p1 to p2 and moveX and Y are just like velocity!!
                //           We multiply it by ny and nx in order to apply the right amount of force in each axis.
                //           Without nx,ny, the particles would only move in the x direction or y direction only.
                //           nx,ny guides the movement in the correct direction. 
                // 
                //           For example,
                //           nx = 1.0, ny = 0.0:        move only in x direction
                //           nx = 0.0, ny = 1.0:        move only in y direction
                //           nx = 0.707, ny = 0.707:    move diagonally (45 degrees)
                // 
                // nx = dx / dist, which is equivalent to the normalised x direction vector from p1 to p2.
                // ny = dy / dist, which is equivalent to the normalised y direction vector from p1 to p2.


                f32 moveX = nx * (overlap * 0.4f);
                f32 moveY = ny * (overlap * 0.4f);



                // EFFECT 4. Weighted Particle Movement Based on Vertical Position
                // 
                // If P1 is below P2, P1 is supporting weight. Thus, P1 should move LESS.
                if (p1.transform_.pos_.y < p2.transform_.pos_.y) {
                    p1.physics_.mass_ = 0.2f;
                    p2.physics_.mass_ = 0.6f;
                } else {
                    p1.physics_.mass_ = 0.6f;
                    p2.physics_.mass_ = 0.2f;
                }

                // Apply the calculated movement to both particles directly to transform_.pos so that they
                // stop colliding instantly
                p1.transform_.pos_.x += moveX * p1.physics_.mass_;
                p1.transform_.pos_.y += moveY * p1.physics_.mass_;
                p2.transform_.pos_.x -= moveX * p2.physics_.mass_;
                p2.transform_.pos_.y -= moveY * p2.physics_.mass_;

                // ---------------------------------------------------- //
                // PROBLEM 2: INFINITE COLLISION LOOP
                // ---------------------------------------------------- //
                /*
                *   Okay, now that we have resolved particle-particle collision by pushing them apart, we have also
                *   unfortunately introduced a new problem:
                *
                *   When particle A pushes particle B away to the right, particle B's velocity is still the same as before.
                *   (p1.physics_.velocity_.x and y is unchanged).
                * 
                * 
                *   Hence, 
                *   If velocity was initially positive in the x axis, it continues moving to the right and colliding into particle C, 
                *   causing particle C to do the same thing.
                *   If velocity was initially negative in the x axis, it continues moving to the left and colliding into particle B again,
                *   and thus, a never ending infinite collision loop is created.
                */



                //  EFFECT 5. Velocity Resolution Upon Collision (Impulse)
                //
                // Upon collision, we should calculate the relative difference in velocity between the two particles
                // For example, if relativeVX > 0, it means p1 is moving faster than p2 in the x axis
                f32 relativeVX = p1.physics_.velocity_.x - p2.physics_.velocity_.x;
                f32 relativeVY = p1.physics_.velocity_.y - p2.physics_.velocity_.y;

                // Now we multiply by nx,ny to give them the correct "direction" again.
                f32 velAlongNormalX = relativeVX * nx;
                f32 velAlongNormalY = relativeVY * ny;

                if (velAlongNormalX + velAlongNormalY < 0.0f) {
                    // An impulse is composed of two distinct phases: Compression and Restitution
                    // The value below 

                    f32 e = 0.0f;

                    f32 restitution = 1.0f; // <--- BOUNCINESS FACTOR (original: 0.5f)

                }


                // EFFECT X. Simple Particle Velocity Damping (remove if effect cannot be seen anymore)
                //
                p1.physics_.velocity_.x *= 0.99f;
                p1.physics_.velocity_.y *= 0.99f;

            }
        }
    }





}   


// Sub-function, called under UpdateMain(), AFTER UpdateCollision
void FluidSystem::UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt) {

    //  load new constants with config values
    f32 mass = particlePool[0].physics_.mass_;
    f32 gravity = particlePool[0].physics_.gravity_ ;

    for (auto& p : particlePool) {

        // EFFECT 1: Gravity
        // 
        // Applies gravity to the particle's velocity.
        // We multiply by dt to make the simulation frame rate independent, then update position
        p.physics_.velocity_.y += gravity * dt;









        // GLOBAL FUNCTION
        // Updates Position after UpdateCollision and main physics calculations within UpdatePhysics has been done.
        p.transform_.pos_.x += p.physics_.velocity_.x * dt; 
        p.transform_.pos_.y += p.physics_.velocity_.y * dt;


        // OPTIMISATION: STOPS VERY SLOW PARTICLES
        // 
        // If the particle is moving very slowly, we set velocity to 0 to stop it completely to 
        // save calculations while also preventing "ghost jittering" on the floor. 
        // 
        // 1.99f refers to the THRESHOLD (1.41f) velocity squared.
        // For example, if the particle is moving slower than 1.41 units per second, stop it completely.
        //
        // NOTE: 1.99f is a MAGIC NUMBER, it was chosen coz if the particle is moving at less than (1 pixel, 1 pixel) velocity,
        //       it is considered miniscule.
        if ((p.physics_.velocity_.x * p.physics_.velocity_.x) +
           (p.physics_.velocity_.y * p.physics_.velocity_.y) <
            1.99f) {
            p.physics_.velocity_.x = 0.0f;
            p.physics_.velocity_.y = 0.0f;
        }
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
