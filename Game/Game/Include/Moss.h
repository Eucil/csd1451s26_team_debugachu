/*!
@file       Moss.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date       March, 31, 2026

@brief      This header file contains the declarations of classes and functions
            that implement the moss obstacle system, which includes:

                - Moss, a struct representing a single moss obstacle with
                  transform, collider, health, and animation state
                - MossSystem, a manager class that handles loading, updating,
                  drawing, and freeing all moss obstacles in a level

            Moss absorbs water particles on contact and reduces in health
            until it becomes inactive. It is drawn using a UV-baked sprite
            sheet with three animation frames, falling back to procedural
            colour geometry if the texture is unavailable.

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
#include "Components.h"
#include "FluidSystem.h"
#include "VFXSystem.h"

// Forward declarations
class StartEndPoint;
enum class StartEndType;

// ==========================================
// MOSS TYPE
// ==========================================
enum class MossType {
    Basic,   // Reserved, not used
    Spiky,   // Active type
    Glowing, // Reserved, not used
    Count
};

// ==========================================
// MOSS
// ==========================================
struct Moss {
    Transform transform_;
    Collider2D collider_;

    MossType type_; // Always Spiky
    bool active_{true};
    float absorptionRate_{1.0f};
    float growthTimer_{0.0f};
    float maxHealth_{10.0f};
    float currentHealth_{10.0f};

    Moss();
    Moss(AEVec2 pos, MossType type);
};

// ==========================================
// MOSS SYSTEM
// ==========================================
class MossSystem {
private:
    // Procedural colour meshes (fallback when texture is missing)
    AEGfxVertexList* spikyMossMesh_{nullptr};
    AEGfxVertexList* basicMossMesh_{nullptr};
    AEGfxVertexList* glowingMossMesh_{nullptr};

    // Per-frame UV-baked quads (3 frames, sheet is 48x16).
    // Each mesh samples exactly 1/3 of the U axis to avoid stretch.
    AEGfxVertexList* mossFrameMeshes_[3]{nullptr, nullptr, nullptr};
    AEGfxVertexList* mossQuadMesh_{nullptr}; // Points at mossFrameMeshes_[0]
    AEGfxTexture* mossTexture_{nullptr};

    float mossFrameTimer_{0.0f};
    int mossFrame_{0};

    // Values read from Moss.json in load() -- exposed here so loadLevelMoss()
    // can apply them to each newly created Moss instance.
    float mossFrameTimeConfig_{0.35f};  // seconds per animation frame
    float mossMaxHealth_{10.0f};        // starting health for each moss
    float mossAbsorptionRate_{1.0f};    // water particles absorbed per hit
    float mossHitVfxCooldownMax_{0.1f}; // minimum seconds between hit VFX

    std::vector<Moss> mosses_;

    s8 font_{0};
    bool showDebug_{false};

    float globalTimer_{0.0f};

public:
    void load(s8 font);
    void unload();
    void initialize();
    void loadLevelMoss(AEVec2 pos, MossType type);
    void update(f32 dt, std::vector<FluidParticle>& particlePool,
                StartEndPoint& startEndPointSystem, VFXSystem& vfx);
    void draw();
    void drawPreview();
    void free();

    void spawnAtMousePos();
    void destroyAtMousePos();

    std::vector<Moss>& getMosses() { return mosses_; }

private:
    void createMeshes();
    bool checkCollisionWithWater(const Moss& moss, const FluidParticle& particle);
    void drawMoss(const Moss& m);
};