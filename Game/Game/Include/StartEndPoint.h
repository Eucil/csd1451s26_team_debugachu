#pragma once
#include "AEEngine.h"
#include "Components.h"
#include "FluidSystem.h"
#include <vector>

enum class StartEndType { Pipe, Flower, Count };

enum class GoalDirection { Up, Down, Left, Right };

struct StartEnd {

    Transform transform_;

    Collider2D collider_;

    StartEndType type_;

    GoalDirection direction_;

    bool release_water_{false};
    bool release_water_iframe_{false};

    StartEnd();
    StartEnd(AEVec2 pos, AEVec2 scale, StartEndType type, GoalDirection direction);
};

class StartEndPoint {
private:
    AEGfxVertexList* rectMesh = nullptr;

    // graphic configs for each StartEnd type
    Graphics graphicsConfigs_[static_cast<int>(StartEndType::Count)];

public:
    // Can have multiple start points but only one end point
    std::vector<StartEnd> startPoints_;
    StartEnd endPoint_ = {};

    s32 particlesCollected_{0};

    void Initialize();

    void SetupStartPoint(AEVec2 pos, AEVec2 scale, StartEndType type, GoalDirection direction);

    void SetupEndPoint(AEVec2 pos, AEVec2 scale, StartEndType type, GoalDirection direction);

    bool CollisionCheckWithWater(StartEnd startend, FluidParticle particle);

    void Update(f32 dt, std::vector<FluidParticle>& particlePool);

    void DrawColor();

    void DrawTexture();

    void Free();

    void CheckMouseClick();

    void ResetIframe();

    bool CheckWinCondition(s32 particle_max_count);
};
