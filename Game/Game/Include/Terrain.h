/*!
@file       Terrain.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This header file  contains the declarations of the Terrain class,
            which manages a marching-squares grid of cells with shared mesh and collider
            libraries for node-based terrain editing and rendering.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <random>
#include <vector>

// Third-party
#include <AEEngine.h>

// Project
#include "Components.h"

enum class TerrainMaterial { Dirt, Stone, Magic };

class Cell {
public:
    Transform transform_;
    Graphics graphics_;
    Collider2D colliders_[3];

private:
};

class Terrain {
public:
    Terrain(TerrainMaterial terrainMaterial, AEGfxTexture* pTex, AEVec2 centerPosition,
            u32 cellRows, u32 cellCols, u32 cellSize, bool collidable);

    void initCellsTransform();

    void initCellsGraphics();

    void initCellsCollider();

    void updateTerrain();

    bool destroyAtMouse(f32 radius);

    void buildAtMouse(f32 radius);

    void renderTerrain();

    static void createMeshLibrary();

    static void freeMeshLibrary();

    static void createColliderLibrary();

    u32 getCellRows() const { return kCellRows_; }

    u32 getCellCols() const { return kCellCols_; }

    u32 getCellSize() const { return kCellSize_; }

    AEVec2 getBottomLeftPos() const { return bottomLeftPos_; }

    std::vector<Cell>& getCells() { return cells_; }

    std::vector<f32>& getNodes() { return nodes_; }

    std::vector<bool>& getCachedHasColliders() { return cachedHasColliders_; }
    bool isCollidersCacheDirty() const { return collidersCacheDirty_; }
    void markCollidersCacheDirty() { collidersCacheDirty_ = true; }
    void markCollidersCacheClean() { collidersCacheDirty_ = false; }

    bool isNearestNodeToMouseAtThreshold();

private:
    TerrainMaterial terrainMaterial_;

    Transform transform_; // Position represents the centre of the terrain

    Graphics graphics_; // Only for AEGfxTexture* texture_

    const u32 kCellRows_;
    const u32 kCellCols_;
    const u32 kCellSize_;

    const u32 kNodeRows_;
    const u32 kNodeCols_;

    std::vector<f32> nodes_;
    std::vector<Cell> cells_;

    f32 threshold_{1.0f};

    static AEGfxVertexList* meshLibrary_[16];  // There are 16 possible meshes
    static Collider2D colliderLibrary_[16][3]; // There are 16 possible colliders, [3] as each cell
                                               // uses 1, 2, or 3 colliders

    f32 halfWidth_;
    f32 halfHeight_;
    AEVec2 bottomLeftPos_;

    void destroyTerrain(f32 worldX, f32 worldY);

    bool destroyTerrainRadius(f32 worldX, f32 worldY, f32 radius);

    void buildTerrainRadius(f32 worldX, f32 worldY, f32 radius);

    bool collidable_;

    static AEGfxVertexList* debugTriMesh_;
    static AEGfxVertexList* debugBoxMesh_;

    std::vector<bool> cachedHasColliders_;
    bool collidersCacheDirty_ = true;
};
