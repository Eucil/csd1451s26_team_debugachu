#include "AEEngine.h"

#include "Utils.h"
#include <string>

AEGfxVertexList* CreateCircleMesh(f32 slices) {
    AEGfxMeshStart();

    // 1. Constants
    f32 radius = 0.5f; // <--- DO NOT CHANGE THIS RADIUS
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

AEGfxVertexList* CreateRectMesh() {
    AEGfxMeshStart();

    // This shape has 2 triangles that makes up a square
    // Color parameters represent colours as ARGB
    // UV coordinates to read from loaded textures
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    // Saving the rect mesh
    return AEGfxMeshEnd();
}

AEVec2 GetMouseWorldPos() {
    s32 mouse_x = 0, mouse_y = 0;
    AEInputGetCursorPosition(&mouse_x, &mouse_y);
    AEVec2 worldPos{};
    worldPos.x = (f32)mouse_x - (AEGfxGetWindowWidth() / 2.0f);
    worldPos.y = (AEGfxGetWindowHeight() / 2.0f) - (f32)mouse_y;

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