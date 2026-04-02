/*!
@file       Utils.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

#include <cmath>
#include <string>

#include <AEEngine.h>

#include "Components.h"

/*
usage: call AEGfxMeshStart -> AddCircleMesh -> meshPtr = AEGfxMeshEnd; -> waterParticle.mesh_ =
meshPtr;
*/

AEGfxVertexList* CreateCircleMesh(u32 slices, f32 radius);
AEGfxVertexList* CreateWireCircleMesh(u32 slices);

AEGfxVertexList* CreateRectMesh();
AEGfxVertexList* CreateWireRectMesh();
AEGfxVertexList* CreateWireLineMesh();
AEVec2 GetMouseWorldPos();

struct TiledBackground {
    void loadFromJson(const std::string& file, const std::string& section);
    void draw() const;
    void unload();

    Graphics graphics_;
};

void inputNumbers(std::string& inputStr);
