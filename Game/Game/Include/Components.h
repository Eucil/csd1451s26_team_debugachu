#pragma once

#include "AEEngine.h"

struct Transform {
    AEVec2 pos_{0.0f, 0.0f};
    AEVec2 scale_{1.0f, 1.0f};
    f32 rotationRad_{0.0f};
    AEMtx33 worldMtx_{};
    f32 radius_{0.0f};
};

struct Graphics {
    AEGfxVertexList* mesh_{nullptr};
    AEGfxTexture* texture_{nullptr};
    u32 layer_{0};
};

struct RigidBody2D {
    f32 mass_{1.0f}; 
    f32 gravity_{-1000.0f}; 
    AEVec2 velocity_{0.0f, 0.0f}; 
    // AEVec2 acceleration_{0.0f, 0.0f};
    // AEVec2 forces_{0.0f, 0.0f};
};

enum class ColliderShape { Circle, Box };

struct Collider2D {
    ColliderShape colliderShape_{ColliderShape::Circle};
    AEVec2 offset_{0.0f, 0.0f};
    f32 radius_{1.0f};        // for circle
    AEVec2 size_{1.0f, 1.0f}; // for box
};
