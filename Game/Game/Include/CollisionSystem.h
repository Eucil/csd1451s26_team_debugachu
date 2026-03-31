/*!
@file       CollisionSystem.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <AEEngine.h>

#include "Components.h"
#include "FluidSystem.h"
#include "Terrain.h"

struct CollisionInfo {
    bool hasCollision_ = false;
    AEVec2 normal_ = {0.0f, 1.0f};
    f32 penetration_ = 0.0f;
};

class CollisionSystem {
public:
    static void terrainToFluidCollision(Terrain& terrain, FluidSystem& fluidSystem, f32 dt = {});

private:
    using BucketEntry = std::pair<FluidType, u32>;

    // -----------------------------
    // Minimal vector helpers (avoid AE const-pointer issues)
    // -----------------------------
    static AEVec2 vAdd(const AEVec2& a, const AEVec2& b) { return AEVec2{a.x + b.x, a.y + b.y}; }
    static AEVec2 vSub(const AEVec2& a, const AEVec2& b) { return AEVec2{a.x - b.x, a.y - b.y}; }
    static AEVec2 vMul(const AEVec2& a, f32 s) { return AEVec2{a.x * s, a.y * s}; }
    static f32 vDot(const AEVec2& a, const AEVec2& b) { return a.x * b.x + a.y * b.y; }
    static f32 vLenSq(const AEVec2& v) { return vDot(v, v); }

    static AEVec2 vNormalizeOr(const AEVec2& v, const AEVec2& fallback);

    static AEVec2 localToWorldPoint(const AEVec2& local, const Transform& t);

    static AEVec2 closestPointOnSegment(const AEVec2& a, const AEVec2& b, const AEVec2& p);

    // Point in triangle (barycentric)
    static bool pointInTriangle(const AEVec2& p, const AEVec2& a, const AEVec2& b, const AEVec2& c);

    // Helper function (terrainToFluidCollision):
    // Generates a CollisionContact struct containing information about the collision (normal,
    // penetration).
    static CollisionInfo cellToFluidParticleCollision(const Cell& cell,
                                                      const FluidParticle& fluidParticle);

    // Helper function (cellToFluidParticleCollision): circle vs AABB (axis-aligned box) in world
    static bool detectCircleVsAABB(const AEVec2& circleCenter, f32 radius, const AEVec2& velocity,
                                   const AEVec2& boxCenter, const AEVec2& halfExt,
                                   AEVec2& outNormal, f32& outPenetration);

    // Helper function (cellToFluidParticleCollision): circle vs Triangle in world
    static bool detectCircleVsTriangle(const AEVec2& circleCenter, f32 radius,
                                       const AEVec2& velocity, const AEVec2& v0, const AEVec2& v1,
                                       const AEVec2& v2, AEVec2& outNormal, f32& outPenetration);

    // Collision Resolution function.
    static void pushOutAndSlide(FluidParticle& p, const AEVec2& n, f32 penetration, f32 radius,
                                f32 dt);

    static void resolveFluidParticlePair(FluidParticle& p1, FluidParticle& p2);

    static void buildGrid(std::vector<std::vector<BucketEntry>>& fluidGrid,
                          FluidSystem& fluidSystem, const AEVec2& gridBottomLeftPos, u32 gridCols,
                          u32 gridRows, u32 gridSize);
};
