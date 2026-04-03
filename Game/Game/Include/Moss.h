/*!
@file       Moss.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#pragma once
#include "AEEngine.h"
#include "Components.h"
#include "FluidSystem.h"
#include <vector>

// Forward declarations instead of includes
class StartEndPoint;
enum class StartEndType;

enum class MossType {
    Basic,   // Not used anymore
    Spiky,   // The only type used
    Glowing, // Not used anymore
    Count
};

struct Moss {
    Transform transform_;
    Collider2D collider_;

    MossType type_; // Will always be Spiky
    bool active_{true};
    float absorptionRate_{1.0f};
    float growthTimer_{0.0f};
    float maxHealth_{10.0f};
    float currentHealth_{10.0f};

    Moss();
    Moss(AEVec2 pos, MossType type);
};

class MossSystem {
private:
    // Procedural colour meshes (fallback when texture is missing)
    AEGfxVertexList* spikyMossMesh_{nullptr};
    AEGfxVertexList* basicMossMesh_{nullptr};
    AEGfxVertexList* glowingMossMesh_{nullptr};

    // Per-frame UV-baked quads (3 frames, sheet is 48x16).
    // Each mesh has UVs pre-set to the correct 1/3 strip so the
    // sprite is not stretched when drawn on a square quad.
    AEGfxVertexList* mossFrameMeshes_[3]{nullptr, nullptr, nullptr};

    // mossQuadMesh_ kept for internal use; points at mossFrameMeshes_[0]
    AEGfxVertexList* mossQuadMesh_{nullptr};
    AEGfxTexture* mossTexture_{nullptr};

    // Sprite animation state
    float mossFrameTimer_{0.0f};
    int mossFrame_{0};
    static constexpr float kMossFrameTime = 0.35f; // seconds per frame

    std::vector<Moss> mosses_;

    s8 font_;
    bool showDebug_{false};

    float globalTimer_{0.0f};

public:
    void Load(s8 font);
    void Unload();
    void Initialize();
    void LoadLevelMoss(AEVec2 pos, MossType type);
    void Update(f32 dt, std::vector<FluidParticle>& particlePool,
                StartEndPoint& startEndPointSystem);
    void Draw();
    void DrawPreview();
    void Free();

    void spawnAtMousePos();
    void destroyAtMousePos();

    std::vector<Moss>& GetMosses() { return mosses_; }

private:
    void CreateMeshes();
    bool CheckCollisionWithWater(const Moss& moss, const FluidParticle& particle);
    void DrawMoss(const Moss& m);
};