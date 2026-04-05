/*!
@file       TileBackground.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the definition of functions that make the TiledBackground
            class, which loads, draws, and unloads a screen-filling tiled texture.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "TileBackground.h"

// Standard library
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "ConfigManager.h"

// =========================================================
//
// TiledBackground::loadFromJson
//
// Reads half-extents and tile repeat counts from JSON,
// loads the texture, and builds a two-triangle mesh that
// spans the full background with tiled UV coordinates.
//
// =========================================================
void TiledBackground::loadFromJson(const std::string& file, const std::string& section) {
    const Json::Value& s = g_configManager.getSection(file, section);
    f32 halfW = s.get("halfW", 800.0f).asFloat();
    f32 halfH = s.get("halfH", 450.0f).asFloat();
    f32 tileX = s.get("tileX", 8.0f).asFloat();
    f32 tileY = s.get("tileY", 4.5f).asFloat();

    graphics_.texture_ = AEGfxTextureLoad(s["texture"].asCString());

    AEGfxMeshStart();
    AEGfxTriAdd(-halfW, -halfH, 0xFFFFFFFF, 0.0f, tileY, halfW, -halfH, 0xFFFFFFFF, tileX, tileY,
                -halfW, halfH, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(halfW, -halfH, 0xFFFFFFFF, tileX, tileY, halfW, halfH, 0xFFFFFFFF, tileX, 0.0f,
                -halfW, halfH, 0xFFFFFFFF, 0.0f, 0.0f);
    graphics_.mesh_ = AEGfxMeshEnd();
}

// =========================================================
//
// TiledBackground::draw
//
// Renders the tiled background at the identity transform
// so it stays fixed in screen space regardless of camera.
//
// =========================================================
void TiledBackground::draw() const {
    if (!graphics_.texture_ || !graphics_.mesh_)
        return;
    AEMtx33 identity;
    AEMtx33Identity(&identity);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(graphics_.texture_, 0.0f, 0.0f);
    AEGfxSetTransform(identity.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
}

// =========================================================
//
// TiledBackground::unload
//
// Frees the texture and mesh, nulling both pointers.
//
// =========================================================
void TiledBackground::unload() {
    if (graphics_.texture_) {
        AEGfxTextureUnload(graphics_.texture_);
        graphics_.texture_ = nullptr;
    }
    if (graphics_.mesh_) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }
}