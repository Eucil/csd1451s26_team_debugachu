/*!
@file       MeshUtils.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This header file declares utility functions for creating common
            AEGfxVertexList meshes used for rendering and debug wireframes.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <cmath>
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "Components.h"

AEGfxVertexList* createCircleMesh(u32 slices, f32 radius);
AEGfxVertexList* createWireCircleMesh(u32 slices);

AEGfxVertexList* createRectMesh();
AEGfxVertexList* createWireRectMesh();
AEGfxVertexList* createWireLineMesh();
