#include "AEEngine.h"

#include "Utils.h"

AEGfxVertexList* CreateCircleMesh(f32 slices) {
    // 1. Constants
    f32 radius = 0.5f;
    f32 angle_step = (3.14159265f * 2.0f) / (f32)slices;

    // 2. Loop to create pizza slices
    for (u32 i = 0; i < slices; i++) {
        // Calculate angles
        f32 theta1 = i * angle_step;
        f32 theta2 = (i + 1) * angle_step;

        // Calculate Vertex A (Current) -> Renamed to ax, ay
        f32 ax = std::cos(theta1) * radius;
        f32 ay = std::sin(theta1) * radius;

        // Calculate Vertex B (Next) -> Renamed to bx, by
        f32 bx = std::cos(theta2) * radius;
        f32 by = std::sin(theta2) * radius;

        // UVs (Texture Coordinates) -> Renamed to au, av / bu, bv
        f32 au = ax + 0.5f;
        f32 av = ay + 0.5f;
        f32 bu = bx + 0.5f;
        f32 bv = by + 0.5f;

        // Add the triangle: Center(0,0) -> A -> B
        AEGfxTriAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.5f, 0.5f, // Center
                    ax, ay, 0xFFFFFFFF, au, av,         // Point A
                    bx, by, 0xFFFFFFFF, bu, bv          // Point B
        );
    }
    return AEGfxMeshEnd();
}