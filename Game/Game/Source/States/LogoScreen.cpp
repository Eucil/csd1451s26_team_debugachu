/*!
@file       LogoScreen.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/LogoScreen.h"

#include <cstdio>

#include <AEEngine.h>

#include "DebugSystem.h"
#include "GameStateManager.h"
#include "Utils.h"

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
static constexpr f32 FADE_SPEED = 1.5f;    // seconds to complete a fade in or out
static constexpr f32 HOLD_DURATION = 1.5f; // seconds to hold logo at full opacity

// Actual pixel dimensions of DigiPen_Singapore_WEB_WHITE_2026.png (1525 x 900)
static constexpr f32 LOGO_ASPECT = 1525.0f / 900.0f;

// ----------------------------------------------------------------------------
// State
// ----------------------------------------------------------------------------
static AEGfxTexture* logoTexture = nullptr;
static AEGfxVertexList* logoMesh = nullptr;

static f32 alpha_ = 0.0f;
static f32 holdTimer_ = 0.0f;
static bool fadingIn_ = true;
static bool done_ = false;

// Tracks how many frames have been drawn -- lets us confirm Draw() is running
static int frameCount_ = 0;

// ----------------------------------------------------------------------------
// Helper
// ----------------------------------------------------------------------------
static void SkipToMainMenu(GameStateManager& GSM) {
    if (!done_) {
        done_ = true;
        printf("[LogoScreen] Transitioning to MainMenu\n");
        GSM.nextState_ = StateId::MainMenu;
    }
}

// ----------------------------------------------------------------------------
// Load
// ----------------------------------------------------------------------------
void LoadLogoScreen() {
    printf("[LogoScreen] LoadLogoScreen() called\n");

    // --- CHECK 1: Texture load ---
    const char* texturePath = "Assets/Logo/DigiPen_Singapore_WEB_WHITE_2026.png";
    printf("[LogoScreen] Attempting to load texture: %s\n", texturePath);
    logoTexture = AEGfxTextureLoad(texturePath);

    if (logoTexture == nullptr) {
        printf("[LogoScreen] ERROR: Texture failed to load!\n");
        printf("[LogoScreen]        Check that the file exists at the path above,\n");
        printf("[LogoScreen]        relative to your executable's working directory.\n");
    } else {
        printf("[LogoScreen] OK: Texture loaded successfully (ptr=%p)\n", (void*)logoTexture);
    }

    // --- CHECK 2: Mesh creation ---
    printf("[LogoScreen] Creating quad mesh...\n");
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    logoMesh = AEGfxMeshEnd();

    if (logoMesh == nullptr) {
        printf("[LogoScreen] ERROR: Mesh creation failed!\n");
    } else {
        printf("[LogoScreen] OK: Mesh created successfully (ptr=%p)\n", (void*)logoMesh);
    }

    printf("[LogoScreen] LoadLogoScreen() complete\n");
}

// ----------------------------------------------------------------------------
// Initialize
// ----------------------------------------------------------------------------
void InitializeLogoScreen() {
    printf("[LogoScreen] InitializeLogoScreen() called\n");

    alpha_ = 0.0f;
    holdTimer_ = HOLD_DURATION;
    fadingIn_ = true;
    done_ = false;
    frameCount_ = 0;

    // --- CHECK 3: Confirm state is set ---
    printf("[LogoScreen] State reset: alpha=%.2f, fadingIn=%s, done=%s\n", alpha_,
           fadingIn_ ? "true" : "false", done_ ? "true" : "false");

    // --- CHECK 4: Confirm GSM was started with LogoScreen ---
    printf("[LogoScreen] Texture ptr: %p  Mesh ptr: %p\n", (void*)logoTexture, (void*)logoMesh);
    if (logoTexture == nullptr) {
        printf("[LogoScreen] WARNING: Texture is null at Initialize time.\n");
        printf("[LogoScreen]          This means LoadLogoScreen() was not called,\n");
        printf("[LogoScreen]          OR the starting state is not StateId::LogoScreen.\n");
        printf("[LogoScreen]          Check GSM.init() call in your game entry point.\n");
    }

    printf("[LogoScreen] InitializeLogoScreen() complete\n");
}

// ----------------------------------------------------------------------------
// Update
// ----------------------------------------------------------------------------
void UpdateLogoScreen(GameStateManager& GSM, f32 deltaTime) {

    // Cap deltaTime to 1/30s so a spike never skips through phases.
    // The first frame often has an inflated dt due to load time.
    if (deltaTime > 0.0333f)
        deltaTime = 0.0333f;

    // --- CHECK 5: Confirm Update() is being called ---
    if (frameCount_ <= 3) {
        printf("[LogoScreen] UpdateLogoScreen() frame=%d  dt=%.4f  alpha=%.3f  phase=%s\n",
               frameCount_, deltaTime, alpha_,
               fadingIn_ ? "FADE_IN" : (holdTimer_ > 0.0f ? "HOLD" : "FADE_OUT"));
    }

    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        // Skip on any key / click
        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_RETURN) ||
            AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(AEVK_LBUTTON)) {
            printf("[LogoScreen] Skip input detected -- jumping to MainMenu\n");
            SkipToMainMenu(GSM);
            return;
        }

        if (done_)
            return;

        if (fadingIn_) {
            alpha_ += deltaTime / FADE_SPEED;
            if (alpha_ >= 1.0f) {
                alpha_ = 1.0f;
                fadingIn_ = false;
                printf("[LogoScreen] Fade-in complete, entering HOLD phase (%.1fs)\n",
                       HOLD_DURATION);
            }
        } else if (holdTimer_ > 0.0f) {
            holdTimer_ -= deltaTime;
        } else {
            alpha_ -= deltaTime / FADE_SPEED;
            if (alpha_ <= 0.0f) {
                alpha_ = 0.0f;
                printf("[LogoScreen] Fade-out complete\n");
                SkipToMainMenu(GSM);
            }
        }
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }
}

// ----------------------------------------------------------------------------
// Draw
// ----------------------------------------------------------------------------
void DrawLogoScreen() {
    ++frameCount_;

    // Black background
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // --- CHECK 6: Confirm Draw() is being called ---
    if (frameCount_ <= 3) {
        printf("[LogoScreen] DrawLogoScreen() frame=%d  alpha=%.3f\n", frameCount_, alpha_);
    }

    // --- CHECK 7: Guard on null resources ---
    if (logoTexture == nullptr) {
        printf("[LogoScreen] WARNING: DrawLogoScreen() -- logoTexture is null, "
               "nothing will render!\n");
        return;
    }
    if (logoMesh == nullptr) {
        printf("[LogoScreen] WARNING: DrawLogoScreen() -- logoMesh is null, "
               "nothing will render!\n");
        return;
    }

    // --- CHECK 8: Confirm window dimensions are non-zero ---
    f32 winW = static_cast<f32>(AEGfxGetWindowWidth());
    f32 winH = static_cast<f32>(AEGfxGetWindowHeight());
    if (frameCount_ <= 3) {
        printf("[LogoScreen] Window size: %.0f x %.0f\n", winW, winH);
    }
    if (winW <= 0.0f || winH <= 0.0f) {
        printf("[LogoScreen] ERROR: Window dimensions are zero or negative!\n");
        return;
    }

    // -------------------------------------------------------------------------
    // Compute a scale that fits the logo at 75% of the screen while
    // preserving its original aspect ratio (1525 x 445 px).
    //
    // Strategy: fit within a box that is 75% of the window on both axes,
    // then let the logo's own aspect ratio determine the final size.
    // -------------------------------------------------------------------------
    f32 maxW = winW * 0.75f;
    f32 maxH = winH * 0.75f;

    f32 scaleW, scaleH;
    if (maxW / LOGO_ASPECT <= maxH) {
        // Constrained by width
        scaleW = maxW;
        scaleH = maxW / LOGO_ASPECT;
    } else {
        // Constrained by height
        scaleH = maxH;
        scaleW = maxH * LOGO_ASPECT;
    }

    if (frameCount_ <= 3) {
        printf("[LogoScreen] Logo draw size: %.1f x %.1f\n", scaleW, scaleH);
    }

    // Draw logo centered on screen with correct aspect ratio and current alpha
    AEMtx33 scale, trans, world;
    AEMtx33Scale(&scale, scaleW, scaleH);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&world, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(alpha_);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransform(world.m);
    AEGfxTextureSet(logoTexture, 0.0f, 0.0f);
    AEGfxMeshDraw(logoMesh, AE_GFX_MDM_TRIANGLES);

    g_debugSystem.drawAll();
}

// ----------------------------------------------------------------------------
// Free / Unload
// ----------------------------------------------------------------------------
void FreeLogoScreen() { printf("[LogoScreen] FreeLogoScreen() called\n"); }

void UnloadLogoScreen() {
    printf("[LogoScreen] UnloadLogoScreen() called\n");

    if (logoTexture) {
        AEGfxTextureUnload(logoTexture);
        logoTexture = nullptr;
        printf("[LogoScreen] Texture unloaded\n");
    }
    if (logoMesh) {
        AEGfxMeshFree(logoMesh);
        logoMesh = nullptr;
        printf("[LogoScreen] Mesh freed\n");
    }
}