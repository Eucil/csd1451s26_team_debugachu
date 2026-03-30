#pragma once
#include "AEEngine.h"
#include "Components.h"
#include "FluidSystem.h"
#include <vector>

// Forward declarations instead of includes
class StartEndPoint;
enum class StartEndType;

// Keep enum for backward compatibility but all will be Spiky
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
    float absorptionRate_{1.0f}; // How much water it absorbs per particle
    float growthTimer_{0.0f};    // For animation
    float maxHealth_{10.0f};     // How many particles it can absorb before dying
    float currentHealth_{10.0f};

    Moss();
    Moss(AEVec2 pos, MossType type); // Type parameter ignored, always Spiky
};

class MossSystem {
private:
    // Only need one mesh now
    AEGfxVertexList* spikyMossMesh_{nullptr};

    // Keep these for backward compatibility but don't use them
    AEGfxVertexList* basicMossMesh_{nullptr};
    AEGfxVertexList* glowingMossMesh_{nullptr};

    std::vector<Moss> mosses_;

    s8 font_;
    bool showDebug_{true}; // set to true to show hp of moss. false to off it

    // Visual effects
    float globalTimer_{0.0f};

public:
    void Load(s8 font);
    void Initialize();
    void LoadLevelMoss(AEVec2 pos, MossType type); // Type ignored, always Spiky
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