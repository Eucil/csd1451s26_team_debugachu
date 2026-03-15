#include "CollisionSystem.h"
void CollisionSystem::terrainToFluidCollision(Terrain& terrain, FluidSystem& fluidSystem) {
    using BucketEntry = std::pair<FluidType, u32>; // (type, index)

    const u32 gridRows = terrain.getCellRows();
    const u32 gridCols = terrain.getCellCols();
    const u32 gridSize = terrain.getCellSize();
    const AEVec2 gridBottomLeftPos = terrain.getBottomLeftPos();

    // ====================================================================
    // STEP 1: PRE-TERRAIN PASS (Defusing the Gravity Bomb)
    // ====================================================================
    // Gravity just pulled particles into the floor. We MUST push them back
    // to the surface BEFORE they crush together, otherwise they explode horizontally!
    for (u32 t = 0; t < static_cast<u32>(FluidType::Count); ++t) {
        std::vector<FluidParticle>& particlePool =
            fluidSystem.GetParticlePool(static_cast<FluidType>(t));
        for (size_t pIdx = 0; pIdx < particlePool.size(); ++pIdx) {
            FluidParticle& p = particlePool[pIdx];

            s32 cx = static_cast<s32>(std::floor((p.transform_.pos_.x - gridBottomLeftPos.x) /
                                                 static_cast<f32>(gridSize)));
            s32 cy = static_cast<s32>(std::floor((p.transform_.pos_.y - gridBottomLeftPos.y) /
                                                 static_cast<f32>(gridSize)));

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;

                    if (nx < 0 || nx >= static_cast<int>(gridCols) || ny < 0 ||
                        ny >= static_cast<int>(gridRows))
                        continue;

                    size_t neighbourIndex =
                        static_cast<size_t>(ny) * static_cast<size_t>(gridCols) +
                        static_cast<size_t>(nx);
                    Cell& terrainCell = terrain.getCells()[neighbourIndex];

                    CollisionInfo contact = cellToFluidParticleCollision(terrainCell, p);
                    if (contact.hasCollision) {
                        pushOutAndSlide(p, contact.normal, contact.penetration,
                                        p.collider_.shapeData_.circle_.radius);
                    }
                }
            }
        }
    }

    // ====================================================================
    // STEP 2: BUILD FLUID GRID
    // ====================================================================
    // Now that particles are safely on the surface, build the grid for fluid math.
    std::vector<std::vector<BucketEntry>> fluidGrid;
    fluidGrid.resize(static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols));

    for (u32 t = 0; t < static_cast<u32>(FluidType::Count); ++t) {
        std::vector<FluidParticle>& particlePool =
            fluidSystem.GetParticlePool(static_cast<FluidType>(t));
        for (size_t pIdx = 0; pIdx < particlePool.size(); ++pIdx) {
            FluidParticle& particle = particlePool[pIdx];
            AEVec2 particlePos = particle.transform_.pos_;

            s32 particleCellX = static_cast<s32>(
                std::floor((particlePos.x - gridBottomLeftPos.x) / static_cast<f32>(gridSize)));
            s32 particleCellY = static_cast<s32>(
                std::floor((particlePos.y - gridBottomLeftPos.y) / static_cast<f32>(gridSize)));

            if (particleCellX < 0 || particleCellX >= static_cast<int>(gridCols) ||
                particleCellY < 0 || particleCellY >= static_cast<int>(gridRows))
                continue;

            const size_t cellIndex =
                static_cast<size_t>(particleCellY) * static_cast<size_t>(gridCols) +
                static_cast<size_t>(particleCellX);
            fluidGrid[cellIndex].emplace_back(static_cast<FluidType>(t), static_cast<u32>(pIdx));
        }
    }

    const size_t totalCells = static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols);

    // ====================================================================
    // STEP 3: FLUID vs FLUID
    // ====================================================================
    for (size_t cell = 0; cell < totalCells; ++cell) {
        if (fluidGrid[cell].empty())
            continue;

        const u32 cx = static_cast<u32>(cell % gridCols);
        const u32 cy = static_cast<u32>(cell / gridCols);

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = static_cast<int>(cx) + dx;
                const int ny = static_cast<int>(cy) + dy;

                if (nx < 0 || nx >= static_cast<int>(gridCols) || ny < 0 ||
                    ny >= static_cast<int>(gridRows))
                    continue;

                const size_t neighbourIndex =
                    static_cast<size_t>(ny) * static_cast<size_t>(gridCols) +
                    static_cast<size_t>(nx);
                std::vector<BucketEntry>& neighbourParticles = fluidGrid[neighbourIndex];

                if (neighbourParticles.empty())
                    continue;

                for (const BucketEntry& a : fluidGrid[cell]) {
                    FluidParticle& fluidParticleA = fluidSystem.GetParticlePool(a.first)[a.second];
                    for (const BucketEntry& b : neighbourParticles) {
                        FluidParticle& fluidParticleB =
                            fluidSystem.GetParticlePool(b.first)[b.second];
                        if (&fluidParticleA < &fluidParticleB) {
                            resolveFluidParticlePair(fluidParticleA, fluidParticleB);
                        }
                    }
                }
            }
        }
    }

    // ====================================================================
    // STEP 4: POST-TERRAIN PASS (Cleanup)
    // ====================================================================
    // Fluid math in Step 3 might have gently pushed particles into the walls.
    // Run one final cleanup pass to guarantee they are inside the bounds.
    for (u32 t = 0; t < static_cast<u32>(FluidType::Count); ++t) {
        std::vector<FluidParticle>& particlePool =
            fluidSystem.GetParticlePool(static_cast<FluidType>(t));
        for (size_t pIdx = 0; pIdx < particlePool.size(); ++pIdx) {
            FluidParticle& p = particlePool[pIdx];

            s32 cx = static_cast<s32>(std::floor((p.transform_.pos_.x - gridBottomLeftPos.x) /
                                                 static_cast<f32>(gridSize)));
            s32 cy = static_cast<s32>(std::floor((p.transform_.pos_.y - gridBottomLeftPos.y) /
                                                 static_cast<f32>(gridSize)));

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;

                    if (nx < 0 || nx >= static_cast<int>(gridCols) || ny < 0 ||
                        ny >= static_cast<int>(gridRows))
                        continue;

                    size_t neighbourIndex =
                        static_cast<size_t>(ny) * static_cast<size_t>(gridCols) +
                        static_cast<size_t>(nx);
                    Cell& terrainCell = terrain.getCells()[neighbourIndex];

                    CollisionInfo contact = cellToFluidParticleCollision(terrainCell, p);
                    if (contact.hasCollision) {
                        pushOutAndSlide(p, contact.normal, contact.penetration,
                                        p.collider_.shapeData_.circle_.radius);
                    }
                }
            }
        }
    }
}
/*

void CollisionSystem::terrainToFluidCollision(Terrain& terrain, FluidSystem& fluidSystem) {
    using BucketEntry = std::pair<FluidType, u32>; // (type, index)

    // terrainGrid info
    const u32 gridRows = terrain.getCellRows();
    const u32 gridCols = terrain.getCellCols();
    const u32 gridSize = terrain.getCellSize();
    const AEVec2 gridBottomLeftPos = terrain.getBottomLeftPos();

    // Create fluidGrid and resize it according to terrainGrid (only used for PASS 1)
    std::vector<std::vector<BucketEntry>> fluidGrid;
    fluidGrid.resize(static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols));

    // Loop each particle type (e.g. water, lava) and POPULATE the fluidGrid with the indices of
    // which particles are in which
    for (u32 t = 0; t < static_cast<u32>(FluidType::Count); ++t) {

        // Get the particlepool for the currently iterated particle type (e.g. water or mud)
        std::vector<FluidParticle>& particlePool =
            fluidSystem.GetParticlePool(static_cast<FluidType>(t));

        // Loop through every particle within this pool and calculate which cell of the grid it
        // belongs to based on
        for (size_t pIdx = 0; pIdx < particlePool.size(); ++pIdx) {

            // Obtain the currently iterated particle and its position
            FluidParticle& particle = particlePool[pIdx];
            AEVec2 particlePos = particle.transform_.pos_;

            // std::floor calculates the largest integer value not greater than the division result,
            // which gives us the correct cell index for negative positions as well.
            s32 particleCellX = static_cast<s32>(
                std::floor((particlePos.x - gridBottomLeftPos.x) / static_cast<f32>(gridSize)));
            s32 particleCellY = static_cast<s32>(
                std::floor((particlePos.y - gridBottomLeftPos.y) / static_cast<f32>(gridSize)));

            // If particle cell index is out of bounds, we go to the next particle
            if (particleCellX < 0 || particleCellX >= static_cast<int>(gridCols) ||
                particleCellY < 0 || particleCellY >= static_cast<int>(gridRows)) {
                continue;
            }

            // Calculate the cell index of the grid (y * maxX + x) and save the index of the current
            // particle and its type into the fluidGrid
            const size_t cellIndex =
                static_cast<size_t>(particleCellY) * static_cast<size_t>(gridCols) +
                static_cast<size_t>(particleCellX);
            fluidGrid[cellIndex].emplace_back(static_cast<FluidType>(t), static_cast<u32>(pIdx));
        }
    }

    // Obtain the total number of cells in the grid so that we can loop through every cell and
    // resolve collisions
    const size_t totalCells = static_cast<size_t>(gridRows) * static_cast<size_t>(gridCols);

    // ====================================================================
    // PASS 1: FLUID vs FLUID (Soft Constraints)
    // ====================================================================
    // Resolve fluid to fluid particle collisions first
    for (size_t cell = 0; cell < totalCells; ++cell) {
        if (fluidGrid[cell].empty())
            continue;

        const u32 cx = static_cast<u32>(cell % gridCols);
        const u32 cy = static_cast<u32>(cell / gridCols);

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = static_cast<int>(cx) + dx;
                const int ny = static_cast<int>(cy) + dy;

                if (nx < 0 || nx >= static_cast<int>(gridCols) || ny < 0 ||
                    ny >= static_cast<int>(gridRows))
                    continue;

                const size_t neighbourIndex =
                    static_cast<size_t>(ny) * static_cast<size_t>(gridCols) +
                    static_cast<size_t>(nx);
                std::vector<BucketEntry>& neighbourParticles = fluidGrid[neighbourIndex];

                if (neighbourParticles.empty())
                    continue;

                for (const BucketEntry& a : fluidGrid[cell]) {
                    FluidParticle& fluidParticleA = fluidSystem.GetParticlePool(a.first)[a.second];

                    for (const BucketEntry& b : neighbourParticles) {
                        FluidParticle& fluidParticleB =
                            fluidSystem.GetParticlePool(b.first)[b.second];

                        // Compare memory addresses to only resolve Pair(A, B) once
                        if (&fluidParticleA < &fluidParticleB) {
                            resolveFluidParticlePair(fluidParticleA, fluidParticleB);
                        }
                    }
                }
            }
        }
    }
    // Now that fluidparticle to fluidparticle collisions have been resolved, we should not
    // use fluidGrid cell indices anymore as it contains the positions of the old fluidParticle
    // values
    // ====================================================================
    // PASS 2: FLUID vs TERRAIN (Hard Constraints)
    // ====================================================================
    // Loop over the raw particle pools to get their TRUE, post-squish positions!
    for (u32 t = 0; t < static_cast<u32>(FluidType::Count); ++t) {
        std::vector<FluidParticle>& particlePool =
            fluidSystem.GetParticlePool(static_cast<FluidType>(t));

        for (size_t pIdx = 0; pIdx < particlePool.size(); ++pIdx) {
            FluidParticle& p = particlePool[pIdx];

            // Calculate the exact grid cell using the NEW position from Pass 1
            s32 cx = static_cast<s32>(std::floor((p.transform_.pos_.x - gridBottomLeftPos.x) /
                                                 static_cast<f32>(gridSize)));
            s32 cy = static_cast<s32>(std::floor((p.transform_.pos_.y - gridBottomLeftPos.y) /
                                                 static_cast<f32>(gridSize)));

            // Check the 3x3 dirt neighborhood around the true location
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;

                    // Bounds check
                    if (nx < 0 || nx >= static_cast<int>(gridCols) || ny < 0 ||
                        ny >= static_cast<int>(gridRows)) {
                        continue;
                    }

                    size_t neighbourIndex =
                        static_cast<size_t>(ny) * static_cast<size_t>(gridCols) +
                        static_cast<size_t>(nx);
                    Cell& terrainCell = terrain.getCells()[neighbourIndex];

                    CollisionInfo contact = cellToFluidParticleCollision(terrainCell, p);
                    if (contact.hasCollision) {
                        pushOutAndSlide(p, contact.normal, contact.penetration,
                                        p.collider_.shapeData_.circle_.radius);
                    }
                }
            }
        }
    }

}
*/

// @todo comment entire function

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

// Helper function: Determines whether a point (p) is within a triangle with vertices (a,b,c) or
// not.
bool CollisionSystem::pointInTriangle(const AEVec2& p, const AEVec2& a, const AEVec2& b,
                                      const AEVec2& c) {
    // Calculate the direction vectors of the three line segments which form the triangle
    const AEVec2 edgeAB = vSub(b, a);
    const AEVec2 edgeBC = vSub(c, b);
    const AEVec2 edgeCA = vSub(a, c);

    // Calculate vectors from the vertices to the point P
    const AEVec2 vectorAP = vSub(p, a);
    const AEVec2 vectorBP = vSub(p, b);
    const AEVec2 vectorCP = vSub(p, c);

    // // Calculate the 2D Cross Product for each edge.
    //
    // MATH EXPLANATION:
    // By treating these 2D vectors as 3D vectors with a Z of 0 (homogeneous coordinates),
    // this formula extracts the resulting Z-component.
    //
    // This scalar value represents the exact surface area of the parallelogram they form.
    // Formula: AB x BC = Area of Parallelogram
    //
    // Because it is a signed area, the positive or negative sign tells us the "winding order"
    //
    // This refers to which direction the edge vector would need to rotate to face point P.
    //
    // For example, the value obtained in cross1 is a scalar value for how much rotation should be
    // applied to edgeAB to reach AP
    const f32 cross1 = (edgeAB.x * vectorAP.y) - (edgeAB.y * vectorAP.x);
    const f32 cross2 = (edgeBC.x * vectorBP.y) - (edgeBC.y * vectorBP.x);
    const f32 cross3 = (edgeCA.x * vectorCP.y) - (edgeCA.y * vectorCP.x);

    // After the calculations above, we evaluate the signs cross1, cross2 and cross3.
    // If a cross is positive, this means that vector A has to rotate left to reach vector B

    // If point P is really within the triangle, all 3 variations of vector A will turn right,
    // resulting in hasNegative = true, hasPositive = false.
    bool hasNegative = (cross1 < 0.0f) || (cross2 < 0.0f) || (cross3 < 0.0f);
    bool hasPositive = (cross1 > 0.0f) || (cross2 > 0.0f) || (cross3 > 0.0f);
    return !(hasNegative && hasPositive);
}

CollisionInfo CollisionSystem::cellToFluidParticleCollision(const Cell& cell,
                                                            const FluidParticle& fluidParticle) {

    CollisionInfo contact;

    const AEVec2 circleCenter =
        vAdd(fluidParticle.transform_.pos_, fluidParticle.collider_.shapeData_.circle_.offset_);
    const f32 radius = fluidParticle.collider_.shapeData_.circle_.radius;
    const AEVec2 velocity = fluidParticle.physics_.velocity_;

    for (u32 i = 0; i < 3; ++i) {
        const Collider2D& col = cell.colliders_[i];
        if (col.colliderShape_ == ColliderShape::Empty)
            continue;

        AEVec2 n{0.0f, 1.0f};
        f32 penetration = 0.0f;
        bool hit = false;

        if (col.colliderShape_ == ColliderShape::Box) {
            // This takes the box's local offset and stretches it by the cell's actual physical size
            // on the screen.
            const AEVec2 offsetWorld{col.shapeData_.box_.offset_.x * cell.transform_.scale_.x,
                                     col.shapeData_.box_.offset_.y * cell.transform_.scale_.y};

            // This takes the exact world coordinates of the Grid Cell itself,
            // and adds the offsetWorld to get the exact pixel coordinate of the Box's center.
            const AEVec2 boxCenter = vAdd(cell.transform_.pos_, offsetWorld);

            // This stretches the local width/height of the box by the cell's scale to get the true
            // pixel dimensions.
            const AEVec2 sizeWorld{col.shapeData_.box_.size_.x * cell.transform_.scale_.x,
                                   col.shapeData_.box_.size_.y * cell.transform_.scale_.y};

            // halfExt just refers to half-width/half-height, calculated now so that future
            // calculations arent needed.
            const AEVec2 halfExt{sizeWorld.x * 0.5f, sizeWorld.y * 0.5f};

            hit = detectCircleVsAABB(circleCenter, radius, velocity, boxCenter, halfExt, n,
                                     penetration);
        } else if (col.colliderShape_ == ColliderShape::Triangle) {
            // @todo comment
            const AEVec2 v0 =
                localToWorldPoint(col.shapeData_.triangle_.vertices_[0], cell.transform_);
            const AEVec2 v1 =
                localToWorldPoint(col.shapeData_.triangle_.vertices_[1], cell.transform_);
            const AEVec2 v2 =
                localToWorldPoint(col.shapeData_.triangle_.vertices_[2], cell.transform_);

            hit =
                detectCircleVsTriangle(circleCenter, radius, velocity, v0, v1, v2, n, penetration);
        }

        // If any of the two collisions above occur, we return contact information so that collision
        // response can occur in the calling function.
        //
        // Even if there are more than 2 colliders in the cell, the particle should only collide
        // with the FIRST collider it hits. Collision response will then occur based on that first
        // collision's normal and penetration values. If the particle gets pushed into another
        // collider in the same frame, its okay as it will be resolved in the next frame when we
        // check for collisions again.

        if (hit) {
            contact.hasCollision = true;
            contact.normal = vNormalizeOr(n, AEVec2{0.0f, 1.0f});
            contact.penetration = penetration;
            return contact;
        }
    }

    return contact; // Returns hasCollision = false if nothing was hit
}

// Helper function for cellToFluidParticleCollision: detects Circle vs Triangle collision in world
// NOTE: Collider radius has already been accounted for in cellToFluidParticleCollision
bool CollisionSystem::detectCircleVsTriangle(const AEVec2& circleCenter, f32 radius,
                                             const AEVec2& velocity, const AEVec2& v0,
                                             const AEVec2& v1, const AEVec2& v2, AEVec2& outNormal,
                                             f32& outPenetration) {

    // Find the closest point on the triangle's perimeter so that we know which side of the triangle
    // to focus on later on Right now, the position of our particle within the cell is UNKNOWN!!!
    const AEVec2 c0 = closestPointOnSegment(v0, v1, circleCenter);
    const AEVec2 c1 = closestPointOnSegment(v1, v2, circleCenter);
    const AEVec2 c2 = closestPointOnSegment(v2, v0, circleCenter);

    // vSub first to get the 'direction vector' from the circle center to the closest point for each
    // segment of the triangle vLenSq calculates the squared distance/magnitude from the circle
    // center to each of the closest points on the triangle edges (c0, c1, c2). vLenSq is used
    // instead of vLen to avoid std::sqrt which is expensive to calculate for every particle.
    const f32 d0 = vLenSq(vSub(circleCenter, c0));
    const f32 d1 = vLenSq(vSub(circleCenter, c1));
    const f32 d2 = vLenSq(vSub(circleCenter, c2));

    // Compares all distances (d0,d1,d2) obtained above to find the closest point (c0,c1,c2) on the
    // triangle to the circle center. This is our chosen point of contact.
    AEVec2 closest = (d0 < d1) ? ((d0 < d2) ? c0 : c2) : ((d1 < d2) ? c1 : c2);

    // Now that we have the closest point, we can treat it like a circle vs point collision and find
    // the normal vector and penetration/resolution amount. vSub gives a direction vector pointing
    // from the circle center to the closest point on the triangle.
    const AEVec2 d = vSub(circleCenter, closest);
    const f32 distSq = vLenSq(d);
    const f32 dist = std::sqrt((std::max)(distSq, 1e-8f));

    // @todo comment/not my code
    bool isInside = pointInTriangle(circleCenter, v0, v1, v2);

    // If particle is inside the triangle, this code runs
    if (isInside) {
        // Vector 'd' points from the edge INWARD to the center.
        // We flip it so the normal points OUTWARD to eject the particle.

        outNormal = AEVec2{-d.x / dist, -d.y / dist};
        outPenetration = radius + dist;
    }

    // If particle is outside the triangle, this code runs
    else {

        // This only runs if the particle is intersecting the point but outside the triangle
        // If distance from circle center to closest point (distSq) is > magnitude of radius
        // (distance of penetration), then there is no collision.
        if (distSq > radius * radius)
            return false;

        // If we are here, it means we are intersecting the triangle edge but outside the triangle.
        outNormal = AEVec2{d.x / dist, d.y / dist};
        outPenetration = radius - dist;

        // Optimisation: If we are moving away from the triangle anyways, it will never collide with
        // the triangle,
        //               so we can early-out and skip the collision response.
        f32 dotProduct = (velocity.x * outNormal.x) + (velocity.y * outNormal.y);
        if (dotProduct > 0.0f) {
            return false;
        }
    }

    // return true so that collision response can occur
    return true;
}

// Helper function for cellToFluidParticleCollision: detects Circle vs AABB collision in world
// NOTE: Collider radius has already been accounted for in cellToFluidParticleCollision
bool CollisionSystem::detectCircleVsAABB(const AEVec2& circleCenter, f32 radius,
                                         const AEVec2& velocity, const AEVec2& boxCenter,
                                         const AEVec2& halfExt, AEVec2& outNormal,
                                         f32& outPenetration) {

    // Calculate the closest point on the box to the circle center
    const f32 boxLeft = boxCenter.x - halfExt.x;
    const f32 boxRight = boxCenter.x + halfExt.x;
    const f32 boxBottom = boxCenter.y - halfExt.y;
    const f32 boxTop = boxCenter.y + halfExt.y;

    // Clamp the circle center to the box boundaries to get the closest point to the circle center
    //
    // For e.g. if boxLeft = 0, boxRight = 10, circleCenter.x = 15, circleCenter.x gets clamped to
    // 10
    AEVec2 closest;
    closest.x = AEClamp(circleCenter.x, boxLeft, boxRight);
    closest.y = AEClamp(circleCenter.y, boxBottom, boxTop);

    // isInside is true as long as AEClamp does not actually clamp the circle's coordinates.
    // This means that in this current frame, the circle is already inside the box.
    bool isInside = (circleCenter.x == closest.x && circleCenter.y == closest.y);

    if (isInside) {

        // direction/'distance' vector from circle center to box center
        f32 dx = circleCenter.x - boxCenter.x;
        f32 dy = circleCenter.y - boxCenter.y;

        // Finding the nearest edge to exit from
        //
        // We use std::abs here to get a positive value of dxdy
        // as the circle center can be on the left side or right side of the box center
        f32 distToEdgeX = halfExt.x - std::abs(dx);
        f32 distToEdgeY = halfExt.y - std::abs(dy);

        // Find the nearest edge to exit from
        if (distToEdgeX < distToEdgeY) {

            // If dx > 0, it means the circle center is on the right side of the box center, so we
            // want to push it to the right, hence normal (1,0) If dx < 0, it means the circle
            // center is on the left side of the box center....
            outNormal = (dx > 0) ? AEVec2{1.0f, 0.0f} : AEVec2{-1.0f, 0.0f};
            outPenetration = radius + distToEdgeX;
        } else {
            // If dy > 0, it means the circle center is above the box center, so we want to push it
            // up, hence normal (0,1) If dy < 0, it means the circle center is below the box
            // center....
            outNormal = (dy > 0) ? AEVec2{0.0f, 1.0f} : AEVec2{0.0f, -1.0f};
            outPenetration = radius + distToEdgeY;
        }

        // Return true to proceed with collision response using the output parameters: normal and
        // penetration calculated above.
        return true;

    } else {

        // If the circle is not inside, we check if the closest point on the box to the circle
        // center is within the radius distance.
        const AEVec2 d = vSub(circleCenter, closest);
        const f32 distSq = vLenSq(d);

        // If distSq is greater than radius squared, it means the circle is too far from the box and
        // there is no collision right now.
        if (distSq > radius * radius)
            return false;

        // std::max returns the larger of distSq and 1e-8f to avoid taking the square root of a very
        // small number. This prevents dividing by zero if the circle center just so happens to be
        // right within the box edge, which would make distSq very close to zero and cause division
        // by zero resulting in NaN values for the normal.
        const f32 dist = std::sqrt((std::max)(distSq, 1e-8f));
        outNormal = AEVec2{d.x / dist, d.y / dist};
        outPenetration = radius - dist;

        f32 dotProduct = (velocity.x * outNormal.x) + (velocity.y * outNormal.y);
        if (dotProduct > 0.0f) {
            return false;
        }
    }
    // Return true to proceed with collision response using the output parameters: normal and
    // penetration calculated above.
    return true;
}

// Collision Response
void CollisionSystem::pushOutAndSlide(FluidParticle& p, const AEVec2& n, f32 penetration,
                                      f32 radius) {

    // We add a tiny slop to prevent floating point errors from causing continuous micro-collisions
    const f32 slop = 0.01f;
    f32 push = (std::max)(0.0f, penetration + slop);

    p.transform_.pos_.x += n.x * push;
    p.transform_.pos_.y += n.y * push;

    // vn = dot product between particle velocity direction vector and the collider's outNormal
    // vector if vn < 0.0f, that means that they are going to collide!!! (just an extra safety
    // check)
    const f32 vn = p.physics_.velocity_.x * n.x + p.physics_.velocity_.y * n.y;
    if (vn < 0.0f) {
        p.physics_.velocity_.x -= vn * n.x;
        p.physics_.velocity_.y -= vn * n.y;

        // ==========================================
        // HEAVY WALL DAMPING (Fixes Horizontal Leaks)
        // ==========================================
        // If the collision normal is mostly horizontal (|n.x| > 0.5), it means
        // the particle just smashed into a vertical dirt wall.
        // We aggressively absorb the horizontal kinetic energy (Hydrostatic pressure)
        // so it doesn't violently jitter through the 1-block thin terrain.
        if (std::abs(n.x) > 0.4f) {
            // Kill 80% of horizontal velocity instantly
            p.physics_.velocity_.x *= 0.05f;
        }

        // EFFECT A: Variable Friction (Stretches the stream)
        // We generate a random multiplier between 0.75f and 0.95f.
        // Because it is ALWAYS less than 1.0f, it safely absorbs energy, but
        // ensures particles leave the ramp at slightly different speeds!
        f32 randomFriction = 0.98f + ((rand() % 100) * 0.0001f);
        p.physics_.velocity_.x *= randomFriction;
        p.physics_.velocity_.y *= randomFriction;

        // EFFECT B: Micro-Bumps (Thickens the stream)
        // Real dirt has tiny pebbles. When water hits them, it splashes outward.
        // We apply a microscopic random kick purely along the OUTWARD normal vector.
        // (Generates a tiny push between 0.0f and 15.0f velocity units)
        f32 microBump = (rand() % 100) * 0.15f;
        p.physics_.velocity_.x += n.x * microBump;
        p.physics_.velocity_.y += n.y * microBump;

        /*
        f32 noise = ((rand() % 100) * 0.001f) - 0.04f;
        noise = std::abs(noise);
        p.physics_.velocity_.x *= 0.85f + noise;
        p.physics_.velocity_.y *= 0.95f + noise;
        */
    }
}
/*
void CollisionSystem::resolveFluidParticlePair(FluidParticle& p1, FluidParticle& p2) {

    f32 dx = p1.transform_.pos_.x - p2.transform_.pos_.x;
    f32 dy = p1.transform_.pos_.y - p2.transform_.pos_.y;

    // Jitter fix to break perfect vertical towers
    if (std::abs(dx) < 0.001f) {
        f32 noise = ((rand() % 100) * 0.001f) - 0.05f;
        dx += noise;
    }

    f32 minDist = p1.collider_.shapeData_.circle_.radius + p2.collider_.shapeData_.circle_.radius;
    f32 distSq = dx * dx + dy * dy;

    if (distSq < minDist * minDist) {
        f32 dist = std::sqrt((std::max)(distSq, 0.0001f));

        f32 nx = dx / dist;
        f32 ny = dy / dist;

        f32 overlap = minDist - dist;

        // ==========================================
        // 1. OVERLAP CAP (Stops single-frame teleports)
        // ==========================================
        f32 maxOverlap = p1.collider_.shapeData_.circle_.radius * 0.25f;
        if (overlap > maxOverlap) {
            overlap = maxOverlap;
        }

        f32 repulsion = 0.5f;

        // ==========================================
        // 2. THE ANVIL (Protects the Floor)
        // ==========================================
        // We MUST keep this so the bottom particles don't get
        // crushed through the dirt by the weight of the stack above them!
        f32 p1Weight = 1.0f;
        f32 p2Weight = 1.0f;

        if (std::abs(ny) > 0.2f) {
            if (dy > 0.0f) {
                p1Weight = 1.0f; // Top particle moves easily
                p2Weight = 0.2f; // Bottom particle stubbornly resists downward motion
            } else {
                p1Weight = 0.2f;
                p2Weight = 1.0f;
            }
        }

        f32 moveX1 = nx * (overlap * repulsion) * p1Weight;
        f32 moveY1 = ny * (overlap * repulsion) * p1Weight;
        f32 moveX2 = nx * (overlap * repulsion) * p2Weight;
        f32 moveY2 = ny * (overlap * repulsion) * p2Weight;

        // Notice: NO HYDROSTATIC SLIP HERE!
        // By removing the slip, the particles won't generate artificial horizontal
        // pressure. This is what protects your thin side-walls from breaking!

        p1.transform_.pos_.x += moveX1;
        p1.transform_.pos_.y += moveY1;
        p2.transform_.pos_.x -= moveX2;
        p2.transform_.pos_.y -= moveY2;

        f32 relativeVX = p1.physics_.velocity_.x - p2.physics_.velocity_.x;
        f32 relativeVY = p1.physics_.velocity_.y - p2.physics_.velocity_.y;
        f32 velAlongNormal = relativeVX * nx + relativeVY * ny;

        if (velAlongNormal < 0.0f) {
            // ==========================================
            // 3. DEAD RESTITUTION (Granular Sand)
            // ==========================================
            // Absorbs energy quickly so the particles "thud" into place
            // and form a mound instead of bouncing endlessly.
            f32 restitution = -0.3f;
            f32 j = restitution * velAlongNormal * 0.5f;

            p1.physics_.velocity_.x += j * nx;
            p1.physics_.velocity_.y += j * ny;
            p2.physics_.velocity_.x -= j * nx;
            p2.physics_.velocity_.y -= j * ny;
        }
    }
}

*/

void CollisionSystem::resolveFluidParticlePair(FluidParticle& p1, FluidParticle& p2) {

    // Calculate distance between p1 and p2
    f32 dx = p1.transform_.pos_.x - p2.transform_.pos_.x;
    f32 dy = p1.transform_.pos_.y - p2.transform_.pos_.y;

    // Optimisation: Jitter fix for vertical stacking of particles
    if (std::abs(dx) < 0.001f) {
        // Generate a tiny random float between - 0.05 and 0.05

        f32 noise = ((rand() % 100) * 0.001f) - 0.05f;
        dx += noise;

        // Add a small constant value to dx to prevent perfect vertical stacking
        // dx += 0.01f;
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

        // ==========================================
        // PRESSURE RELIEF VALVE
        // ==========================================
        f32 maxOverlap = p1.collider_.shapeData_.circle_.radius * 0.15f;
        if (overlap > maxOverlap) {
            overlap = maxOverlap;
        }

        f32 repulsion = 0.5f;

        // ==========================================
        // WEIGHTED PARTICLES
        // ==========================================
        f32 p1Weight = 1.0f;
        f32 p2Weight = 1.0f;

        // If it's a vertical collision, harden the bottom particle!
        if (std::abs(ny) > 0.4f) {
            if (dy > 0.0f) {
                p1Weight = 1.05f; // p1 is on top (light)
                p2Weight = 0.05f; // p2 is on bottom (heavy anvil)
            } else {
                p1Weight = 0.05f;
                p2Weight = 1.05f;
            }
        }

        // Calculate weighted push
        f32 moveX1 = nx * (overlap * repulsion) * p1Weight;
        f32 moveY1 = ny * (overlap * repulsion) * p1Weight;
        f32 moveX2 = nx * (overlap * repulsion) * p2Weight;
        f32 moveY2 = ny * (overlap * repulsion) * p2Weight;

        /*
                // ==========================================
        // HYDROSTATIC SLIP (Forces Leveling)
        // ==========================================
        if (std::abs(ny) > 0.4f) {
            f32 slip = overlap * 0.08f;
            moveX1 += (nx > 0.0f) ? slip : -slip;
            moveX2 += (nx > 0.0f) ? slip : -slip;
        }

        */

        // Apply movement
        p1.transform_.pos_.x += moveX1;
        p1.transform_.pos_.y += moveY1;
        p2.transform_.pos_.x -= moveX2;
        p2.transform_.pos_.y -= moveY2;

        // --- Velocity Impulse ---
        // Upon collision, we should calculate the relative difference in velocity
        // between the two particles For example, if relativeVX > 0, it means p1 is
        // moving faster than p2 in the x axis
        f32 relativeVX = p1.physics_.velocity_.x - p2.physics_.velocity_.x;
        f32 relativeVY = p1.physics_.velocity_.y - p2.physics_.velocity_.y;

        // This is the dot product between relative velocity of
        f32 velAlongNormal = relativeVX * nx + relativeVY * ny;

        // If velAlongNormal < 0.0f this means that the particles are moving TOWARDS
        // each other.
        if (velAlongNormal < 0.0f) {

            // Restitution determines the "Bounciness" of the collision.
            // It calculates how much relative velocity is preserved or lost after impact.
            // 0.0f   : Absorbs all energy
            // -1.0f  : Perfectly Inelastic
            // -2.0f  : Perfectly Elastic
            f32 restitution =
                -0.03f; // A multiplier for how much velocity should be applied to each particle
            f32 j =
                restitution * velAlongNormal * 0.5f; // 0.5f added because each particle should only
                                                     // receive half the velocity multiplier value

            p1.physics_.velocity_.x += j * nx;
            p1.physics_.velocity_.y += j * ny;
            p2.physics_.velocity_.x -= j * nx;
            p2.physics_.velocity_.y -= j * ny;
        }
    }
}
