#include "AEEngine.h"

#include "FluidSystem.h"
#include "Utils.h"

#include <cmath>
#include <iostream>

// ==========================================
// FluidParticle
// ==========================================
// @todo incomplete, should set physics as well
FluidParticle::FluidParticle(f32 posX, f32 posY, f32 radius, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {radius * 2.0f,
                         radius * 2.0f}; // Multiply by 2.0f as scale represents diameter
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius =
        radius * 0.7f; // * 0.7f so that collider is smaller than mesh

    type_ = type;
}

// ==========================================
// FluidSystem
// ==========================================

void FluidSystem::Initialize() {
    InitializeMesh();

    // Reduces memory reallocation
    int typeCount{static_cast<int>(FluidType::Count)};
    for (int i{0}; i < typeCount; i++) {
        particlePools_[i].reserve(1000);
    }

    SetTypeColor(0.0f, 0.5f, 1.0f, 1.0f, FluidType ::Water);
    SetTypeColor(1.0f, 0.2f, 0.0f, 1.0f, FluidType::Lava);
}

// This function affects ALL particles (used after all other sub-Update functions)
void FluidSystem::UpdateMain(f32 dt) {
    // ================================================ //
    // OPTIMISATION: DT CLAMPING
    // ================================================ //
    //
    // We clamp dt to a maximum value to avoid instability
    // For example, if there is a lag spike, dt could be very high and cause particles to teleport
    // Hence, we set it to 0.016f (60 FPS) max
    if (dt > 0.016f) {
        dt = 0.016f;
    }
    // ================================================ //
    // OPTIMISATION: SUB-STEPS FOR STABILITY
    // ================================================ //
    //
    // How?
    // We divide the dt into smaller chunks and run the physics and collision multiple times
    //
    // Assume gravity = -1000. Without the substep, it would be -1000 * 0.016 = -16 units per frame
    // This results in -16 units per frame movement, which the collision solver has to resolve all
    // at once.
    //
    // Now assume gravity = -1000 with a substep of 4. With the substep, each subDt = 0.004f = -4
    // units per sub-frame. This means that the collision solver only has to resolve -4 units of
    // movement per sub-frame,
    const int subSteps = 4;
    f32 subDt = dt / (f32)subSteps;

    // run the simulation substep number of times per frame
    for (int s = 0; s < subSteps; s++) {
        for (int i = 0; i < (int)FluidType::Count; i++) {
            if (particlePools_[i].empty())
                continue;
            UpdatePhysics(particlePools_[i], subDt);
            UpdateCollision(particlePools_[i], subDt);
        }
    }

    for (int i = 0; i < (int)FluidType::Count; i++) {

        // Skip empty pools to save time/performance
        if (particlePools_[i].empty()) {
            continue;
        } else {
            // Update the graphics matrix
            UpdateTransforms(particlePools_[i]);
            UpdatePortalIframes(dt, particlePools_[i]);
        }
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
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // Loops through (0) Water, (1) Lava, ...
    for (int i = 0; i < (int)FluidType::Count; ++i) {

        // if particle pool is empty, completely skip this pool
        if (particlePools_[i].empty()) {
            continue;
        }

        AEGfxTextureSet(graphicsConfigs_[i].texture_, 0, 0);

        AEGfxSetColorToMultiply(colorConfigs_[i][0],  //  <-- r
                                colorConfigs_[i][1],  //  <-- g
                                colorConfigs_[i][2],  //  <-- b
                                colorConfigs_[i][3]); //  <-- alpha

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(colorConfigs_[i][3]);

        // Loop through each particle
        for (auto& p : particlePools_[i]) {
            AEGfxSetTransform(p.transform_.worldMtx_.m);
            AEGfxMeshDraw(graphicsConfigs_[i].mesh_, AE_GFX_MDM_TRIANGLES);
        }
    }
}

void FluidSystem::Free() {
    // Free the mesh once and then set the rest
    if (graphicsConfigs_[0].mesh_ != nullptr) {
        AEGfxMeshFree(graphicsConfigs_[0].mesh_);
    }

    // Nullify all mesh pointers so we don't accidentally use dead memory
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        graphicsConfigs_[i].mesh_ = nullptr;
    }

    // Free textures
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        if (graphicsConfigs_[i].texture_ != nullptr) {
            AEGfxTextureUnload(
                graphicsConfigs_[i].texture_); // Or AEGfxTextureFree depending on version
            graphicsConfigs_[i].texture_ = nullptr;
        }
    }

    // Empty the particle pool
    for (int i = 0; i < (int)FluidType::Count; ++i) {
        particlePools_[i].clear();
    }
}

void FluidSystem::SpawnParticle(f32 posX, f32 posY, f32 radius, FluidType type) {
    int i = (int)type;
    FluidParticle newParticle(posX, posY, radius, type);
    particlePools_[i].push_back(newParticle);
}

int FluidSystem::GetParticleCount(FluidType type) { return particlePools_[(int)type].size(); }

std::vector<FluidParticle>& FluidSystem::GetParticlePool(FluidType type) {
    return particlePools_[(int)type];
}

// UPDATE (MAYBE BROKEN)
void FluidSystem::InitializeMesh() {
    AEGfxVertexList* circleMesh = CreateCircleMesh(20);

    // Assign circle mesh to each particle type
    for (int i{0}; i < static_cast<int>(FluidType::Count); ++i) {
        if (circleMesh != nullptr) {
            graphicsConfigs_[i].mesh_ = circleMesh;
        }
    }
}

void FluidSystem::UpdateTransforms(std::vector<FluidParticle>& particlePool) {

    for (auto& p : particlePool) {

        AEMtx33 scale, rot, trans;

        AEMtx33Scale(&scale, p.transform_.scale_.x, p.transform_.scale_.y);
        AEMtx33Rot(&rot, p.transform_.rotationRad_);
        AEMtx33Trans(&trans, p.transform_.pos_.x, p.transform_.pos_.y);

        // worldMtx = trans * rot * scale
        AEMtx33Concat(&p.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&p.transform_.worldMtx_, &trans, &p.transform_.worldMtx_);
    }
}

void FluidSystem::UpdateCollision(std::vector<FluidParticle>& particlePool, f32 dt) {
    size_t count = particlePool.size();
    s32 typeCount{static_cast<int>(FluidType::Count)};

    // ================================================ //
    // EFFECT 1. Simple Wall Collisions
    // ================================================ //

    for (auto& p : particlePool) {

        // If particle goes beyond left wall, set its position back to the wall
        if (p.transform_.pos_.x > 800.0f) {
            p.transform_.pos_.x = 800.0f;
            p.physics_.velocity_.x *= -0.5f;
        }
        // If particle goes beyond right wall, set its position back to the wall
        if (p.transform_.pos_.x < -800.0f) {
            p.transform_.pos_.x = -800.0f;
            p.physics_.velocity_.x *= -0.5f;
        }
        // If particle goes beyond the floor, set its position back to the floor
        if (p.transform_.pos_.y < -450.0f) {
            p.transform_.pos_.y = -450.0f;
            // ================================================ //
            // EFFECT 2: Simple bounce/friction
            // ================================================ //
            //
            // When particle hits the floor, reverse its velocity and reduce it by half
            // If velocity is HIGH, we bounce it back up
            if (p.physics_.velocity_.y < -20.0f) {
                p.physics_.velocity_.y *= -0.5f;
            } else {
                // If velocity is too small, stop dead
                p.physics_.velocity_.y *= -0.2f;
            }
        }
    }
    // ================================================ //
    // PROBLEM 1: COLLISION (Particle vs Particle)
    // ================================================ //

    // Time complexity: O(n^2)

    // Runs the collision solver multiple times to improve stability
    // For example, even after one iteration of collision resolution,
    // some particles may still be overlapping slightly.
    int solverIterations = 2;
    std::cout << count << '\n';

    for (int iter = 0; iter < solverIterations; ++iter) {
        for (size_t i = 0; i < count; ++i) {

            for (size_t j = i + 1; j < count; ++j) {
                FluidParticle& p1 = particlePool[i];
                FluidParticle& p2 = particlePool[j];

                // Calculate distance between p1 and p2 (this is our direction vector from p1 to p2)
                f32 dx = p1.transform_.pos_.x - p2.transform_.pos_.x;
                f32 dy = p1.transform_.pos_.y - p2.transform_.pos_.y;

                // ================================================ //
                // EFFECT 3: JITTER FIX FOR VERTICAL STACKS
                // ================================================ //
                //
                // If dx is extremely small (meaning that particles are vertically aligned),
                // we add a tiny amount of random noise to dx.
                if (std::abs(dx) < 0.001f) {
                    // Generate a tiny random float between -0.05 and 0.05
                    f32 noise = ((i * 12345) % 100) * 0.001f - 0.5f;
                    dx += noise;
                }

                // Calculate minimum distance between p1 and p2 for collision to occur
                f32 minDist =
                    p1.collider_.shapeData_.circle_.radius + p2.collider_.shapeData_.circle_.radius;

                // Calculate squared distance from p1 to p2
                f32 distSq = dx * dx + dy * dy;

                // Check for collision
                if (distSq < minDist * minDist) {
                    // Only calculate square root when collision is detected
                    f32 dist = sqrtf(distSq);

                    // Prevent division by zero if dist is extremely small
                    if (dist < 0.0001f) {
                        dist = 0.0001f;
                    }

                    // Calculate unit vector
                    // Any calculations involving moving posX and posY should be multiplied by nx
                    // and ny
                    f32 nx = dx / dist;
                    f32 ny = dy / dist;

                    f32 overlap = minDist - dist;

                    // ================================================ //
                    // EFFECT 4. Particle Repulsion
                    // ================================================ //
                    //
                    // Resolve the collision by pushing them apart based on the amount of overlap
                    // calculated above
                    //
                    // We multiply it by 0.2f to 0.5f since we want to move both particles equally.
                    //
                    // For example,
                    // If 0.5f is used,
                    // the particles resolve collision instantly as p1 gets half of moveX to the
                    // left, p2 gets half of moveX to the right If 0.2f is used, the particles act
                    // more like liquid and "squish" against each other before separating fully.
                    //
                    //
                    // In this case, moveX and moveY acts just like velocity except it is calculated
                    // based on how much they overlap.
                    //
                    // ADJUST the repulsion value to change how strongly they repel each other
                    // (original: 0.5f)
                    //
                    // REMEMBER: nx,ny is a UNIT VECTOR of the DIRECTION VECTOR from p1 to p2 and
                    // moveX and Y are just like velocity!!
                    //           We multiply it by ny and nx in order to apply the right amount of
                    //           force in each axis. Without nx,ny, the particles would only move in
                    //           the x direction or y direction only. nx,ny guides the movement in
                    //           the correct direction.
                    //
                    //           For example,
                    //           nx = 1.0, ny = 0.0:        move only in x direction
                    //           nx = 0.0, ny = 1.0:        move only in y direction
                    //           nx = 0.707, ny = 0.707:    move diagonally (45 degrees)
                    //
                    // nx = dx / dist, which is equivalent to the normalised x direction vector from
                    // p1 to p2. ny = dy / dist, which is equivalent to the normalised y direction
                    // vector from p1 to p2.

                    f32 repulsion = 0.0f;

                    // If overlap is small, use weaker repulsion
                    if (overlap < (p1.collider_.shapeData_.circle_.radius +
                                   p2.collider_.shapeData_.circle_.radius) /
                                      2 * 0.2f) {
                        repulsion = 0.2f;
                    }
                    // If overlap is large, use stronger repulsion
                    else {
                        repulsion = 0.5f;
                    }

                    f32 moveX = nx * (overlap * repulsion);
                    f32 moveY = ny * (overlap * repulsion);

                    // Apply the calculated movement to both particles directly to transform_.pos so
                    // that they stop colliding instantly
                    p1.transform_.pos_.x += moveX;
                    p1.transform_.pos_.y += moveY;
                    p2.transform_.pos_.x -= moveX;
                    p2.transform_.pos_.y -= moveY;

                    // ================================================ //
                    // PROBLEM 2: INFINITE COLLISION LOOP
                    // ================================================ //
                    /*
                     *   Okay, now that we have resolved particle-particle collision by pushing them
                     * apart, we have also unfortunately introduced a new problem:
                     *
                     *   When particle A pushes particle B away to the right, particle B's velocity
                     * is still the same as before. (p1.physics_.velocity_.x and y is unchanged).
                     *
                     *
                     *   Hence,
                     *   If velocity was initially positive in the x axis, it continues moving to
                     * the right and colliding into particle C, causing particle C to do the same
                     * thing. If velocity was initially negative in the x axis, it continues moving
                     * to the left and colliding into particle B again, and thus, a never ending
                     * infinite collision loop is created.
                     */

                    // =========================================================== //
                    //  EFFECT 5. Velocity Resolution Upon Collision (Impulse)
                    // =========================================================== //
                    //
                    // Upon collision, we should calculate the relative difference in velocity
                    // between the two particles For example, if relativeVX > 0, it means p1 is
                    // moving faster than p2 in the x axis
                    f32 relativeVX = p1.physics_.velocity_.x - p2.physics_.velocity_.x;
                    f32 relativeVY = p1.physics_.velocity_.y - p2.physics_.velocity_.y;

                    // Now we multiply by nx,ny to give them the correct "direction" again.
                    f32 velAlongNormalX = relativeVX * nx;
                    f32 velAlongNormalY = relativeVY * ny;

                    if (velAlongNormalX + velAlongNormalY < 0.0f) {

                        // Restitution determines the "Bounciness" of the collision.
                        // It calculates how much relative velocity is preserved or lost after
                        // impact.
                        //
                        // The variable 'restitution' is composed of two ideas:
                        //
                        //   1. A change in direction (-): We must reverse velocity to push
                        //   particles APART,
                        //      similar to how in real life a ball changes direction upon collision.
                        //
                        //   2. Magnitude :  Amount of energy conserved.
                        //
                        // -1.0f  : Perfectly Inelastic
                        //          Particles stop dead upon collision. No bounce energy is added.
                        //          Best for: Calm water, heavy fluids, stable stacks.
                        // -2.0f  : Perfectly Elastic
                        //          Particles bounce back with 100% of their original speed.
                        //          Best for: Billiards, Pong. Bad for fluids (never settles).

                        f32 restitution = -1.08f; // Original: -1.05f;

                        f32 j = restitution * (velAlongNormalX + velAlongNormalY) * 0.5f;
                        p1.physics_.velocity_.x += j * nx;
                        p1.physics_.velocity_.y += j * ny;
                        p2.physics_.velocity_.x -= j * nx;
                        p2.physics_.velocity_.y -= j * ny;
                    }
                }
            }
        }
    }
}

void FluidSystem::UpdatePhysics(std::vector<FluidParticle>& particlePool, f32 dt) {

    //  load new constants with config values
    f32 mass = particlePool[0].physics_.mass_;
    f32 gravity = particlePool[0].physics_.gravity_;

    for (auto& p : particlePool) {

        // ================================================ //
        // EFFECT 1: Gravity
        // ================================================ //
        //
        // Applies gravity to the particle's velocity.
        // We multiply by dt to make the simulation frame rate independent, then update position
        p.physics_.velocity_.y += gravity * dt;

        // Add a tiny random kick to every particle.
        // This prevents them from ever stacking perfectly still.
        f32 noiseX = ((rand() % 100) / 50.0f) - 1.0f; // Range -1.0 to 1.0
        f32 noiseY = ((rand() % 100) / 50.0f) - 1.0f; // Range -1.0 to 1.0

        p.physics_.velocity_.x += noiseX * dt * 3.0f;
        p.physics_.velocity_.y += noiseY * dt * 3.0f;

        // GLOBAL FUNCTION
        // Updates Position after UpdateCollision and main physics calculations within UpdatePhysics
        // has been done.
        p.transform_.pos_.x += p.physics_.velocity_.x * dt;
        p.transform_.pos_.y += p.physics_.velocity_.y * dt;

        // ================================================ //
        // OPTIMISATION: STOPS VERY SLOW PARTICLES
        // ================================================ //
        //
        // If the particle is moving very slowly, we set velocity to 0 to stop it completely to
        // save calculations while also preventing "ghost jittering" on the floor.
        //
        // 1.99f refers to the THRESHOLD (1.41f) velocity squared.
        // For example, if the particle is moving slower than 1.41 units per second, stop it
        // completely.
        //
        // NOTE: 1.99f is a MAGIC NUMBER, it was chosen coz if the particle is moving at less than
        // (1 pixel, 1 pixel) velocity,
        //       it is considered miniscule.
        if ((p.physics_.velocity_.x * p.physics_.velocity_.x) +
                (p.physics_.velocity_.y * p.physics_.velocity_.y) <
            1.99f) {
            p.physics_.velocity_.x = 0.0f;
            p.physics_.velocity_.y = 0.0f;
        }
    }
}

void FluidSystem::UpdatePortalIframes(f32 dt, std::vector<FluidParticle>& particlePool) {
    // Loop through all particles in the current pool
    for (auto& p : particlePool) {
        // If the particle is in iframe, reduce the iframe timer
        if (p.portal_iframe_) {
            p.portal_iframe_timer_ -= dt;
            // If the timer reaches zero, disable iframe
            if (p.portal_iframe_timer_ <= 0.0f) {
                p.portal_iframe_ = false;
                p.portal_iframe_timer_ = p.portal_iframe_maxduration_;
            }
        }
    }
}

void FluidSystem::SetTypeColor(f32 r, f32 g, f32 b, f32 a, FluidType type) {
    int i = (int)type;

    colorConfigs_[i][0] = r;
    colorConfigs_[i][1] = g;
    colorConfigs_[i][2] = b;
    colorConfigs_[i][3] = a;
}

void FluidSystem::SetTypeGraphics(AEGfxVertexList* mesh, AEGfxTexture* texture, u32 layer,
                                  FluidType type) {
    int i = (int)type;

    graphicsConfigs_[i].mesh_ = mesh;
    graphicsConfigs_[i].texture_ = texture;
    graphicsConfigs_[i].layer_ = layer;
}
