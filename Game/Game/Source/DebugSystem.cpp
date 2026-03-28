#include "DebugSystem.h"

#include <AEEngine.h>

#include "Components.h"
#include "ConfigManager.h"
#include "Utils.h"

DebugSystem g_debugSystem;

void DebugSystem::load(s8 font) {
    font_ = font;

    graphics_.mesh_ = CreateRectMesh();

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
}

void DebugSystem::unload() {
    if (graphics_.mesh_ != nullptr) {
        AEGfxMeshFree(graphics_.mesh_);
        graphics_.mesh_ = nullptr;
    }
    buttonClose_.unload();
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
}

void DebugSystem::draw() {
    if (!open_)
        return;

    renderBackground();

    headerText_.draw(true);

    buttonClose_.draw();
}
