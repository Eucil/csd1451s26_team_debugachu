/*!
@file       PortalSystem.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "PortalSystem.h"

#include <cmath>
#include <iostream>

#include <AEEngine.h>

#include "AudioSystem.h"
#include "ConfigManager.h"
#include "Utils.h"

Portal::Portal() {
    // Set up transform
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {1.f, 1.f};
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

    red_ = 0.0f;
    green_ = 0.0f;
    blue_ = 0.0f;

    linkedPortal_ = nullptr;
}

Portal::Portal(AEVec2 pos, AEVec2 scale, f32 rotationDeg) {
    transform_.pos_ = pos;
    transform_.scale_ = scale;
    transform_.rotationRad_ = AEDegToRad(rotationDeg);

    // Set up world matrix
    AEMtx33 scaleMtx, rotMtx, transMtx;

    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);

    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);

    // Set collider
    collider_.colliderShape_ = ColliderShape::Box;
    collider_.shapeData_.box_.size_.x = scale.x * 0.9f;
    collider_.shapeData_.box_.size_.y = scale.y * 0.9f;

    red_ = AERandFloat();
    green_ = AERandFloat();
    blue_ = AERandFloat();

    linkedPortal_ = nullptr;
}

void PortalSystem::Initialize(int const& portalMax) {

    rectMesh_ = CreateRectMesh();
    portal_graphicsConfigs_.mesh_ = rectMesh_;
    portal_graphicsConfigs_.texture_ = AEGfxTextureLoad("Assets/Textures/wormhole.png");

    arrow_graphicsConfigs_.mesh_ = rectMesh_;
    arrow_graphicsConfigs_.texture_ = AEGfxTextureLoad("Assets/Textures/portal_arrow.png");

    portalScale_ =
        g_configManager.getAEVec2("PortalSystem", "default", "portalScale_", AEVec2{30.f, 60.f});
    rotationValue_ = 0.0f;
    current_portal_ = nullptr;
    clickIframe_ = false;
    portalLimit_ = portalMax;

    nextRed_ = AERandFloat();
    nextGreen_ = AERandFloat();
    nextBlue_ = AERandFloat();
}

bool PortalSystem::SetupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg) {
    // Limit number of portals that can be placed
    if (portalVec_.size() >= static_cast<size_t>(portalLimit_)) {
        return false;
    }

    Portal* portalToAdd = new Portal(pos, scale, rotationDeg);
    portalVec_.push_back(portalToAdd);

    if (current_portal_ == nullptr) {
        current_portal_ = portalToAdd;
        portalToAdd->red_ = nextRed_;
        portalToAdd->green_ = nextGreen_;
        portalToAdd->blue_ = nextBlue_;
    } else {
        // Link the current portal to the new portal
        current_portal_->linkedPortal_ = portalToAdd;
        portalToAdd->linkedPortal_ = current_portal_;
        // Set both to have same colors
        portalToAdd->red_ = current_portal_->red_;
        portalToAdd->green_ = current_portal_->green_;
        portalToAdd->blue_ = current_portal_->blue_;
        // Reset current_portal_ to nullptr to look for next unlinked portal
        current_portal_ = nullptr;

        // Get next set of colors for the next portal pair
        nextRed_ = AERandFloat();
        nextGreen_ = AERandFloat();
        nextBlue_ = AERandFloat();
    }
    return true;
}

bool PortalSystem::CollisionCheckWithWater(Portal portal, FluidParticle particle) {
    // Circle to Rectangle Collision Detection

    // Transform the particle position into the portal�s local space
    // Undo the portal's rotation
    f32 cosAngle = AECos(-portal.transform_.rotationRad_);
    f32 sinAngle = AESin(-portal.transform_.rotationRad_);

    // Translate particle position to portal space
    f32 translatedX = particle.transform_.pos_.x - portal.transform_.pos_.x;
    f32 translatedY = particle.transform_.pos_.y - portal.transform_.pos_.y;

    // Rotate the particle position into the portal's local frame
    f32 localX = translatedX * cosAngle - translatedY * sinAngle;
    f32 localY = translatedX * sinAngle + translatedY * cosAngle;

    // Find the closest point to the circle within the rectangle
    f32 rectHalfWidth = portal.collider_.shapeData_.box_.size_.x / 2.0f;
    f32 rectHalfHeight = portal.collider_.shapeData_.box_.size_.y / 2.0f;
    f32 closest_x = fmaxf(-rectHalfWidth, fminf(localX, rectHalfWidth));
    f32 closest_y = fmaxf(-rectHalfHeight, fminf(localY, rectHalfHeight));

    // Calculate the distance between the circle's center and this closest point
    f32 distanceX = localX - closest_x;
    f32 distanceY = localY - closest_y;
    // If the distance is less than the circle's radius, an intersection occurs
    f32 distance_squared = (distanceX * distanceX) + (distanceY * distanceY);
    f32 radius = particle.collider_.shapeData_.circle_.radius_;

    return distance_squared < (radius * radius);
}

void PortalSystem::Update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfx) {
    portalVfxCooldown_ -= dt;
    // Look for unlinked portals to set to current_portal_
    if (current_portal_ == nullptr) {
        for (auto& portal : portalVec_) {
            if (portal->linkedPortal_ == nullptr) {
                current_portal_ = portal;
                // std::cout << "Found unlinked portal to set as current_portal_\n";
                break;
            }
        }
    }

    // Check
    // for each portal with each water particle
    for (auto& portal : portalVec_) {
        // Skip if portal is not linked
        if (portal->linkedPortal_ == nullptr) {
            continue;
        }
        for (auto& particle : particlePool) {
            // Skip if particle is in iframe
            if (particle.portalIframe_) {
                continue;
            }
            if (CollisionCheckWithWater(*portal, particle)) {
                // Teleport the particle to the linked portal's position
                // Get relative position to entrance portal
                f32 relativePosX = (particle.transform_.pos_.x - portal->transform_.pos_.x) /
                                   portal->transform_.scale_.x;
                f32 relativePosY = (particle.transform_.pos_.y - portal->transform_.pos_.y) /
                                   portal->transform_.scale_.y;

                // Rotate relative position based on portal rotations
                f32 cosEntry = AECos(portal->transform_.rotationRad_);
                f32 sinEntry = AESin(portal->transform_.rotationRad_);
                f32 cosExit = AECos(portal->linkedPortal_->transform_.rotationRad_);
                f32 sinExit = AESin(portal->linkedPortal_->transform_.rotationRad_);

                // Normalize position to portal's local space
                f32 normalizedPosX = relativePosX * cosEntry + relativePosY * sinEntry;
                f32 normalizedPosY = -relativePosX * sinEntry + relativePosY * cosEntry;

                // Adjust position to linked portal's orientation
                f32 adjustedPosX = normalizedPosX * cosExit - normalizedPosY * sinExit;
                f32 adjustedPosY = normalizedPosX * sinExit + normalizedPosY * cosExit;

                particle.transform_.pos_.x =
                    portal->linkedPortal_->transform_.pos_.x + adjustedPosX;
                particle.transform_.pos_.y =
                    portal->linkedPortal_->transform_.pos_.y + adjustedPosY;

                const f32 popBoost = 50.0f;
                f32 speed = AEVec2Length(&particle.physics_.velocity_);
                particle.physics_.velocity_.x = speed * cosExit + (popBoost * cosExit);
                particle.physics_.velocity_.y = speed * sinExit + (popBoost * sinExit);

                //  Activate iframe to prevent immediate re-teleportation
                particle.portalIframe_ = true;

                if (portalVfxCooldown_ <= 0.0f) {

                    // Spawn particles at the INPUT portal pos
                    vfx.SpawnVFX(VFXType::PortalBurst, portal->transform_.pos_);

                    // Spawn the particles at the OUTPUT portal pos
                    vfx.SpawnVFX(VFXType::PortalBurst, portal->linkedPortal_->transform_.pos_);

                    // Lock the timer for 0.25 seconds (Only 4 bursts allowed per second)
                    portalVfxCooldown_ = 0.25f;
                }
            }
        }
    }
}

void PortalSystem::Draw() {

    // If texture doesn't exist, draw as color
    if (portal_graphicsConfigs_.texture_ == nullptr) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        // Render start points
        for (auto& portal : portalVec_) {
            AEGfxSetColorToMultiply(portal->red_, portal->green_, portal->blue_, 1.0f);
            AEGfxSetTransform(portal->transform_.worldMtx_.m);
            AEGfxMeshDraw(portal_graphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);
        }
    } else {
        // Normal texture rendering
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        for (auto& portal : portalVec_) {
            AEGfxSetColorToMultiply(portal->red_, portal->green_, portal->blue_, 1.0f);
            AEGfxSetTransform(portal->transform_.worldMtx_.m);
            AEGfxTextureSet(portal_graphicsConfigs_.texture_, 0.0f, 0.0f);
            AEGfxMeshDraw(portal_graphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);
        }
    }
}

void PortalSystem::DrawPreview() {
    // Get mouse position in world space
    AEVec2 mousePos_ = GetMouseWorldPos();

    // Build world matrix for the preview portal
    AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;
    AEMtx33Scale(&scaleMtx, portalScale_.x, portalScale_.y);
    AEMtx33Rot(&rotMtx, AEDegToRad(rotationValue_));
    AEMtx33Trans(&transMtx, static_cast<f32>(mousePos_.x), static_cast<f32>(mousePos_.y));
    AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    // RGB set based on next portal colors
    // If current_portal exist, use its colors
    f32 r = nextRed_, g = nextGreen_, b = nextBlue_;
    if (current_portal_ != nullptr) {
        r = current_portal_->red_;
        g = current_portal_->green_;
        b = current_portal_->blue_;
    }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetColorToMultiply(r, g, b, 1.0f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxTextureSet(portal_graphicsConfigs_.texture_, 0.0f, 0.0f);
    AEGfxMeshDraw(portal_graphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);

    // Draw arrow indicating rotation
    f32 rotRad = AEDegToRad(rotationValue_);
    f32 exitCos = AECos(rotRad);
    f32 exitSin = AESin(rotRad);

    f32 offset = portalScale_.y * 0.6f;
    AEVec2 arrowPos = {mousePos_.x + exitCos * offset, mousePos_.y + exitSin * offset};

    AEMtx33 arrow_scaleMtx, arrow_rotMtx, arrow_transMtx, arrow_worldMtx;
    AEVec2 arrowScale = {portalScale_.x, portalScale_.x};
    AEMtx33Scale(&arrow_scaleMtx, arrowScale.x, arrowScale.y);
    AEMtx33Rot(&arrow_rotMtx, rotRad);
    AEMtx33Trans(&arrow_transMtx, arrowPos.x, arrowPos.y);
    AEMtx33Concat(&arrow_worldMtx, &arrow_rotMtx, &arrow_scaleMtx);
    AEMtx33Concat(&arrow_worldMtx, &arrow_transMtx, &arrow_worldMtx);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransform(arrow_worldMtx.m);
    AEGfxTextureSet(arrow_graphicsConfigs_.texture_, 0.0f, 0.0f);
    AEGfxMeshDraw(arrow_graphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);
}

void PortalSystem::Free() {

    AEGfxMeshFree(rectMesh_);
    rectMesh_ = nullptr;
    portal_graphicsConfigs_.mesh_ = nullptr;
    arrow_graphicsConfigs_.mesh_ = nullptr;

    if (portal_graphicsConfigs_.texture_ != nullptr) {
        AEGfxTextureUnload(portal_graphicsConfigs_.texture_);
        portal_graphicsConfigs_.texture_ = nullptr;
    }
    if (arrow_graphicsConfigs_.texture_ != nullptr) {
        AEGfxTextureUnload(arrow_graphicsConfigs_.texture_);
        arrow_graphicsConfigs_.texture_ = nullptr;
    }
    for (auto& portal : portalVec_) {
        portal->linkedPortal_ = nullptr;
        delete portal;
    }
    portalVec_.clear();

    current_portal_ = nullptr;
}

void PortalSystem::CheckMouseClick() {
    if (clickIframe_) {
        return;
    }
    // Get mouse position
    AEVec2 mousePos_ = GetMouseWorldPos();

    // Use mouse pos to check collision with portal
    // Check by checking if mouse pos falls within the portal's collider box
    // If it collides with a portal, delete portal and return
    for (auto portal = portalVec_.begin(); portal != portalVec_.end();) {
        Portal* currentPortal = *portal;
        f32 rectHalfWidth = currentPortal->collider_.shapeData_.box_.size_.x / 2.0f;
        f32 rectHalfHeight = currentPortal->collider_.shapeData_.box_.size_.y / 2.0f;
        if (mousePos_.x >= (currentPortal->transform_.pos_.x - rectHalfWidth) &&
            mousePos_.x <= (currentPortal->transform_.pos_.x + rectHalfWidth) &&
            mousePos_.y >= (currentPortal->transform_.pos_.y - rectHalfHeight) &&
            mousePos_.y <= (currentPortal->transform_.pos_.y + rectHalfHeight)) {
            // std::cout << "Mouse is over portal!\n";
            //  Remove portal
            if (currentPortal->linkedPortal_ != nullptr) {
                currentPortal->linkedPortal_->linkedPortal_ = nullptr;
                currentPortal->linkedPortal_ = nullptr;
            }
            portal = portalVec_.erase(portal);
            if (currentPortal == current_portal_) {
                current_portal_ = nullptr;
            }
            delete currentPortal;
            g_audioSystem.playSound("wormhole_place", "sfx", 2.0f, 1.0f);
            clickIframe_ = true;
            return;
        } else {
            ++portal;
        }
    }
    // Else setup new portal at mouse position
    AEVec2 portalPos = {static_cast<f32>(mousePos_.x), static_cast<f32>(mousePos_.y)};
    if (SetupPortal(portalPos, portalScale_, rotationValue_)) {
        g_audioSystem.playSound("wormhole_place", "sfx", 2.0f, 1.0f);
    }
    clickIframe_ = true;
}

void PortalSystem::ResetIframe() { clickIframe_ = false; }

void PortalSystem::RotatePortal() {

    s32 scrollInput{};
    AEInputMouseWheelDelta(&scrollInput);
    if (scrollInput > 0) {
        rotationValue_ += 15.0f; // Increase rotation
    } else if (scrollInput < 0) {
        rotationValue_ -= 15.0f; // Decrease rotation
    }

    if (rotationValue_ > 360.0f) {
        rotationValue_ = 0.0f;
    } else if (rotationValue_ < 0.0f) {
        rotationValue_ = 360.0f;
    }
}

f32 PortalSystem::GetRotationValue() const { return rotationValue_; }

const std::vector<Portal*>& PortalSystem::GetPortals() const { return portalVec_; }

int PortalSystem::GetPortalLimit() const { return portalLimit_; }