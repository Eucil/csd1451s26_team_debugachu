/*!
@file       StartEndPoint.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu

@date		March, 31, 2026

@brief      This header file declares the StartEnd struct and StartEndPoint class,
            which manage pipe start points, the flower end point, water flow,
            particle collection, and related rendering.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// =============================
// Standard library
// =============================
#include <vector>

// =============================
// Third-party
// =============================
#include <AEEngine.h>

// =============================
// Project
// =============================
#include "Components.h"
#include "FluidSystem.h"
#include "VFXSystem.h"

// =============================
// StartEnd Types
// =============================
enum class StartEndType { Pipe, Flower, Count };

// =============================
// Goal Direction Types
// =============================
enum class GoalDirection { Up, Down, Left, Right };

struct StartEnd {

    f32 vfxTimer_ = 0.0f;

    Transform transform_;
    Collider2D collider_;
    StartEndType type_;
    GoalDirection direction_;

    bool releaseWater_{false};
    bool releaseWaterIframe_{false};

    bool active_{true};

    float waterCapacity_{100.0f};  // Total water available for this start point
    float waterRemaining_{100.0f}; // Water remaining
    bool infiniteWater_{false};    // Debug/infinite mode

    StartEnd(StartEndType type);
    StartEnd(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type, GoalDirection direction);
};

class StartEndPoint {
private:
    AEGfxVertexList* rectMesh_ = nullptr;
    AEGfxVertexList* flowerMesh_ = nullptr;

    // graphic configs for each StartEnd type
    Graphics graphicsConfigs_[static_cast<int>(StartEndType::Count)];

    //  Water indicator UI
    AEGfxVertexList* barMesh_{nullptr};
    s8 font_{0};

public:
    // Can have multiple start points but only one end point
    std::vector<StartEnd> startPoints_;
    StartEnd endPoint_{StartEndType::Flower};
    AEVec2 startendScale_{50.0f, 50.0f};

    s32 particlesCollected_{0};

    // ==========================================
    // Lifecycle
    // ==========================================
    void initialize();
    void initializeUI(s8 font);
    void free();

    // ==========================================
    // Setup
    // ==========================================
    void setupPoint(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type,
                    GoalDirection direction);
    void spawnAtMousePos(StartEndType type, GoalDirection direction);
    void deleteAtMousePos();

    // ==========================================
    // Simulation
    // ==========================================
    bool collisionCheckWithWater(StartEnd startend, FluidParticle particle);

    void update(f32 dt, std::vector<FluidParticle>& particlePool, VFXSystem& vfxSystem);

    // ==========================================
    // Rendering
    // ==========================================
    void drawColor();
    void drawWaterIndicator(const StartEnd& startPoint, const AEVec2& screenPos);
    void drawTexture(s32 particleMaxCount);
    void drawPreview(StartEndType type);

    // ==========================================
    // Input
    // ==========================================
    void checkMouseClick();
    void resetIframe();

    // ==========================================
    // Win Condition
    // ==========================================
    bool checkWinCondition(s32 particleMaxCount) const;

    // ==========================================
    // Water Management
    // ==========================================
    float getWaterRemaining(int startPointIndex) const;
    void setWaterRemaining(int startPointIndex, float amount);
    void refillAllWater();
    void toggleInfiniteWater();
};