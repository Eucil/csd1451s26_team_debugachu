#pragma once
#include "AEEngine.h"
#include <cmath>

/*
usage: call AEGfxMeshStart -> AddCircleMesh -> meshPtr = AEGfxMeshEnd; -> waterParticle.mesh_ =
meshPtr;
*/

AEGfxVertexList* CreateCircleMesh(f32 slices);

AEGfxVertexList* CreateRectMesh();
