#pragma once

#include <cmath>
#include <string>

#include <AEEngine.h>

/*
usage: call AEGfxMeshStart -> AddCircleMesh -> meshPtr = AEGfxMeshEnd; -> waterParticle.mesh_ =
meshPtr;
*/

AEGfxVertexList* CreateCircleMesh(u32 slices, f32 radius);

AEGfxVertexList* CreateRectMesh();

AEVec2 GetMouseWorldPos();

void inputNumbers(std::string& inputStr);
