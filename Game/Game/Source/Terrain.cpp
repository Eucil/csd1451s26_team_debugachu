/*!
@file       Terrain.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the implementation of the Terrain class,
            including marching-squares mesh and collider generation, node-based
            terrain editing, and per-cell transform and render updates.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#include "Terrain.h"

// Standard library
#include <iostream>
#include <vector>

// Third-party
#include <AEEngine.h>

// Project
#include "CollisionSystem.h"

AEGfxVertexList* Terrain::meshLibrary_[16]{nullptr}; // There are 16 possible meshes
Collider2D Terrain::colliderLibrary_[16][3]{}; // There are 16 possible colliders, [3] as each cell
                                               // uses 1, 2, or 3 colliders
AEGfxVertexList* Terrain::debugTriMesh_{nullptr};
AEGfxVertexList* Terrain::debugBoxMesh_{nullptr};

// =========================================================
//
// Terrain::Terrain
//
// Constructs the terrain grid by initialising dimensions,
// allocating cell and node vectors, and zeroing all nodes.
//
// =========================================================
Terrain::Terrain(TerrainMaterial terrainMaterial, AEGfxTexture* pTex, AEVec2 centerPosition,
                 u32 cellRows, u32 cellCols, u32 cellSize, bool collidable)
    : terrainMaterial_(terrainMaterial), kCellRows_(cellRows), kCellCols_(cellCols),
      kCellSize_(cellSize), kNodeRows_(kCellRows_ + 1), kNodeCols_(kCellCols_ + 1),
      cells_(kCellRows_ * kCellCols_), nodes_(kNodeRows_ * kNodeCols_), collidable_{collidable} {

    transform_.pos_ = centerPosition;

    graphics_.texture_ = pTex;

    // Calculate half the width and height of the whole terrain
    halfWidth_ = (static_cast<f32>(kCellCols_) * kCellSize_) / 2.0f;
    halfHeight_ = (static_cast<f32>(kCellRows_) * kCellSize_) / 2.0f;

    // Calculate the bottom-left corner of the terrain
    bottomLeftPos_ = {transform_.pos_.x - halfWidth_, transform_.pos_.y - halfHeight_};

    // Initialize nodes to random values for testing
    for (u32 r{0}; r < kNodeRows_; ++r) {
        for (u32 c{0}; c < kNodeCols_; ++c) {
            nodes_[static_cast<size_t>(r) * kNodeCols_ + c] = 0.0f;
        }
    }
}

// =========================================================
//
// Terrain::initCellsTransform
//
// Sets the world position and scale for every cell based
// on the terrain's bottom-left corner and cell size.
//
// =========================================================
void Terrain::initCellsTransform() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            Cell& cell = cells_[static_cast<size_t>(r) * kCellCols_ + c];

            // Set position and scale
            cell.transform_.pos_.x = bottomLeftPos_.x + (c + 0.5f) * kCellSize_;
            cell.transform_.pos_.y = bottomLeftPos_.y + (r + 0.5f) * kCellSize_;
            cell.transform_.scale_ = {(f32)kCellSize_, (f32)kCellSize_};
        }
    }
}

// =========================================================
//
// Terrain::initCellsGraphics
//
// Assigns the correct mesh from the mesh library to each
// cell by computing the 4-bit marching-squares node mask.
//
// =========================================================
void Terrain::initCellsGraphics() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            // Determine mesh case (TL=8, TR=4, BR=2, BL=1)
            u32 index{0};
            if (nodes_[(static_cast<size_t>(r) + 1) * kNodeCols_ + c] >= threshold_)
                index |= 8;
            if (nodes_[(static_cast<size_t>(r) + 1) * kNodeCols_ + c + 1] >= threshold_)
                index |= 4;
            if (nodes_[static_cast<size_t>(r) * kNodeCols_ + c + 1] >= threshold_)
                index |= 2;
            if (nodes_[static_cast<size_t>(r) * kNodeCols_ + c] >= threshold_)
                index |= 1;

            cells_[static_cast<size_t>(r) * kCellCols_ + c].graphics_.mesh_ = meshLibrary_[index];
        }
    }
}

// =========================================================
//
// Terrain::initCellsCollider
//
// Assigns colliders from the collider library to each cell
// using the same 4-bit node mask. Skipped when collidable_
// is false (leaves all colliders as Empty).
//
// =========================================================
void Terrain::initCellsCollider() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            u32 index{0};
            // If collidable_ is false, keep index at 0 (no colliders)
            if (collidable_ == true) {
                // Determine collider case (TL=8, TR=4, BR=2, BL=1)
                if (nodes_[static_cast<size_t>(r + 1) * kNodeCols_ + c] >= threshold_)
                    index |= 8;
                if (nodes_[static_cast<size_t>(r + 1) * kNodeCols_ + c + 1] >= threshold_)
                    index |= 4;
                if (nodes_[static_cast<size_t>(r) * kNodeCols_ + c + 1] >= threshold_)
                    index |= 2;
                if (nodes_[static_cast<size_t>(r) * kNodeCols_ + c] >= threshold_)
                    index |= 1;
            }

            Cell& cell = cells_[static_cast<size_t>(r) * kCellCols_ + c];

            // Assign the collider shape from the library
            for (u32 i{0}; i < 3; ++i) {
                cell.colliders_[i] = colliderLibrary_[index][i];
            }
        }
    }
}

// =========================================================
//
// Terrain::updateTerrain
//
// Rebuilds the world matrix for every cell from its stored
// position, rotation, and scale.
//
// =========================================================
void Terrain::updateTerrain() {
    for (u32 r{0}; r < kCellRows_; ++r) {
        for (u32 c{0}; c < kCellCols_; ++c) {
            Cell& cell = cells_[static_cast<size_t>(r) * kCellCols_ + c];

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

// =========================================================
//
// Terrain::destroyAtMouse
//
// Converts the current mouse cursor position to world space
// and forwards it to destroyTerrainRadius.
//
// =========================================================
bool Terrain::destroyAtMouse(f32 radius) {
    s32 screenX, screenY;
    AEInputGetCursorPosition(&screenX, &screenY);

    // Convert screen to world coordinates
    f32 worldX{static_cast<f32>(screenX) - (AEGfxGetWindowWidth() / 2.0f)};
    f32 worldY{(AEGfxGetWindowHeight() / 2.0f) - static_cast<f32>(screenY)};

    return destroyTerrainRadius(worldX, worldY, radius);
}

// =========================================================
//
// Terrain::buildAtMouse
//
// Converts the current mouse cursor position to world space
// and forwards it to buildTerrainRadius.
//
// =========================================================
void Terrain::buildAtMouse(f32 radius) {
    s32 screenX, screenY;
    AEInputGetCursorPosition(&screenX, &screenY);

    // Convert screen to world coordinates
    f32 worldX{static_cast<f32>(screenX) - (AEGfxGetWindowWidth() / 2.0f)};
    f32 worldY{(AEGfxGetWindowHeight() / 2.0f) - static_cast<f32>(screenY)};

    buildTerrainRadius(worldX, worldY, radius);
}

// =========================================================
//
// Terrain::renderTerrain
//
// Draws every cell using the terrain texture, offsetting
// the UV per column and row to tile the spritesheet atlas.
//
// =========================================================
void Terrain::renderTerrain() {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    /*
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
    */

    for (u32 y{0}; y < kCellRows_; ++y) {
        for (u32 x{0}; x < kCellCols_; ++x) {
            Cell& cell{cells_[static_cast<size_t>(y) * kCellCols_ + x]};

            AEGfxSetTransform(cell.transform_.worldMtx_.m);
            AEGfxTextureSet(graphics_.texture_, x * (1.0f / 16.0f), y * -(1.0f / 16.0f));
            AEGfxMeshDraw(cell.graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
        }
    }
}

// =========================================================
//
// Terrain::createMeshLibrary
//
// Builds all 16 marching-squares meshes and stores them in
// the shared meshLibrary_ array. Each entry corresponds to
// a unique combination of the four corner node bits.
//
// =========================================================
void Terrain::createMeshLibrary() {
    // Corners (Positions)
    constexpr f32 kXLeft{-0.5f}, kXRight{0.5f}, kYBottom{-0.5f}, kYTop{0.5f};

    // Corners (Texture coordinates)
    constexpr f32 kXTekXLeft{kXLeft * (1.0f / 16.0f) + (1 / 16.0f * 2.0f)};
    constexpr f32 kXTekXRight{kXRight * (1.0f / 16.0f) + (1 / 16.0f * 2.0f)};
    constexpr f32 kYTexBottom{kYBottom * -(1.0f / 16.0f) + (1 / 16.0f * 2.0f)};
    constexpr f32 kYTexTop{kYTop * -(1.0f / 16.0f) + (1 / 16.0f * 2.0f)};

    // Midpoints (Positions)
    constexpr f32 kXMiddle{0.0f}, kYMiddle{0.0f};

    // Midpoints (Texture coordinates)
    constexpr f32 kXTekXMiddle{kXMiddle * (1.0f / 16.0f) + (1 / 16.0f * 2.0f)};
    constexpr f32 kYTekXMiddle{kYMiddle * -(1.0f / 16.0f) + (1 / 16.0f * 2.0f)};

    u32 color{0xFFFFFFFF};

    for (u32 i{0}; i < 16; ++i) {
        AEGfxMeshStart();
        switch (i) {
        case 0:
            break; // Empty

        case 1: // BL
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXMiddle, kYBottom, color,
                        kXTekXMiddle, kYTexBottom, kXLeft, kYMiddle, color, kXTekXLeft,
                        kYTekXMiddle);
            break;

        case 2: // BR
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYMiddle,
                        color, kXTekXRight, kYTekXMiddle, kXMiddle, kYBottom, color, kXTekXMiddle,
                        kYTexBottom);
            break;

        case 3: // BL and BR (Bottom rectangle)
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXRight, kYBottom, color,
                        kXTekXRight, kYTexBottom, kXLeft, kYMiddle, color, kXTekXLeft,
                        kYTekXMiddle);
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYMiddle,
                        color, kXTekXRight, kYTekXMiddle, kXLeft, kYMiddle, color, kXTekXLeft,
                        kYTekXMiddle);
            break;

        case 4: // TR
            AEGfxTriAdd(kXRight, kYTop, color, kXTekXRight, kYTexTop, kXMiddle, kYTop, color,
                        kXTekXMiddle, kYTexTop, kXRight, kYMiddle, color, kXTekXRight,
                        kYTekXMiddle);
            break;

        case 5: // BL and TR
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXMiddle, kYBottom, color,
                        kXTekXMiddle, kYTexBottom, kXLeft, kYMiddle, color, kXTekXLeft,
                        kYTekXMiddle);
            AEGfxTriAdd(kXRight, kYTop, color, kXTekXRight, kYTexTop, kXMiddle, kYTop, color,
                        kXTekXMiddle, kYTexTop, kXRight, kYMiddle, color, kXTekXRight,
                        kYTekXMiddle);
            break;

        case 6: // BR and TR (Right rectangle)
            AEGfxTriAdd(kXMiddle, kYBottom, color, kXTekXMiddle, kYTexBottom, kXRight, kYBottom,
                        color, kXTekXRight, kYTexBottom, kXMiddle, kYTop, color, kXTekXMiddle,
                        kYTexTop);
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYTop, color,
                        kXTekXRight, kYTexTop, kXMiddle, kYTop, color, kXTekXMiddle, kYTexTop);
            break;

        case 7: // BL, BR, and TR
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXRight, kYBottom, color,
                        kXTekXRight, kYTexBottom, kXLeft, kYMiddle, color, kXTekXLeft,
                        kYTekXMiddle);
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYTop, color,
                        kXTekXRight, kYTexTop, kXLeft, kYMiddle, color, kXTekXLeft, kYTekXMiddle);
            AEGfxTriAdd(kXRight, kYTop, color, kXTekXRight, kYTexTop, kXMiddle, kYTop, color,
                        kXTekXMiddle, kYTexTop, kXLeft, kYMiddle, color, kXTekXLeft, kYTekXMiddle);
            break;

        case 8: // TL
            AEGfxTriAdd(kXLeft, kYTop, color, kXTekXLeft, kYTexTop, kXLeft, kYMiddle, color,
                        kXTekXLeft, kYTekXMiddle, kXMiddle, kYTop, color, kXTekXMiddle, kYTexTop);
            break;

        case 9: // BL and TL (Left rectangle)
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXMiddle, kYBottom, color,
                        kXTekXMiddle, kYTexBottom, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            AEGfxTriAdd(kXMiddle, kYBottom, color, kXTekXMiddle, kYTexBottom, kXMiddle, kYTop,
                        color, kXTekXMiddle, kYTexTop, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            break;

        case 10: // BR and TL
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYMiddle,
                        color, kXTekXRight, kYTekXMiddle, kXMiddle, kYBottom, color, kXTekXMiddle,
                        kYTexBottom);
            AEGfxTriAdd(kXLeft, kYTop, color, kXTekXLeft, kYTexTop, kXLeft, kYMiddle, color,
                        kXTekXLeft, kYTekXMiddle, kXMiddle, kYTop, color, kXTekXMiddle, kYTexTop);
            break;

        case 11: // BL, BR, and TL
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXRight, kYBottom, color,
                        kXTekXRight, kYTexBottom, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYMiddle,
                        color, kXTekXRight, kYTekXMiddle, kXLeft, kYTop, color, kXTekXLeft,
                        kYTexTop);
            AEGfxTriAdd(kXRight, kYMiddle, color, kXTekXRight, kYTekXMiddle, kXMiddle, kYTop, color,
                        kXTekXMiddle, kYTexTop, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            break;

        case 12: // TR and TL (Top rectangle)
            AEGfxTriAdd(kXLeft, kYMiddle, color, kXTekXLeft, kYTekXMiddle, kXRight, kYMiddle, color,
                        kXTekXRight, kYTekXMiddle, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            AEGfxTriAdd(kXRight, kYMiddle, color, kXTekXRight, kYTekXMiddle, kXRight, kYTop, color,
                        kXTekXRight, kYTexTop, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            break;

        case 13: // BL, TR, and TL
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXMiddle, kYBottom, color,
                        kXTekXMiddle, kYTexBottom, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            AEGfxTriAdd(kXMiddle, kYBottom, color, kXTekXMiddle, kYTexBottom, kXRight, kYMiddle,
                        color, kXTekXRight, kYTekXMiddle, kXLeft, kYTop, color, kXTekXLeft,
                        kYTexTop);
            AEGfxTriAdd(kXRight, kYMiddle, color, kXTekXRight, kYTekXMiddle, kXRight, kYTop, color,
                        kXTekXRight, kYTexTop, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            break;

        case 14: // BR, TR, and TL
            AEGfxTriAdd(kXMiddle, kYBottom, color, kXTekXMiddle, kYTexBottom, kXRight, kYBottom,
                        color, kXTekXRight, kYTexBottom, kXLeft, kYMiddle, color, kXTekXLeft,
                        kYTekXMiddle);
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYTop, color,
                        kXTekXRight, kYTexTop, kXLeft, kYMiddle, color, kXTekXLeft, kYTekXMiddle);
            AEGfxTriAdd(kXRight, kYTop, color, kXTekXRight, kYTexTop, kXLeft, kYTop, color,
                        kXTekXLeft, kYTexTop, kXLeft, kYMiddle, color, kXTekXLeft, kYTekXMiddle);
            break;

        case 15: // Full square
            AEGfxTriAdd(kXLeft, kYBottom, color, kXTekXLeft, kYTexBottom, kXRight, kYBottom, color,
                        kXTekXRight, kYTexBottom, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            AEGfxTriAdd(kXRight, kYBottom, color, kXTekXRight, kYTexBottom, kXRight, kYTop, color,
                        kXTekXRight, kYTexTop, kXLeft, kYTop, color, kXTekXLeft, kYTexTop);
            break;
        }
        meshLibrary_[i] = AEGfxMeshEnd();
    }
}

// =========================================================
//
// Terrain::freeMeshLibrary
//
// Frees all 16 GPU meshes stored in meshLibrary_.
//
// =========================================================
void Terrain::freeMeshLibrary() {
    for (auto& mesh : meshLibrary_) {
        AEGfxMeshFree(mesh);
    }
}

// =========================================================
//
// Terrain::createColliderLibrary
//
// Populates colliderLibrary_ with up to three collider
// shapes per marching-squares case (box or triangle),
// matching the geometry built by createMeshLibrary.
//
// =========================================================
void Terrain::createColliderLibrary() {

    // Corners
    constexpr f32 kXLeft{-0.5f}, kXRight{0.5f}, kYBottom{-0.5f}, kYTop{0.5f};

    // Midpoints
    constexpr f32 kXMiddle{0.0f}, kYMiddle{0.0f};

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
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXLeft,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXMiddle,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
            break;

        case 2: // BR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXMiddle,
                                                                                    kYBottom};
            break;

        case 3: // BL and BR (Bottom rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {0.0f, -0.25f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {1.0f, 0.5f};
            break;

        case 4: // TR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXMiddle,
                                                                                    kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXRight,
                                                                                    kYMiddle};
            break;

        case 5: // BL and TR
            // BL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXLeft,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXMiddle,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
            colliderCount++;
            // TR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXMiddle,
                                                                                    kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXRight,
                                                                                    kYMiddle};
            break;

        case 6: // BR and TR (Right rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {0.25f, 0.0f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {0.5f, 1.0f};
            break;

        case 7: // BL, BR, and TR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXLeft,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXMiddle,
                                                                                    kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
            break;

        case 8: // TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXLeft, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXLeft,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXMiddle,
                                                                                    kYTop};
            break;

        case 9: // BL and TL (Left rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {-0.25f, 0.0f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {0.5f, 1.0f};
            break;

        case 10: // BR and TL
            // BR
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXMiddle,
                                                                                    kYBottom};
            colliderCount++;
            // TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXLeft, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXLeft,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXMiddle,
                                                                                    kYTop};
            break;

        case 11: // BL, BR, and TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXLeft,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft, kYTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft, kYTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXMiddle,
                                                                                    kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft, kYTop};
            break;

        case 12: // TR and TL (Top rectangle)
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Box;
            colliderLibrary_[i][colliderCount].shapeData_.box_.offset_ = {0.0f, 0.25f};
            colliderLibrary_[i][colliderCount].shapeData_.box_.size_ = {1.0f, 0.5f};
            colliderCount++;
            break;

        case 13: // BL, TR, and TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXLeft,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXMiddle,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft, kYTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXMiddle,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft, kYTop};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight,
                                                                                    kYMiddle};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft, kYTop};
            break;

        case 14: // BR, TR, and TL
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXMiddle,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight,
                                                                                    kYBottom};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXRight, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
            colliderCount++;
            colliderLibrary_[i][colliderCount].colliderShape_ = ColliderShape::Triangle;
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[0] = {kXRight, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[1] = {kXLeft, kYTop};
            colliderLibrary_[i][colliderCount].shapeData_.triangle_.vertices_[2] = {kXLeft,
                                                                                    kYMiddle};
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

// =========================================================
//
// Terrain::destroyTerrain
//
// Zeroes the node nearest to the given world position and
// refreshes meshes, matrices, and colliders.
//
// =========================================================
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
        nodes_[static_cast<size_t>(row) * kNodeCols_ + col] = 0.0f;

        // Update meshes
        initCellsGraphics(); // Unoptimal as all cells are being updated
        updateTerrain();
        initCellsCollider();
        markCollidersCacheDirty();
    }
}

// =========================================================
//
// Terrain::destroyTerrainRadius
//
// Zeroes all nodes within a circular radius of the given
// world position, then refreshes meshes and colliders.
// Returns true if any node was changed.
//
// =========================================================
bool Terrain::destroyTerrainRadius(f32 worldX, f32 worldY, f32 radius) {
    bool changed{false};

    for (u32 r{0}; r < kNodeRows_; ++r) {
        for (u32 c{0}; c < kNodeCols_; ++c) {
            f32 nodeWorldX = bottomLeftPos_.x + (c * kCellSize_);
            f32 nodeWorldY = bottomLeftPos_.y + (r * kCellSize_);

            // Pythagorean distance check
            f32 dx = nodeWorldX - worldX;
            f32 dy = nodeWorldY - worldY;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                size_t nodeIndex = static_cast<size_t>(r) * kNodeCols_ + c;
                if (nodes_[nodeIndex] > 0.0f) {
                    nodes_[nodeIndex] = 0.0f;
                    changed = true;
                }
            }
        }
    }

    if (changed) {
        // std::cout << "terrain modified----------------------------\n";
        initCellsGraphics();
        updateTerrain();
        initCellsCollider();
        markCollidersCacheDirty();
    }
    return changed;
}

// =========================================================
//
// Terrain::buildTerrainRadius
//
// Sets all nodes within a circular radius of the given
// world position to 1.0, then refreshes meshes and
// colliders if any node changed.
//
// =========================================================
void Terrain::buildTerrainRadius(f32 worldX, f32 worldY, f32 radius) {
    bool changed{false};

    for (u32 r{0}; r < kNodeRows_; ++r) {
        for (u32 c{0}; c < kNodeCols_; ++c) {
            f32 nodeWorldX = bottomLeftPos_.x + (c * kCellSize_);
            f32 nodeWorldY = bottomLeftPos_.y + (r * kCellSize_);

            // Pythagorean distance check
            f32 dx = nodeWorldX - worldX;
            f32 dy = nodeWorldY - worldY;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                nodes_[static_cast<size_t>(r) * kNodeCols_ + c] = 1.0f;
                changed = true;
            }
        }
    }

    if (changed) {
        // std::cout << "terrain modified----------------------------\n";
        initCellsGraphics();
        updateTerrain();
        initCellsCollider();
        markCollidersCacheDirty();
    }
}

// =========================================================
//
// Terrain::isNearestNodeToMouseAtThreshold
//
// Returns true if the node nearest to the current mouse
// cursor is exactly at the threshold value.
//
// =========================================================
bool Terrain::isNearestNodeToMouseAtThreshold() {
    s32 screenX{0}, screenY{0};
    AEInputGetCursorPosition(&screenX, &screenY);

    // Convert screen to world coordinates
    f32 worldX{static_cast<f32>(screenX) - (AEGfxGetWindowWidth() / 2.0f)};
    f32 worldY{(AEGfxGetWindowHeight() / 2.0f) - static_cast<f32>(screenY)};

    f32 localX{worldX - bottomLeftPos_.x};
    f32 localY{worldY - bottomLeftPos_.y};
    u32 col{static_cast<u32>(std::round(localX / kCellSize_))};
    u32 row{static_cast<u32>(std::round(localY / kCellSize_))};
    if (col < kNodeCols_ && row < kNodeRows_) {
        return nodes_[static_cast<size_t>(row) * kNodeCols_ + col] == threshold_;
    }
    return false;
}
