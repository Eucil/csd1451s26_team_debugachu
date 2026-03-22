#pragma once

#include <vector>

#include <AEEngine.h>

#include "Components.h"
#include "FluidSystem.h"

enum class StartEndType { Pipe, Flower, Count };

enum class GoalDirection { Up, Down, Left, Right };

struct StartEnd {

    Transform transform_;

    Collider2D collider_;

    StartEndType type_;

    GoalDirection direction_;

    bool releaseWater_{false};
    bool releaseWaterIframe_{false};

    bool active_{true};

    // tc added start
    float waterCapacity_{100.0f};  // Total water available for this start point
    float waterRemaining_{100.0f}; // Water remaining
    bool infiniteWater_{false};    // Debug/infinite mode
    // tc added end

    StartEnd();
    StartEnd(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type, GoalDirection direction);
};

class StartEndPoint {
private:
    AEGfxVertexList* rectMesh_ = nullptr;

    // graphic configs for each StartEnd type
    Graphics graphicsConfigs_[static_cast<int>(StartEndType::Count)];

    // tc added start
    //  Water indicator UI
    AEGfxVertexList* barMesh_{nullptr};
    s8 font_;
    // tc added end

public:
    // Can have multiple start points but only one end point
    std::vector<StartEnd> startPoints_;
    StartEnd endPoint_ = {};
    AEVec2 startendScale_ = {50.0f, 50.0f};

    s32 particlesCollected_{0};

    // TODO
    // Shift variables to private later on if needed
    // Make use of direction to determine where the water should come out of
    // Use draw texture instead

    void Initialize();

    // tc added start
    void InitializeUI(s8 font);
    // tc added end

    void SetupPoint(AEVec2 pos, AEVec2 scale, f32 rotation, StartEndType type,
                    GoalDirection direction);

    void SpawnAtMousePos(StartEndType type, GoalDirection direction);

    void DeleteAtMousePos();

    bool CollisionCheckWithWater(StartEnd startend, FluidParticle particle);

    void Update(f32 dt, std::vector<FluidParticle>& particlePool);

    void DrawColor();

    // tc added start
    void DrawWaterIndicator(const StartEnd& startPoint, const AEVec2& screenPos);
    // tc added end

    void DrawTexture();

    void DrawColorPreview(StartEndType type);

    void Free();

    void CheckMouseClick();

    void ResetIframe();

    bool CheckWinCondition(s32 particleMaxCount);

    // tc added start
    //  getter/setter for water
    float GetWaterRemaining(int startPointIndex) const;
    void SetWaterRemaining(int startPointIndex, float amount);
    void RefillAllWater();
    void ToggleInfiniteWater();
    // tc added end
};