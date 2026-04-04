/*!
@file       DebugSystem.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "DebugSystem.h"

// Standard library
#include <cmath>
#include <cstdio>

// Third-party
#include <AEEngine.h>

// Project
#include "Collectible.h"
#include "CollisionSystem.h"
#include "Components.h"
#include "ConfigManager.h"
#include "FluidSystem.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "Terrain.h"
#include "Utils.h"
#include "VFXSystem.h"

DebugSystem g_debugSystem;

void DebugSystem::load(s8 font) {
    font_ = font;

    graphics_.mesh_ = createRectMesh();
    hudMesh_ = createRectMesh();
    wireRectMesh_ = createWireRectMesh();
    wireCircleMesh_ = createWireCircleMesh(24);
    wireLineMesh_ = createWireLineMesh();

    buttonClose_.loadMesh();
    buttonClose_.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
}

void DebugSystem::initFromJson(const std::string& file, const std::string& section) {
    const Json::Value& debugSection = g_configManager.getSection(file, section);
    graphics_.red_ = debugSection["graphics"]["red"].asFloat();
    graphics_.green_ = debugSection["graphics"]["green"].asFloat();
    graphics_.blue_ = debugSection["graphics"]["blue"].asFloat();
    graphics_.alpha_ = debugSection["graphics"]["alpha"].asFloat();

    headerText_.initFromJson(file, "Header");
    headerText_.font_ = font_;

    buttonClose_.initFromJson(file, section + "_CloseButton");
    buttonClose_.setTextFont(font_);

    // Load toggle buttons
    const Json::Value& layout = g_configManager.getSection("debug_system_buttons", "Layout");
    const Json::Value& keysList = g_configManager.getSection("debug_system_buttons", "Toggles");

    float startX = layout["startX"].asFloat();
    float startY = layout["startY"].asFloat();
    float spacingY = layout["spacingY"].asFloat();
    float cbSize = layout["checkboxSize"].asFloat();
    float labelOffX = layout["labelOffsetX"].asFloat();
    float labelScale = layout["labelScale"].asFloat();

    f32 halfWinW = static_cast<f32>(AEGfxGetWindowWidth()) / 2.0f;
    f32 halfWinH = static_cast<f32>(AEGfxGetWindowHeight()) / 2.0f;

    for (auto& t : toggles_) {
        t.checkbox_.unload();
    }

    toggles_.clear();
    options_.clear();

    for (int i = 0; i < static_cast<int>(keysList.size()); ++i) {
        std::string key = keysList[i].asString();
        float x = startX;
        float y = startY - i * spacingY;

        DebugToggle t;
        t.key_ = key;
        t.checkbox_.loadMesh();
        t.checkbox_.loadTexture("Assets/Textures/white_square.png");
        t.checkbox_.setTextFont(font_);
        t.checkbox_.setTransform({x, y}, {cbSize, cbSize});
        t.checkbox_.updateTransform();

        const Json::Value& entry = g_configManager.getSection("debug_system_buttons", key);
        t.hudFormat_ = entry.get("hudFormat", "").asString();
        t.label_.content_ = entry["content"].asString();
        t.label_.r_ = entry["red"].asFloat();
        t.label_.g_ = entry["green"].asFloat();
        t.label_.b_ = entry["blue"].asFloat();
        t.label_.a_ = entry["alpha"].asFloat();
        t.label_.scale_ = labelScale;
        t.label_.font_ = font_;
        t.label_.x_ = (x + cbSize / 2.0f + labelOffX) / halfWinW;
        t.label_.y_ = y / halfWinH;

        options_[key] = false;
        toggles_.push_back(std::move(t));
    }
}

void DebugSystem::unload() {
    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }
    if (hudMesh_ != nullptr) {
        AEGfxMeshFree(hudMesh_);
        hudMesh_ = nullptr;
    }
    if (wireRectMesh_ != nullptr) {
        AEGfxMeshFree(wireRectMesh_);
        wireRectMesh_ = nullptr;
    }
    if (wireCircleMesh_ != nullptr) {
        AEGfxMeshFree(wireCircleMesh_);
        wireCircleMesh_ = nullptr;
    }
    if (wireLineMesh_ != nullptr) {
        AEGfxMeshFree(wireLineMesh_);
        wireLineMesh_ = nullptr;
    }
    buttonClose_.unload();
    for (auto& t : toggles_)
        t.checkbox_.unload();
    toggles_.clear();
}

void DebugSystem::open() { open_ = true; }

void DebugSystem::close() { open_ = false; }

void DebugSystem::toggle() { open_ = !open_; }

bool DebugSystem::isOpen() const { return open_; }

void DebugSystem::updateTransform() {
    // Fill entire screen
    s32 windowWidth{AEGfxGetWindowWidth()};
    s32 windowHeight{AEGfxGetWindowHeight()};
    f32 worldMinX{AEGfxGetWinMinX()};
    f32 worldMinY{AEGfxGetWinMinY()};

    transform_.pos_ = {worldMinX + (static_cast<f32>(windowWidth) / 2.0f),
                       worldMinY + (static_cast<f32>(windowHeight) / 2.0f)};
    transform_.scale_ = {static_cast<f32>(windowWidth), static_cast<f32>(windowHeight)};
    transform_.rotationRad_ = 0.0f;

    AEMtx33 scaleMtx, rotMtx, transMtx;
    AEMtx33Scale(&scaleMtx, transform_.scale_.x, transform_.scale_.y);
    AEMtx33Rot(&rotMtx, transform_.rotationRad_);
    AEMtx33Trans(&transMtx, transform_.pos_.x, transform_.pos_.y);
    AEMtx33Concat(&transform_.worldMtx_, &rotMtx, &scaleMtx);
    AEMtx33Concat(&transform_.worldMtx_, &transMtx, &transform_.worldMtx_);

    buttonClose_.updateTransform();
}

void DebugSystem::renderBackground() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(graphics_.red_, graphics_.green_, graphics_.blue_, graphics_.alpha_);
    AEGfxSetTransform(transform_.worldMtx_.m);
    AEGfxMeshDraw(graphics_.mesh_, AE_GFX_MDM_TRIANGLES);
}

void DebugSystem::update() {
    if (!open_)
        return;

    updateTransform();

    if (buttonClose_.checkMouseClick())
        close();

    for (auto& t : toggles_) {
        if (t.checkbox_.checkMouseClick())
            options_[t.key_] = !options_[t.key_];
    }
}

void DebugSystem::draw() {
    if (!open_)
        return;

    renderBackground();

    headerText_.draw(true);

    buttonClose_.draw();

    for (auto& t : toggles_) {
        bool on = options_.count(t.key_) && options_.at(t.key_);
        t.checkbox_.setRGBA(on ? 0.0f : 0.3f, on ? 0.8f : 0.3f, on ? 0.0f : 0.3f, 1.0f);
        t.checkbox_.draw();
        t.label_.draw(false);
    }
}

void DebugSystem::drawHUD() {
    hudValues_["ShowFps"] = static_cast<float>(AEFrameRateControllerGetFrameRate());
    const auto& values = hudValues_;
    if (font_ == 0 || hudMesh_ == nullptr)
        return;

    // Count the number of active HUD lines (toggles that are on and have a hudFormat)
    int lineCount = 0;
    for (const auto& t : toggles_) {
        if (!t.hudFormat_.empty() && options_.count(t.key_) && options_.at(t.key_))
            lineCount++;
    }
    if (lineCount == 0)
        return;

    const f32 padding = 10.0f;
    const f32 lineHeight = 26.0f;
    const f32 boxWidth = 240.0f;
    const f32 textScale = 0.45f;

    f32 boxHeight = padding * 2.0f + lineHeight * static_cast<f32>(lineCount);
    f32 halfW = static_cast<f32>(AEGfxGetWindowWidth()) / 2.0f;
    f32 halfH = static_cast<f32>(AEGfxGetWindowHeight()) / 2.0f;

    f32 boxCX = -halfW + boxWidth / 2.0f;
    f32 boxCY = halfH - boxHeight / 2.0f;

    // Draw translucent black background
    AEMtx33 scaleMtx, transMtx, worldMtx;
    AEMtx33Scale(&scaleMtx, boxWidth, boxHeight);
    AEMtx33Trans(&transMtx, boxCX, boxCY);
    AEMtx33Concat(&worldMtx, &transMtx, &scaleMtx);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.7f);
    AEGfxSetTransform(worldMtx.m);
    AEGfxMeshDraw(hudMesh_, AE_GFX_MDM_TRIANGLES);

    // Draw text lines from top of box downward
    f32 textX = (-halfW + padding) / halfW;
    f32 textY = (halfH - padding - lineHeight) / halfH;
    f32 step = lineHeight / halfH;

    char buf[64];

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    for (const auto& t : toggles_) {
        if (t.hudFormat_.empty() || !options_.count(t.key_) || !options_.at(t.key_)) {
            continue;
        }

        float val = values.count(t.key_) ? values.at(t.key_) : 0.f;
        snprintf(buf, sizeof(buf), t.hudFormat_.c_str(), val);
        AEGfxPrint(font_, buf, textX, textY, textScale, 1.f, 1.f, 1.f, 1.f);
        textY -= step;
    }
}

void DebugSystem::setScene(Terrain* dirt, Terrain* stone, Terrain* magic, FluidSystem* fluidSystem,
                           CollectibleSystem* collectibles, PortalSystem* portals,
                           StartEndPoint* startEnd, VFXSystem* vfx) {
    dirt_ = dirt;
    stone_ = stone;
    magic_ = magic;
    fluidSystem_ = fluidSystem;
    collectibles_ = collectibles;
    portals_ = portals;
    startEnd_ = startEnd;
    vfx_ = vfx;
}

void DebugSystem::clearScene() {
    dirt_ = stone_ = magic_ = nullptr;
    fluidSystem_ = nullptr;
    collectibles_ = nullptr;
    portals_ = nullptr;
    startEnd_ = nullptr;
    vfx_ = nullptr;
}

void DebugSystem::drawAll() {
    u32 totalFluidParticles = 0;
    if (fluidSystem_) {
        for (int fi = 0; fi < static_cast<int>(FluidType::Count); ++fi) {
            totalFluidParticles += fluidSystem_->getParticleCount(static_cast<FluidType>(fi));
        }
    }
    hudValues_["ShowFluidParticleCount"] = static_cast<float>(totalFluidParticles);
    hudValues_["ShowVfxParticleCount"] =
        vfx_ ? static_cast<float>(vfx_->getActiveParticleCount()) : 0.0f;
    hudValues_["ShowCollisionCount"] =
        static_cast<float>(CollisionSystem::getLastFrameCollisionCount());
    CollisionSystem::resetCollisionCount();

    if (startEnd_) {
        const bool unlimitedWater =
            options_.count("UnlimitedWater") && options_.at("UnlimitedWater");
        for (auto& sp : startEnd_->startPoints_) {
            if (sp.active_ && sp.type_ == StartEndType::Pipe)
                sp.infiniteWater_ = unlimitedWater;
        }
    }

    if (dirt_)
        drawTerrainColliders(*dirt_);
    if (stone_)
        drawTerrainColliders(*stone_);
    if (magic_)
        drawTerrainColliders(*magic_);
    if (fluidSystem_)
        drawFluidColliders(*fluidSystem_);
    if (collectibles_)
        drawCollectibleColliders(*collectibles_);
    if (portals_)
        drawPortalColliders(*portals_);
    if (startEnd_)
        drawStartEndColliders(*startEnd_);

    if (isOpen())
        draw();

    drawHUD();
}

void DebugSystem::drawSingleCollider(const Transform& transform, const Collider2D& col) {
    AEMtx33 scale, rot, trans, world;

    if (col.colliderShape_ == ColliderShape::Box) {
        const AEVec2 center{transform.pos_.x + col.shapeData_.box_.offset_.x,
                            transform.pos_.y + col.shapeData_.box_.offset_.y};
        AEMtx33Scale(&scale, col.shapeData_.box_.size_.x, col.shapeData_.box_.size_.y);
        AEMtx33Rot(&rot, transform.rotationRad_);
        AEMtx33Trans(&trans, center.x, center.y);
        AEMtx33Concat(&world, &rot, &scale);
        AEMtx33Concat(&world, &trans, &world);
        AEGfxSetTransform(world.m);
        AEGfxMeshDraw(wireRectMesh_, AE_GFX_MDM_LINES_STRIP);
    } else if (col.colliderShape_ == ColliderShape::Circle) {
        const AEVec2 center{transform.pos_.x + col.shapeData_.circle_.offset_.x,
                            transform.pos_.y + col.shapeData_.circle_.offset_.y};
        const f32 diameter = col.shapeData_.circle_.radius_ * 2.0f;
        AEMtx33Scale(&scale, diameter, diameter);
        AEMtx33Trans(&trans, center.x, center.y);
        AEMtx33Concat(&world, &trans, &scale);
        AEGfxSetTransform(world.m);
        AEGfxMeshDraw(wireCircleMesh_, AE_GFX_MDM_LINES_STRIP);
    }
}

void DebugSystem::drawCollectibleColliders(CollectibleSystem& system) {
    if (!options_.count("RenderColliders") || !options_.at("RenderColliders"))
        return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);

    for (const auto& c : system.getCollectibles()) {
        if (!c.active_ || c.collected_)
            continue;
        drawSingleCollider(c.transform_, c.collider_);
    }
}

void DebugSystem::drawPortalColliders(PortalSystem& system) {
    if (!options_.count("RenderColliders") || !options_.at("RenderColliders"))
        return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);

    for (const Portal* p : system.getPortals()) {
        drawSingleCollider(p->transform_, p->collider_);
    }
}

void DebugSystem::drawStartEndColliders(StartEndPoint& system) {
    if (!options_.count("RenderColliders") || !options_.at("RenderColliders"))
        return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);

    for (const auto& sp : system.startPoints_) {
        drawSingleCollider(sp.transform_, sp.collider_);
    }
    drawSingleCollider(system.endPoint_.transform_, system.endPoint_.collider_);
}

void DebugSystem::drawTerrainColliders(Terrain& terrain) {
    if (!options_.count("RenderColliders") || !options_.at("RenderColliders"))
        return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f); // bright green

    AEMtx33 scale, rot, trans, world;

    // Terrain colliders
    const auto& cells = terrain.getCells();
    for (const auto& cell : cells) {
        for (int i = 0; i < 3; ++i) {
            const Collider2D& col = cell.colliders_[i];
            if (col.colliderShape_ == ColliderShape::Empty)
                continue;

            if (col.colliderShape_ == ColliderShape::Box) {
                const AEVec2 worldCenter{cell.transform_.pos_.x + col.shapeData_.box_.offset_.x *
                                                                      cell.transform_.scale_.x,
                                         cell.transform_.pos_.y + col.shapeData_.box_.offset_.y *
                                                                      cell.transform_.scale_.y};
                const AEVec2 worldSize{col.shapeData_.box_.size_.x * cell.transform_.scale_.x,
                                       col.shapeData_.box_.size_.y * cell.transform_.scale_.y};
                AEMtx33Scale(&scale, worldSize.x, worldSize.y);
                AEMtx33Trans(&trans, worldCenter.x, worldCenter.y);
                AEMtx33Concat(&world, &trans, &scale);
                AEGfxSetTransform(world.m);
                AEGfxMeshDraw(wireRectMesh_, AE_GFX_MDM_LINES_STRIP);

            } else if (col.colliderShape_ == ColliderShape::Triangle) {
                AEVec2 verts[3];
                for (int v = 0; v < 3; ++v) {
                    verts[v] = {col.shapeData_.triangle_.vertices_[v].x * cell.transform_.scale_.x +
                                    cell.transform_.pos_.x,
                                col.shapeData_.triangle_.vertices_[v].y * cell.transform_.scale_.y +
                                    cell.transform_.pos_.y};
                }
                for (int e = 0; e < 3; ++e) {
                    const AEVec2& a = verts[e];
                    const AEVec2& b = verts[(e + 1) % 3];
                    const f32 dx = b.x - a.x, dy = b.y - a.y;
                    const f32 len = std::sqrt(dx * dx + dy * dy);
                    if (len < 0.001f)
                        continue;
                    AEMtx33Scale(&scale, len, 1.0f);
                    AEMtx33Rot(&rot, std::atan2(dy, dx));
                    AEMtx33Trans(&trans, a.x, a.y);
                    AEMtx33Concat(&world, &rot, &scale);
                    AEMtx33Concat(&world, &trans, &world);
                    AEGfxSetTransform(world.m);
                    AEGfxMeshDraw(wireLineMesh_, AE_GFX_MDM_LINES_STRIP);
                }
            }
        }
    }
}

void DebugSystem::drawFluidColliders(FluidSystem& fluidSystem) {
    if (!options_.count("RenderColliders") || !options_.at("RenderColliders"))
        return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);

    for (int fi = 0; fi < static_cast<int>(FluidType::Count); ++fi) {
        const auto& pool = fluidSystem.getParticlePool(static_cast<FluidType>(fi));
        for (const auto& p : pool) {
            drawSingleCollider(p.transform_, p.collider_);
        }
    }
}
