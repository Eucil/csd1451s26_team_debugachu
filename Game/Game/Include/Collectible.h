#pragma once
#include "AEEngine.h"
#include "Components.h"
#include "FluidSystem.h"
#include "UISystem.h"
#include <vector>

enum class CollectibleType {
    Star, // Yellow
    Gem,  // Purple
    Leaf, // Green
    Count
};

struct Collectible {
    Transform transform_;
    Collider2D collider_;

    CollectibleType type_;
    bool active_{true};
    bool collected_{false};

    // Visual properties
    float pulseTimer_{0.0f};
    float rotationSpeed_{1.0f};

    Collectible();
    Collectible(AEVec2 pos, CollectibleType type);
};

class CollectibleSystem {
private:
    AEGfxVertexList* starMesh_{nullptr};
    AEGfxVertexList* gemMesh_{nullptr};
    AEGfxVertexList* leafMesh_{nullptr};

    std::vector<Collectible> collectibles_;

    s8 font_;
    Text collectionText_;
    int collectedCount_{0};
    int totalCollectibles_{0};

    // Visual effects
    float globalTimer_{0.0f};

public:
    void Initialize(s8 font);
    void LoadLevel1Collectibles();
    void Update(f32 dt, std::vector<FluidParticle>& particlePool);
    void Draw();
    void Free();

    bool CheckAllCollected() const { return collectedCount_ >= totalCollectibles_; }
    int GetCollectedCount() const { return collectedCount_; }
    int GetTotalCount() const { return totalCollectibles_; }
    void ResetCollection();

private:
    void CreateMeshes();
    bool CheckCollisionWithWater(const Collectible& collectible, const FluidParticle& particle);
    void DrawCollectible(const Collectible& c);
};