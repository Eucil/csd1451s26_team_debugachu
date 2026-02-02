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
    static void terrainToFluidCollision(Terrain& terrain, FluidSystem& fluidSystem) {
        using BucketEntry = std::pair<FluidType, u32>; // (type, index)

        // Grid info
        const u32 gridRows = terrain.getCellRows();
        const u32 gridCols = terrain.getCellCols();
        const u32 gridSize = terrain.getCellSize();
        const AEVec2 gridBottomLeftPos = terrain.getBottomLeftPos();

        // One bucket per cell (flattened)
        std::vector<std::vector<BucketEntry>> fluidGrid;
        fluidGrid.resize(static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols));

        // Loop each particle type
        for (u32 t = 0; t < static_cast<u32>(FluidType::Count); ++t) {
            std::vector<FluidParticle>& particlePool =
                fluidSystem.GetParticlePool(static_cast<FluidType>(t));

            // Loop through each particle for the current particle type
            for (size_t pIdx = 0; pIdx < particlePool.size(); ++pIdx) {
                FluidParticle& particle = particlePool[pIdx];

                // Read particle position
                AEVec2 particlePos = particle.transform_.pos_;

                // Convert world position to cell coordinates
                s32 particleCellX = static_cast<s32>(
                    std::floor((particlePos.x - gridBottomLeftPos.x) / static_cast<f32>(gridSize)));
                s32 particleCellY = static_cast<s32>(
                    std::floor((particlePos.y - gridBottomLeftPos.y) / static_cast<f32>(gridSize)));

                // Skip particles outside the grid (or clamp if desired)
                if (particleCellX < 0 || particleCellX >= static_cast<int>(gridCols) ||
                    particleCellY < 0 || particleCellY >= static_cast<int>(gridRows)) {
                    continue;
                }

                // Flattened index: row-major (cy * cols + cx)
                const size_t cellIndex =
                    static_cast<size_t>(particleCellY) * static_cast<size_t>(gridCols) +
                    static_cast<size_t>(particleCellX);

                // Push the pair (type, index)
                fluidGrid[cellIndex].emplace_back(static_cast<FluidType>(t),
                                                  static_cast<u32>(pIdx));
            }
        }

        // Loop over every bucket (flattened)
        const size_t totalCells = static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols);
        for (size_t cell = 0; cell < totalCells; ++cell) {
            // Convert flattened index back to 2D coords
            const u32 cx = static_cast<u32>(cell % gridCols);
            const u32 cy = static_cast<u32>(cell / gridCols);

            // Iterate over the 3x3 neighborhood centered on (cx, cy)
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = static_cast<int>(cx) + dx;
                    const int ny = static_cast<int>(cy) + dy;

                    // Skip out-of-bounds neighbours
                    if (nx < 0 || nx >= static_cast<int>(gridCols) || ny < 0 ||
                        ny >= static_cast<int>(gridRows)) {
                        continue;
                    }

                    const size_t neighbourIndex =
                        static_cast<size_t>(ny) * static_cast<size_t>(gridCols) +
                        static_cast<size_t>(nx);

                    // Get the neighbouring terrain cell
                    Cell& neighbourTerrainCell = terrain.getCells()[neighbourIndex];

                    // Now loop through each particle in the current bucket
                    for (const BucketEntry& a : fluidGrid[cell]) {
                        auto currentFluidType = a.first;
                        auto currentFluidIndex = a.second;

                        auto& particlePool =
                            fluidSystem.GetParticlePool(static_cast<FluidType>(currentFluidType));
                        FluidParticle& fluidParticle = particlePool[currentFluidIndex];

                        // Resolve particle collision with cell
                        cellToFluidParticleCollision(neighbourTerrainCell, fluidParticle);
                    }
                }
            }
        }
    }

private:
    static bool circleToBoxIntersect(Collider2D circleCollider2D, Transform circleTransform,
                                     Collider2D boxCollider2D, Transform boxTransform) {
        // Ensure correct shapes
        if (circleCollider2D.colliderShape_ != ColliderShape::Circle ||
            boxCollider2D.colliderShape_ != ColliderShape::Box) {
            return false;
        }

        // World-space circle center = transform.pos + circle offset
        AEVec2 circleWorldPos{0.0f, 0.0f};
        AEVec2Add(&circleWorldPos, &circleTransform.pos_,
                  &circleCollider2D.shapeData_.circle_.offset_);

        // World-space box center = transform.pos + box offset
        AEVec2 boxWorldPos{0.0f, 0.0f};
        AEVec2Add(&boxWorldPos, &boxTransform.pos_, &boxCollider2D.shapeData_.box_.offset_);

        // Circle radius in world units (conservative: use max scale component)
        f32 circleScale =
            (std::max)(std::abs(circleTransform.scale_.x), std::abs(circleTransform.scale_.y));
        if (circleScale < 1e-8f)
            circleScale = 1.0f;
        f32 circleRadius = circleCollider2D.shapeData_.circle_.radius * circleScale;

        // Box size is already in world units after transform scaling
        AEVec2 boxSizeWorld = boxCollider2D.shapeData_.box_.size_;

        // AABB min/max
        AEVec2 halfExt{boxSizeWorld.x * 0.5f, boxSizeWorld.y * 0.5f};
        AEVec2 boxMin{boxWorldPos.x - halfExt.x, boxWorldPos.y - halfExt.y};
        AEVec2 boxMax{boxWorldPos.x + halfExt.x, boxWorldPos.y + halfExt.y};

        // Closest point on AABB to circle center (clamp)
        AEVec2 closest;
        closest.x = (std::max)(boxMin.x, (std::min)(circleWorldPos.x, boxMax.x));
        closest.y = (std::max)(boxMin.y, (std::min)(circleWorldPos.y, boxMax.y));

        // Distance squared test
        f32 dx = circleWorldPos.x - closest.x;
        f32 dy = circleWorldPos.y - closest.y;
        return (dx * dx + dy * dy) <= (circleRadius * circleRadius);
    }

    // Returns true if circle intersects triangle. Assumes triangle vertices are in triangle local
    // space.
    static bool circleToTriangleIntersect(Collider2D circleCollider2D, Transform circleTransform,
                                          Collider2D triangleCollider2D,
                                          Transform triangleTransform) {
        if (circleCollider2D.colliderShape_ != ColliderShape::Circle ||
            triangleCollider2D.colliderShape_ != ColliderShape::Triangle) {
            return false;
        }

        // Helper: apply transform (scale -> rotate -> translate) to a local point
        auto applyTransform = [](const AEVec2& local, const Transform& t) -> AEVec2 {
            // rotate ONLY (NO scaling here)
            const f32 c = std::cos(t.rotationRad_);
            const f32 sn = std::sin(t.rotationRad_);
            AEVec2 r{local.x * c - local.y * sn, local.x * sn + local.y * c};
            // translate
            AEVec2 out{r.x + t.pos_.x, r.y + t.pos_.y};
            return out;
        };

        // Helper: closest point on segment AB to point P
        auto closestPointOnSegment = [](const AEVec2& a, const AEVec2& b,
                                        const AEVec2& p) -> AEVec2 {
            AEVec2 ab{b.x - a.x, b.y - a.y};
            f32 abLenSq = ab.x * ab.x + ab.y * ab.y;
            if (abLenSq <= 0.0f)
                return a;
            AEVec2 ap{p.x - a.x, p.y - a.y};
            f32 t = (ap.x * ab.x + ap.y * ab.y) / abLenSq;
            if (t < 0.0f)
                t = 0.0f;
            else if (t > 1.0f)
                t = 1.0f;
            return AEVec2{a.x + ab.x * t, a.y + ab.y * t};
        };

        // Helper: point-in-triangle using barycentric coordinates
        auto pointInTriangle = [](const AEVec2& p, const AEVec2& a, const AEVec2& b,
                                  const AEVec2& c) -> bool {
            AEVec2 v0{b.x - a.x, b.y - a.y};
            AEVec2 v1{c.x - a.x, c.y - a.y};
            AEVec2 v2{p.x - a.x, p.y - a.y};

            f32 d00 = v0.x * v0.x + v0.y * v0.y;
            f32 d01 = v0.x * v1.x + v0.y * v1.y;
            f32 d11 = v1.x * v1.x + v1.y * v1.y;
            f32 d20 = v2.x * v0.x + v2.y * v0.y;
            f32 d21 = v2.x * v1.x + v2.y * v1.y;

            f32 denom = d00 * d11 - d01 * d01;
            if (std::fabs(denom) < 1e-8f)
                return false; // degenerate
            f32 v = (d11 * d20 - d01 * d21) / denom;
            f32 w = (d00 * d21 - d01 * d20) / denom;
            f32 u = 1.0f - v - w;
            return (u >= 0.0f && v >= 0.0f && w >= 0.0f);
        };

        // Circle center in world space (transform.pos + offset)
        AEVec2 circleWorldPos{0.0f, 0.0f};
        AEVec2Add(&circleWorldPos, &circleTransform.pos_,
                  &circleCollider2D.shapeData_.circle_.offset_);

        // Circle radius in world units (conservative: use max scale)
        f32 circleScale =
            (std::max)(std::abs(circleTransform.scale_.x), std::abs(circleTransform.scale_.y));
        if (circleScale < 1e-8f)
            circleScale = 1.0f;
        f32 circleRadius = circleCollider2D.shapeData_.circle_.radius * circleScale;
        f32 circleRadiusSq = circleRadius * circleRadius;

        // Transform triangle vertices to world space
        AEVec2 v0 =
            applyTransform(triangleCollider2D.shapeData_.triangle_.vertices_[0], triangleTransform);
        AEVec2 v1 =
            applyTransform(triangleCollider2D.shapeData_.triangle_.vertices_[1], triangleTransform);
        AEVec2 v2 =
            applyTransform(triangleCollider2D.shapeData_.triangle_.vertices_[2], triangleTransform);

        // 1) If circle center is inside triangle -> intersect
        if (pointInTriangle(circleWorldPos, v0, v1, v2)) {
            return true;
        }

        // 2) Otherwise check distance to each edge
        AEVec2 cp0 = closestPointOnSegment(v0, v1, circleWorldPos);
        AEVec2 cp1 = closestPointOnSegment(v1, v2, circleWorldPos);
        AEVec2 cp2 = closestPointOnSegment(v2, v0, circleWorldPos);

        AEVec2 d0{circleWorldPos.x - cp0.x, circleWorldPos.y - cp0.y};
        AEVec2 d1{circleWorldPos.x - cp1.x, circleWorldPos.y - cp1.y};
        AEVec2 d2{circleWorldPos.x - cp2.x, circleWorldPos.y - cp2.y};

        f32 distSq0 = d0.x * d0.x + d0.y * d0.y;
        f32 distSq1 = d1.x * d1.x + d1.y * d1.y;
        f32 distSq2 = d2.x * d2.x + d2.y * d2.y;

        f32 minDistSq = (std::min)(distSq0, (std::min)(distSq1, distSq2));
        return minDistSq <= circleRadiusSq;
    }

    static void cellToFluidParticleCollision(Cell cell, FluidParticle& fluidParticle) {
        for (u32 i{0}; i < 3; ++i) {
            if (cell.colliders_[i].colliderShape_ != ColliderShape::Empty) {
                if (cell.colliders_[i].colliderShape_ == ColliderShape::Box) {
                    // Handle particle to box collision
                    if (circleToBoxIntersect(fluidParticle.collider_, fluidParticle.transform_,
                                             cell.colliders_[i], cell.transform_)) {
                        // World-space fluid particle center = transform.pos + circle offset
                        AEVec2 fluidParticleWorldPos{0.0f, 0.0f};
                        AEVec2Add(&fluidParticleWorldPos, &fluidParticle.transform_.pos_,
                                  &fluidParticle.collider_.shapeData_.circle_.offset_);

                        // World-space cell box collider center = transform.pos + box offset
                        AEVec2 cellBoxWorldPos{0.0f, 0.0f};
                        AEVec2Add(&cellBoxWorldPos, &cell.transform_.pos_,
                                  &cell.colliders_[i].shapeData_.box_.offset_);

                        // Calculate a normalized vector from cell's box collider to particle
                        AEVec2 boxToCircle{0.0f, 0.0f};
                        AEVec2Sub(&boxToCircle, &fluidParticleWorldPos, &cellBoxWorldPos);
                        AEVec2Normalize(&boxToCircle, &boxToCircle);

                        // 0.0f, 0.0f edge case
                        if (AEVec2Length(&boxToCircle) == 0.0f) {
                            AEVec2Set(&boxToCircle, 1.0f, 0.0f);
                        }

                        AEVec2Normalize(&boxToCircle, &boxToCircle);

                        // Store the current speed
                        f32 speed{AEVec2Length(&fluidParticle.physics_.velocity_)};

                        // New velocity of the particle
                        AEVec2Scale(&boxToCircle, &boxToCircle, speed * 0.95f);

                        // Change velocity
                        std::cout << "fluid to box -> move\n";
                        fluidParticle.physics_.velocity_ = boxToCircle;
                    }
                } else if (cell.colliders_[i].colliderShape_ == ColliderShape::Triangle) {
                    // Handle particle to triangle collision
                    if (circleToTriangleIntersect(fluidParticle.collider_, fluidParticle.transform_,
                                                  cell.colliders_[i], cell.transform_)) {
                        // World-space fluid particle center = transform.pos + circle offset
                        AEVec2 fluidParticleWorldPos{0.0f, 0.0f};
                        AEVec2Add(&fluidParticleWorldPos, &fluidParticle.transform_.pos_,
                                  &fluidParticle.collider_.shapeData_.circle_.offset_);

                        // World-space cell triangle collider center = transform.pos + triangle
                        // offset
                        AEVec2 cellTriangleWorldPos{0.0f, 0.0f};
                        AEVec2Set(&cellTriangleWorldPos, cell.transform_.pos_.x,
                                  cell.transform_.pos_.y);

                        // Add 1/3 of each of the vertices of the triangle collider
                        for (int i{0}; i < 3; ++i) {
                            AEVec2 vertexPos{cell.colliders_[i].shapeData_.triangle_.vertices_[0]};
                            AEVec2Scale(&vertexPos, &vertexPos, 0.3333f);
                            AEVec2Add(&cellTriangleWorldPos, &cellTriangleWorldPos, &vertexPos);
                        }

                        // Calculate a normalized vector from cell's triangle collider to particle
                        AEVec2 triangleToCircle{0.0f, 0.0f};
                        AEVec2Sub(&triangleToCircle, &fluidParticleWorldPos, &cellTriangleWorldPos);

                        // 0.0f, 0.0f edge case
                        if (AEVec2Length(&triangleToCircle) == 0.0f) {
                            AEVec2Set(&triangleToCircle, 1.0f, 0.0f);
                        }

                        AEVec2Normalize(&triangleToCircle, &triangleToCircle);

                        // Store the current speed
                        f32 speed{AEVec2Length(&fluidParticle.physics_.velocity_)};

                        // New velocity of the particle
                        AEVec2Scale(&triangleToCircle, &triangleToCircle, speed * 0.95f);

                        // Change velocity
                        std::cout << "fluid to box -> move\n";
                        fluidParticle.physics_.velocity_ = triangleToCircle;
                    }
                }
            }
        }
    }
};
