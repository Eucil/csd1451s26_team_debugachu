#include "CollisionSystem.h"

void CollisionSystem::terrainToFluidCollision(Terrain& terrain, FluidSystem& fluidSystem) {
    using BucketEntry = std::pair<FluidType, u32>; // (type, index)

    // Grid info
    const u32 gridRows = terrain.getCellRows();
    const u32 gridCols = terrain.getCellCols();
    const u32 gridSize = terrain.getCellSize();
    const AEVec2 gridBottomLeftPos = terrain.getBottomLeftPos();

    // One bucket per cell (flattened)
    // Clear and resize grid
    std::vector<std::vector<BucketEntry>> fluidGrid;
    fluidGrid.resize(static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols));

    // Loop each particle type and POPULATE the grid
    for (u32 t = 0; t < static_cast<u32>(FluidType::Count); ++t) {
        std::vector<FluidParticle>& particlePool =
            fluidSystem.GetParticlePool(static_cast<FluidType>(t));

        // Loop through every particle within this pool
        for (size_t pIdx = 0; pIdx < particlePool.size(); ++pIdx) {
            FluidParticle& particle = particlePool[pIdx];

            AEVec2 particlePos = particle.transform_.pos_;

            //
            s32 particleCellX = static_cast<s32>(
                std::floor((particlePos.x - gridBottomLeftPos.x) / static_cast<f32>(gridSize)));
            s32 particleCellY = static_cast<s32>(
                std::floor((particlePos.y - gridBottomLeftPos.y) / static_cast<f32>(gridSize)));

            if (particleCellX < 0 || particleCellX >= static_cast<int>(gridCols) ||
                particleCellY < 0 || particleCellY >= static_cast<int>(gridRows)) {
                continue;
            }

            // Calculate the cell index of the grid (y * maxX + x)
            const size_t cellIndex =
                static_cast<size_t>(particleCellY) * static_cast<size_t>(gridCols) +
                static_cast<size_t>(particleCellX);

            // Save the index of the current particle and its type into the grid
            fluidGrid[cellIndex].emplace_back(static_cast<FluidType>(t), static_cast<u32>(pIdx));
        }
    }

    // Loop over every bucket
    const size_t totalCells = static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols);

    for (size_t cell = 0; cell < totalCells; ++cell) {

        if (fluidGrid[cell].empty())
            continue;

        // calculate the column number
        const u32 cx = static_cast<u32>(cell % gridCols);
        // calculate the row number
        const u32 cy = static_cast<u32>(cell / gridCols);

        // Check 3x3 neighborhood
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = static_cast<int>(cx) + dx;
                const int ny = static_cast<int>(cy) + dy;

                if (nx < 0 || nx >= static_cast<int>(gridCols) || ny < 0 ||
                    ny >= static_cast<int>(gridRows)) {
                    continue;
                }
                const size_t neighbourIndex =
                    static_cast<size_t>(ny) * static_cast<size_t>(gridCols) +
                    static_cast<size_t>(nx);

                Cell& neighbourTerrainCell = terrain.getCells()[neighbourIndex];
                std::vector<BucketEntry>& neighbourParticles = fluidGrid[neighbourIndex];

                for (const BucketEntry& a : fluidGrid[cell]) {
                    const auto currentFluidTypeA = a.first;
                    const auto currentFluidIndexA = a.second;

                    auto& particlePoolA =
                        fluidSystem.GetParticlePool(static_cast<FluidType>(currentFluidTypeA));

                    // obtain our fluidparticle within this cell of the grid
                    FluidParticle& fluidParticleA = particlePoolA[currentFluidIndexA];

                    cellToFluidParticleCollision(neighbourTerrainCell, fluidParticleA);
                    if (!neighbourParticles.empty()) {
                        for (const BucketEntry& b : neighbourParticles) {
                            const auto currentFluidTypeB = b.first;
                            const auto currentFluidIndexB = b.second;
                            auto& particlePoolB = fluidSystem.GetParticlePool(
                                static_cast<FluidType>(currentFluidTypeB));
                            FluidParticle& fluidParticleB = particlePoolB[b.second];

                            // Comparing memory addresses ensures we only solve Pair(A, B) once.
                            // If we didn't do this, we would solve it when A visits B,
                            // and when B visits A.
                            if (&fluidParticleA < &fluidParticleB) {
                                resolveFluidParticlePair(fluidParticleA, fluidParticleB);
                            }
                        }
                    }
                }
            }
        }
    }
}
AEVec2 CollisionSystem::vNormalizeOr(const AEVec2& v, const AEVec2& fallback) {
    const f32 l2 = vLenSq(v);
    if (l2 <= 1e-8f)
        return fallback;
    const f32 inv = 1.0f / std::sqrt(l2);
    return AEVec2{v.x * inv, v.y * inv};
}

// Apply terrain cell transform to a local point (scale -> rotate -> translate)
AEVec2 CollisionSystem::localToWorldPoint(const AEVec2& local, const Transform& t) {
    // scale
    AEVec2 s{local.x * t.scale_.x, local.y * t.scale_.y};

    // rotate
    const f32 c = std::cos(t.rotationRad_);
    const f32 sn = std::sin(t.rotationRad_);
    AEVec2 r{s.x * c - s.y * sn, s.x * sn + s.y * c};

    // translate
    return AEVec2{r.x + t.pos_.x, r.y + t.pos_.y};
}

// Closest point on a segment AB to point P
AEVec2 CollisionSystem::closestPointOnSegment(const AEVec2& a, const AEVec2& b, const AEVec2& p) {
    const AEVec2 ab = vSub(b, a);
    const f32 abLenSq = vLenSq(ab);
    if (abLenSq <= 1e-8f)
        return a;

    const AEVec2 ap = vSub(p, a);
    f32 t = vDot(ap, ab) / abLenSq;
    t = (std::max)(0.0f, (std::min)(1.0f, t));
    return vAdd(a, vMul(ab, t));
}

// Narrow phase: circle vs triangle in world (closest point on triangle)
bool CollisionSystem::pointInTriangle(const AEVec2& p, const AEVec2& a, const AEVec2& b,
                                      const AEVec2& c) {
    const AEVec2 v0 = vSub(b, a);
    const AEVec2 v1 = vSub(c, a);
    const AEVec2 v2 = vSub(p, a);

    const f32 d00 = vDot(v0, v0);
    const f32 d01 = vDot(v0, v1);
    const f32 d11 = vDot(v1, v1);
    const f32 d20 = vDot(v2, v0);
    const f32 d21 = vDot(v2, v1);

    const f32 denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < 1e-8f)
        return false;

    const f32 v = (d11 * d20 - d01 * d21) / denom;
    const f32 w = (d00 * d21 - d01 * d20) / denom;
    const f32 u = 1.0f - v - w;

    return (u >= 0.0f && v >= 0.0f && w >= 0.0f);
}

// Narrow phase: circle vs AABB (axis-aligned box) in world
bool CollisionSystem::resolveCircleVsAABB(const AEVec2& circleCenter, f32 radius,
                                          const AEVec2& boxCenter, const AEVec2& halfExt,
                                          AEVec2& outNormal, f32& outPenetration) {
    // Closest point on AABB to circle center
    AEVec2 closest;
    closest.x =
        (std::max)(boxCenter.x - halfExt.x, (std::min)(circleCenter.x, boxCenter.x + halfExt.x));
    closest.y =
        (std::max)(boxCenter.y - halfExt.y, (std::min)(circleCenter.y, boxCenter.y + halfExt.y));

    const AEVec2 d = vSub(circleCenter, closest);
    const f32 distSq = vLenSq(d);
    if (distSq > radius * radius)
        return false;

    const f32 dist = std::sqrt((std::max)(distSq, 1e-8f));
    outNormal = AEVec2{d.x / dist, d.y / dist};
    outPenetration = radius - dist;
    return true;
}

// Narrow phase: circle vs triangle in world (closest point on triangle)
bool CollisionSystem::resolveCircleVsTriangle(const AEVec2& circleCenter, f32 radius,
                                              const AEVec2& v0, const AEVec2& v1, const AEVec2& v2,
                                              AEVec2& outNormal, f32& outPenetration) {
    AEVec2 closest = circleCenter;

    if (!pointInTriangle(circleCenter, v0, v1, v2)) {
        const AEVec2 c0 = closestPointOnSegment(v0, v1, circleCenter);
        const AEVec2 c1 = closestPointOnSegment(v1, v2, circleCenter);
        const AEVec2 c2 = closestPointOnSegment(v2, v0, circleCenter);

        const f32 d0 = vLenSq(vSub(circleCenter, c0));
        const f32 d1 = vLenSq(vSub(circleCenter, c1));
        const f32 d2 = vLenSq(vSub(circleCenter, c2));

        closest = (d0 < d1) ? ((d0 < d2) ? c0 : c2) : ((d1 < d2) ? c1 : c2);
    }

    const AEVec2 d = vSub(circleCenter, closest);
    const f32 distSq = vLenSq(d);
    if (distSq > radius * radius)
        return false;

    const f32 dist = std::sqrt((std::max)(distSq, 1e-8f));
    outNormal = AEVec2{d.x / dist, d.y / dist};
    outPenetration = radius - dist;
    return true;
}

// Response: push out + slide (prevents teleporting)
void CollisionSystem::pushOutAndSlide(FluidParticle& p, const AEVec2& n, f32 penetration,
                                      f32 radius) {

    // Push out of the surface (position correction)
    const f32 slop = 0.10f;
    f32 push = (std::max)(0.0f, penetration + slop);

    // Prevent large corrections that look like teleporting
    const f32 maxPush = radius * 0.5f; // tune 0.1..0.5
    push = (std::min)(push, maxPush);

    p.transform_.pos_.x += n.x * push;
    p.transform_.pos_.y += n.y * push;

    // Slide: remove velocity component into the surface
    const f32 vn = p.physics_.velocity_.x * n.x + p.physics_.velocity_.y * n.y;
    if (vn < 0.0f) {
        p.physics_.velocity_.x -= vn * n.x;
        p.physics_.velocity_.y -= vn * n.y;

        // mild damping so it settles
        // Generates a random value between -0.15f and 0.15f
        f32 noise = ((rand() % 100) * 0.001f) - 0.04f;
        noise = std::abs(noise);
        p.physics_.velocity_.x *= 0.95f + noise;
        p.physics_.velocity_.y *= 0.95f + noise;
    }
}

void CollisionSystem::cellToFluidParticleCollision(const Cell& cell, FluidParticle& fluidParticle) {
    // Circle center in world
    const AEVec2 circleCenter =
        vAdd(fluidParticle.transform_.pos_, fluidParticle.collider_.shapeData_.circle_.offset_);

    // Circle radius in world units (same rule you used)
    // f32 circleScale = (std::max)(std::abs(fluidParticle.transform_.scale_.x),
    //                             std::abs(fluidParticle.transform_.scale_.y));
    // if (circleScale < 1e-8f)
    //    circleScale = 1.0f;
    // const f32 radius = fluidParticle.collider_.shapeData_.circle_.radius * circleScale;
    const f32 radius = fluidParticle.collider_.shapeData_.circle_.radius;

    // Only resolve ONE collision per call to prevent stacking (teleport)
    for (u32 i = 0; i < 3; ++i) {
        const Collider2D& col = cell.colliders_[i];
        if (col.colliderShape_ == ColliderShape::Empty)
            continue;

        AEVec2 n{0.0f, 1.0f};
        f32 penetration = 0.0f;
        bool hit = false;

        if (col.colliderShape_ == ColliderShape::Box) {
            // IMPORTANT: collider library is in CELL-LOCAL units.
            // Convert offset and size into WORLD using the cell scale.
            const AEVec2 offsetWorld{col.shapeData_.box_.offset_.x * cell.transform_.scale_.x,
                                     col.shapeData_.box_.offset_.y * cell.transform_.scale_.y};
            const AEVec2 boxCenter = vAdd(cell.transform_.pos_, offsetWorld);

            const AEVec2 sizeWorld{col.shapeData_.box_.size_.x * cell.transform_.scale_.x,
                                   col.shapeData_.box_.size_.y * cell.transform_.scale_.y};
            const AEVec2 halfExt{sizeWorld.x * 0.5f, sizeWorld.y * 0.5f};

            hit = resolveCircleVsAABB(circleCenter, radius, boxCenter, halfExt, n, penetration);
        } else if (col.colliderShape_ == ColliderShape::Triangle) {
            // Convert triangle vertices to WORLD using cell transform
            const AEVec2 v0 =
                localToWorldPoint(col.shapeData_.triangle_.vertices_[0], cell.transform_);
            const AEVec2 v1 =
                localToWorldPoint(col.shapeData_.triangle_.vertices_[1], cell.transform_);
            const AEVec2 v2 =
                localToWorldPoint(col.shapeData_.triangle_.vertices_[2], cell.transform_);

            hit = resolveCircleVsTriangle(circleCenter, radius, v0, v1, v2, n, penetration);
        }

        if (hit) {
            n = vNormalizeOr(n, AEVec2{0.0f, 1.0f});
            pushOutAndSlide(fluidParticle, n, penetration, radius);
            return; // stop after first hit (prevents teleport)
        }
    }
}

void CollisionSystem::resolveFluidParticlePair(FluidParticle& p1, FluidParticle& p2) {

    // Calculate distance
    f32 dx = p1.transform_.pos_.x - p2.transform_.pos_.x;
    f32 dy = p1.transform_.pos_.y - p2.transform_.pos_.y;

    // Jitter fix for vertical stacking of particles
    if (std::abs(dx) < 0.001f) {
        // Generate a tiny random float between - 0.05 and 0.05
        f32 noise = ((rand() % 100) * 0.001f) - 0.05f;
        dx += noise;
    }

    // Calculate minimum distance between p1 and p2 for collision to occur
    f32 minDist = p1.collider_.shapeData_.circle_.radius + p2.collider_.shapeData_.circle_.radius;
    f32 distSq = dx * dx + dy * dy;

    // Check collision
    if (distSq < minDist * minDist) {
        // std::max prevents division by zero if dist is extremely small
        f32 dist = std::sqrt((std::max)(distSq, 0.0001f));

        // Normal direction unit vector
        // Any calculations involving moving posX and posY should be multiplied by nx and ny
        f32 nx = dx / dist;
        f32 ny = dy / dist;

        f32 overlap = minDist - dist;

        // --- Repulsion ---
        // Resolve the collision by pushing them apart based on the amount of overlap
        // calculated above
        //
        // ADJUST the repulsion value to change how strongly they repel each other
        // (original: 0.5f)
        //
        // If overlap is small, use a lower repulsion value (0.2f).
        f32 repulsion = (overlap < minDist * 0.1f) ? 0.2f : 0.5f;

        f32 moveX = nx * (overlap * repulsion);
        f32 moveY = ny * (overlap * repulsion);

        // Apply the calculated movement to both particles directly to transform_.pos
        p1.transform_.pos_.x += moveX;
        p1.transform_.pos_.y += moveY;
        p2.transform_.pos_.x -= moveX;
        p2.transform_.pos_.y -= moveY;

        // --- Velocity Impulse ---
        // Upon collision, we should calculate the relative difference in velocity
        // between the two particles For example, if relativeVX > 0, it means p1 is
        // moving faster than p2 in the x axis
        f32 relativeVX = p1.physics_.velocity_.x - p2.physics_.velocity_.x;
        f32 relativeVY = p1.physics_.velocity_.y - p2.physics_.velocity_.y;
        f32 velAlongNormal = relativeVX * nx + relativeVY * ny;

        if (velAlongNormal < 0.0f) {

            // Restitution determines the "Bounciness" of the collision.
            // It calculates how much relative velocity is preserved or lost after impact.
            //
            // -1.0f  : Perfectly Inelastic
            // -2.0f  : Perfectly Elastic
            f32 restitution = -1.08f; // Bounciness
            f32 j = restitution * velAlongNormal * 0.5f;

            p1.physics_.velocity_.x += j * nx;
            p1.physics_.velocity_.y += j * ny;
            p2.physics_.velocity_.x -= j * nx;
            p2.physics_.velocity_.y -= j * ny;
        }
    }
}