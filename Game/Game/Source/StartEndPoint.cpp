#include "StartEndPoint.h"

#include <cmath>
#include <cstdio>
#include <iostream>

#include <AEEngine.h>

#include "AudioSystem.h"
#include "Utils.h"

StartEnd::StartEnd() {
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
    type_ = {StartEndType::Pipe};
    direction_ = {GoalDirection::Up};
    releaseWater_ = false;
    releaseWaterIframe_ = {false};
    active_ = {false};

    //// tc added start
    waterCapacity_ = 100.0f;
    waterRemaining_ = 100.0f;
    infiniteWater_ = false;
    //// tc added end
}

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

    // tc added start
    active_ = true;
    waterCapacity_ = 100.0f;
    waterRemaining_ = 100.0f;
    infiniteWater_ = false;
    // tc added end
}

void StartEndPoint::Initialize() {
    // Make mesh
    rectMesh_ = CreateRectMesh();

    // Assign rect mesh to all StartEnd types
    for (int i{0}; i < static_cast<int>(StartEndType::Count); ++i) {
        if (rectMesh_ != nullptr) {
            graphicsConfigs_[i].mesh_ = rectMesh_;
        }
    }
    particlesCollected_ = {0};
}

// tc added start
void StartEndPoint::InitializeUI(s8 font) {
    font_ = font;
    barMesh_ = CreateRectMesh();
}
// tc added end

void StartEndPoint::SetupPoint(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type,
                               GoalDirection direction) {
    if (type == StartEndType::Pipe) {
        startPoints_.emplace_back(pos, scale, rotation, type, direction);
    } else if (type == StartEndType::Flower) {
        endPoint_ = StartEnd(pos, scale, rotation, type, direction);
    }
}

// tc added start
void StartEndPoint::DrawWaterIndicator(const StartEnd& startPoint, const AEVec2& screenPos) {
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
    AEMtx33Identity(&worldMtx);                       // Start with identity
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

float StartEndPoint::GetWaterRemaining(int startPointIndex) const {
    if (startPointIndex >= 0 && startPointIndex < static_cast<int>(startPoints_.size())) {
        return startPoints_[startPointIndex].waterRemaining_;
    }
    return 0.0f;
}

void StartEndPoint::SetWaterRemaining(int startPointIndex, float amount) {
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

void StartEndPoint::RefillAllWater() {
    for (auto& startPoint : startPoints_) {
        if (startPoint.active_ && startPoint.type_ == StartEndType::Pipe) {
            startPoint.waterRemaining_ = startPoint.waterCapacity_;
        }
    }
}

void StartEndPoint::ToggleInfiniteWater() {
    for (auto& startPoint : startPoints_) {
        if (startPoint.active_ && startPoint.type_ == StartEndType::Pipe) {
            startPoint.infiniteWater_ = !startPoint.infiniteWater_;
        }
    }
}
// tc added end

void StartEndPoint::SpawnAtMousePos(StartEndType type, GoalDirection direction) {
    // Get mouse position
    AEVec2 mousePos = GetMouseWorldPos();
    AEVec2 pos = {static_cast<f32>(mousePos.x), static_cast<f32>(mousePos.y)};
    AEVec2 scale = {50.0f, 50.0f};
    f32 rotation = 0.0f;

    SetupPoint(pos, scale, rotation, type, direction);
}

void StartEndPoint::DeleteAtMousePos() {
    // Get mouse position
    AEVec2 mousePos = GetMouseWorldPos();
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
        endPoint_ = StartEnd();
    }
}

bool StartEndPoint::CollisionCheckWithWater(StartEnd startend, FluidParticle particle) {
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

void StartEndPoint::Update(f32 dt, std::vector<FluidParticle>& particlePool) {

    // Check collision for each start/end point with each water particle
    for (auto& startPoint : startPoints_) {
        if (startPoint.active_ == false) {
            continue;
        }
        for (auto& particle : particlePool) {
            if (CollisionCheckWithWater(startPoint, particle)) {
                // Handle collision with start point
                // For example, you can reset the particle's position or apply some effect
                // std::cout << "Particle collided with start point!\n";
            }
        }
    }

    // Check collision for end point with each water particle
    for (auto particleIt = particlePool.begin(); particleIt != particlePool.end();) {
        if (CollisionCheckWithWater(endPoint_, *particleIt)) {
            // Handle collision with end point
            // std::cout << "Particle collided with end point! Removing particle.\n";
            particlesCollected_++;
            // Remove this particle from the pool
            particleIt = particlePool.erase(particleIt); // Erase returns the next valid iterator

            // Play pop sound
            g_audioSystem.playSound("pop", "sfx", 0.8f, 1.0f);
        } else {
            ++particleIt; // Increment iterator only if no deletion occurred
        }
    }
}

void StartEndPoint::DrawColor() {
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

void StartEndPoint::DrawTexture() {}

void StartEndPoint::DrawColorPreview(StartEndType type) {
    // Set transform matrix based on mouse position
    AEVec2 mousePos = GetMouseWorldPos();

    // Set up world matrix
    AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;

    AEMtx33Scale(&scaleMtx, startendScale_.x, startendScale_.y);
    AEMtx33Rot(&rotMtx, 0.0f);
    AEMtx33Trans(&transMtx, mousePos.x, mousePos.y);

    AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    switch (type) {
    case StartEndType::Pipe:
        AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 0.5f); // Grey color for start point preview
        break;
    case StartEndType::Flower:
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); // Red color for end point preview
        break;
    }
    AEGfxSetTransform(worldMtx.m);
    AEGfxMeshDraw(graphicsConfigs_[static_cast<int>(type)].mesh_, AE_GFX_MDM_TRIANGLES);
}

void StartEndPoint::Free() {

    if (rectMesh_) {
        AEGfxMeshFree(rectMesh_);
        rectMesh_ = nullptr;
    }

    // tc added start
    if (barMesh_) {
        AEGfxMeshFree(barMesh_);
        barMesh_ = nullptr;
    }
    // tc added end

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
    endPoint_ = StartEnd();
}

void StartEndPoint::CheckMouseClick() {
    // Get mouse position
    AEVec2 mousePos = GetMouseWorldPos();
    f32 mouseX = static_cast<f32>(mousePos.x);
    f32 mouseY = static_cast<f32>(mousePos.y);

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
            g_audioSystem.playSound("faucet_squeak", "sfx", 0.25f, 1.0f);
            break;
        }
    }
}

void StartEndPoint::ResetIframe() {
    for (auto& startPoint : startPoints_) {
        if (startPoint.releaseWaterIframe_) {
            startPoint.releaseWaterIframe_ = false;
        }
    }
}

bool StartEndPoint::CheckWinCondition(s32 particleMaxCount) {
    if (particlesCollected_ >= (particleMaxCount / 3.f)) {
        return true;
    }
    return false;
}