/*!
@file       MeshUtils.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the implementation of utility functions
            for creating filled and wireframe AEGfxVertexList meshes.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "MeshUtils.h"

// Standard library
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "ConfigManager.h"

// =========================================================
//
// createCircleMesh
//
// Creates a filled circle mesh by tessellating the circle
// into the given number of pizza-slice triangles, each
// fanning out from the origin to the circumference.
//
// =========================================================
AEGfxVertexList* createCircleMesh(u32 slices, f32 radius) {
    AEGfxMeshStart();

    // 1. Constants
    f32 angleStep = (3.14159265f * 2.0f) / static_cast<f32>(slices);

    // 2. Loop to create pizza slices
    for (u32 i = 0; i < slices; i++) {
        // Calculate angles
        f32 theta1 = i * angleStep;
        f32 theta2 = (i + 1) * angleStep;

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

// =========================================================
//
// createRectMesh
//
// Creates a filled unit rectangle mesh centered at the
// origin, made of two triangles spanning (-0.5, -0.5)
// to (0.5, 0.5) with UV coordinates.
//
// =========================================================
AEGfxVertexList* createRectMesh() {
    AEGfxMeshStart();

    // This shape has 2 triangles that makes up a square
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxVertexList* mesh = AEGfxMeshEnd();

    if (mesh == nullptr) {
        printf("ERROR: Failed to create rectangle mesh!\n");
    }

    return mesh;
}

// =========================================================
//
// createWireRectMesh
//
// Creates a wireframe unit rectangle mesh as a closed
// line strip spanning (-0.5, -0.5) to (0.5, 0.5).
//
// =========================================================
AEGfxVertexList* createWireRectMesh() {
    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f); // close loop
    return AEGfxMeshEnd();
}

// =========================================================
//
// createWireCircleMesh
//
// Creates a wireframe circle mesh as a closed line strip
// with the given number of segments at radius 0.5.
//
// =========================================================
AEGfxVertexList* createWireCircleMesh(u32 slices) {
    AEGfxMeshStart();
    f32 angleStep = (3.14159265f * 2.0f) / static_cast<f32>(slices);
    for (u32 i = 0; i <= slices; ++i) {
        f32 theta = i * angleStep;
        AEGfxVertexAdd(std::cos(theta) * 0.5f, std::sin(theta) * 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    }
    return AEGfxMeshEnd();
}

// =========================================================
//
// createWireLineMesh
//
// Creates a two-vertex line mesh from (0,0) to (1,0).
// Scale and rotate the transform to draw arbitrary lines.
//
// =========================================================
AEGfxVertexList* createWireLineMesh() {
    AEGfxMeshStart();
    AEGfxVertexAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(1.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}