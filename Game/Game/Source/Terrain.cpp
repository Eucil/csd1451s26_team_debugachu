#include "Terrain.h"

#include <iostream>
#include <vector>

#include <AEEngine.h>

AEGfxVertexList* Terrain::meshLibrary_[16]{nullptr}; // There are 16 possible meshes
Collider2D Terrain::colliderLibrary_[16][3]{}; // There are 16 possible colliders, [3] as each cell
                                               // uses 1, 2, or 3 colliders
AEGfxVertexList* Terrain::debugTriMesh_{nullptr};
AEGfxVertexList* Terrain::debugBoxMesh_{nullptr};

Terrain::Terrain(TerrainMaterial terrainMaterial, AEVec2 centerPosition, u32 cellRows, u32 cellCols,
                 u32 cellSize)
    : terrainMaterial_(terrainMaterial), kCellRows_(cellRows), kCellCols_(cellCols),
      kCellSize_(cellSize), kNodeRows_(kCellRows_ + 1), kNodeCols_(kCellCols_ + 1),
      cells_(kCellRows_ * kCellCols_), nodes_(kNodeRows_ * kNodeCols_) {

    transform_.pos_ = centerPosition;

    // Random generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 1); // Generates 0 or 1

    // Calculate half the width and height of the whole terrain
    halfWidth_ = (static_cast<f32>(kCellCols_) * kCellSize_) / 2.0f;
    halfHeight_ = (static_cast<f32>(kCellRows_) * kCellSize_) / 2.0f;

    // Calculate the bottom-left corner of the terrain
    bottomLeftPos_ = {transform_.pos_.x - halfWidth_, transform_.pos_.y - halfHeight_};

    // Initialize nodes to random values for testing
    for (u32 r{0}; r < kNodeRows_; ++r) {
        for (u32 c{0}; c < kNodeCols_; ++c) {
            nodes_[r * kNodeCols_ + c] = static_cast<f32>(dist(gen));
        }
    }
}

void Terrain::initCellsTransform() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            Cell& cell = cells_[r * kCellCols_ + c];

            // Set position and scale
            cell.transform_.pos_.x = bottomLeftPos_.x + (c + 0.5f) * kCellSize_;
            cell.transform_.pos_.y = bottomLeftPos_.y + (r + 0.5f) * kCellSize_;
            cell.transform_.scale_ = {(f32)kCellSize_, (f32)kCellSize_};
        }
    }
}

void Terrain::initCellsGraphics() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            // Determine mesh case (TL=8, TR=4, BR=2, BL=1)
            u32 index{0};
            if (nodes_[(r + 1) * kNodeCols_ + c] >= threshold_)
                index |= 8;
            if (nodes_[(r + 1) * kNodeCols_ + c + 1] >= threshold_)
                index |= 4;
            if (nodes_[r * kNodeCols_ + c + 1] >= threshold_)
                index |= 2;
            if (nodes_[r * kNodeCols_ + c] >= threshold_)
                index |= 1;

            cells_[r * kCellCols_ + c].graphics_.mesh_ = meshLibrary_[index];
        }
    }
}

void Terrain::initCellsCollider() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            // Determine collider case (TL=8, TR=4, BR=2, BL=1)
            u32 index{0};
            if (nodes_[(r + 1) * kNodeCols_ + c] >= threshold_)
                index |= 8;
            if (nodes_[(r + 1) * kNodeCols_ + c + 1] >= threshold_)
                index |= 4;
            if (nodes_[r * kNodeCols_ + c + 1] >= threshold_)
                index |= 2;
            if (nodes_[r * kNodeCols_ + c] >= threshold_)
                index |= 1;

            Cell& cell = cells_[r * kCellCols_ + c];

            // Assign the collider shape from the library
            for (u32 i{0}; i < 3; ++i) {
                cell.colliders_[i] = colliderLibrary_[index][i];
            }
        }
    }
}

void Terrain::updateTerrain() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            Cell& cell = cells_[r * kCellCols_ + c];

            AEMtx33 scaleMtx, rotMtx, transMtx;
            AEMtx33Scale(&scaleMtx, cell.transform_.scale_.x, cell.transform_.scale_.y);
            AEMtx33Rot(&rotMtx, cell.transform_.rotationRad_);
            AEMtx33Trans(&transMtx, cell.transform_.pos_.x, cell.transform_.pos_.y);

            // transform = trans * rot * scale
            AEMtx33Concat(&cell.transform_.worldMtx_, &rotMtx, &scaleMtx);
            AEMtx33Concat(&cell.transform_.worldMtx_, &transMtx, &cell.transform_.worldMtx_);
        }
    }
}

void Terrain::destroyAtMouse(f32 radius) {
    s32 screenX, screenY;
    AEInputGetCursorPosition(&screenX, &screenY);

    // Convert screen to world coordinates
    f32 worldX{static_cast<f32>(screenX) - (AEGfxGetWindowWidth() / 2.0f)};
    f32 worldY{(AEGfxGetWindowHeight() / 2.0f) - static_cast<f32>(screenY)};

    destroyTerrainRadius(worldX, worldY, radius);
}

void Terrain::renderTerrain() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Set the color of the cell
    if (terrainMaterial_ == TerrainMaterial::Dirt) {
        AEGfxSetColorToMultiply(0.59f, 0.39f, 0.29f, 1.0f);
    } else if (terrainMaterial_ == TerrainMaterial::Stone) {
        AEGfxSetColorToMultiply(0.28f, 0.28f, 0.32f, 1.0f);
    } else {
        // Magenta color if terrainMaterial_ was not set properly
        AEGfxSetColorToMultiply(1.0f, 0.0f, 1.0f, 1.0f);
    }

    for (Cell& cell : cells_) {
        AEGfxSetTransform(cell.transform_.worldMtx_.m);
        AEGfxTextureSet(cell.graphics_.texture_, 0.0f, 0.0f);
        AEGfxMeshDraw(cell.graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
    }
}

void Terrain::createMeshLibrary() {
    // Corners
    constexpr f32 xLeft{-0.5f}, xRight{0.5f}, yBottom{-0.5f}, yTop{0.5f};

    // Midpoints
    constexpr f32 xMiddle{0.0f}, yMiddle{0.0f};

    u32 color{0xFFFFFFFF};

    for (u32 i{0}; i < 16; ++i) {
        AEGfxMeshStart();
        switch (i) {
        case 0:
            break; // Empty

        case 1: // BL
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xMiddle, yBottom, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            break;

        case 2: // BR
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yMiddle, color, 0, 0, xMiddle,
                        yBottom, color, 0, 0);
            break;

        case 3: // BL and BR (Bottom rectangle)
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xRight, yBottom, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yMiddle, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            break;

        case 4: // TR
            AEGfxTriAdd(xRight, yTop, color, 0, 0, xMiddle, yTop, color, 0, 0, xRight, yMiddle,
                        color, 0, 0);
            break;

        case 5: // BL and TR
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xMiddle, yBottom, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yTop, color, 0, 0, xMiddle, yTop, color, 0, 0, xRight, yMiddle,
                        color, 0, 0);
            break;

        case 6: // BR and TR (Right rectangle)
            AEGfxTriAdd(xMiddle, yBottom, color, 0, 0, xRight, yBottom, color, 0, 0, xMiddle, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yTop, color, 0, 0, xMiddle, yTop,
                        color, 0, 0);
            break;

        case 7: // BL, BR, and TR
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xRight, yBottom, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yTop, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yTop, color, 0, 0, xMiddle, yTop, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            break;

        case 8: // TL
            AEGfxTriAdd(xLeft, yTop, color, 0, 0, xLeft, yMiddle, color, 0, 0, xMiddle, yTop, color,
                        0, 0);
            break;

        case 9: // BL and TL (Left rectangle)
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xMiddle, yBottom, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xMiddle, yBottom, color, 0, 0, xMiddle, yTop, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            break;

        case 10: // BR and TL
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yMiddle, color, 0, 0, xMiddle,
                        yBottom, color, 0, 0);
            AEGfxTriAdd(xLeft, yTop, color, 0, 0, xLeft, yMiddle, color, 0, 0, xMiddle, yTop, color,
                        0, 0);
            break;

        case 11: // BL, BR, and TL
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xRight, yBottom, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yMiddle, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yMiddle, color, 0, 0, xMiddle, yTop, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            break;

        case 12: // TR and TL (Top rectangle)
            AEGfxTriAdd(xLeft, yMiddle, color, 0, 0, xRight, yMiddle, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yMiddle, color, 0, 0, xRight, yTop, color, 0, 0, xLeft, yTop, color,
                        0, 0);
            break;

        case 13: // BL, TR, and TL
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xMiddle, yBottom, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xMiddle, yBottom, color, 0, 0, xRight, yMiddle, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yMiddle, color, 0, 0, xRight, yTop, color, 0, 0, xLeft, yTop, color,
                        0, 0);
            break;

        case 14: // BR, TR, and TL
            AEGfxTriAdd(xMiddle, yBottom, color, 0, 0, xRight, yBottom, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yTop, color, 0, 0, xLeft, yMiddle,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yTop, color, 0, 0, xLeft, yTop, color, 0, 0, xLeft, yMiddle, color,
                        0, 0);
            break;

        case 15: // Full square
            AEGfxTriAdd(xLeft, yBottom, color, 0, 0, xRight, yBottom, color, 0, 0, xLeft, yTop,
                        color, 0, 0);
            AEGfxTriAdd(xRight, yBottom, color, 0, 0, xRight, yTop, color, 0, 0, xLeft, yTop, color,
                        0, 0);
            break;
        }
        meshLibrary_[i] = AEGfxMeshEnd();
    }
}

void Terrain::freeMeshLibrary() {
    for (auto& mesh : meshLibrary_) {
        AEGfxMeshFree(mesh);
    }
}

void Terrain::createColliderLibrary() {

    // Corners
    constexpr f32 xLeft{-0.5f}, xRight{0.5f}, yBottom{-0.5f}, yTop{0.5f};

    // Midpoints
    constexpr f32 xMiddle{0.0f}, yMiddle{0.0f};

    for (u32 i{0}; i < 16; ++i) {
        // Reset all to empty first
        for (u32 j{0}; j < 3; ++j) {
            colliderLibrary_[i][j].colliderShape_ = ColliderShape::Empty;
        }

        u32 colliderCount{0};

        switch (i) {
        case 0:
            break;

        case 1: // BL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xLeft, yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xMiddle,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            break;

        case 2: // BR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight,
                                                                                    yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xMiddle,
                                                                                    yBottom};
            break;

        case 3: // BL and BR (Bottom rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {0.0f, -0.25f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {1.0f, 0.5f};
            break;

        case 4: // TR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xMiddle, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xRight,
                                                                                    yMiddle};
            break;

        case 5: // BL and TR
            // BL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xLeft, yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xMiddle,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            colliderCount++;
            // TR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xMiddle, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xRight,
                                                                                    yMiddle};
            break;

        case 6: // BR and TR (Right rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {0.25f, 0.0f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {0.5f, 1.0f};
            break;

        case 7: // BL, BR, and TR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xLeft, yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xMiddle, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            break;

        case 8: // TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xLeft, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xLeft, yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xMiddle, yTop};
            break;

        case 9: // BL and TL (Left rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {-0.25f, 0.0f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {0.5f, 1.0f};
            break;

        case 10: // BR and TL
            // BR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight,
                                                                                    yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xMiddle,
                                                                                    yBottom};
            colliderCount++;
            // TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xLeft, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xLeft, yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xMiddle, yTop};
            break;

        case 11: // BL, BR, and TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xLeft, yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight,
                                                                                    yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight,
                                                                                    yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xMiddle, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yTop};
            break;

        case 12: // TR and TL (Top rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {0.0f, 0.25f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {1.0f, 0.5f};
            colliderCount++;
            break;

        case 13: // BL, TR, and TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xLeft, yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xMiddle,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xMiddle,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight,
                                                                                    yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight,
                                                                                    yMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yTop};
            break;

        case 14: // BR, TR, and TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xMiddle,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight,
                                                                                    yBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xRight, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {xRight, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {xLeft, yTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {xLeft, yMiddle};
            break;

        case 15: // Full square
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {0.0f, 0.0f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {1.0f, 1.0f};
            colliderCount++;
            break;
        }
    }
}

void Terrain::destroyTerrain(f32 worldX, f32 worldY) {
    // Calculate position relative to the terrain's bottom-left corner
    f32 localX{worldX - bottomLeftPos_.x};
    f32 localY{worldY - bottomLeftPos_.y};

    // Get the local position to the nearest Node index
    // Use round() to ensure we grab the corner closest to the click
    u32 col{static_cast<u32>(std::round(localX / kCellSize_))};
    u32 row{static_cast<u32>(std::round(localY / kCellSize_))};

    // Ensure the click is inside our grid
    if (col < kNodeCols_ && row < kNodeRows_) {

        // Update the node
        nodes_[row * kNodeCols_ + col] = 0.0f;

        // Update meshes
        initCellsGraphics(); // Unoptimal as all cells are being updated
        updateTerrain();
        initCellsCollider();
    }
}

void Terrain::destroyTerrainRadius(f32 worldX, f32 worldY, f32 radius) {
    bool changed{false};

    for (u32 r{0}; r < kNodeRows_; ++r) {
        for (u32 c{0}; c < kNodeCols_; ++c) {
            f32 nodeWorldX = bottomLeftPos_.x + (c * kCellSize_);
            f32 nodeWorldY = bottomLeftPos_.y + (r * kCellSize_);

            // Pythagorean distance check
            f32 dx = nodeWorldX - worldX;
            f32 dy = nodeWorldY - worldY;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                nodes_[r * kNodeCols_ + c] = 0.0f;
                changed = true;
            }
        }
    }

    if (changed) {
        std::cout << "terrain modified----------------------------\n";
        initCellsGraphics();
        updateTerrain();
        initCellsCollider();
    }
}

void Terrain::createDebugColliderMeshes() {
    // A thin "filled line" thickness in local space.
    // After scaling by cellSize (e.g. 32), this becomes visible.
    constexpr f32 t = 0.03f;
    constexpr u32 color = 0xFF00FFFF; // cyan (ARGB or ABGR depending on engine; tweak if needed)

    // -------- Triangle mesh (unit right-triangle placeholder) --------
    // We'll use this mesh by rebuilding per-triangle? No: we can just draw triangles directly
    // by creating a mesh per collider OR we can prebuild a generic triangle and ignore vertices.
    // Since your triangles differ per case, we won't use a single tri mesh for all.
    // So keep debugTriMesh_ unused or omit it. We'll render triangle colliders by building a mesh
    // on the fly.
    debugTriMesh_ = nullptr;

    // -------- Box outline mesh (centered square outline) --------
    // This mesh draws an outline around [-0.5..0.5] square using 4 thin rectangles.
    // We'll scale it later per collider size by temporarily scaling via transform if needed,
    // but easiest: build per box collider on the fly too (since sizes vary: 1x1, 1x0.5, 0.5x1).
    // So similarly, we can skip this library too.
    debugBoxMesh_ = nullptr;
}

void Terrain::renderCollidersDebug() const {
    // Debug render state
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.35f); // semi-transparent so you can see terrain under it
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 1.0f, 1.0f); // cyan tint (multiply)

    const u32 color = 0xFF00FFFF; // per-vertex color (format may vary; if odd, change)

    for (const Cell& cell : cells_) {
        // Use the same transform as terrain rendering
        AEMtx33 m = cell.transform_.worldMtx_; // makes a non-const copy
        AEGfxSetTransform(m.m);
        AEGfxTextureSet(nullptr, 0.0f, 0.0f); // no texture

        for (u32 i = 0; i < 3; ++i) {
            const Collider2D& col = cell.colliders_[i];

            if (col.colliderShape_ == ColliderShape::Empty) {
                continue;
            }

            if (col.colliderShape_ == ColliderShape::Box) {
                // Box is defined by offset_ (local) and size_ (local, relative to cell)
                const AEVec2 o = col.shapeData_.box_.offset_;
                const AEVec2 s = col.shapeData_.box_.size_;

                const f32 hx = s.x * 0.5f;
                const f32 hy = s.y * 0.5f;

                const f32 x0 = o.x - hx;
                const f32 x1 = o.x + hx;
                const f32 y0 = o.y - hy;
                const f32 y1 = o.y + hy;

                // Build and draw a rectangle (two triangles) in local space
                AEGfxMeshStart();
                AEGfxTriAdd(x0, y0, color, 0, 0, x1, y0, color, 0, 0, x0, y1, color, 0, 0);

                AEGfxTriAdd(x1, y0, color, 0, 0, x1, y1, color, 0, 0, x0, y1, color, 0, 0);

                AEGfxVertexList* mesh = AEGfxMeshEnd();
                AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
                AEGfxMeshFree(mesh);
            } else if (col.colliderShape_ == ColliderShape::Triangle) {
                const AEVec2 v0 = col.shapeData_.triangle_.vertices_[0];
                const AEVec2 v1 = col.shapeData_.triangle_.vertices_[1];
                const AEVec2 v2 = col.shapeData_.triangle_.vertices_[2];

                AEGfxMeshStart();
                AEGfxTriAdd(v0.x, v0.y, color, 0, 0, v1.x, v1.y, color, 0, 0, v2.x, v2.y, color, 0,
                            0);

                AEGfxVertexList* mesh = AEGfxMeshEnd();
                AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
                AEGfxMeshFree(mesh);
            }
        }
    }

    // restore transparency if needed by your pipeline (optional)
    AEGfxSetTransparency(1.0f);
}
