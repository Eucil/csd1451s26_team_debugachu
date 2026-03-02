#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <AEEngine.h>

#include "Components.h"
#include "FluidSystem.h"
#include "Terrain.h"

class CollisionSystem {
public:
    static void terrainToFluidCollision(Terrain& terrain, FluidSystem& fluidSystem);

private:
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

    // Narrow phase: circle vs AABB (axis-aligned box) in world
    static bool resolveCircleVsAABB(const AEVec2& circleCenter, f32 radius, const AEVec2& boxCenter,
                                    const AEVec2& halfExt, AEVec2& outNormal, f32& outPenetration);

    // Narrow phase: circle vs triangle in world (closest point on triangle)
    static bool resolveCircleVsTriangle(const AEVec2& circleCenter, f32 radius, const AEVec2& v0,
                                        const AEVec2& v1, const AEVec2& v2, AEVec2& outNormal,
                                        f32& outPenetration);

    // Response: push out + slide (prevents teleporting)
    static void pushOutAndSlide(FluidParticle& p, const AEVec2& n, f32 penetration, f32 radius);

    static void cellToFluidParticleCollision(const Cell& cell, FluidParticle& fluidParticle);

    static void resolveFluidParticlePair(FluidParticle& p1, FluidParticle& p2);
};