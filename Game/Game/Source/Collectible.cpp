/*!
@file       Collectible.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date       March, 31, 2026

@brief      This source file contains the definitions of functions and classes
            that implement the collectible system, including:

                - Collectible constructors, which set up transform, collider,
                  type, and visual animation properties
                - CollectibleSystem, which manages loading, updating, drawing,
                  and freeing all collectibles in a level

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Collectible.h"

// Standard library
#include <cmath>

// Project
#include "AudioSystem.h"
#include "CollisionSystem.h"
#include "ConfigManager.h"
#include "MouseUtils.h"

// ==========================================
//            COLLECTIBLE
// ==========================================

// =========================================================
//
// Collectible::Collectible()
//
// - Sets position to the origin and scale to 30x30.
// - Sets the collider to a circle with radius 15.
// - Defaults the type to Star and resets animation timers.
//
// =========================================================
Collectible::Collectible() {
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {30.0f, 30.0f};
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius_ = 15.0f;
    collider_.shapeData_.circle_.offset_ = {0.0f, 0.0f};

    type_ = CollectibleType::Star;
    pulseTimer_ = 0.0f;
    rotationSpeed_ = 1.0f;
}

// =========================================================
//
// Collectible::Collectible(pos, type)
//
// - Sets position to the given coordinates and scale to 30x30.
// - Sets the collider to a circle with radius 15.
// - Assigns the given type and marks the collectible as active
//   and not yet collected.
// - Assigns a random rotation speed between 1.0 and 3.0.
//
// =========================================================
Collectible::Collectible(AEVec2 pos, CollectibleType type) {
    transform_.pos_ = pos;
    transform_.scale_ = {30.0f, 30.0f};
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius_ = 15.0f;
    collider_.shapeData_.circle_.offset_ = {0.0f, 0.0f};

    type_ = type;
    active_ = true;
    collected_ = false;
    pulseTimer_ = 0.0f;
    rotationSpeed_ = 1.0f + (AERandFloat() * 2.0f);
}

// ==========================================
//            COLLECTIBLE SYSTEM
// ==========================================

// =========================================================
//
// CollectibleSystem::load()
//
// - Stores the font handle for later use by the collection text.
// - Reads the collection text position and scale from JSON config.
// - Sets the initial content of the collection text.
//
// =========================================================
void CollectibleSystem::load(s8 font) {
    font_ = font;
    collectionText_.x_ =
        g_configManager.getFloat("Collectible", "default", "collectionText_.x_", -0.9f);
    collectionText_.y_ =
        g_configManager.getFloat("Collectible", "default", "collectionText_.y_", 0.92f);
    collectionText_.scale_ =
        g_configManager.getFloat("Collectible", "default", "collectionText_.scale_", 0.5f);
    collectionText_.content_ =
        g_configManager.getString("Collectible", "default", "collectionText_.x_", "Items: 0/3");
    collectionText_.font_ = font_;
}

// =========================================================
//
// CollectibleSystem::initialize()
//
// - Builds the star, gem, and leaf meshes.
// - Resets the collected count, total count, and global timer.
// - Clears all collectibles from the previous level.
//
// =========================================================
void CollectibleSystem::initialize() {
    createMeshes();
    collectedCount_ = 0;
    totalCollectibles_ = 0;
    globalTimer_ = 0.0f;
    collectibles_.clear();
}

// =========================================================
//
// CollectibleSystem::createMeshes()
//
// - Builds the star mesh as a 5-pointed shape using outer (0.5)
//   and inner (0.25) radii, forming two triangles per point.
// - Builds the gem mesh as a diamond using two triangles.
// - Builds the leaf mesh as a teardrop using 12 fan segments
//   with a sinusoidally varying radius per segment.
//
// =========================================================
void CollectibleSystem::createMeshes() {
    constexpr int kStarPoints = 5;
    constexpr f32 kOuterRadius = 0.5f;
    constexpr f32 kInnerRadius = 0.25f;

    AEGfxMeshStart();
    for (int i = 0; i < kStarPoints; i++) {
        f32 angle1 = (i * 2.0f * 3.14159f / kStarPoints) - 3.14159f / 2.0f;
        f32 angle2 = ((i + 1) * 2.0f * 3.14159f / kStarPoints) - 3.14159f / 2.0f;
        f32 midAngle = (angle1 + angle2) / 2.0f;
        f32 x1 = cosf(angle1) * kOuterRadius, y1 = sinf(angle1) * kOuterRadius;
        f32 x2 = cosf(angle2) * kOuterRadius, y2 = sinf(angle2) * kOuterRadius;
        f32 xi = cosf(midAngle) * kInnerRadius, yi = sinf(midAngle) * kInnerRadius;
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFF00, 0.5f, 0.5f, x1, y1, 0xFFFFFF00, x1 + 0.5f, y1 + 0.5f,
                    xi, yi, 0xFFFFFF00, xi + 0.5f, yi + 0.5f);
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFF00, 0.5f, 0.5f, xi, yi, 0xFFFFFF00, xi + 0.5f, yi + 0.5f,
                    x2, y2, 0xFFFFFF00, x2 + 0.5f, y2 + 0.5f);
    }
    starMesh_ = AEGfxMeshEnd();

    AEGfxMeshStart();
    AEGfxTriAdd(0.0f, 0.5f, 0xFFFF00FF, 0.5f, 1.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f, 0.0f,
                0xFFFF00FF, 1.0f, 0.5f);
    AEGfxTriAdd(0.0f, -0.5f, 0xFFFF00FF, 0.5f, 0.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f,
                0.0f, 0xFFFF00FF, 1.0f, 0.5f);
    gemMesh_ = AEGfxMeshEnd();

    constexpr int kLeafSegments = 12;
    AEGfxMeshStart();
    for (int i = 0; i < kLeafSegments; i++) {
        f32 angle1 = (i * 2.0f * 3.14159f / kLeafSegments);
        f32 angle2 = ((i + 1) * 2.0f * 3.14159f / kLeafSegments);
        f32 r1 = 0.3f + 0.2f * sinf(angle1 * 2.0f);
        f32 r2 = 0.3f + 0.2f * sinf(angle2 * 2.0f);
        f32 x1 = cosf(angle1) * r1, y1 = sinf(angle1) * r1;
        f32 x2 = cosf(angle2) * r2, y2 = sinf(angle2) * r2;
        AEGfxTriAdd(0.0f, 0.0f, 0xFF00FF00, 0.5f, 0.5f, x1, y1, 0xFF00FF00, x1 + 0.5f, y1 + 0.5f,
                    x2, y2, 0xFF00FF00, x2 + 0.5f, y2 + 0.5f);
    }
    leafMesh_ = AEGfxMeshEnd();
}

// =========================================================
//
// CollectibleSystem::loadLevelCollectibles()
//
// - Constructs a new Collectible at the given position with the given type.
// - Appends it to the collectibles vector.
// - Updates the total collectible count to match the vector size.
//
// =========================================================
void CollectibleSystem::loadLevelCollectibles(AEVec2 pos, CollectibleType type) {
    collectibles_.emplace_back(pos, type);
    totalCollectibles_ = static_cast<int>(collectibles_.size());
}

// =========================================================
//
// CollectibleSystem::checkCollisionWithWater()
//
// - Returns false immediately if the collectible is inactive or collected.
// - Computes the squared distance between the particle and collectible centres.
// - Returns true if the squared distance is less than the squared sum of radii.
//
// =========================================================
bool CollectibleSystem::checkCollisionWithWater(const Collectible& collectible,
                                                const FluidParticle& particle) {
    if (!collectible.active_ || collectible.collected_)
        return false;

    AEVec2 delta = {particle.transform_.pos_.x - collectible.transform_.pos_.x,
                    particle.transform_.pos_.y - collectible.transform_.pos_.y};

    f32 distSq = delta.x * delta.x + delta.y * delta.y;
    f32 radiusSum = particle.collider_.shapeData_.circle_.radius_ +
                    collectible.collider_.shapeData_.circle_.radius_;
    return distSq < (radiusSum * radiusSum);
}

// =========================================================
//
// CollectibleSystem::update()
//
// - Advances the global timer by delta time.
// - For each active uncollected collectible:
//   - Applies a sinusoidal pulse to the scale.
//   - Applies a continuous rotation by rotationSpeed_.
//   - Rebuilds the world transform matrix.
//   - Checks collision against all water particles.
//   - On collision, marks the collectible as collected, increments the
//     count, spawns the appropriate VFX, and plays the bell sound.
// - Updates the collection counter text string.
//
// =========================================================
void CollectibleSystem::update(f32 dt, std::vector<FluidParticle>& particlePool,
                               VFXSystem& vfxSystem) {
    globalTimer_ += dt;

    for (auto& c : collectibles_) {
        if (!c.active_ || c.collected_)
            continue;

        c.pulseTimer_ += dt * 3.0f;
        f32 pulse = sinf(c.pulseTimer_) * 0.1f + 1.0f;
        c.transform_.scale_ = {30.0f * pulse, 30.0f * pulse};

        c.transform_.rotationRad_ += dt * c.rotationSpeed_;

        AEMtx33 scale, rot, trans;
        AEMtx33Scale(&scale, c.transform_.scale_.x, c.transform_.scale_.y);
        AEMtx33Rot(&rot, c.transform_.rotationRad_);
        AEMtx33Trans(&trans, c.transform_.pos_.x, c.transform_.pos_.y);
        AEMtx33Concat(&c.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&c.transform_.worldMtx_, &trans, &c.transform_.worldMtx_);

        for (const auto& particle : particlePool) {
            if (checkCollisionWithWater(c, particle)) {
                CollisionSystem::incrementCollisionCount();
                c.collected_ = true;
                collectedCount_++;
                switch (c.type_) {
                case CollectibleType::Star:
                    vfxSystem.spawnVFX(VFXType::StarCollect, c.transform_.pos_);
                    break;
                case CollectibleType::Gem:
                    vfxSystem.spawnVFX(VFXType::GemCollect, c.transform_.pos_);
                    break;
                case CollectibleType::Leaf:
                    vfxSystem.spawnVFX(VFXType::LeafCollect, c.transform_.pos_);
                    break;
                }
                g_audioSystem.playSound("bell", "sfx", 0.4f, 0.7f);
                break;
            }
        }
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Items: %d/%d", collectedCount_, totalCollectibles_);
    collectionText_.content_ = buffer;
}

// =========================================================
//
// CollectibleSystem::drawCollectible()
//
// - Sets color render mode with alpha blending.
// - Applies the collectible's world transform matrix.
// - Draws the appropriate mesh (star, gem, or leaf) with the
//   correct colour multiplier for its type.
//
// =========================================================
void CollectibleSystem::drawCollectible(const Collectible& c) {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEMtx33 transformCopy = c.transform_.worldMtx_;
    AEGfxSetTransform(transformCopy.m);

    switch (c.type_) {
    case CollectibleType::Star:
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
        AEGfxMeshDraw(starMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Gem:
        AEGfxSetColorToMultiply(1.0f, 0.0f, 1.0f, 1.0f);
        AEGfxMeshDraw(gemMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Leaf:
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
        AEGfxMeshDraw(leafMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    }
}

// =========================================================
//
// CollectibleSystem::draw()
//
// - Iterates over all collectibles.
// - Skips any that are inactive or already collected.
// - Calls drawCollectible() for each remaining one.
//
// =========================================================
void CollectibleSystem::draw() {
    for (const auto& c : collectibles_) {
        if (!c.active_ || c.collected_)
            continue;
        drawCollectible(c);
    }
}

// =========================================================
//
// CollectibleSystem::drawPreview()
//
// - Gets the current mouse world position.
// - Builds a scale-rotate-translate matrix at the mouse position.
// - Draws a semi-transparent preview of the next collectible type
//   that would be placed (cycles Star -> Gem -> Leaf).
//
// =========================================================
void CollectibleSystem::drawPreview() {
    AEVec2 mousePos = getMouseWorldPos();

    AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;
    AEMtx33Scale(&scaleMtx, 30.0f, 30.0f);
    AEMtx33Rot(&rotMtx, 0.0f);
    AEMtx33Trans(&transMtx, mousePos.x, mousePos.y);
    AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetTransform(worldMtx.m);

    CollectibleType type = static_cast<CollectibleType>(totalCollectibles_ % 3);
    switch (type) {
    case CollectibleType::Star:
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
        AEGfxMeshDraw(starMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Gem:
        AEGfxSetColorToMultiply(1.0f, 0.0f, 1.0f, 1.0f);
        AEGfxMeshDraw(gemMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Leaf:
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
        AEGfxMeshDraw(leafMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    }
}

// =========================================================
//
// CollectibleSystem::drawUI()
//
// - Draws the collection counter text to the screen.
//
// =========================================================
void CollectibleSystem::drawUI() { collectionText_.draw(); }

// =========================================================
//
// CollectibleSystem::free()
//
// - Frees the star, gem, and leaf GPU meshes if they are valid.
// - Nulls all mesh pointers to prevent dangling references.
// - Clears the collectibles vector.
//
// =========================================================
void CollectibleSystem::free() {
    if (starMesh_) {
        AEGfxMeshFree(starMesh_);
        starMesh_ = nullptr;
    }
    if (gemMesh_) {
        AEGfxMeshFree(gemMesh_);
        gemMesh_ = nullptr;
    }
    if (leafMesh_) {
        AEGfxMeshFree(leafMesh_);
        leafMesh_ = nullptr;
    }
    collectibles_.clear();
}

// =========================================================
//
// CollectibleSystem::spawnAtMousePos()
//
// - Gets the current mouse world position.
// - Spawns a collectible of the next type in the cycle at that position.
// - Updates the total collectible count.
//
// =========================================================
void CollectibleSystem::spawnAtMousePos() {
    AEVec2 mousePos = getMouseWorldPos();
    collectibles_.emplace_back(mousePos, static_cast<CollectibleType>(totalCollectibles_ % 3));
    totalCollectibles_ = static_cast<int>(collectibles_.size());
}

// =========================================================
//
// CollectibleSystem::destroyAtMousePos()
//
// - Gets the current mouse world position.
// - Iterates over all active uncollected collectibles.
// - Removes the first one whose bounding box contains the mouse position.
// - Updates the total collectible count after removal.
//
// =========================================================
void CollectibleSystem::destroyAtMousePos() {
    AEVec2 mousePos = getMouseWorldPos();
    f32 mouseX = static_cast<f32>(mousePos.x);
    f32 mouseY = static_cast<f32>(mousePos.y);

    for (auto it = collectibles_.begin(); it != collectibles_.end(); ++it) {
        if (!it->active_ || it->collected_)
            continue;

        f32 radius = it->collider_.shapeData_.circle_.radius_;
        if (mouseX >= (it->transform_.pos_.x - radius) &&
            mouseX <= (it->transform_.pos_.x + radius) &&
            mouseY >= (it->transform_.pos_.y - radius) &&
            mouseY <= (it->transform_.pos_.y + radius)) {
            collectibles_.erase(it);
            totalCollectibles_ = static_cast<int>(collectibles_.size());
            return;
        }
    }
}

// =========================================================
//
// CollectibleSystem::resetCollection()
//
// - Resets the collected count to zero.
// - Marks all collectibles in the vector as not collected.
//
// =========================================================
void CollectibleSystem::resetCollection() {
    collectedCount_ = 0;
    for (auto& c : collectibles_) {
        c.collected_ = false;
    }
}