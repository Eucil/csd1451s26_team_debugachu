/*!
@file       Moss.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Moss.h"
#include "CollisionSystem.h"

#include <cmath>
#include <cstdio>

#include "Utils.h"

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

Moss::Moss(AEVec2 pos, MossType type) {
    (void)type; // Ignore the type parameter since we only use Spiky moss
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

void MossSystem::Load(s8 font) {
    font_ = font;

    // Load moss sprite sheet (48x16, 3 frames: idle / pulsing / collected)
    mossTexture_ = AEGfxTextureLoad("Assets/Textures/moss_sprite_sheet.png");
    if (!mossTexture_) {
        printf("[MossSystem] WARNING: moss_sprite_sheet.png failed to load.\n");
        printf("[MossSystem]          Falling back to colour-only draw.\n");
    } else {
        printf("[MossSystem] OK: moss_sprite_sheet.png loaded (ptr=%p)\n", (void*)mossTexture_);
    }
    // Note: meshes are created in Initialize(), not here.
}

void MossSystem::Initialize() {
    CreateMeshes();
    mosses_.clear();
    globalTimer_ = 0.0f;
    mossFrameTimer_ = 0.0f;
    mossFrame_ = 0;
}

void MossSystem::CreateMeshes() {
    // -----------------------------------------------------------------
    // Moss cluster mesh  (replaces old "spiky moss")
    //
    // Visual design: dark-green stem base with asymmetric moss spikes
    // whose tips shade into purple, matching the pixel-art concept.
    //
    // All geometry is in [-0.5, 0.5] unit space so it scales with
    // transform_.scale_ (default 40x40 world units).
    //
    // Colours (AABBGGRR format used by AEGfxTriAdd):
    //   stem base  : 0xFF1A5C1A  (dark green)
    //   spine body : 0xFF2E7D2E  (mid green)
    //   moss tip  : 0xFF8B1A8B  (dark purple)
    //   moss mid  : 0xFF6A1F6A  (purple)
    // -----------------------------------------------------------------

    // ---- Layer 0: stem / body  (spikyMossMesh_) ----
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

    // ---- Layer 1: moss spikes  (basicMossMesh_) ----
    AEGfxMeshStart();

    struct SpikeParams {
        float bx, by, angleDeg, halfW, len;
    };
    SpikeParams spikes[] = {
        {-0.38f, 0.08f, 175.0f, 0.04f, 0.28f},  {-0.30f, 0.14f, 155.0f, 0.035f, 0.24f},
        {0.38f, 0.08f, 5.0f, 0.04f, 0.28f},     {0.30f, 0.14f, 25.0f, 0.035f, 0.24f},
        {-0.06f, 0.28f, 200.0f, 0.03f, 0.20f},  {0.06f, 0.28f, 340.0f, 0.03f, 0.20f},
        {-0.14f, -0.10f, 220.0f, 0.03f, 0.18f}, {0.14f, -0.10f, 320.0f, 0.03f, 0.18f},
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

    // ---- Layer 2: glow tip pentagon  (glowingMossMesh_) ----
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

    // ---- Per-frame UV quad meshes (3 frames, sheet is 48x16) ----
    // Each frame occupies exactly 1/3 of the U axis (0.333 wide).
    // Baking the UVs per mesh avoids the stretch caused by sampling a
    // 0.333-wide strip with UVs that span the full 0.0->1.0 range.
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
    // Keep mossQuadMesh_ pointing at frame 0 for compatibility
    mossQuadMesh_ = mossFrameMeshes_[0];

    if (!spikyMossMesh_ || !basicMossMesh_ || !glowingMossMesh_ || !mossFrameMeshes_[0]) {
        printf("ERROR: Failed to create one or more moss meshes!\n");
    }
}

void MossSystem::LoadLevelMoss(AEVec2 pos, MossType type) {
    // Ignore the type parameter and always use Spiky
    (void)type;
    mosses_.emplace_back(pos, MossType::Spiky);
}

bool MossSystem::CheckCollisionWithWater(const Moss& moss, const FluidParticle& particle) {
    if (!moss.active_ || moss.currentHealth_ <= 0.0f)
        return false;

    AEVec2 delta = {particle.transform_.pos_.x - moss.transform_.pos_.x,
                    particle.transform_.pos_.y - moss.transform_.pos_.y};

    f32 distSq = delta.x * delta.x + delta.y * delta.y;
    f32 radiusSum =
        particle.collider_.shapeData_.circle_.radius_ + moss.collider_.shapeData_.circle_.radius_;

    return distSq < (radiusSum * radiusSum);
}

void MossSystem::Update(f32 dt, std::vector<FluidParticle>& particlePool,
                        StartEndPoint& startEndPointSystem) {
    (void)startEndPointSystem;
    globalTimer_ += dt;

    // Animate moss: bounce between frame 0 (idle) and frame 1 (bouncy).
    // Frame 2 (dying) is selected in DrawMoss based on health, not here.
    mossFrameTimer_ += dt;
    if (mossFrameTimer_ >= kMossFrameTime) {
        mossFrameTimer_ -= kMossFrameTime;
        mossFrame_ = (mossFrame_ == 0) ? 1 : 0;
    }

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
            if (CheckCollisionWithWater(m, *it)) {
                CollisionSystem::incrementCollisionCount();
                m.currentHealth_ -= m.absorptionRate_;
                it = particlePool.erase(it);
                if (m.currentHealth_ <= 0.0f) {
                    m.active_ = false;
                    break;
                }
            } else {
                ++it;
            }
        }
    }
}

void MossSystem::DrawMoss(const Moss& m) {
    if (!m.active_ || m.currentHealth_ <= 0.0f)
        return;

    float healthPct = m.currentHealth_ / m.maxHealth_;

    // ------------------------------------------------------------------
    // PATH A: texture draw using moss_sprite_sheet.png
    //   Sheet is 48x16 with 3 frames of 16x16.
    //   Frame 0 (U 0.000..0.333) = idle
    //   Frame 1 (U 0.333..0.667) = pulsing / glowing
    //   Frame 2 (U 0.667..1.000) = collected / dead
    //
    //   We pick the frame based on mossFrame_ (animated in Update).
    //   When health < 25% we force frame 2 (dead look).
    //
    //   AEGfxTextureSet(tex, offsetU, offsetV) shifts the sampled UV
    //   region; each frame occupies 1/3 of the sheet width.
    // ------------------------------------------------------------------
    if (mossTexture_ && mossQuadMesh_) {
        // Frame 0 = idle, frame 1 = bouncy, frame 2 = dying (health < 40%)
        int frame = (healthPct < 0.40f) ? 2 : mossFrame_;

        // U offset: frame 0 = 0.0, frame 1 = 0.333, frame 2 = 0.667
        float uOffset = static_cast<float>(frame) / 3.0f;

        AEMtx33 worldCopy = m.transform_.worldMtx_;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransform(worldCopy.m);
        // Use the per-frame mesh (UVs baked in) - no UV offset needed
        AEGfxTextureSet(mossTexture_, 0.0f, 0.0f);
        AEGfxMeshDraw(mossFrameMeshes_[frame], AE_GFX_MDM_TRIANGLES);
    }
    // ------------------------------------------------------------------
    // PATH B: fallback colour draw (no texture loaded)
    // ------------------------------------------------------------------
    else {
        if (!spikyMossMesh_ || !basicMossMesh_ || !glowingMossMesh_) {
            printf("ERROR: Moss mesh not initialized!\n");
            return;
        }

        AEMtx33 worldCopy = m.transform_.worldMtx_;
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetTransform(worldCopy.m);

        // Fallback colour draw - fluffy green palette
        // Layer 0: body - rich green fading to brown as health drops
        float bodyG = 0.63f * healthPct + 0.33f * (1.0f - healthPct);
        float bodyR = 0.12f * healthPct + 0.31f * (1.0f - healthPct);
        float bodyB = 0.08f * healthPct + 0.08f * (1.0f - healthPct);
        AEGfxSetColorToMultiply(bodyR, bodyG, bodyB, 1.0f);
        AEGfxMeshDraw(spikyMossMesh_, AE_GFX_MDM_TRIANGLES);

        // Layer 1: fluffy bumps - lighter green
        float bumpG = 0.78f * healthPct + 0.40f * (1.0f - healthPct);
        float bumpR = 0.23f * healthPct + 0.35f * (1.0f - healthPct);
        AEGfxSetColorToMultiply(bumpR, bumpG, bumpR * 0.5f, 1.0f);
        AEGfxMeshDraw(basicMossMesh_, AE_GFX_MDM_TRIANGLES);

        // Layer 2: highlight glow - bounces with growthTimer
        if (healthPct > 0.30f) {
            float pulse = 0.70f + 0.30f * sinf(m.growthTimer_ * 4.0f);
            float glowAlpha = healthPct * pulse * 0.8f;
            AEGfxSetTransparency(glowAlpha);
            AEGfxSetColorToMultiply(0.39f * pulse, 0.88f * pulse, 0.24f * pulse, 1.0f);
            AEGfxMeshDraw(glowingMossMesh_, AE_GFX_MDM_TRIANGLES);
            AEGfxSetTransparency(1.0f);
        }
    }

    // Debug health label
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

void MossSystem::DrawPreview() {
    AEVec2 mousePos = GetMouseWorldPos();

    AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;
    AEMtx33Scale(&scaleMtx, 40.0f, 40.0f);
    AEMtx33Rot(&rotMtx, 0.0f);
    AEMtx33Trans(&transMtx, mousePos.x, mousePos.y);
    AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetColorToMultiply(0.0f, 0.5f, 0.0f, 1.0f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxMeshDraw(spikyMossMesh_, AE_GFX_MDM_TRIANGLES);
}

void MossSystem::Draw() {
    for (const auto& m : mosses_) {
        DrawMoss(m);
    }
}

void MossSystem::Free() {
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
    // Free per-frame meshes (mossQuadMesh_ points to mossFrameMeshes_[0], skip it)
    for (int fi = 0; fi < 3; ++fi) {
        if (mossFrameMeshes_[fi]) {
            AEGfxMeshFree(mossFrameMeshes_[fi]);
            mossFrameMeshes_[fi] = nullptr;
        }
    }
    mossQuadMesh_ = nullptr;
    mosses_.clear();
}

void MossSystem::Unload() {
    if (mossTexture_) {
        AEGfxTextureUnload(mossTexture_);
        mossTexture_ = nullptr;
    }
}

void MossSystem::spawnAtMousePos() {
    AEVec2 mousePos = GetMouseWorldPos();
    mosses_.emplace_back(mousePos, MossType::Spiky);
}

void MossSystem::destroyAtMousePos() {
    AEVec2 mousePos = GetMouseWorldPos();
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