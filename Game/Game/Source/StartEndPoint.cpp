#include "AEEngine.h"

#include "StartEndPoint.h"
#include "Utils.h"

#include <cmath>
#include <iostream>

StartEnd::StartEnd() {
    // Set up transform
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {0.f, 0.f};
    transform_.rotationRad_ = {0.0f};

    // Set up world matrix
    AEMtx33 scale_mtx, rot_mtx, trans_mtx;

    AEMtx33Scale(&scale_mtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rot_mtx, transform_.rotationRad_);
    AEMtx33Trans(&trans_mtx, transform_.pos_.x, transform_.pos_.y);

    AEMtx33Concat(&transform_.worldMtx_, &rot_mtx, &scale_mtx);
    AEMtx33Concat(&transform_.worldMtx_, &trans_mtx, &transform_.worldMtx_);

    // Set collider
    collider_.colliderShape_ = ColliderShape::Box;
    collider_.shapeData_.box_.size_ = {1.f, 1.f};

    // Set object type
    type_ = {StartEndType::Pipe};
    direction_ = {GoalDirection::Up};
    release_water_ = false;
    release_water_iframe_ = {false};
    active_ = {false};
}

StartEnd::StartEnd(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type,
                   GoalDirection direction) {
    // Set up transform
    transform_.pos_ = pos;
    transform_.scale_ = scale;
    transform_.rotationRad_ = rotation;

    // Set up world matrix
    AEMtx33 scale_mtx, rot_mtx, trans_mtx;

    AEMtx33Scale(&scale_mtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rot_mtx, transform_.rotationRad_);
    AEMtx33Trans(&trans_mtx, transform_.pos_.x, transform_.pos_.y);

    AEMtx33Concat(&transform_.worldMtx_, &rot_mtx, &scale_mtx);
    AEMtx33Concat(&transform_.worldMtx_, &trans_mtx, &transform_.worldMtx_);

    // Set collider
    collider_.colliderShape_ = ColliderShape::Box;
    collider_.shapeData_.box_.size_ = scale;

    // Set object type
    type_ = type;
    direction_ = direction;

    release_water_ = {false};
    release_water_iframe_ = {false};
}

void StartEndPoint::Initialize() {
    // Make mesh
    rectMesh = CreateRectMesh();

    // Assign rect mesh to all StartEnd types
    for (int i{0}; i < static_cast<int>(StartEndType::Count); ++i) {
        if (rectMesh != nullptr) {
            graphicsConfigs_[i].mesh_ = rectMesh;
        }
    }
    particlesCollected_ = {0};
}

void StartEndPoint::SetupPoint(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type,
                               GoalDirection direction) {
    if (type == StartEndType::Pipe) {
        if (free_start_point_indices_.empty()) {
            // No free indices, add new start point
            startPoints_.emplace_back(pos, scale, rotation, type, direction);
        } else {
            // Reuse a free index
            int index = free_start_point_indices_.back();
            free_start_point_indices_.pop_back();
            startPoints_[index] = StartEnd(pos, scale, rotation, type, direction);
        }
    } else if (type == StartEndType::Flower) {
        endPoint_ = StartEnd(pos, scale, rotation, type, direction);
    }
}

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
    f32 mouse_x = static_cast<f32>(mousePos.x);
    f32 mouse_y = static_cast<f32>(mousePos.y);
    // Check if mouse is over any start point
    for (size_t i = 0; i < startPoints_.size(); ++i) {
        auto& startPoint = startPoints_[i];
        f32 rect_half_width = startPoint.collider_.shapeData_.box_.size_.x / 2.0f;
        f32 rect_half_height = startPoint.collider_.shapeData_.box_.size_.y / 2.0f;
        if (mouse_x >= (startPoint.transform_.pos_.x - rect_half_width) &&
            mouse_x <= (startPoint.transform_.pos_.x + rect_half_width) &&
            mouse_y >= (startPoint.transform_.pos_.y - rect_half_height) &&
            mouse_y <= (startPoint.transform_.pos_.y + rect_half_height)) {
            // Mark this index as free and remove the start point
            free_start_point_indices_.push_back(static_cast<int>(i));
            startPoints_[i] = StartEnd();
            return;
        }
    }
    // Check if mouse is over end point
    f32 rect_half_width = endPoint_.collider_.shapeData_.box_.size_.x / 2.0f;
    f32 rect_half_height = endPoint_.collider_.shapeData_.box_.size_.y / 2.0f;
    if (mouse_x >= (endPoint_.transform_.pos_.x - rect_half_width) &&
        mouse_x <= (endPoint_.transform_.pos_.x + rect_half_width) &&
        mouse_y >= (endPoint_.transform_.pos_.y - rect_half_height) &&
        mouse_y <= (endPoint_.transform_.pos_.y + rect_half_height)) {
        endPoint_ = StartEnd();
    }
}

bool StartEndPoint::CollisionCheckWithWater(StartEnd startend, FluidParticle particle) {
    // Circle to Rectangle Collision Detection
    // Find the closest point to the circle within the rectangle
    f32 rect_half_width = startend.collider_.shapeData_.box_.size_.x / 2.0f;
    f32 rect_half_height = startend.collider_.shapeData_.box_.size_.y / 2.0f;
    f32 closest_x =
        fmaxf(startend.transform_.pos_.x - rect_half_width,
              fminf(particle.transform_.pos_.x, startend.transform_.pos_.x + rect_half_width));
    f32 closest_y =
        fmaxf(startend.transform_.pos_.y - rect_half_height,
              fminf(particle.transform_.pos_.y, startend.transform_.pos_.y + rect_half_height));
    // Calculate the distance between the circle's center and this closest point
    f32 distance_x = particle.transform_.pos_.x - closest_x;
    f32 distance_y = particle.transform_.pos_.y - closest_y;
    // If the distance is less than the circle's radius, an intersection occurs
    f32 distance_squared = (distance_x * distance_x) + (distance_y * distance_y);
    f32 radius = particle.collider_.shapeData_.circle_.radius;

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
    AEMtx33 scale_mtx, rot_mtx, trans_mtx, world_mtx;

    AEMtx33Scale(&scale_mtx, startendScale_.x, startendScale_.y);
    AEMtx33Rot(&rot_mtx, 0.0f);
    AEMtx33Trans(&trans_mtx, mousePos.x, mousePos.y);

    AEMtx33Concat(&world_mtx, &rot_mtx, &scale_mtx);
    AEMtx33Concat(&world_mtx, &trans_mtx, &world_mtx);

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
    AEGfxSetTransform(world_mtx.m);
    AEGfxMeshDraw(graphicsConfigs_[static_cast<int>(type)].mesh_, AE_GFX_MDM_TRIANGLES);
}

void StartEndPoint::Free() {

    AEGfxMeshFree(rectMesh);
    rectMesh = nullptr;

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
    f32 mouse_x = static_cast<f32>(mousePos.x);
    f32 mouse_y = static_cast<f32>(mousePos.y);

    // Use mouse pos to check collision with start point
    // Check by checking if mouse pos falls within the start point's collider box
    for (auto& startPoint : startPoints_) {
        f32 rect_half_width = startPoint.collider_.shapeData_.box_.size_.x / 2.0f;
        f32 rect_half_height = startPoint.collider_.shapeData_.box_.size_.y / 2.0f;
        if (mousePos.x >= (startPoint.transform_.pos_.x - rect_half_width) &&
            mousePos.x <= (startPoint.transform_.pos_.x + rect_half_width) &&
            mousePos.y >= (startPoint.transform_.pos_.y - rect_half_height) &&
            mousePos.y <= (startPoint.transform_.pos_.y + rect_half_height) &&
            startPoint.release_water_iframe_ == false) {
            // std::cout << "Mouse is over start point!\n";
            startPoint.release_water_ = !startPoint.release_water_;
            startPoint.release_water_iframe_ = true;
            break;
        }
    }
}

void StartEndPoint::ResetIframe() {
    for (auto& startPoint : startPoints_) {
        if (startPoint.release_water_iframe_) {
            startPoint.release_water_iframe_ = false;
        }
    }
}

bool StartEndPoint::CheckWinCondition(s32 particle_max_count) {
    if (particlesCollected_ >= (particle_max_count / 3.f)) {
        return true;
    }
    return false;
}