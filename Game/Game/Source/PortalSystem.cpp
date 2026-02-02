#include "AEEngine.h"

#include "PortalSystem.h"
#include "Utils.h"

#include <cmath>
#include <iostream>

Portal::Portal() {
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

    red = 0.0f;
    green = 0.0f;
    blue = 0.0f;

    linked_portal_ = nullptr;
}

Portal::Portal(AEVec2 pos, AEVec2 scale, f32 rotationDeg) {
    transform_.pos_ = pos;
    transform_.scale_ = scale;
    transform_.rotationRad_ = AEDegToRad(rotationDeg);

    // Set up world matrix
    AEMtx33 scale_mtx, rot_mtx, trans_mtx;

    AEMtx33Scale(&scale_mtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rot_mtx, transform_.rotationRad_);
    AEMtx33Trans(&trans_mtx, transform_.pos_.x, transform_.pos_.y);

    AEMtx33Concat(&transform_.worldMtx_, &rot_mtx, &scale_mtx);
    AEMtx33Concat(&transform_.worldMtx_, &trans_mtx, &transform_.worldMtx_);

    // Set collider
    collider_.colliderShape_ = ColliderShape::Box;
    collider_.shapeData_.box_.size_.x = scale.x * 0.9f;
    collider_.shapeData_.box_.size_.y = scale.y * 0.9f;

    red = AERandFloat();
    green = AERandFloat();
    blue = AERandFloat();

    linked_portal_ = nullptr;
}

void PortalSystem::Initialize() {

    rectMesh = CreateRectMesh();
    graphicsConfigs_.mesh_ = rectMesh;

    current_portal_ = nullptr;
    click_iframe = false;
}

void PortalSystem::SetupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg) {
    Portal* portal_to_add = new Portal(pos, scale, rotationDeg);
    portal_vec.push_back(portal_to_add);

    if (current_portal_ == nullptr) {
        current_portal_ = portal_to_add;
    } else {
        // Link the current portal to the new portal
        current_portal_->linked_portal_ = portal_to_add;
        portal_to_add->linked_portal_ = current_portal_;
        // Set both to have same colors
        portal_to_add->red = current_portal_->red;
        portal_to_add->green = current_portal_->green;
        portal_to_add->blue = current_portal_->blue;
        // Reset current_portal_ to nullptr to look for next unlinked portal
        current_portal_ = nullptr;
    }
}

bool PortalSystem::CollisionCheckWithWater(Portal portal, FluidParticle particle) {
    // Circle to Rectangle Collision Detection

    // Transform the particle position into the portal’s local space
    // Undo the portal's rotation
    f32 cos_angle = AECos(-portal.transform_.rotationRad_);
    f32 sin_angle = AESin(-portal.transform_.rotationRad_);

    // Translate particle position to portal space
    f32 translated_x = particle.transform_.pos_.x - portal.transform_.pos_.x;
    f32 translated_y = particle.transform_.pos_.y - portal.transform_.pos_.y;

    // Rotate the particle position into the portal's local frame
    f32 local_x = translated_x * cos_angle - translated_y * sin_angle;
    f32 local_y = translated_x * sin_angle + translated_y * cos_angle;

    // Find the closest point to the circle within the rectangle
    f32 rect_half_width = portal.collider_.shapeData_.box_.size_.x / 2.0f;
    f32 rect_half_height = portal.collider_.shapeData_.box_.size_.y / 2.0f;
    f32 closest_x = fmaxf(-rect_half_width, fminf(local_x, rect_half_width));
    f32 closest_y = fmaxf(-rect_half_height, fminf(local_y, rect_half_height));

    // Calculate the distance between the circle's center and this closest point
    f32 distance_x = local_x - closest_x;
    f32 distance_y = local_y - closest_y;
    // If the distance is less than the circle's radius, an intersection occurs
    f32 distance_squared = (distance_x * distance_x) + (distance_y * distance_y);
    f32 radius = particle.collider_.shapeData_.circle_.radius;

    return distance_squared < (radius * radius);
}

void PortalSystem::Update(f32 dt, std::vector<FluidParticle>& particlePool) {

    // Look for unlinked portals to set to current_portal_
    if (current_portal_ == nullptr) {
        for (auto& portal : portal_vec) {
            if (portal->linked_portal_ == nullptr) {
                current_portal_ = portal;
                std::cout << "Found unlinked portal to set as current_portal_\n";
                break;
            }
        }
    }

    // Check collision for each portal with each water particle
    for (auto& portal : portal_vec) {
        // Skip if portal is not linked
        if (portal->linked_portal_ == nullptr) {
            continue;
        }
        for (auto& particle : particlePool) {
            // Skip if particle is in iframe
            if (particle.portal_iframe_) {
                continue;
            }
            if (CollisionCheckWithWater(*portal, particle)) {
                // Teleport the particle to the linked portal's position
                // Get relative position to entrance portal
                f32 relative_pos_x = (particle.transform_.pos_.x - portal->transform_.pos_.x) /
                                     portal->transform_.scale_.x;
                f32 relative_pos_y = (particle.transform_.pos_.y - portal->transform_.pos_.y) /
                                     portal->transform_.scale_.y;

                // Rotate relative position based on portal rotations
                f32 cosEntry = AECos(portal->transform_.rotationRad_);
                f32 sinEntry = AESin(portal->transform_.rotationRad_);
                f32 cosExit = AECos(portal->linked_portal_->transform_.rotationRad_);
                f32 sinExit = AESin(portal->linked_portal_->transform_.rotationRad_);

                // Normalize position to portal's local space
                f32 normalized_pos_x = relative_pos_x * cosEntry + relative_pos_y * sinEntry;
                f32 normalized_pos_y = -relative_pos_x * sinEntry + relative_pos_y * cosEntry;

                // Adjust position to linked portal's orientation
                f32 adjusted_pos_x = normalized_pos_x * cosExit - normalized_pos_y * sinExit;
                f32 adjusted_pos_y = normalized_pos_x * sinExit + normalized_pos_y * cosExit;

                particle.transform_.pos_.x =
                    portal->linked_portal_->transform_.pos_.x + adjusted_pos_x;
                particle.transform_.pos_.y =
                    portal->linked_portal_->transform_.pos_.y + adjusted_pos_y;

                const f32 popBoost = 50.0f;
                f32 speed = AEVec2Length(&particle.physics_.velocity_);
                particle.physics_.velocity_.x = speed * cosExit + (popBoost * cosExit);
                particle.physics_.velocity_.y = speed * sinExit + (popBoost * sinExit);

                //  Activate iframe to prevent immediate re-teleportation
                particle.portal_iframe_ = true;
            }
        }
    }
}

void PortalSystem::DrawColor() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    // Render start points
    for (auto& portal : portal_vec) {
        AEGfxSetColorToMultiply(portal->red, portal->green, portal->blue, 1.0f);
        AEGfxSetTransform(portal->transform_.worldMtx_.m);
        AEGfxMeshDraw(graphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);
    }
}

void PortalSystem::DrawTexture() {}

void PortalSystem::Free() {

    AEGfxMeshFree(rectMesh);
    rectMesh = nullptr;
    graphicsConfigs_.mesh_ = nullptr;
    for (auto& portal : portal_vec) {
        portal->linked_portal_ = nullptr;
        delete portal;
    }
    portal_vec.clear();

    current_portal_ = nullptr;
}

void PortalSystem::CheckMouseClick() {
    if (click_iframe) {
        return;
    }
    // Get mouse position
    s32 mouse_x = 0, mouse_y = 0;
    AEInputGetCursorPosition(&mouse_x, &mouse_y);
    mouse_x -= AEGfxGetWindowWidth() / 2;
    mouse_y = (AEGfxGetWindowHeight() / 2) - mouse_y;

    // Use mouse pos to check collision with portal
    // Check by checking if mouse pos falls within the portal's collider box
    // If it collides with a portal, delete portal and return
    for (auto portal = portal_vec.begin(); portal != portal_vec.end();) {
        Portal* currentPortal = *portal;
        f32 rect_half_width = currentPortal->collider_.shapeData_.box_.size_.x / 2.0f;
        f32 rect_half_height = currentPortal->collider_.shapeData_.box_.size_.y / 2.0f;
        if (mouse_x >= (currentPortal->transform_.pos_.x - rect_half_width) &&
            mouse_x <= (currentPortal->transform_.pos_.x + rect_half_width) &&
            mouse_y >= (currentPortal->transform_.pos_.y - rect_half_height) &&
            mouse_y <= (currentPortal->transform_.pos_.y + rect_half_height)) {
            std::cout << "Mouse is over portal!\n";
            // Remove portal
            if (currentPortal->linked_portal_ != nullptr) {
                currentPortal->linked_portal_->linked_portal_ = nullptr;
                currentPortal->linked_portal_ = nullptr;
            }
            portal = portal_vec.erase(portal);
            if (currentPortal == current_portal_) {
                current_portal_ = nullptr;
            }
            delete currentPortal;
            click_iframe = true;
            return;
        } else {
            ++portal;
        }
    }
    // Else setup new portal at mouse position
    AEVec2 portal_pos = {static_cast<f32>(mouse_x), static_cast<f32>(mouse_y)};
    AEVec2 portal_scale = {30.0f, 60.0f};
    SetupPortal(portal_pos, portal_scale, rotation_value);
    click_iframe = true;
}

void PortalSystem::ResetIframe() { click_iframe = false; }

void PortalSystem::RotatePortal() {
    rotation_value += 45.0f;
    if (rotation_value > 360.0f) {
        rotation_value = 0.0f;
    }
    std::cout << "Portal rotation set to " << rotation_value << " degrees\n";
}

f32 PortalSystem::GetRotationValue() { return rotation_value; }