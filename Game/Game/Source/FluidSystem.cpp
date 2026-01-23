#include "AEEngine.h"

#include "FluidSystem.h"
#include "Utils.h"

#include <cmath>

// ==========================================
// FluidParticle
// ==========================================

FluidParticle::FluidParticle() {
    transform_.pos_ = {0.0f, 0.0f};
    transform_.scale_ = {100.0f, 100.0f};
    type_ = FluidType::Water;
}

FluidParticle::FluidParticle(f32 posX, f32 posY, f32 scaleX, f32 scaleY, FluidType type) {
    transform_.pos_ = {posX, posY};
    transform_.scale_ = {scaleX, scaleY};
    type_ = type;
}

// ==========================================
// FluidSystem
// ==========================================

void FluidSystem::Initialize() {}
void FluidSystem::Update() {}

void FluidSystem::ApplyGraphics() {

    // render mode will be set within the draw functions

    // blend options
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(waterColor_[3]);

    // colors
    AEGfxSetColorToMultiply(waterColor_[0], waterColor_[1], waterColor_[2], waterColor_[3]);
    // AEGfxSetColorToAdd(add_red, add_green, add_blue, add_alpha);
}

void FluidSystem::DrawColor() {

    if (waterGraphics_.mesh_ != nullptr) {
        AEGfxMeshDraw(waterGraphics_.mesh_, AE_GFX_MDM_TRIANGLES);
    }
}

void FluidSystem::DrawTexture() {
    // ------------------- DRAW WATER ------------------- //
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxTextureSet(waterGraphics_.texture_, 0, 0); // Bind Water Texture
    // Set global color tint for water
    //AEGfxSetColorToMultiply(waterColor_[0], waterColor_[1], ...);

    for (const auto& p : particles) {
        if (p.type_ == FluidType::Water) {
            // ... Set Transform and Draw Mesh ...
        }
    }

    // ------------------- DRAW LAVA ------------------- //
    //AEGfxTextureSet(lavaGraphics_.texture_, 0, 0); // switch to Lava Texture
    // AEGfxSetColor(lavaColor_[0], lavaColor_[1], ...);  // switch to Lava Color

    for (const auto& p : particles) {
        if (p.type_ == FluidType::Lava) {
            // ... Set Transform and Draw Mesh ...
        }
    }
}

void FluidSystem::SetMesh(AEGfxVertexList* pMesh) { waterGraphics_.mesh_ = pMesh; }

void FluidSystem::SetWaterColor(f32 r, f32 g, f32 b, f32 a) {
    waterColor_[0] = r;
    waterColor_[1] = g;
    waterColor_[2] = b;
    waterColor_[3] = a;
}
void FluidSystem::SpawnParticle() {}