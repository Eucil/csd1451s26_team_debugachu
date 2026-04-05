/*!
@file       LogoScreen.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date       March, 31, 2026

@brief      This source file contains the definitions of functions that
            implement the LogoScreen state, which displays the DigiPen logo
            with a fade-in, hold, and fade-out sequence before transitioning
            to the MainMenu state.

            Functions defined:
                - loadLogoScreen,       loads the logo texture and quad mesh
                - initializeLogoScreen, resets the fade sequence state
                - updateLogoScreen,     drives fade-in, hold, and fade-out phases
                - drawLogoScreen,       renders the logo at the correct aspect ratio
                - freeLogoScreen,       no runtime resources to release
                - unloadLogoScreen,     unloads the texture and frees the mesh

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#include "States/LogoScreen.h"

// Third-party
#include <AEEngine.h>

// Project
#include "DebugSystem.h"
#include "GameStateManager.h"

// ==========================================
//            CONSTANTS
// ==========================================
static constexpr f32 FADE_SPEED = 1.5f;    // seconds to complete a fade in or out
static constexpr f32 HOLD_DURATION = 1.5f; // seconds to hold logo at full opacity

// Actual pixel dimensions of DigiPen_Singapore_WEB_WHITE_2026.png (1525 x 900)
static constexpr f32 LOGO_ASPECT = 1525.0f / 900.0f;

// ==========================================
//            STATIC STATE
// ==========================================
static AEGfxTexture* logoTexture = nullptr;
static AEGfxVertexList* logoMesh = nullptr;

static f32 alpha_ = 0.0f;
static f32 holdTimer_ = 0.0f;
static bool fadingIn_ = true;
static bool done_ = false;

static void skipToMainMenu(GameStateManager& GSM);

// ==========================================
//            LOGO SCREEN STATE
// ==========================================

// =========================================================
//
// loadLogoScreen()
//
// - Loads the DigiPen logo texture from Assets/Logo/.
// - Creates the fullscreen quad mesh used to display the texture.
//
// =========================================================
void loadLogoScreen() {
    logoTexture = AEGfxTextureLoad("Assets/Logo/DigiPen_Singapore_WEB_WHITE_2026.png");

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    logoMesh = AEGfxMeshEnd();
}

// =========================================================
//
// initializeLogoScreen()
//
// - Resets alpha to 0.0 to begin the fade-in from transparent.
// - Resets the hold timer to HOLD_DURATION seconds.
// - Sets fadingIn_ to true to start the fade-in phase immediately.
// - Sets done_ to false so the sequence is allowed to run.
//
// =========================================================
void initializeLogoScreen() {
    alpha_ = 0.0f;
    holdTimer_ = HOLD_DURATION;
    fadingIn_ = true;
    done_ = false;
}

// =========================================================
//
// updateLogoScreen()
//
// - Caps deltaTime to 1/30s to prevent the first-frame load spike
//   from skipping through multiple fade phases in a single step.
// - Checks for any key or mouse click input and skips to MainMenu.
// - During the fade-in phase, increments alpha toward 1.0.
// - During the hold phase, decrements the hold timer until zero.
// - During the fade-out phase, decrements alpha toward 0.0 then transitions.
// - Delegates to the debug system when it is open.
//
// =========================================================
void updateLogoScreen(GameStateManager& GSM, f32 deltaTime) {
    if (deltaTime > 0.0333f)
        deltaTime = 0.0333f;

    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        if (AEInputCheckReleased(AEVK_ESCAPE) || AEInputCheckReleased(AEVK_RETURN) ||
            AEInputCheckReleased(AEVK_SPACE) || AEInputCheckReleased(AEVK_LBUTTON)) {
            skipToMainMenu(GSM);
            return;
        }

        if (done_)
            return;

        if (fadingIn_) {
            alpha_ += deltaTime / FADE_SPEED;
            if (alpha_ >= 1.0f) {
                alpha_ = 1.0f;
                fadingIn_ = false;
            }
        } else if (holdTimer_ > 0.0f) {
            holdTimer_ -= deltaTime;
        } else {
            alpha_ -= deltaTime / FADE_SPEED;
            if (alpha_ <= 0.0f) {
                alpha_ = 0.0f;
                skipToMainMenu(GSM);
            }
        }
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }
}

// =========================================================
//
// drawLogoScreen()
//
// - Sets the background color to black.
// - Guards against null texture or mesh pointers.
// - Guards against zero or negative window dimensions.
// - Computes scaleW and scaleH to fit the logo at 75% of the window
//   while preserving the 1525 x 900 pixel aspect ratio.
// - Renders the logo texture with the current alpha transparency.
// - Draws the debug system overlay.
//
// =========================================================
void drawLogoScreen() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    if (!logoTexture || !logoMesh)
        return;

    f32 winW = static_cast<f32>(AEGfxGetWindowWidth());
    f32 winH = static_cast<f32>(AEGfxGetWindowHeight());

    if (winW <= 0.0f || winH <= 0.0f)
        return;

    f32 maxW = winW * 0.75f;
    f32 maxH = winH * 0.75f;

    f32 scaleW, scaleH;
    if (maxW / LOGO_ASPECT <= maxH) {
        scaleW = maxW;
        scaleH = maxW / LOGO_ASPECT;
    } else {
        scaleH = maxH;
        scaleW = maxH * LOGO_ASPECT;
    }

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

// =========================================================
//
// freeLogoScreen()
//
// - No runtime resources to release for this state.
//
// =========================================================
void freeLogoScreen() {}

// =========================================================
//
// unloadLogoScreen()
//
// - Unloads the logo texture from the GPU if the pointer is valid.
// - Frees the quad mesh from the GPU if the pointer is valid.
// - Nulls both pointers to prevent dangling references.
//
// =========================================================
void unloadLogoScreen() {
    if (logoTexture) {
        AEGfxTextureUnload(logoTexture);
        logoTexture = nullptr;
    }
    if (logoMesh) {
        AEGfxMeshFree(logoMesh);
        logoMesh = nullptr;
    }
}

// ==========================================
//            HELPER
// ==========================================

// =========================================================
//
// skipToMainMenu()
//
// - Guards against being called more than once using the done_ flag.
// - Sets done_ to true to prevent further state transitions.
// - Assigns StateId::MainMenu to the GameStateManager's nextState_.
//
// =========================================================
static void skipToMainMenu(GameStateManager& GSM) {
    if (!done_) {
        done_ = true;
        GSM.nextState_ = StateId::MainMenu;
    }
}