#pragma once

#include <random>
#include <vector>

#include <AEEngine.h>

#include "Components.h"

enum class TerrainMaterial { Dirt, Stone };

class Cell {
public:
    Transform transform_;
    Graphics graphics_;
    Collider2D colliders_[3];

private:
};

class Terrain {
public:
    Terrain(TerrainMaterial terrainMaterial, AEVec2 centerPosition, u32 cellRows, u32 cellCols,
            u32 cellSize);

    void initCellsTransform();

    void initCellsGraphics();

    void initCellsCollider();

    void updateTerrain();

    void destroyAtMouse(f32 radius);

    void renderTerrain();

    static void createMeshLibrary();

    static void freeMeshLibrary();

    static void createColliderLibrary();

    u32 getCellRows() { return kCellRows_; }

    u32 getCellCols() { return kCellCols_; }

    u32 getCellSize() { return kCellSize_; }

    AEVec2 getBottomLeftPos() { return bottomLeftPos_; }

    std::vector<Cell>& getCells() { return cells_; }

private:
    TerrainMaterial terrainMaterial_;

    Transform transform_; // Position represents the centre of the terrain

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

    void destroyTerrainRadius(f32 worldX, f32 worldY, f32 radius);
};
