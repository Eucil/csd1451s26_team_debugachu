/*!
@file       Utils.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "Utils.h"

#include <string>

#include <AEEngine.h>

AEGfxVertexList* CreateCircleMesh(u32 slices, f32 radius) {
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

AEGfxVertexList* CreateRectMesh() {
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

AEGfxVertexList* CreateWireRectMesh() {
    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd( 0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd( 0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(-0.5f,  0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f); // close loop
    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateWireCircleMesh(u32 slices) {
    AEGfxMeshStart();
    f32 angleStep = (3.14159265f * 2.0f) / static_cast<f32>(slices);
    for (u32 i = 0; i <= slices; ++i) {
        f32 theta = i * angleStep;
        AEGfxVertexAdd(std::cos(theta) * 0.5f, std::sin(theta) * 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    }
    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateWireLineMesh() {
    AEGfxMeshStart();
    AEGfxVertexAdd(0.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxVertexAdd(1.0f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

AEVec2 GetMouseWorldPos() {
    s32 mouseX = 0, mouseY = 0;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    AEVec2 worldPos{};
    worldPos.x = (f32)mouseX - (AEGfxGetWindowWidth() / 2.0f);
    worldPos.y = (AEGfxGetWindowHeight() / 2.0f) - (f32)mouseY;

    return worldPos;
}

void inputNumbers(std::string& inputStr) {
    if (AEInputCheckReleased(AEVK_1)) {
        inputStr += "1";
    } else if (AEInputCheckReleased(AEVK_2)) {
        inputStr += "2";
    } else if (AEInputCheckReleased(AEVK_3)) {
        inputStr += "3";
    } else if (AEInputCheckReleased(AEVK_4)) {
        inputStr += "4";
    } else if (AEInputCheckReleased(AEVK_5)) {
        inputStr += "5";
    } else if (AEInputCheckReleased(AEVK_6)) {
        inputStr += "6";
    } else if (AEInputCheckReleased(AEVK_7)) {
        inputStr += "7";
    } else if (AEInputCheckReleased(AEVK_8)) {
        inputStr += "8";
    } else if (AEInputCheckReleased(AEVK_9)) {
        inputStr += "9";
    } else if (AEInputCheckReleased(AEVK_0)) {
        inputStr += "0";
    } else if (AEInputCheckReleased(AEVK_BACK)) {
        if (!inputStr.empty()) {
            inputStr.pop_back();
        }
    }
}