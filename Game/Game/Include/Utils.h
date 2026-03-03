#pragma once
#include "AEEngine.h"
#include <cmath>
#include <string>

/*
usage: call AEGfxMeshStart -> AddCircleMesh -> meshPtr = AEGfxMeshEnd; -> waterParticle.mesh_ =
meshPtr;
*/

AEGfxVertexList* CreateCircleMesh(f32 slices);

AEGfxVertexList* CreateRectMesh();

AEVec2 GetMouseWorldPos();

void inputNumbers(std::string& inputStr);
