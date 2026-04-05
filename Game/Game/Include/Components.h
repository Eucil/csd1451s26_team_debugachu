/*!
@file       Components.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Chia Hanxin/c.hanxin@digipen.edu

@date		March, 31, 2026

@brief      This header file defines the core component structs used across
            game objects: Transform, Graphics, RigidBody2D, and Collider2D.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Third-party
#include <AEEngine.h>

// Stores position, scale, rotation, and the computed world matrix.
struct Transform {
    AEVec2 pos_{0.0f, 0.0f};
    AEVec2 scale_{1.0f, 1.0f};
    f32 rotationRad_{0.0f};
    AEMtx33 worldMtx_{};
};

// Stores mesh, texture, draw layer, and RGBA tint for rendering.
struct Graphics {
    AEGfxVertexList* mesh_{nullptr};
    AEGfxTexture* texture_{nullptr};
    u32 layer_{0};
    f32 red_{1.0f};
    f32 green_{1.0f};
    f32 blue_{1.0f};
    f32 alpha_{1.0f};
};

// Stores mass, gravity scale, and velocity for physics simulation.
struct RigidBody2D {
    f32 mass_{1.0f};
    f32 gravity_{0.0f};
    AEVec2 velocity_{0.0f, 0.0f};
    // AEVec2 acceleration_{0.0f, 0.0f};
    // AEVec2 forces_{0.0f, 0.0f};
};

// Identifies which collider shape variant is active in Collider2D.
enum class ColliderShape { Empty, Circle, Box, Triangle };

// Collider data for a circle: offset from the object's position and radius.
struct CircleColliderData {
    AEVec2 offset_;
    f32 radius_;
};

// Collider data for an axis-aligned box: offset and half-extents size.
struct BoxColliderData {
    AEVec2 offset_;
    AEVec2 size_;
};

// Collider data for a triangle: three vertices in local space.
struct TriangleColliderData {
    AEVec2 vertices_[3];
};

// Holds a collider shape tag and the corresponding shape data as a union.
struct Collider2D {
    ColliderShape colliderShape_{ColliderShape::Empty};
    union {
        CircleColliderData circle_;
        BoxColliderData box_;
        TriangleColliderData triangle_;
    } shapeData_{};
};
