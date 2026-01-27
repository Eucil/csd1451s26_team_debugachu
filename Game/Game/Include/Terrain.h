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

private:
};

class Terrain {
public:
    Terrain(TerrainMaterial terrainMaterial, AEVec2 centerPosition, u32 cellRows, u32 cellCols,
            u32 cellSize);

    void initCellsTransform();

    void initCellsGraphics();

    void updateTerrain();

    void destroyAtMouse(f32 radius);

    void renderTerrain();

    static void createMeshLibrary();

    static void freeMeshLibrary();

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

    static AEGfxVertexList* meshLibrary_[16]; // There are 16 possible meshes

    f32 halfWidth_;
    f32 halfHeight_;
    AEVec2 bottomLeftPos_;

    void destroyTerrain(f32 worldX, f32 worldY);

    void destroyTerrainRadius(f32 worldX, f32 worldY, f32 radius);
};
