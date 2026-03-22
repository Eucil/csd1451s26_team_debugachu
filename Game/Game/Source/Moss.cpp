#include "Moss.h"

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
    // Verify mesh was created
    if (!spikyMossMesh_) {
        printf("ERROR: Failed to create moss mesh!\n");
    }
}

void MossSystem::Initialize() {
    CreateMeshes();
    mosses_.clear();
    globalTimer_ = 0.0f;
}

void MossSystem::CreateMeshes() {

    // Spiky moss mesh (dark green)
    AEGfxMeshStart();
    constexpr int kSpikes = 8;
    for (int i = 0; i < kSpikes; i++) {
        float angle1 = (i * 2.0f * 3.14159f / kSpikes);
        float angle2 = ((i + 1) * 2.0f * 3.14159f / kSpikes);

        float x1 = cosf(angle1) * 0.5f;
        float y1 = sinf(angle1) * 0.5f;
        float x2 = cosf(angle2) * 0.5f;
        float y2 = sinf(angle2) * 0.5f;

        // Create kSpikes
        float spikeX = cosf(angle1 + 3.14159f / kSpikes) * 0.7f;
        float spikeY = sinf(angle1 + 3.14159f / kSpikes) * 0.7f;

        AEGfxTriAdd(0.0f, 0.0f, 0xFF00AA00, 0.5f, 0.5f, x1, y1, 0xFF00AA00, x1 + 0.5f, y1 + 0.5f,
                    spikeX, spikeY, 0xFF00AA00, spikeX + 0.5f, spikeY + 0.5f);

        AEGfxTriAdd(0.0f, 0.0f, 0xFF00AA00, 0.5f, 0.5f, spikeX, spikeY, 0xFF00AA00, spikeX + 0.5f,
                    spikeY + 0.5f, x2, y2, 0xFF00AA00, x2 + 0.5f, y2 + 0.5f);
    }
    spikyMossMesh_ = AEGfxMeshEnd();

    // Set other mesh pointers to nullptr since they're not used
    basicMossMesh_ = nullptr;
    glowingMossMesh_ = nullptr;

    // Check if mesh was created successfully
    if (!spikyMossMesh_) {
        printf("ERROR: Failed to create moss mesh!\n");
    }
}

void MossSystem::LoadLevelMoss(AEVec2 pos, MossType type) {
    // Ignore the type parameter and always use Spiky
    mosses_.emplace_back(pos, MossType::Spiky);
}

bool MossSystem::CheckCollisionWithWater(const Moss& moss, const FluidParticle& particle) {
    if (!moss.active_ || moss.currentHealth_ <= 0.0f)
        return false;

    // Circle-circle collision
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

    for (auto& m : mosses_) {
        if (!m.active_ || m.currentHealth_ <= 0.0f)
            continue;

        // Growth animation (bobbing effect)
        m.growthTimer_ += dt * 2.0f;
        float pulse = sinf(m.growthTimer_) * 0.1f + 1.0f;
        m.transform_.scale_ = {40.0f * pulse, 40.0f * pulse};

        // Update transform matrix
        AEMtx33 scale, rot, trans;
        AEMtx33Scale(&scale, m.transform_.scale_.x, m.transform_.scale_.y);
        AEMtx33Rot(&rot, m.transform_.rotationRad_);
        AEMtx33Trans(&trans, m.transform_.pos_.x, m.transform_.pos_.y);

        AEMtx33Concat(&m.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&m.transform_.worldMtx_, &trans, &m.transform_.worldMtx_);

        // Check collision with water particles
        for (auto it = particlePool.begin(); it != particlePool.end();) {
            if (CheckCollisionWithWater(m, *it)) {
                // Moss absorbs the particle
                m.currentHealth_ -= m.absorptionRate_;

                // Remove the water particle
                it = particlePool.erase(it);

                // If moss is dead, break out of particle loop
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

    // SAFETY CHECK: Make sure mesh exists
    if (!spikyMossMesh_) {
        printf("ERROR: Moss mesh not initialized!\n");
        return;
    }

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEMtx33 transformCopy = m.transform_.worldMtx_;
    AEGfxSetTransform(transformCopy.m);

    // Color based on health percentage (darker as it takes damage)
    float healthPercent = m.currentHealth_ / m.maxHealth_;

    // Dark green that gets darker with damage
    // Base color: 0.0f, 0.5f, 0.0f at full health
    // Becomes: 0.0f, 0.1f, 0.0f at low health
    AEGfxSetColorToMultiply(0.0f, 0.5f * healthPercent, 0.0f, 1.0f);
    AEGfxMeshDraw(spikyMossMesh_, AE_GFX_MDM_TRIANGLES);

    // Debug: Draw health number
    if (showDebug_) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.0f", m.currentHealth_);
        float screenX = m.transform_.pos_.x / 800.0f;
        float screenY = m.transform_.pos_.y / 450.0f;

        // Save current render mode
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        AEGfxPrint(font_, buffer, screenX, screenY, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void MossSystem::Draw() {
    for (const auto& m : mosses_) {
        DrawMoss(m);
    }
}

void MossSystem::Free() {
    // Only free the spiky mesh since it's the only one used
    if (spikyMossMesh_) {
        AEGfxMeshFree(spikyMossMesh_);
        spikyMossMesh_ = nullptr;
    }
    // Set other mesh pointers to nullptr (they should already be null)
    basicMossMesh_ = nullptr;
    glowingMossMesh_ = nullptr;

    mosses_.clear();
}

void MossSystem::spawnAtMousePos() {
    AEVec2 mousePos = GetMouseWorldPos();
    // Always spawn Spiky type
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