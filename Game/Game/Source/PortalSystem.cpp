/*!
@file       PortalSystem.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu

@date		March, 31, 2026

@brief      This source file implements Portal struct constructors and PortalSystem,
            handling portal placement, particle teleportation with rotation,
            VFX, and rendering.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
           Reproduction or disclosure of this file or its contents
           without the prior written consent of DigiPen Institute of
           Technology is prohibited.
*//*______________________________________________________________________*/
#include "PortalSystem.h"

// =============================
// Standard library
// =============================
#include <cmath>
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
#include "ConfigManager.h"
#include "MouseUtils.h"

// =========================================================
//
// Portal::Portal()
//
// - Default constructor. Initializes transform at the origin with unit scale,
// - sets a box collider of unit size, clears the tint color, and nulls the link.
//
// =========================================================
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

// =========================================================
//
// Portal::Portal(pos, scale, rotationDeg)
//
// - Constructs a portal at the given world position with a random tint color.
// - The collider is inset to 90% of the scale to allow slight portal overlap.
// - Rotation is stored in radians converted from the given degrees.
//
// =========================================================
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

// =========================================================
//
// PortalSystem::initialize(int const& portalMax)
//
// - Creates the shared rect mesh, loads the portal wormhole and arrow textures,
// - reads the portal scale from config, and resets all runtime state.
// - portalMax sets the portal count limit for this level (0 = unlimited).
//
// =========================================================
void PortalSystem::initialize(int const& portalMax) {

    rectMesh_ = createRectMesh();
    portalGraphicsConfigs_.mesh_ = rectMesh_;
    portalGraphicsConfigs_.texture_ = AEGfxTextureLoad("Assets/Textures/wormhole.png");

    arrowGraphicsConfigs_.mesh_ = rectMesh_;
    arrowGraphicsConfigs_.texture_ = AEGfxTextureLoad("Assets/Textures/portal_arrow.png");

    portalScale_ =
        g_configManager.getAEVec2("PortalSystem", "default", "portalScale_", AEVec2{30.f, 60.f});
    rotationValue_ = 0.0f;
    currentPortal_ = nullptr;
    clickIframe_ = false;
    portalLimit_ = portalMax;

    nextRed_ = AERandFloat();
    nextGreen_ = AERandFloat();
    nextBlue_ = AERandFloat();
}

// =========================================================
//
// PortalSystem::setupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg)
//
// - Allocates a new Portal and appends it to portalVec_.
// - The first portal of a pair is held in currentPortal_ and assigned the pending color.
// - The second portal is linked to the first bidirectionally; both share the same tint.
// - A new random color is then queued for the next pair.
// - Returns false without placing if the portal limit has been reached.
//
// =========================================================
bool PortalSystem::setupPortal(AEVec2 pos, AEVec2 scale, f32 rotationDeg) {
    // Limit number of portals that can be placed
    if (portalVec_.size() >= static_cast<size_t>(portalLimit_)) {
        return false;
    }

    Portal* portalToAdd = new Portal(pos, scale, rotationDeg);
    portalVec_.push_back(portalToAdd);

    if (currentPortal_ == nullptr) {
        currentPortal_ = portalToAdd;
        portalToAdd->red_ = nextRed_;
        portalToAdd->green_ = nextGreen_;
        portalToAdd->blue_ = nextBlue_;
    } else {
        // Link the current portal to the new portal
        currentPortal_->linkedPortal_ = portalToAdd;
        portalToAdd->linkedPortal_ = currentPortal_;
        // Set both to have same colors
        portalToAdd->red_ = currentPortal_->red_;
        portalToAdd->green_ = currentPortal_->green_;
        portalToAdd->blue_ = currentPortal_->blue_;
        // Reset currentPortal_ to nullptr to look for next unlinked portal
        currentPortal_ = nullptr;

        // Get next set of colors for the next portal pair
        nextRed_ = AERandFloat();
        nextGreen_ = AERandFloat();
        nextBlue_ = AERandFloat();
    }
    return true;
}

// =========================================================
//
// PortalSystem::collisionCheckWithWater(Portal portal, FluidParticle particle)
//
// - Rotated AABB-circle collision test.
// - Transforms the particle position into the portal's local (unrotated) space,
// - finds the closest point on the axis-aligned box, and checks whether
// - the squared distance is less than the particle radius squared.
//
// =========================================================
bool PortalSystem::collisionCheckWithWater(Portal portal, FluidParticle particle) {
    // Circle to Rectangle Collision Detection

    // Transform the particle position into the portal's local space
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

// =========================================================
//
// PortalSystem::update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfx)
//
// - Each frame: finds any unlinked portal to hold as currentPortal_,
// - then checks every linked portal against every particle.
// - On collision: maps the particle position through the entry-to-exit rotation,
// - applies a pop-boost in the exit direction, sets a portal iframe,
// - and spawns VFX at the exit portal on cooldown.
//
// =========================================================
void PortalSystem::update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfx) {
    portalVfxCooldown_ -= dt;
    // Look for unlinked portals to set to currentPortal_
    if (currentPortal_ == nullptr) {
        for (auto& portal : portalVec_) {
            if (portal->linkedPortal_ == nullptr) {
                currentPortal_ = portal;
                // std::cout << "Found unlinked portal to set as currentPortal_\n";
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
            if (collisionCheckWithWater(*portal, particle)) {
                CollisionSystem::incrementCollisionCount();
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
                    // vfx.spawnVFX(VFXType::PortalBurst, portal->transform_.pos_,
                    // portal->transform_.rotationRad_);

                    // Spawn the particles at the OUTPUT portal pos
                    vfx.spawnVFX(VFXType::PortalBurst, portal->linkedPortal_->transform_.pos_,
                                 portal->linkedPortal_->transform_.rotationRad_);

                    // Lock the timer for 0.25 seconds (Only 4 bursts allowed per second)
                    portalVfxCooldown_ = 0.25f;
                }
            }
        }
    }
}

// =========================================================
//
// PortalSystem::draw()
//
// - Draws all portals with their individual tint color.
// - Uses texture mode if the portal texture is loaded,
// - otherwise falls back to solid color mode.
//
// =========================================================
void PortalSystem::draw() {

    // If texture doesn't exist, draw as color
    if (portalGraphicsConfigs_.texture_ == nullptr) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        // Render start points
        for (auto& portal : portalVec_) {
            AEGfxSetColorToMultiply(portal->red_, portal->green_, portal->blue_, 1.0f);
            AEGfxSetTransform(portal->transform_.worldMtx_.m);
            AEGfxMeshDraw(portalGraphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);
        }
    } else {
        // Normal texture rendering
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        for (auto& portal : portalVec_) {
            AEGfxSetColorToMultiply(portal->red_, portal->green_, portal->blue_, 1.0f);
            AEGfxSetTransform(portal->transform_.worldMtx_.m);
            AEGfxTextureSet(portalGraphicsConfigs_.texture_, 0.0f, 0.0f);
            AEGfxMeshDraw(portalGraphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);
        }
    }
}

// =========================================================
//
// PortalSystem::drawPreview()
//
// - Draws a ghost portal at the current mouse position at 50% transparency.
// - Color matches the pending pair color (or the open currentPortal_ color).
// - Also draws a direction arrow offset along the portal's exit axis.
//
// =========================================================
void PortalSystem::drawPreview() {
    // Get mouse position in world space
    AEVec2 mousePos_ = getMouseWorldPos();

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
    if (currentPortal_ != nullptr) {
        r = currentPortal_->red_;
        g = currentPortal_->green_;
        b = currentPortal_->blue_;
    }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetColorToMultiply(r, g, b, 1.0f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxTextureSet(portalGraphicsConfigs_.texture_, 0.0f, 0.0f);
    AEGfxMeshDraw(portalGraphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);

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
    AEGfxTextureSet(arrowGraphicsConfigs_.texture_, 0.0f, 0.0f);
    AEGfxMeshDraw(arrowGraphicsConfigs_.mesh_, AE_GFX_MDM_TRIANGLES);
}

// =========================================================
//
// PortalSystem::free()
//
// - Frees the shared rect mesh and both textures.
// - Deletes all heap-allocated Portal objects and clears the vector.
// - Resets currentPortal_ to nullptr.
//
// =========================================================
void PortalSystem::free() {

    AEGfxMeshFree(rectMesh_);
    rectMesh_ = nullptr;
    portalGraphicsConfigs_.mesh_ = nullptr;
    arrowGraphicsConfigs_.mesh_ = nullptr;

    if (portalGraphicsConfigs_.texture_ != nullptr) {
        AEGfxTextureUnload(portalGraphicsConfigs_.texture_);
        portalGraphicsConfigs_.texture_ = nullptr;
    }
    if (arrowGraphicsConfigs_.texture_ != nullptr) {
        AEGfxTextureUnload(arrowGraphicsConfigs_.texture_);
        arrowGraphicsConfigs_.texture_ = nullptr;
    }
    for (auto& portal : portalVec_) {
        portal->linkedPortal_ = nullptr;
        delete portal;
    }
    portalVec_.clear();

    currentPortal_ = nullptr;
}

// =========================================================
//
// PortalSystem::checkMouseClick()
//
// - If the click iframe is active, returns immediately.
// - Checks whether the mouse overlaps any portal's AABB.
// - If so, unlinks and deletes that portal, then returns.
// - Otherwise, places a new portal at the mouse position via setupPortal().
// - Sets the iframe after any action to prevent double-triggering.
//
// =========================================================
void PortalSystem::checkMouseClick() {
    if (clickIframe_) {
        return;
    }
    // Get mouse position
    AEVec2 mousePos_ = getMouseWorldPos();

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
            if (currentPortal == currentPortal_) {
                currentPortal_ = nullptr;
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
    if (setupPortal(portalPos, portalScale_, rotationValue_)) {
        g_audioSystem.playSound("wormhole_place", "sfx", 2.0f, 1.0f);
    }
    clickIframe_ = true;
}

// =========================================================
//
// PortalSystem::resetIframe()
//
// - Clears the click debounce flag so the next click is accepted.
//
// =========================================================
void PortalSystem::resetIframe() { clickIframe_ = false; }

// =========================================================
//
// PortalSystem::rotatePortal()
//
// - Reads scroll wheel input and adjusts rotationValue_ by +/-15 degrees.
// - Wraps the value within [0, 360).
//
// =========================================================
void PortalSystem::rotatePortal() {

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

f32 PortalSystem::getRotationValue() const { return rotationValue_; }

const std::vector<Portal*>& PortalSystem::getPortals() const { return portalVec_; }

int PortalSystem::getPortalLimit() const { return portalLimit_; }

int PortalSystem::getPortalCount() const { return static_cast<int>(portalVec_.size()); }