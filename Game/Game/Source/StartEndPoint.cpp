/*!
@file       StartEndPoint.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu

@date		March, 31, 2026

@brief      This source file implements StartEnd constructors and StartEndPoint class,
            managing pipe start points, the flower end point, water flow, particle collection,
            and rendering.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
                        Reproduction or disclosure of this file or its contents
                        without the prior written consent of DigiPen Institute of
                        Technology is prohibited.
*//*______________________________________________________________________*/
#include "StartEndPoint.h"

// =============================
// Standard library
// =============================
#include <cmath>
#include <cstdio>
#include <iostream>

// =============================
// Third-party
// =============================
#include <AEEngine.h>

// =============================
// Project
// =============================
#include "AudioSystem.h"
#include "CollisionSystem.h"
#include "MouseUtils.h"

// =========================================================
//
// StartEnd::StartEnd(type)
//
// - Default constructor. Initializes at the origin with zero scale,
// - sets a unit box collider, marks the point as inactive,
// - and gives it full water capacity.
//
// =========================================================
StartEnd::StartEnd(StartEndType type) {
    // Set up transform
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {0.f, 0.f};
    transform_.rotationRad_ = {0.0f};

    // Set up world matrix
    AEMtx33 scaleMtx, rotMtx, transMtx;

    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);

    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);

    // Set collider
    collider_.colliderShape_ = ColliderShape::Box;
    collider_.shapeData_.box_.size_ = {1.f, 1.f};

    // Set object type
    type_ = type;
    direction_ = {GoalDirection::Up};
    releaseWater_ = false;
    releaseWaterIframe_ = {false};
    active_ = {false};

    waterCapacity_ = 100.0f;
    waterRemaining_ = 100.0f;
    infiniteWater_ = false;
}

// =========================================================
//
// StartEnd::StartEnd(pos, scale, rotation, type, direction)
//
// - Full constructor. Initializes at the given world position as active,
// - sets the collider to match the scale exactly, and gives full water capacity.
//
// =========================================================
StartEnd::StartEnd(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type,
                   GoalDirection direction) {
    // Set up transform
    transform_.pos_ = pos;
    transform_.scale_ = scale;
    transform_.rotationRad_ = rotation;

    // Set up world matrix
    AEMtx33 scaleMtx, rotMtx, transMtx;

    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);

    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);

    // Set collider
    collider_.colliderShape_ = ColliderShape::Box;
    collider_.shapeData_.box_.size_ = scale;

    // Set object type
    type_ = type;
    direction_ = direction;

    releaseWater_ = {false};
    releaseWaterIframe_ = {false};

    active_ = true;
    waterCapacity_ = 100.0f;
    waterRemaining_ = 100.0f;
    infiniteWater_ = false;
}

// =========================================================
//
// StartEndPoint::initialize()
//
// - Creates the shared rect mesh and assigns it to all StartEnd type configs.
// - Loads the pipe and flower textures.
// - Builds the flower sprite-sheet mesh covering exactly one-quarter UV width
// - so AEGfxTextureSet offsets can select individual frames.
// - Resets the particle count to zero.
//
// =========================================================
void StartEndPoint::initialize() {
    // Make mesh
    rectMesh_ = createRectMesh();

    // Assign rect mesh to all StartEnd types
    for (int i{0}; i < static_cast<int>(StartEndType::Count); ++i) {
        if (rectMesh_ != nullptr) {
            graphicsConfigs_[i].mesh_ = rectMesh_;
        }
    }

    // Load textures
    graphicsConfigs_[static_cast<int>(StartEndType::Pipe)].texture_ =
        AEGfxTextureLoad("Assets/Textures/overgrown_pipe_end.png");

    graphicsConfigs_[static_cast<int>(StartEndType::Flower)].texture_ =
        AEGfxTextureLoad("Assets/Textures/pink_flower_sprite_sheet.png");

    // Build a mesh that covers one frame (U: 0 to 1/4) for a 4-frame sprite sheet
    // use AEGfxTextureSet offset to pick the frame
    constexpr f32 kFrameWidth = 1.0f / 4.0f;
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, kFrameWidth, 1.0f,
                -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, kFrameWidth, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, kFrameWidth,
                0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    flowerMesh_ = AEGfxMeshEnd();

    particlesCollected_ = {0};
}

// =========================================================
//
// StartEndPoint::initializeUI()
//
// - Stores the font handle and creates the rect mesh used for water bar rendering.
// - Must be called before drawWaterIndicator() will produce output.
//
// =========================================================
void StartEndPoint::initializeUI(s8 font) {
    font_ = font;
    barMesh_ = createRectMesh();
}

// =========================================================
//
// StartEndPoint::free()
//
// - Frees the rect, flower, and bar meshes.
// - Unloads textures for all StartEnd type configs.
// - Clears startPoints_ and resets endPoint_ to a default Flower.
//
// =========================================================
void StartEndPoint::free() {

    if (rectMesh_) {
        AEGfxMeshFree(rectMesh_);
        rectMesh_ = nullptr;
    }

    if (flowerMesh_) {
        AEGfxMeshFree(flowerMesh_);
        flowerMesh_ = nullptr;
    }

    if (barMesh_) {
        AEGfxMeshFree(barMesh_);
        barMesh_ = nullptr;
    }

    for (int i{}; i < (int)StartEndType::Count; ++i) {

        if (graphicsConfigs_[i].mesh_ != nullptr) {
            graphicsConfigs_[i].mesh_ = nullptr;
        }

        if (graphicsConfigs_[i].texture_ != nullptr) {
            AEGfxTextureUnload(graphicsConfigs_[i].texture_);
            graphicsConfigs_[i].texture_ = nullptr;
        }
    }

    startPoints_.clear();
    endPoint_ = StartEnd(StartEndType::Flower);
}

// =========================================================
//
// StartEndPoint::setupPoint()
//
// - Appends a new pipe start point to startPoints_ if type is Pipe,
// - or replaces the single end point if type is Flower.
//
// =========================================================
void StartEndPoint::setupPoint(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type,
                               GoalDirection direction) {
    if (type == StartEndType::Pipe) {
        startPoints_.emplace_back(pos, scale, rotation, type, direction);
    } else if (type == StartEndType::Flower) {
        endPoint_ = StartEnd(pos, scale, rotation, type, direction);
    }
}

// =========================================================
//
// StartEndPoint::spawnAtMousePos()
//
// - Calls setupPoint() at the current mouse world position with a default 50x50 scale.
//
// =========================================================
void StartEndPoint::spawnAtMousePos(StartEndType type, GoalDirection direction) {
    // Get mouse position
    AEVec2 mousePos = getMouseWorldPos();
    AEVec2 pos = {static_cast<f32>(mousePos.x), static_cast<f32>(mousePos.y)};
    AEVec2 scale = {50.0f, 50.0f};
    f32 rotation = 0.0f;

    setupPoint(pos, scale, rotation, type, direction);
}

// =========================================================
//
// StartEndPoint::deleteAtMousePos()
//
// - Removes the first start point whose AABB contains the mouse position.
// - If no start point is hit, resets the end point if the mouse is over it.
//
// =========================================================
void StartEndPoint::deleteAtMousePos() {
    // Get mouse position
    AEVec2 mousePos = getMouseWorldPos();
    f32 mouseX = static_cast<f32>(mousePos.x);
    f32 mouseY = static_cast<f32>(mousePos.y);
    // Check if mouse is over any start point
    for (size_t i = 0; i < startPoints_.size(); ++i) {
        auto& startPoint = startPoints_[i];
        f32 rectHalfWidth = startPoint.collider_.shapeData_.box_.size_.x / 2.0f;
        f32 rectHalfHeight = startPoint.collider_.shapeData_.box_.size_.y / 2.0f;
        if (mouseX >= (startPoint.transform_.pos_.x - rectHalfWidth) &&
            mouseX <= (startPoint.transform_.pos_.x + rectHalfWidth) &&
            mouseY >= (startPoint.transform_.pos_.y - rectHalfHeight) &&
            mouseY <= (startPoint.transform_.pos_.y + rectHalfHeight)) {
            // Delete this start point
            startPoints_.erase(startPoints_.begin() + i);
            return;
        }
    }
    // Check if mouse is over end point
    f32 rectHalfWidth = endPoint_.collider_.shapeData_.box_.size_.x / 2.0f;
    f32 rectHalfHeight = endPoint_.collider_.shapeData_.box_.size_.y / 2.0f;
    if (mouseX >= (endPoint_.transform_.pos_.x - rectHalfWidth) &&
        mouseX <= (endPoint_.transform_.pos_.x + rectHalfWidth) &&
        mouseY >= (endPoint_.transform_.pos_.y - rectHalfHeight) &&
        mouseY <= (endPoint_.transform_.pos_.y + rectHalfHeight)) {
        endPoint_ = StartEnd(StartEndType::Flower);
    }
}

// =========================================================
//
// StartEndPoint::collisionCheckWithWater(StartEnd startend, FluidParticle particle)
//
// - Simple axis-aligned AABB-circle collision test (no rotation).
// - Finds the closest point on the box to the circle center and
// - checks whether the squared distance is less than the radius squared.
//
// =========================================================
bool StartEndPoint::collisionCheckWithWater(StartEnd startend, FluidParticle particle) {
    // Circle to Rectangle Collision Detection
    // Find the closest point to the circle within the rectangle
    f32 rectHalfWidth = startend.collider_.shapeData_.box_.size_.x / 2.0f;
    f32 rectHalfHeight = startend.collider_.shapeData_.box_.size_.y / 2.0f;
    f32 closest_x =
        fmaxf(startend.transform_.pos_.x - rectHalfWidth,
              fminf(particle.transform_.pos_.x, startend.transform_.pos_.x + rectHalfWidth));
    f32 closest_y =
        fmaxf(startend.transform_.pos_.y - rectHalfHeight,
              fminf(particle.transform_.pos_.y, startend.transform_.pos_.y + rectHalfHeight));
    // Calculate the distance between the circle's center and this closest point
    f32 distance_x = particle.transform_.pos_.x - closest_x;
    f32 distance_y = particle.transform_.pos_.y - closest_y;
    // If the distance is less than the circle's radius, an intersection occurs
    f32 distance_squared = (distance_x * distance_x) + (distance_y * distance_y);
    f32 radius = particle.collider_.shapeData_.circle_.radius_;

    return distance_squared < (radius * radius);
}

// =========================================================
//
// StartEndPoint::update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfxSystem)
//
// - For each active start point: checks particle collisions (no action yet)
// - and fires pipe-flow VFX at ~8 bursts per second while water is flowing.
// - For the end point: absorbs any colliding particle — increments
// - particlesCollected_, erases the particle from the pool, and plays a sound.
//
// =========================================================
void StartEndPoint::update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfxSystem) {
    (void)dt; // unused for now

    // Check collision for each start/end point with each water particle
    for (auto& startPoint : startPoints_) {
        if (startPoint.active_ == false) {
            continue;
        }
        for (auto& particle : particlePool) {
            if (collisionCheckWithWater(startPoint, particle)) {
                CollisionSystem::incrementCollisionCount();
                // Handle collision with start point
                // For example, you can reset the particle's position or apply some effect
                // std::cout << "Particle collided with start point!\n";
            }
        }

        // VFX: emit a water-mist burst while the pipe is flowing
        if (startPoint.releaseWater_ &&
            (startPoint.waterRemaining_ > 0.0f || startPoint.infiniteWater_)) {
            startPoint.vfxTimer_ -= dt;
            if (startPoint.vfxTimer_ <= 0.0f) {
                // Spawn at the pipe mouth (bottom-centre of the pipe rect)
                AEVec2 spawnPos = {startPoint.transform_.pos_.x,
                                   startPoint.transform_.pos_.y -
                                       startPoint.transform_.scale_.y * 0.5f};
                vfxSystem.spawnVFX(VFXType::PipeFlow, spawnPos);
                startPoint.vfxTimer_ = 0.12f; // ~8-9 bursts per second
            }
        } else {
            startPoint.vfxTimer_ = 0.0f; // reset when pipe is off so it fires immediately on toggle
        }
    }

    // Check collision for end point with each water particle
    for (auto particleIt = particlePool.begin(); particleIt != particlePool.end();) {
        if (collisionCheckWithWater(endPoint_, *particleIt)) {
            CollisionSystem::incrementCollisionCount();
            // Handle collision with end point
            // std::cout << "Particle collided with end point! Removing particle.\n";
            particlesCollected_++;

            vfxSystem.spawnVFX(VFXType::FlowerCollect, endPoint_.transform_.pos_);

            // Remove this particle from the pool
            particleIt = particlePool.erase(particleIt); // Erase returns the next valid iterator

            // Play pop sound
            g_audioSystem.playSound("drip_water", "sfx", 0.4f, 1.0f);
        } else {
            ++particleIt; // Increment iterator only if no deletion occurred
        }
    }
}

// =========================================================
//
// StartEndPoint::drawColor()
//
// - Renders all start points in grey and the end point in red using color mode.
// - Used as a fallback when textures are not loaded.
//
// =========================================================
void StartEndPoint::drawColor() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    // Render start points
    for (auto& startPoint : startPoints_) {
        AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 1.0f); // Grey color for start points
        AEGfxSetTransform(startPoint.transform_.worldMtx_.m);
        AEGfxMeshDraw(graphicsConfigs_[(int)startPoint.type_].mesh_, AE_GFX_MDM_TRIANGLES);
    }

    // Render end point
    AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); // Red color for end point
    AEGfxSetTransform(endPoint_.transform_.worldMtx_.m);
    AEGfxMeshDraw(graphicsConfigs_[(int)endPoint_.type_].mesh_, AE_GFX_MDM_TRIANGLES);
}

// =========================================================
//
// StartEndPoint::drawWaterIndicator()
//
// - Draws a grey background bar and a blue fill bar above the pipe,
// - sized proportionally to waterRemaining_ / waterCapacity_.
// - Also prints the remaining/total values as text.
//
// =========================================================
void StartEndPoint::drawWaterIndicator(const StartEnd& startPoint, const AEVec2& screenPos) {
    (void)screenPos; // Unused parameter for now, can be used for screen coordinate conversion

    if (!barMesh_ || font_ == 0)
        return;

    // Calculate water percentage
    float percentage = startPoint.waterRemaining_ / startPoint.waterCapacity_;
    if (percentage < 0.0f)
        percentage = 0.0f;

    // Bar dimensions (in world units)
    float barWidth = 80.0f;
    float barHeight = 10.0f;
    float barX = startPoint.transform_.pos_.x;
    float barY = startPoint.transform_.pos_.y + 40.0f; // Position above the start point

    // Draw background bar (gray)
    AEMtx33 scaleMtx, transMtx, worldMtx;

    AEMtx33Scale(&scaleMtx, barWidth, barHeight);
    AEMtx33Trans(&transMtx, barX, barY);

    // Method 1: Use AEMtx33Mult (if available)
    // If your engine has AEMtx33Mult, use this:
    // AEMtx33Mult(&worldMtx, &transMtx, &scaleMtx);

    // Method 2: Use two-step concatenation (more compatible)
    AEMtx33Identity(&worldMtx);                     // Start with identity
    AEMtx33Concat(&worldMtx, &scaleMtx, &worldMtx); // Apply scale
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx); // Apply translation

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.3f);
    AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxMeshDraw(barMesh_, AE_GFX_MDM_TRIANGLES);

    // Draw fill bar (blue) -
    float fillWidth = barWidth * percentage;
    float fillX = barX - (barWidth - fillWidth) / 2.0f;

    AEMtx33Trans(&transMtx, fillX, barY);
    AEMtx33Scale(&worldMtx, fillWidth, barHeight);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    AEGfxSetTransparency(0.8f);
    AEGfxSetColorToMultiply(0.0f, 0.5f, 1.0f, 1.0f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxMeshDraw(barMesh_, AE_GFX_MDM_TRIANGLES);

    // Draw text showing remaining/total
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.0f/%.0f", startPoint.waterRemaining_,
             startPoint.waterCapacity_);

    // Simple screen coordinate conversion
    float textX = barX / 800.0f; // Assuming half screen width is 800
    float textY = barY / 450.0f; // Assuming half screen height is 450

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetTransparency(1.0f);
    AEGfxPrint(font_, buffer, textX, textY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

// =========================================================
//
// StartEndPoint::drawTexture(s32 particleMaxCount)
//
// - Renders all pipe start points with their texture.
// - Selects the flower sprite-sheet frame (0-3) based on goal progress:
// - 0% = frame 0, 33% = frame 1, 66% = frame 2, 100% = frame 3.
// - Goal threshold is 25% of particleMaxCount.
//
// =========================================================
void StartEndPoint::drawTexture(s32 particleMaxCount) {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    // Draw start points (pipe)
    Graphics& pipeGfx = graphicsConfigs_[static_cast<int>(StartEndType::Pipe)];
    AEGfxTextureSet(pipeGfx.texture_, 0.0f, 0.0f);
    for (auto& startPoint : startPoints_) {
        AEGfxSetTransform(startPoint.transform_.worldMtx_.m);
        AEGfxMeshDraw(pipeGfx.mesh_, AE_GFX_MDM_TRIANGLES);
    }

    // Select flower frame based on percentage of the win goal (25% of total water)
    constexpr f32 kFrameWidth = 0.25f;
    int frame = 0;
    if (particleMaxCount > 0) {
        f32 goalCount = static_cast<f32>(particleMaxCount) * 0.25f;
        f32 pct = static_cast<f32>(particlesCollected_) / goalCount;
        if (pct >= 1.0f)
            frame = 3;
        else if (pct >= 0.66f)
            frame = 2;
        else if (pct >= 0.33f)
            frame = 1;
    }

    // Draw end point (flower), UV offset selects the frame
    Graphics& flowerGfx = graphicsConfigs_[static_cast<int>(StartEndType::Flower)];
    AEGfxTextureSet(flowerGfx.texture_, kFrameWidth * frame, 0.0f);
    AEGfxSetTransform(endPoint_.transform_.worldMtx_.m);
    AEGfxMeshDraw(flowerMesh_, AE_GFX_MDM_TRIANGLES);
}

// =========================================================
//
// StartEndPoint::drawPreview(StartEndType type)
//
// - Draws a semi-transparent ghost pipe or flower at the mouse position.
// - Uses the same scale as spawned points for accurate placement feedback.
//
// =========================================================
void StartEndPoint::drawPreview(StartEndType type) {
    // Set transform matrix based on mouse position
    AEVec2 mousePos = getMouseWorldPos();

    // Set up world matrix
    AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;

    AEMtx33Scale(&scaleMtx, startendScale_.x, startendScale_.y);
    AEMtx33Rot(&rotMtx, 0.0f);
    AEMtx33Trans(&transMtx, mousePos.x, mousePos.y);

    AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetTransform(worldMtx.m);
    switch (type) {
    case StartEndType::Pipe:
        AEGfxTextureSet(graphicsConfigs_[static_cast<int>(type)].texture_, 0.0f, 0.0f);
        AEGfxMeshDraw(graphicsConfigs_[static_cast<int>(type)].mesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case StartEndType::Flower:
        AEGfxTextureSet(graphicsConfigs_[static_cast<int>(type)].texture_, 0.0f, 0.0f);
        AEGfxMeshDraw(flowerMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    }
}

// =========================================================
//
// StartEndPoint::checkMouseClick()
//
// - Checks whether the mouse is over any pipe start point.
// - On hit, toggles releaseWater_ and sets the one-frame iframe
// - to prevent the same click from toggling the valve twice.
// - Plays a crank sound on toggle.
//
// =========================================================
void StartEndPoint::checkMouseClick() {
    // Get mouse position
    AEVec2 mousePos = getMouseWorldPos();

    // Use mouse pos to check collision with start point
    // Check by checking if mouse pos falls within the start point's collider box
    for (auto& startPoint : startPoints_) {
        f32 rectHalfWidth = startPoint.collider_.shapeData_.box_.size_.x / 2.0f;
        f32 rectHalfHeight = startPoint.collider_.shapeData_.box_.size_.y / 2.0f;
        if (mousePos.x >= (startPoint.transform_.pos_.x - rectHalfWidth) &&
            mousePos.x <= (startPoint.transform_.pos_.x + rectHalfWidth) &&
            mousePos.y >= (startPoint.transform_.pos_.y - rectHalfHeight) &&
            mousePos.y <= (startPoint.transform_.pos_.y + rectHalfHeight) &&
            startPoint.releaseWaterIframe_ == false) {
            // std::cout << "Mouse is over start point!\n";
            startPoint.releaseWater_ = !startPoint.releaseWater_;
            startPoint.releaseWaterIframe_ = true;

            // Play faucet squeak sound
            g_audioSystem.playSound("crank", "sfx", 0.7f, 1.0f);
            break;
        }
    }
}

// =========================================================
//
// StartEndPoint::resetIframe()
//
// - Clears releaseWaterIframe_ on all start points,
// - allowing the next click to register.
//
// =========================================================
void StartEndPoint::resetIframe() {
    for (auto& startPoint : startPoints_) {
        if (startPoint.releaseWaterIframe_) {
            startPoint.releaseWaterIframe_ = false;
        }
    }
}

// =========================================================
//
// StartEndPoint::checkWinCondition()
//
// - Returns true if particlesCollected_ has reached at least 25% of particleMaxCount.
// - This is the legacy win check; the active win trigger uses goalPercentage in Level.cpp.
//
// =========================================================
bool StartEndPoint::checkWinCondition(s32 particleMaxCount) const {
    if (particlesCollected_ >= (particleMaxCount * 0.25f)) {
        return true;
    }
    return false;
}

// =========================================================
//
// StartEndPoint::getWaterRemaining()
//
// - Returns waterRemaining_ for the start point at startPointIndex (0-based).
// - Returns 0.0f if the index is out of range.
//
// =========================================================
float StartEndPoint::getWaterRemaining(int startPointIndex) const {
    if (startPointIndex >= 0 && startPointIndex < static_cast<int>(startPoints_.size())) {
        return startPoints_[startPointIndex].waterRemaining_;
    }
    return 0.0f;
}

// =========================================================
//
// StartEndPoint::setWaterRemaining()
//
// - Sets waterRemaining_ for the start point at startPointIndex,
// - clamped to the range [0, waterCapacity_].
//
// =========================================================
void StartEndPoint::setWaterRemaining(int startPointIndex, float amount) {
    if (startPointIndex >= 0 && startPointIndex < static_cast<int>(startPoints_.size())) {
        startPoints_[startPointIndex].waterRemaining_ = amount;
        if (startPoints_[startPointIndex].waterRemaining_ >
            startPoints_[startPointIndex].waterCapacity_) {
            startPoints_[startPointIndex].waterRemaining_ =
                startPoints_[startPointIndex].waterCapacity_;
        }
        if (startPoints_[startPointIndex].waterRemaining_ < 0.0f) {
            startPoints_[startPointIndex].waterRemaining_ = 0.0f;
        }
    }
}

// =========================================================
//
// StartEndPoint::refillAllWater()
//
// - Restores waterRemaining_ to waterCapacity_ for every active pipe start point.
//
// =========================================================
void StartEndPoint::refillAllWater() {
    for (auto& startPoint : startPoints_) {
        if (startPoint.active_ && startPoint.type_ == StartEndType::Pipe) {
            startPoint.waterRemaining_ = startPoint.waterCapacity_;
        }
    }
}

// =========================================================
//
// StartEndPoint::toggleInfiniteWater()
//
// - Flips the infiniteWater_ flag on all active pipe start points.
// - When true, the pipe never depletes regardless of spawn rate.
//
// ========================================================
void StartEndPoint::toggleInfiniteWater() {
    for (auto& startPoint : startPoints_) {
        if (startPoint.active_ && startPoint.type_ == StartEndType::Pipe) {
            startPoint.infiniteWater_ = !startPoint.infiniteWater_;
        }
    }
}
