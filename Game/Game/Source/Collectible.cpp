#include "Collectible.h"
#include "AudioSystem.h"
#include "ConfigManager.h"
#include "Utils.h"

#include <cmath>
#include <cstdio>

// constructor to set posit, scale, rotation,collider type, radius
Collectible::Collectible() {
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {30.0f, 30.0f};
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius = 15.0f;
    collider_.shapeData_.circle_.offset_ = {0.0f, 0.0f};

    type_ = CollectibleType::Star;
    pulseTimer_ = 0.0f;
    rotationSpeed_ = 1.0f;
}

// constructor to take posit and param
// Sets position to given coord
Collectible::Collectible(AEVec2 pos, CollectibleType type) {
    transform_.pos_ = pos;
    transform_.scale_ = {30.0f, 30.0f};
    transform_.rotationRad_ = 0.0f;

    collider_.colliderShape_ = ColliderShape::Circle;
    collider_.shapeData_.circle_.radius = 15.0f;
    collider_.shapeData_.circle_.offset_ = {0.0f, 0.0f};

    // type_ set to parameter value (Star, Gem, or Leaf)
    //   active_ = true - collectible exists in world
    // collected_ = false - not yet collected
    type_ = type;
    active_ = true;
    collected_ = false;
    pulseTimer_ = 0.0f;
    rotationSpeed_ = 1.0f + (AERandFloat() * 2.0f); // Random rotation speed
}

// Load system w font
// reads UI posit from JSON config file
void CollectibleSystem::Load(s8 font) {
    font_ = font;
    collectionText_.x_ =
        configManager.getFloat("Collectible", "default", "collectionText_.x_", -0.6f);
    collectionText_.y_ =
        configManager.getFloat("Collectible", "default", "collectionText_.y_", 0.92f);
    collectionText_.scale_ =
        configManager.getFloat("Collectible", "default", "collectionText_.scale_", 0.5f);
    collectionText_.content_ =
        configManager.getString("Collectible", "default", "collectionText_.x_", "Items: 0/3");
}

// CreateMeshes() - builds the 3D shapes for collectibles
// collectedCount_ = 0 - reset collected counter
// totalCollectibles_ = 0 - reset total count
// globalTimer_ = 0 - reset animation timer
// collectibles_.clear() - remove all collectibles from previous level
void CollectibleSystem::Initialize() {
    CreateMeshes();
    collectedCount_ = 0;
    totalCollectibles_ = 0;
    globalTimer_ = 0.0f;

    collectibles_.clear();
}

// AEGfxMeshStart() - starts building a mesh
// outerRadius = 0.5f - distance from center to star tips
// innerRadius = 0.25f - distance from center to inner points
void CollectibleSystem::CreateMeshes() {
    // Star mesh (5-pointed star shape)
    AEGfxMeshStart();
    constexpr int starPoints = 5;
    constexpr f32 outerRadius = 0.5f;
    constexpr f32 innerRadius = 0.25f;

    // calculations to create a star shape
    for (int i = 0; i < starPoints; i++) {
        f32 angle1 = (i * 2.0f * 3.14159f / starPoints) - 3.14159f / 2.0f;
        f32 angle2 = ((i + 1) * 2.0f * 3.14159f / starPoints) - 3.14159f / 2.0f;
        f32 midAngle = (angle1 + angle2) / 2.0f;

        // Using cosine and sine to convert angles to coordinates
        // Multiply by outerRadius to get tip positions
        f32 x1 = cosf(angle1) * outerRadius;
        f32 y1 = sinf(angle1) * outerRadius;
        f32 x2 = cosf(angle2) * outerRadius;
        f32 y2 = sinf(angle2) * outerRadius;

        // Inner point
        f32 xi = cosf(midAngle) * innerRadius;
        f32 yi = sinf(midAngle) * innerRadius;
        // adds a triangle to the mesh
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFF00, 0.5f, 0.5f, x1, y1, 0xFFFFFF00, x1 + 0.5f, y1 + 0.5f,
                    xi, yi, 0xFFFFFF00, xi + 0.5f, yi + 0.5f);
        // Add second triangle and end mesh
        // Together they form one point of the star
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFF00, 0.5f, 0.5f, xi, yi, 0xFFFFFF00, xi + 0.5f, yi + 0.5f,
                    x2, y2, 0xFFFFFF00, x2 + 0.5f, y2 + 0.5f);
    }
    // AEGfxMeshEnd() completes mesh and returns pointer
    // starMesh_ stores this mesh for later drawing
    starMesh_ = AEGfxMeshEnd();

    // Gem mesh (diamond shape)
    AEGfxMeshStart();
    AEGfxTriAdd(0.0f, 0.5f, 0xFFFF00FF, 0.5f, 1.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f, 0.0f,
                0xFFFF00FF, 1.0f, 0.5f);

    AEGfxTriAdd(0.0f, -0.5f, 0xFFFF00FF, 0.5f, 0.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f,
                0.0f, 0xFFFF00FF, 1.0f, 0.5f);
    gemMesh_ = AEGfxMeshEnd();

    // Leaf mesh (simple teardrop shape)
    AEGfxMeshStart();
    constexpr int leafSegments = 12;
    for (int i = 0; i < leafSegments; i++) {
        f32 angle1 = (i * 2.0f * 3.14159f / leafSegments);
        f32 angle2 = ((i + 1) * 2.0f * 3.14159f / leafSegments);

        f32 r1 = 0.3f + 0.2f * sinf(angle1 * 2.0f);
        f32 r2 = 0.3f + 0.2f * sinf(angle2 * 2.0f);

        f32 x1 = cosf(angle1) * r1;
        f32 y1 = sinf(angle1) * r1;
        f32 x2 = cosf(angle2) * r2;
        f32 y2 = sinf(angle2) * r2;

        AEGfxTriAdd(0.0f, 0.0f, 0xFF00FF00, 0.5f, 0.5f, x1, y1, 0xFF00FF00, x1 + 0.5f, y1 + 0.5f,
                    x2, y2, 0xFF00FF00, x2 + 0.5f, y2 + 0.5f);
    }
    leafMesh_ = AEGfxMeshEnd();
}

// Add collectible to level
void CollectibleSystem::LoadLevelCollectibles(AEVec2 pos, CollectibleType type) {

    // emplace_back constructs collectible directly in vector
    // Updates total count based on vector size
    collectibles_.emplace_back(pos, type);

    totalCollectibles_ = static_cast<int>(collectibles_.size());
}

// collision check start
bool CollectibleSystem::CheckCollisionWithWater(const Collectible& collectible,
                                                const FluidParticle& particle) {
    if (!collectible.active_ || collectible.collected_)
        return false;

    // Circle-circle collision
    // delta.x = particle.x - collectible.x
    // delta.y = particle.y - collectible.y
    AEVec2 delta = {particle.transform_.pos_.x - collectible.transform_.pos_.x,
                    particle.transform_.pos_.y - collectible.transform_.pos_.y};

    f32 distSq = delta.x * delta.x + delta.y * delta.y;
    f32 radiusSum = particle.collider_.shapeData_.circle_.radius +
                    collectible.collider_.shapeData_.circle_.radius;
    // If squared distance < squared sum of radii ? Collision
    return distSq < (radiusSum * radiusSum);
}

void CollectibleSystem::Update(f32 dt, std::vector<FluidParticle>& particlePool) {
    globalTimer_ += dt;

    for (auto& c : collectibles_) {
        if (!c.active_ || c.collected_)
            continue;

        // Pulse effect
        c.pulseTimer_ += dt * 3.0f;
        f32 pulse = sinf(c.pulseTimer_) * 0.1f + 1.0f;
        c.transform_.scale_ = {30.0f * pulse, 30.0f * pulse};

        // Rotation effect
        // Creates continuous spinning effect
        c.transform_.rotationRad_ += dt * c.rotationSpeed_;

        // Update transform matrix
        AEMtx33 scale, rot, trans;
        AEMtx33Scale(&scale, c.transform_.scale_.x, c.transform_.scale_.y);
        AEMtx33Rot(&rot, c.transform_.rotationRad_);
        AEMtx33Trans(&trans, c.transform_.pos_.x, c.transform_.pos_.y);

        AEMtx33Concat(&c.transform_.worldMtx_, &rot, &scale);
        AEMtx33Concat(&c.transform_.worldMtx_, &trans, &c.transform_.worldMtx_);

        // Check collision with water particles
        for (const auto& particle : particlePool) {
            if (CheckCollisionWithWater(c, particle)) {
                // if collision, mark collected, increment count, exit particle loop
                c.collected_ = true;
                collectedCount_++;

                gAudioSystem.playSound("ding", "sfx", 1.0f, 1.0f);

                break;
            }
        }
    }

    // Update collection text
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Items: %d/%d", collectedCount_, totalCollectibles_);
    collectionText_.content_ = buffer;
}

void CollectibleSystem::DrawCollectible(const Collectible& c) {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEMtx33 transformCopy = c.transform_.worldMtx_;
    AEGfxSetTransform(transformCopy.m);

    switch (c.type_) {
    case CollectibleType::Star:
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        AEGfxMeshDraw(starMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Gem:
        AEGfxSetColorToMultiply(1.0f, 0.0f, 1.0f, 1.0f); // Purple
        AEGfxMeshDraw(gemMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    case CollectibleType::Leaf:
        AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f); // Green
        AEGfxMeshDraw(leafMesh_, AE_GFX_MDM_TRIANGLES);
        break;
    }
}

// Loop through all collectibles, Skip inactive or collected ones,
// Call DrawCollectible for each active one
void CollectibleSystem::Draw() {
    // Draw collectibles
    for (const auto& c : collectibles_) {
        if (!c.active_ || c.collected_)
            continue;
        DrawCollectible(c);
    }
}

void CollectibleSystem::DrawUI() {
    // Draw collection counter
    collectionText_.draw(font_);
}

// Free each mesh if it exists
void CollectibleSystem::Free() {
    if (starMesh_) {
        AEGfxMeshFree(starMesh_);
        starMesh_ = nullptr;
    }
    if (gemMesh_) {
        AEGfxMeshFree(gemMesh_);
        gemMesh_ = nullptr;
    }
    if (leafMesh_) {
        AEGfxMeshFree(leafMesh_);
        leafMesh_ = nullptr;
    }
    collectibles_.clear();
}

void CollectibleSystem::spawnAtMousePos() {
    AEVec2 mousePos = GetMouseWorldPos();
    collectibles_.emplace_back(mousePos, static_cast<CollectibleType>(totalCollectibles_ % 3));
    totalCollectibles_ = static_cast<int>(collectibles_.size());
}

void CollectibleSystem::destroyAtMousePos() {
    // Get mouse position
    AEVec2 mousePos = GetMouseWorldPos();
    f32 mouse_x = static_cast<f32>(mousePos.x);
    f32 mouse_y = static_cast<f32>(mousePos.y);

    // Check if mouse is over any start point
    for (auto it = collectibles_.begin(); it != collectibles_.end(); ++it) {
        if (!it->active_ || it->collected_)
            continue;

        f32 radius = it->collider_.shapeData_.circle_.radius;
        if (mouse_x >= (it->transform_.pos_.x - radius) &&
            mouse_x <= (it->transform_.pos_.x + radius) &&
            mouse_y >= (it->transform_.pos_.y - radius) &&
            mouse_y <= (it->transform_.pos_.y + radius)) {
            collectibles_.erase(it);
            totalCollectibles_ = static_cast<int>(collectibles_.size());
            return;
        }
    }
}

void CollectibleSystem::ResetCollection() {
    collectedCount_ = 0;
    for (auto& c : collectibles_) {
        c.collected_ = false;
    }
}