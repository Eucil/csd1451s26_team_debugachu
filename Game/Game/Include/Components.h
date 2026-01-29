#pragma once

#include "AEEngine.h"

struct Transform {
    AEVec2 pos_{0.0f, 0.0f};
    AEVec2 scale_{1.0f, 1.0f};
    f32 rotationRad_{0.0f};
    AEMtx33 worldMtx_{};
};

struct Graphics {
    AEGfxVertexList* mesh_{nullptr};
    AEGfxTexture* texture_{nullptr};
    u32 layer_{0};
};

struct RigidBody2D {
    f32 mass_{1.0f};
    AEVec2 velocity_{0.0f, 0.0f};
    AEVec2 acceleration_{0.0f, 0.0f};
    AEVec2 forces_{0.0f, 0.0f};
    f32 gravityScale_{0.0f};
};

enum class ColliderShape { Empty, Circle, Box, Triangle };

struct CircleColliderData {
    AEVec2 offset_;
    f32 radius;
};

struct BoxColliderData {
    AEVec2 offset_;
    AEVec2 size_;
};

struct TriangleColliderData {
    AEVec2 vertices_[3];
};

struct Collider2D {
    ColliderShape colliderShape_{ColliderShape::Empty};
    union {
        CircleColliderData circle_;
        BoxColliderData box_;
        TriangleColliderData triangle_;
    } shapeData_{};
};
