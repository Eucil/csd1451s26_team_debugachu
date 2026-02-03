#include "AEEngine.h"

#include "StartEndPoint.h"
#include "Utils.h"

#include <cmath>
#include <iostream>

StartEnd::StartEnd() {
    // Set up transform
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {1.f, 1.f};
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
}

StartEnd::StartEnd(AEVec2 pos, AEVec2 scale, StartEndType type, GoalDirection direction) {
    // Set up transform
    transform_.pos_ = pos;
    transform_.scale_ = scale;
    transform_.rotationRad_ = 0.0f;

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

void StartEndPoint::SetupStartPoint(AEVec2 pos, AEVec2 scale, StartEndType type,
                                    GoalDirection direction) {
    startPoints_.emplace_back(pos, scale, type, direction);
}

void StartEndPoint::SetupEndPoint(AEVec2 pos, AEVec2 scale, StartEndType type,
                                  GoalDirection direction) {
    endPoint_ = StartEnd(pos, scale, type, direction);
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
    s32 mouse_x = 0, mouse_y = 0;
    AEInputGetCursorPosition(&mouse_x, &mouse_y);
    mouse_x -= AEGfxGetWindowWidth() / 2;
    mouse_y = (AEGfxGetWindowHeight() / 2) - mouse_y;

    // Use mouse pos to check collision with start point
    // Check by checking if mouse pos falls within the start point's collider box
    for (auto& startPoint : startPoints_) {
        f32 rect_half_width = startPoint.collider_.shapeData_.box_.size_.x / 2.0f;
        f32 rect_half_height = startPoint.collider_.shapeData_.box_.size_.y / 2.0f;
        if (mouse_x >= (startPoint.transform_.pos_.x - rect_half_width) &&
            mouse_x <= (startPoint.transform_.pos_.x + rect_half_width) &&
            mouse_y >= (startPoint.transform_.pos_.y - rect_half_height) &&
            mouse_y <= (startPoint.transform_.pos_.y + rect_half_height) &&
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