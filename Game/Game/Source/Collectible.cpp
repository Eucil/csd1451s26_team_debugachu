/*!
@file       Collectible.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Collectible.h"

// Standard library
#include <cmath>
#include <cstdio>

// Project
#include "AudioSystem.h"
#include "CollisionSystem.h"
#include "ConfigManager.h"
#include "MouseUtils.h"

// constructor to set posit, scale, rotation,collider type, radius
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

// constructor to take posit and param
// Sets position to given coord
Collectible::Collectible(AEVec2 pos, CollectibleType type) {
    transform_.pos_ = pos;
    transform_.scale_ = {30.0f, 30.0f};
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius_ = 15.0f;
    collider_.shapeData_.circle_.offset_ = {0.0f, 0.0f};

    // type_ set to parameter value (Star, Gem, or Leaf)
    //   active_ = true - collectible exists in world
    // collected_ = false - not yet collected
    type_ = type;
    active_ = true;
    collected_ = false;
    pulseTimer_ = 0.0f;
    rotationSpeed_ = 1.0f + (AERandFloat() * 2.0f); // Random rotation speed
}

// Load system w font
// reads UI posit from JSON config file
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

// createMeshes() - builds the 3D shapes for collectibles
// collectedCount_ = 0 - reset collected counter
// totalCollectibles_ = 0 - reset total count
// globalTimer_ = 0 - reset animation timer
// collectibles_.clear() - remove all collectibles from previous level
void CollectibleSystem::initialize() {
    createMeshes();
    collectedCount_ = 0;
    totalCollectibles_ = 0;
    globalTimer_ = 0.0f;

    collectibles_.clear();
}

// AEGfxMeshStart() - starts building a mesh
// kOuterRadius = 0.5f - distance from center to star tips
// kInnerRadius = 0.25f - distance from center to inner points
void CollectibleSystem::createMeshes() {
    // Star mesh (5-pointed star shape)
    AEGfxMeshStart();
    constexpr int kStarPoints = 5;
    constexpr f32 kOuterRadius = 0.5f;
    constexpr f32 kInnerRadius = 0.25f;

    // calculations to create a star shape
    for (int i = 0; i < kStarPoints; i++) {
        f32 angle1 = (i * 2.0f * 3.14159f / kStarPoints) - 3.14159f / 2.0f;
        f32 angle2 = ((i + 1) * 2.0f * 3.14159f / kStarPoints) - 3.14159f / 2.0f;
        f32 midAngle = (angle1 + angle2) / 2.0f;

        // Using cosine and sine to convert angles to coordinates
        // Multiply by kOuterRadius to get tip positions
        f32 x1 = cosf(angle1) * kOuterRadius;
        f32 y1 = sinf(angle1) * kOuterRadius;
        f32 x2 = cosf(angle2) * kOuterRadius;
        f32 y2 = sinf(angle2) * kOuterRadius;

        // Inner point
        f32 xi = cosf(midAngle) * kInnerRadius;
        f32 yi = sinf(midAngle) * kInnerRadius;
        // adds a triangle to the mesh
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFF00, 0.5f, 0.5f, x1, y1, 0xFFFFFF00, x1 + 0.5f, y1 + 0.5f,
                    xi, yi, 0xFFFFFF00, xi + 0.5f, yi + 0.5f);
        // Add second triangle and end mesh
        // Together they form one point of the star
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFF00, 0.5f, 0.5f, xi, yi, 0xFFFFFF00, xi + 0.5f, yi + 0.5f,
                    x2, y2, 0xFFFFFF00, x2 + 0.5f, y2 + 0.5f);
    }
    // AEGfxMeshEnd() completes mesh and returns pointer
    // starMesh_ stores this mesh for later drawing
    starMesh_ = AEGfxMeshEnd();

    // Gem mesh (diamond shape)
    AEGfxMeshStart();
    AEGfxTriAdd(0.0f, 0.5f, 0xFFFF00FF, 0.5f, 1.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f, 0.0f,
                0xFFFF00FF, 1.0f, 0.5f);

    AEGfxTriAdd(0.0f, -0.5f, 0xFFFF00FF, 0.5f, 0.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f,
                0.0f, 0xFFFF00FF, 1.0f, 0.5f);
    gemMesh_ = AEGfxMeshEnd();

    // Leaf mesh (simple teardrop shape)
    AEGfxMeshStart();
    constexpr int kLeafSegments = 12;
    for (int i = 0; i < kLeafSegments; i++) {
        f32 angle1 = (i * 2.0f * 3.14159f / kLeafSegments);
        f32 angle2 = ((i + 1) * 2.0f * 3.14159f / kLeafSegments);

        f32 r1 = 0.3f + 0.2f * sinf(angle1 * 2.0f);
        f32 r2 = 0.3f + 0.2f * sinf(angle2 * 2.0f);

        f32 x1 = cosf(angle1) * r1;
        f32 y1 = sinf(angle1) * r1;
        f32 x2 = cosf(angle2) * r2;
        f32 y2 = sinf(angle2) * r2;

        AEGfxTriAdd(0.0f, 0.0f, 0xFF00FF00, 0.5f, 0.5f, x1, y1, 0xFF00FF00, x1 + 0.5f, y1 + 0.5f,
                    x2, y2, 0xFF00FF00, x2 + 0.5f, y2 + 0.5f);
    }
    leafMesh_ = AEGfxMeshEnd();
}

// Add collectible to level
void CollectibleSystem::loadLevelCollectibles(AEVec2 pos, CollectibleType type) {

    // emplace_back constructs collectible directly in vector
    // Updates total count based on vector size
    collectibles_.emplace_back(pos, type);

    totalCollectibles_ = static_cast<int>(collectibles_.size());
}

// collision check start
bool CollectibleSystem::checkCollisionWithWater(const Collectible& collectible,
                                                const FluidParticle& particle) {
    if (!collectible.active_ || collectible.collected_)
        return false;

    // Circle-circle collision
    // delta.x = particle.x - collectible.x
    // delta.y = particle.y - collectible.y
    AEVec2 delta = {particle.transform_.pos_.x - collectible.transform_.pos_.x,
                    particle.transform_.pos_.y - collectible.transform_.pos_.y};

    f32 distSq = delta.x * delta.x + delta.y * delta.y;
    f32 radiusSum = particle.collider_.shapeData_.circle_.radius_ +
                    collectible.collider_.shapeData_.circle_.radius_;
    // If squared distance < squared sum of radii ? Collision
    return distSq < (radiusSum * radiusSum);
}

void CollectibleSystem::update(f32 dt, std::vector<FluidParticle>& particlePool,
                               VFXSystem& vfxSystem) {
    globalTimer_ += dt;

    for (auto& c : collectibles_) {
        if (!c.active_ || c.collected_)
            continue;

        // Pulse effect
        c.pulseTimer_ += dt * 3.0f;
        f32 pulse = sinf(c.pulseTimer_) * 0.1f + 1.0f;
        c.transform_.scale_ = {30.0f * pulse, 30.0f * pulse};

        // Rotation effect
        // Creates continuous spinning effect
        c.transform_.rotationRad_ += dt * c.rotationSpeed_;

        // Update transform matrix
        AEMtx33 scale, rot, trans;
        AEMtx33Scale(&scale, c.transform_.scale_.x, c.transform_.scale_.y);
        AEMtx33Rot(&rot, c.transform_.rotationRad_);
        AEMtx33Trans(&trans, c.transform_.pos_.x, c.transform_.pos_.y);

        AEMtx33Concat(&c.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&c.transform_.worldMtx_, &trans, &c.transform_.worldMtx_);

        // Check collision with water particles
        for (const auto& particle : particlePool) {
            if (checkCollisionWithWater(c, particle)) {
                CollisionSystem::incrementCollisionCount();
                // if collision, mark collected, increment count, exit particle loop
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

    // Update collection text
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Items: %d/%d", collectedCount_, totalCollectibles_);
    collectionText_.content_ = buffer;
}

void CollectibleSystem::drawCollectible(const Collectible& c) {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEMtx33 transformCopy = c.transform_.worldMtx_;
    AEGfxSetTransform(transformCopy.m);

    switch (c.type_) {
    case CollectibleType::Star:
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        AEGfxMeshDraw(starMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Gem:
        AEGfxSetColorToMultiply(1.0f, 0.0f, 1.0f, 1.0f); // Purple
        AEGfxMeshDraw(gemMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Leaf:
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f); // Green
        AEGfxMeshDraw(leafMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    }
}

// Loop through all collectibles, Skip inactive or collected ones,
// Call DrawCollectible for each active one
void CollectibleSystem::draw() {
    // Draw collectibles
    for (const auto& c : collectibles_) {
        if (!c.active_ || c.collected_)
            continue;
        drawCollectible(c);
    }
}

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

void CollectibleSystem::drawUI() {
    // Draw collection counter
    collectionText_.draw();
}

// Free each mesh if it exists
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

void CollectibleSystem::spawnAtMousePos() {
    AEVec2 mousePos = getMouseWorldPos();
    collectibles_.emplace_back(mousePos, static_cast<CollectibleType>(totalCollectibles_ % 3));
    totalCollectibles_ = static_cast<int>(collectibles_.size());
}

void CollectibleSystem::destroyAtMousePos() {
    // Get mouse position
    AEVec2 mousePos = getMouseWorldPos();
    f32 mouseX = static_cast<f32>(mousePos.x);
    f32 mouseY = static_cast<f32>(mousePos.y);

    // Check if mouse is over any start point
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

void CollectibleSystem::resetCollection() {
    collectedCount_ = 0;
    for (auto& c : collectibles_) {
        c.collected_ = false;
    }
}