/*!
@file       Collectible.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <vector>

// Third-party
#include <AEEngine.h>

// Project
#include "Button.h"
#include "Components.h"
#include "FluidSystem.h"
#include "VFXSystem.h"

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

    s8 font_{0};
    TextData collectionText_;
    int collectedCount_{0};
    int totalCollectibles_{0};

    // Visual effects
    float globalTimer_{0.0f};

public:
    void load(s8 font);
    void initialize();
    void loadLevelCollectibles(AEVec2 pos, CollectibleType type);
    void update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfxSystem);

    void draw();
    void drawPreview();
    void drawUI();
    void free();

    void spawnAtMousePos();
    void destroyAtMousePos();

    bool checkAllCollected() const { return collectedCount_ >= totalCollectibles_; }
    int getCollectedCount() const { return collectedCount_; }
    int getTotalCount() const { return totalCollectibles_; }
    std::vector<Collectible>& getCollectibles() { return collectibles_; }
    void resetCollection();

private:
    void createMeshes();
    bool checkCollisionWithWater(const Collectible& collectible, const FluidParticle& particle);
    void drawCollectible(const Collectible& c);
};