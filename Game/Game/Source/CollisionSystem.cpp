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

                    // We first evaluate the current fluidParticleA against the terrain cell it is
                    // visiting. celltoFluidParticleCollision returns the contact information of the
                    // collision which includes:
                    //
                    // a bool indicating whether a collision has occurred,
                    // the outward normal of the collidable object (triangle or square or empty)
                    CollisionInfo contact =
                        cellToFluidParticleCollision(neighbourTerrainCell, fluidParticleA);

                    if (contact.hasCollision) {
                        pushOutAndSlide(fluidParticleA, contact.normal, contact.penetration,
                                        fluidParticleA.collider_.shapeData_.circle_.radius);
                    }

                    // @todo code below may now be irrelevant have to check again
                    //
                    // cellToFluidParticleCollision(neighbourTerrainCell, fluidParticleA);
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

// @todo not finished
// Response: push out + slide
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
/*

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
    // Loop through the 3 colliders in the cell (box, triangle, empty) and resolve against the
particle. for (u32 i = 0; i < 3; ++i) { const Collider2D& col = cell.colliders_[i]; if
(col.colliderShape_ == ColliderShape::Empty) continue;

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

        // If any of the two collisions hit, we push out and slide, then break to prevent multiple
collisions in one frame if (hit) { n = vNormalizeOr(n, AEVec2{0.0f, 1.0f});
            pushOutAndSlide(fluidParticle, n, penetration, radius);
            return; // stop after first hit (prevents teleport)
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

        // @todo comment this
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