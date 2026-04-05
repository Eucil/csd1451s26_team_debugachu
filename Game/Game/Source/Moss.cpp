/*!
@file       Moss.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date       March, 31, 2026

@brief      This source file contains the definitions of classes and functions
            that implement the moss obstacle system. Moss absorbs water
            particles on contact, reducing in health until it becomes
            inactive. It renders using a UV-baked sprite sheet (48x16,
            3 frames) and falls back to procedural colour geometry when
            the texture is unavailable.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Moss.h"

// Standard library
#include <cmath>

// Project
#include "CollisionSystem.h"
#include "ConfigManager.h"
#include "MouseUtils.h"

// ==========================================
//            MOSS
// ==========================================

// =========================================================
//
// Moss::Moss()
//
// - Sets position to the origin and scale to 40x40.
// - Sets the collider to a circle with radius 20.
// - Defaults the type to Spiky and resets all health and timers.
//
// =========================================================
Moss::Moss() {
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {40.0f, 40.0f};
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius_ = 20.0f;
    collider_.shapeData_.circle_.offset_ = {0.0f, 0.0f};

    type_ = MossType::Spiky;
    absorptionRate_ = 1.0f;
    maxHealth_ = 10.0f;
    currentHealth_ = 10.0f;
    growthTimer_ = 0.0f;
}

// =========================================================
//
// Moss::Moss(pos, type)
//
// - Sets position to the given coordinates and scale to 40x40.
// - Sets the collider to a circle with radius 20.
// - Always assigns MossType::Spiky regardless of the type parameter.
// - Marks the moss as active with full health.
//
// =========================================================
Moss::Moss(AEVec2 pos, MossType type) {
    (void)type;
    transform_.pos_ = pos;
    transform_.scale_ = {40.0f, 40.0f};
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius_ = 20.0f;
    collider_.shapeData_.circle_.offset_ = {0.0f, 0.0f};

    type_ = MossType::Spiky;
    active_ = true;
    absorptionRate_ = 1.0f;
    maxHealth_ = 10.0f;
    currentHealth_ = 10.0f;
    growthTimer_ = 0.0f;
}

// ==========================================
//            MOSS SYSTEM
// ==========================================

// =========================================================
//
// MossSystem::load()
//
// - Stores the font handle for debug text.
// - Loads the moss sprite sheet from Assets/Textures/.
// - Meshes are created in initialize(), not here.
//
// =========================================================
void MossSystem::load(s8 font) {
    font_ = font;
    mossTexture_ = AEGfxTextureLoad("Assets/Textures/moss_sprite_sheet.png");

    // Read tunable gameplay values from moss.json
    // Defaults match the original hardcoded values so behaviour is unchanged
    // if the file is missing.
    mossMaxHealth_ = g_configManager.getFloat("moss", "default", "maxHealth", 10.0f);
    mossAbsorptionRate_ = g_configManager.getFloat("moss", "default", "absorptionRate", 1.0f);
    mossHitVfxCooldownMax_ = g_configManager.getFloat("moss", "default", "hitVfxCooldown", 0.1f);
    mossFrameTimeConfig_ = g_configManager.getFloat("moss", "default", "frameTime", 0.35f);
}

// =========================================================
//
// MossSystem::initialize()
//
// - Creates all procedural and UV-baked meshes.
// - Clears the mosses vector and resets all animation timers.
//
// =========================================================
void MossSystem::initialize() {
    createMeshes();
    mosses_.clear();
    globalTimer_ = 0.0f;
    mossFrameTimer_ = 0.0f;
    mossFrame_ = 0;
}

// =========================================================
//
// MossSystem::createMeshes()
//
// - Builds the stem/body procedural mesh (spikyMossMesh_) using
//   dark green and mid green colours in [-0.5, 0.5] unit space.
// - Builds the spike procedural mesh (basicMossMesh_) using
//   8 oriented spike shapes with green-to-purple colour gradients.
// - Builds the glow tip pentagon mesh (glowingMossMesh_).
// - Builds 3 UV-baked quad meshes (mossFrameMeshes_), one per frame
//   of the 48x16 sprite sheet, each covering exactly 1/3 of the U axis.
//   This prevents stretch that occurs when sampling a narrow UV strip
//   with full 0->1 UV coordinates.
// - Sets mossQuadMesh_ to point at mossFrameMeshes_[0].
//
// =========================================================
void MossSystem::createMeshes() {
    // ---- Layer 0: stem / body (spikyMossMesh_) ----
    // Colours: 0xFF1A5C1A = dark green, 0xFF2E7D2E = mid green, 0xFF3A8A3A = light green
    AEGfxMeshStart();
    AEGfxTriAdd(-0.18f, -0.50f, 0xFF1A5C1A, 0.5f, 1.0f, 0.18f, -0.50f, 0xFF1A5C1A, 0.5f, 1.0f,
                0.10f, 0.10f, 0xFF2E7D2E, 0.5f, 0.5f);
    AEGfxTriAdd(-0.18f, -0.50f, 0xFF1A5C1A, 0.5f, 1.0f, 0.10f, 0.10f, 0xFF2E7D2E, 0.5f, 0.5f,
                -0.10f, 0.10f, 0xFF2E7D2E, 0.5f, 0.5f);
    AEGfxTriAdd(-0.10f, 0.00f, 0xFF2E7D2E, 0.5f, 0.5f, -0.40f, 0.05f, 0xFF1A5C1A, 0.0f, 0.5f,
                -0.10f, 0.12f, 0xFF2E7D2E, 0.5f, 0.4f);
    AEGfxTriAdd(0.10f, 0.00f, 0xFF2E7D2E, 0.5f, 0.5f, 0.10f, 0.12f, 0xFF2E7D2E, 0.5f, 0.4f, 0.40f,
                0.05f, 0xFF1A5C1A, 1.0f, 0.5f);
    AEGfxTriAdd(-0.08f, 0.10f, 0xFF2E7D2E, 0.5f, 0.4f, 0.08f, 0.10f, 0xFF2E7D2E, 0.5f, 0.4f, 0.04f,
                0.42f, 0xFF3A8A3A, 0.5f, 0.1f);
    AEGfxTriAdd(-0.08f, 0.10f, 0xFF2E7D2E, 0.5f, 0.4f, 0.04f, 0.42f, 0xFF3A8A3A, 0.5f, 0.1f, -0.04f,
                0.42f, 0xFF3A8A3A, 0.5f, 0.1f);
    spikyMossMesh_ = AEGfxMeshEnd();

    // ---- Layer 1: spikes (basicMossMesh_) ----
    // 8 oriented spikes radiating from the body with green-to-purple gradient
    AEGfxMeshStart();
    struct SpikeParams {
        float bx, by, angleDeg, halfW, len;
    };
    SpikeParams spikes[] = {
        {-0.38f, 0.08f, 175.0f, 0.040f, 0.28f},  {-0.30f, 0.14f, 155.0f, 0.035f, 0.24f},
        {0.38f, 0.08f, 5.0f, 0.040f, 0.28f},     {0.30f, 0.14f, 25.0f, 0.035f, 0.24f},
        {-0.06f, 0.28f, 200.0f, 0.030f, 0.20f},  {0.06f, 0.28f, 340.0f, 0.030f, 0.20f},
        {-0.14f, -0.10f, 220.0f, 0.030f, 0.18f}, {0.14f, -0.10f, 320.0f, 0.030f, 0.18f},
    };
    constexpr float kPi = 3.14159265f;
    for (const auto& s : spikes) {
        float rad = s.angleDeg * kPi / 180.0f;
        float perpR = rad + kPi * 0.5f;
        float tx = s.bx + cosf(rad) * s.len;
        float ty = s.by + sinf(rad) * s.len;
        float lx = s.bx + cosf(perpR) * s.halfW;
        float ly = s.by + sinf(perpR) * s.halfW;
        float rx = s.bx - cosf(perpR) * s.halfW;
        float ry = s.by - sinf(perpR) * s.halfW;
        float mx = s.bx + cosf(rad) * s.len * 0.55f;
        float my = s.by + sinf(rad) * s.len * 0.55f;
        AEGfxTriAdd(lx, ly, 0xFF2A6B2A, lx + 0.5f, ly + 0.5f, rx, ry, 0xFF2A6B2A, rx + 0.5f,
                    ry + 0.5f, mx, my, 0xFF6A1F6A, mx + 0.5f, my + 0.5f);
        AEGfxTriAdd(lx, ly, 0xFF4A1F6A, lx + 0.5f, ly + 0.5f, mx, my, 0xFF6A1F6A, mx + 0.5f,
                    my + 0.5f, tx, ty, 0xFF9B2D9B, tx + 0.5f, ty + 0.5f);
        AEGfxTriAdd(rx, ry, 0xFF4A1F6A, rx + 0.5f, ry + 0.5f, mx, my, 0xFF6A1F6A, mx + 0.5f,
                    my + 0.5f, tx, ty, 0xFF9B2D9B, tx + 0.5f, ty + 0.5f);
    }
    basicMossMesh_ = AEGfxMeshEnd();

    // ---- Layer 2: glow tip pentagon (glowingMossMesh_) ----
    AEGfxMeshStart();
    constexpr int kPetals = 5;
    constexpr float kTipR = 0.08f;
    for (int i = 0; i < kPetals; ++i) {
        float a1 = (i * 2.0f * kPi / kPetals) - kPi * 0.5f;
        float a2 = ((i + 1) * 2.0f * kPi / kPetals) - kPi * 0.5f;
        AEGfxTriAdd(0.0f, 0.44f, 0xFFE0A0E0, 0.5f, 0.5f, cosf(a1) * kTipR, 0.44f + sinf(a1) * kTipR,
                    0xFFB060B0, 0.5f + cosf(a1) * 0.5f, 0.5f + sinf(a1) * 0.5f, cosf(a2) * kTipR,
                    0.44f + sinf(a2) * kTipR, 0xFFB060B0, 0.5f + cosf(a2) * 0.5f,
                    0.5f + sinf(a2) * 0.5f);
    }
    glowingMossMesh_ = AEGfxMeshEnd();

    // ---- Per-frame UV-baked quads (3 frames, sheet 48x16) ----
    // Each frame occupies exactly 1/3 of the U axis.
    constexpr float kFrameW = 1.0f / 3.0f;
    for (int fi = 0; fi < 3; ++fi) {
        float u0 = fi * kFrameW;
        float u1 = u0 + kFrameW;
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, u0, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f, -0.5f,
                    0.5f, 0xFFFFFFFF, u0, 0.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, u1, 0.0f, -0.5f,
                    0.5f, 0xFFFFFFFF, u0, 0.0f);
        mossFrameMeshes_[fi] = AEGfxMeshEnd();
    }
    mossQuadMesh_ = mossFrameMeshes_[0];
}

// =========================================================
//
// MossSystem::loadLevelMoss()
//
// - Ignores the type parameter and always spawns Spiky moss.
// - Appends a new Moss at the given position to the mosses vector.
//
// =========================================================
void MossSystem::loadLevelMoss(AEVec2 pos, MossType type) {
    (void)type;
    mosses_.emplace_back(pos, MossType::Spiky);
    // Apply config-driven values to the newly added moss
    Moss& m = mosses_.back();
    m.maxHealth_ = mossMaxHealth_;
    m.currentHealth_ = mossMaxHealth_;
    m.absorptionRate_ = mossAbsorptionRate_;
}

// =========================================================
//
// MossSystem::checkCollisionWithWater()
//
// - Returns false if the moss is inactive or has no health remaining.
// - Computes the squared distance between the particle and moss centres.
// - Returns true if the squared distance is less than the squared sum of radii.
//
// =========================================================
bool MossSystem::checkCollisionWithWater(const Moss& moss, const FluidParticle& particle) {
    if (!moss.active_ || moss.currentHealth_ <= 0.0f)
        return false;

    AEVec2 delta = {particle.transform_.pos_.x - moss.transform_.pos_.x,
                    particle.transform_.pos_.y - moss.transform_.pos_.y};

    f32 distSq = delta.x * delta.x + delta.y * delta.y;
    f32 radiusSum =
        particle.collider_.shapeData_.circle_.radius_ + moss.collider_.shapeData_.circle_.radius_;
    return distSq < (radiusSum * radiusSum);
}

// =========================================================
//
// MossSystem::update()
//
// - Advances the global timer and frame animation timer.
// - Alternates mossFrame_ between 0 (idle) and 1 (bouncy) on a timer.
// - For each active moss:
//   - Applies a sinusoidal pulse to the scale.
//   - Rebuilds the world transform matrix.
//   - Checks collision against all water particles in the pool.
//   - On collision, decrements health, spawns VFX (with cooldown),
//     erases the particle, and deactivates the moss if health reaches zero.
//
// =========================================================
void MossSystem::update(f32 dt, std::vector<FluidParticle>& particlePool,
                        StartEndPoint& startEndPointSystem, VFXSystem& vfx) {
    (void)startEndPointSystem;
    globalTimer_ += dt;

    mossFrameTimer_ += dt;
    if (mossFrameTimer_ >= mossFrameTimeConfig_) {
        mossFrameTimer_ -= mossFrameTimeConfig_;
        mossFrame_ = (mossFrame_ == 0) ? 1 : 0;
    }

    static f32 mossHitVfxCooldown = 0.0f;
    mossHitVfxCooldown -= dt;

    for (auto& m : mosses_) {
        if (!m.active_ || m.currentHealth_ <= 0.0f)
            continue;

        m.growthTimer_ += dt * 2.0f;
        float pulse = sinf(m.growthTimer_) * 0.1f + 1.0f;
        m.transform_.scale_ = {40.0f * pulse, 40.0f * pulse};

        AEMtx33 scale, rot, trans;
        AEMtx33Scale(&scale, m.transform_.scale_.x, m.transform_.scale_.y);
        AEMtx33Rot(&rot, m.transform_.rotationRad_);
        AEMtx33Trans(&trans, m.transform_.pos_.x, m.transform_.pos_.y);
        AEMtx33Concat(&m.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&m.transform_.worldMtx_, &trans, &m.transform_.worldMtx_);

        for (auto it = particlePool.begin(); it != particlePool.end();) {
            if (checkCollisionWithWater(m, *it)) {
                CollisionSystem::incrementCollisionCount();
                m.currentHealth_ -= m.absorptionRate_;

                if (mossHitVfxCooldown <= 0.0f) {
                    vfx.spawnVFX(VFXType::LeafCollect, it->transform_.pos_);
                    mossHitVfxCooldown = mossHitVfxCooldownMax_;
                }

                it = particlePool.erase(it);

                if (m.currentHealth_ <= 0.0f) {
                    vfx.spawnVFX(VFXType::LeafCollect, m.transform_.pos_);
                    m.active_ = false;
                    break;
                }
            } else {
                ++it;
            }
        }
    }
}

// =========================================================
//
// MossSystem::drawMoss()
//
// - Returns early if the moss is inactive or has no health.
// - PATH A (texture available): selects the animation frame based on
//   health percentage and mossFrame_, then draws using the UV-baked mesh.
//   Frame 2 (dying) is forced when health drops below 40%.
// - PATH B (fallback): draws three colour layers using procedural meshes,
//   interpolating colours from green to brown as health decreases.
//   The glow layer is skipped when health falls below 30%.
// - If showDebug_ is true, prints the current health value on screen.
//
// =========================================================
void MossSystem::drawMoss(const Moss& m) {
    if (!m.active_ || m.currentHealth_ <= 0.0f)
        return;

    float healthPct = m.currentHealth_ / m.maxHealth_;

    if (mossTexture_ && mossQuadMesh_) {
        int frame = (healthPct < 0.40f) ? 2 : mossFrame_;

        AEMtx33 worldCopy = m.transform_.worldMtx_;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransform(worldCopy.m);
        AEGfxTextureSet(mossTexture_, 0.0f, 0.0f);
        AEGfxMeshDraw(mossFrameMeshes_[frame], AE_GFX_MDM_TRIANGLES);
    } else {
        if (!spikyMossMesh_ || !basicMossMesh_ || !glowingMossMesh_)
            return;

        AEMtx33 worldCopy = m.transform_.worldMtx_;
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetTransform(worldCopy.m);

        float bodyG = 0.63f * healthPct + 0.33f * (1.0f - healthPct);
        float bodyR = 0.12f * healthPct + 0.31f * (1.0f - healthPct);
        float bodyB = 0.08f * healthPct + 0.08f * (1.0f - healthPct);
        AEGfxSetColorToMultiply(bodyR, bodyG, bodyB, 1.0f);
        AEGfxMeshDraw(spikyMossMesh_, AE_GFX_MDM_TRIANGLES);

        float bumpG = 0.78f * healthPct + 0.40f * (1.0f - healthPct);
        float bumpR = 0.23f * healthPct + 0.35f * (1.0f - healthPct);
        AEGfxSetColorToMultiply(bumpR, bumpG, bumpR * 0.5f, 1.0f);
        AEGfxMeshDraw(basicMossMesh_, AE_GFX_MDM_TRIANGLES);

        if (healthPct > 0.30f) {
            float pulse = 0.70f + 0.30f * sinf(m.growthTimer_ * 4.0f);
            float glowAlpha = healthPct * pulse * 0.8f;
            AEGfxSetTransparency(glowAlpha);
            AEGfxSetColorToMultiply(0.39f * pulse, 0.88f * pulse, 0.24f * pulse, 1.0f);
            AEGfxMeshDraw(glowingMossMesh_, AE_GFX_MDM_TRIANGLES);
            AEGfxSetTransparency(1.0f);
        }
    }

    if (showDebug_) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.0f", m.currentHealth_);
        float screenX = m.transform_.pos_.x / 800.0f;
        float screenY = m.transform_.pos_.y / 450.0f;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxPrint(font_, buffer, screenX, screenY, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

// =========================================================
//
// MossSystem::drawPreview()
//
// - Gets the current mouse world position.
// - Builds a scale-rotate-translate matrix at the mouse position.
// - Draws a semi-transparent green preview of the moss body mesh.
//
// =========================================================
void MossSystem::drawPreview() {
    AEVec2 mousePos = getMouseWorldPos();

    AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;
    AEMtx33Scale(&scaleMtx, 40.0f, 40.0f);
    AEMtx33Rot(&rotMtx, 0.0f);
    AEMtx33Trans(&transMtx, mousePos.x, mousePos.y);
    AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetColorToMultiply(0.0f, 0.5f, 0.0f, 1.0f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxMeshDraw(spikyMossMesh_, AE_GFX_MDM_TRIANGLES);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxTextureSet(mossTexture_, 0.0f, 0.0f);
    AEGfxMeshDraw(mossFrameMeshes_[1], AE_GFX_MDM_TRIANGLES);
}

// =========================================================
//
// MossSystem::draw()
//
// - Iterates over all mosses and calls drawMoss() for each.
//
// =========================================================
void MossSystem::draw() {
    for (const auto& m : mosses_) {
        drawMoss(m);
    }
}

// =========================================================
//
// MossSystem::free()
//
// - Frees all procedural colour meshes if they are valid.
// - Frees all three per-frame UV-baked meshes if they are valid.
// - Nulls mossQuadMesh_ since it points to a now-freed mesh.
// - Clears the mosses vector.
//
// =========================================================
void MossSystem::free() {
    if (spikyMossMesh_) {
        AEGfxMeshFree(spikyMossMesh_);
        spikyMossMesh_ = nullptr;
    }
    if (basicMossMesh_) {
        AEGfxMeshFree(basicMossMesh_);
        basicMossMesh_ = nullptr;
    }
    if (glowingMossMesh_) {
        AEGfxMeshFree(glowingMossMesh_);
        glowingMossMesh_ = nullptr;
    }
    for (int fi = 0; fi < 3; ++fi) {
        if (mossFrameMeshes_[fi]) {
            AEGfxMeshFree(mossFrameMeshes_[fi]);
            mossFrameMeshes_[fi] = nullptr;
        }
    }
    mossQuadMesh_ = nullptr;
    mosses_.clear();
}

// =========================================================
//
// MossSystem::unload()
//
// - Unloads the moss sprite sheet texture from the GPU if valid.
// - Nulls the texture pointer to prevent dangling references.
//
// =========================================================
void MossSystem::unload() {
    if (mossTexture_) {
        AEGfxTextureUnload(mossTexture_);
        mossTexture_ = nullptr;
    }
}

// =========================================================
//
// MossSystem::spawnAtMousePos()
//
// - Gets the current mouse world position.
// - Spawns a new Spiky moss at that position.
//
// =========================================================
void MossSystem::spawnAtMousePos() {
    AEVec2 mousePos = getMouseWorldPos();
    mosses_.emplace_back(mousePos, MossType::Spiky);
}

// =========================================================
//
// MossSystem::destroyAtMousePos()
//
// - Gets the current mouse world position.
// - Iterates over all active mosses.
// - Removes the first one whose bounding box contains the mouse position.
//
// =========================================================
void MossSystem::destroyAtMousePos() {
    AEVec2 mousePos = getMouseWorldPos();
    float mouseX = mousePos.x;
    float mouseY = mousePos.y;

    for (auto it = mosses_.begin(); it != mosses_.end(); ++it) {
        if (!it->active_ || it->currentHealth_ <= 0.0f)
            continue;

        float radius = it->collider_.shapeData_.circle_.radius_;
        if (mouseX >= (it->transform_.pos_.x - radius) &&
            mouseX <= (it->transform_.pos_.x + radius) &&
            mouseY >= (it->transform_.pos_.y - radius) &&
            mouseY <= (it->transform_.pos_.y + radius)) {
            mosses_.erase(it);
            return;
        }
    }
}