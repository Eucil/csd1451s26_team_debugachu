/*!
@file       Credits.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu

@date       March, 31, 2026

@brief      This source file contains the definitions of functions that
            implement the Credits screen state. Credits lines are loaded
            from a JSON config file and scrolled upward automatically.
            The state shares the live menu background with MainMenu and Controls.

            Functions defined:
                - loadCredits,       loads fonts, credits data, and back button
                - initializeCredits, resets scroll position and initializes animations
                - updateCredits,     handles scrolling, input, and auto-return
                - drawCredits,       renders background, overlay, styled text, and button
                - freeCredits,       releases background and animation resources
                - unloadCredits,     unloads fonts, overlay mesh, and button assets

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#include "States/Credits.h"

// Standard library
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "ConfigManager.h"
#include "DebugSystem.h"
#include "GameStateManager.h"
#include "MenuBackground.h"
#include "MeshUtils.h"

// ==========================================
//            STATIC STATE
// ==========================================
static f32 yPos = 0.0f;
static float lineSpacing = 80.0f;  // read from credits.json DisplaySettings
static float scrollSpeed = 200.0f; // read from credits.json DisplaySettings
static float overlayAlpha = 0.5f;  // read from credits.json DisplaySettings

static s8 creditsFont = 0;
static s8 buttonFont = 0;
static Button buttonBack;

static std::vector<std::string> creditsData;
static AEGfxVertexList* overlayMesh = nullptr;

// ==========================================
//            ANIMATIONS
// ==========================================
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader uiFadeIn_;

// ==========================================
//            CREDITS STATE
// ==========================================

// =========================================================
//
// loadCredits()
//
// - Loads the shared menu background assets.
// - Creates the credits and button fonts from PressStart2P-Regular.ttf.
// - Reads all credits lines from the credits JSON config into creditsData.
// - Builds and caches the fullscreen overlay mesh.
// - Loads the back button mesh and texture.
//
// =========================================================
void loadCredits() {
    MenuBackground::load();

    creditsFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 36);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // Read display settings from credits.json
    Json::Value ds = g_configManager.getSection("credits", "DisplaySettings");
    if (!ds.isNull()) {
        lineSpacing = ds.get("lineSpacing", 80.0f).asFloat();
        scrollSpeed = ds.get("scrollSpeed", 200.0f).asFloat();
        overlayAlpha = ds.get("overlayAlpha", 0.5f).asFloat();
    }

    Json::Value lines = g_configManager.getSection("credits", "CreditsInfo");
    if (!lines.isNull() && lines.isMember("Lines") && lines["Lines"].isArray()) {
        creditsData.clear();
        for (Json::Value const& line : lines["Lines"]) {
            creditsData.push_back(line.asString());
        }
    }

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.0f, 1.0f, 0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFF000000, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f, 0.5f, 0.5f, 0xFF000000, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFF000000, 0.0f, 0.0f);
    overlayMesh = AEGfxMeshEnd();

    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
}

// =========================================================
//
// initializeCredits()
//
// - Initializes the shared menu background simulation.
// - Resets the vertical scroll position to the bottom of the screen.
// - Reads the back button transform from JSON config.
// - Assigns the button font to the back button.
// - Clears and re-registers the screen fader and UI fader with the
//   animation manager, then initializes all animations.
//
// =========================================================
void initializeCredits() {
    MenuBackground::initialize();

    yPos = -450.0f;

    buttonBack.initFromJson("credits_buttons", "Back");
    buttonBack.setTextFont(buttonFont);

    animManager.clear();
    animManager.add(&screenFader);
    animManager.add(&uiFadeIn_);
    animManager.initializeAll();
}

// =========================================================
//
// updateCredits()
//
// - Advances the vertical scroll position upward each frame.
// - Handles escape and space input to trigger a fade-out to MainMenu.
// - Handles back button click to trigger a fade-out to MainMenu.
// - Updates the back button transform each frame.
// - Triggers a fade-out to MainMenu when all lines have scrolled off screen.
// - Destroys dirt at the mouse position when left mouse button is held.
// - Updates the shared menu background simulation.
// - Updates all registered animations each frame.
// - Delegates to the debug system when it is open.
//
// =========================================================
void updateCredits(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        yPos += scrollSpeed * deltaTime;

        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_SPACE)) {
            screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }

        if (buttonBack.checkMouseClick()) {
            screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }
        buttonBack.updateTransform();

        float lastLineY = yPos - (static_cast<int>(creditsData.size()) - 1) * lineSpacing;
        if (lastLineY > 450.0f) {
            screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }

        if (AEInputCheckCurr(AEVK_LBUTTON)) {
            bool hitDirt = MenuBackground::destroyDirtAtMouse(20.0f);
            if (hitDirt) {
                g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
            }
        }

        MenuBackground::update(deltaTime);
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }
    animManager.updateAll(deltaTime);
}

// =========================================================
//
// drawCredits()
//
// - Draws the shared live menu background.
// - Draws the semi-transparent dark overlay for text readability.
// - Iterates over all credits lines, skipping those outside the visible region.
// - Applies index-based styling to avoid strcmp on multi-byte characters.
// - Centers each line horizontally and draws a black outline for legibility.
// - Draws the back button, animation overlays, and debug overlay.
//
// Index map (matches credits.json Lines array order):
//   [0,1]  = empty lines
//   [2]    = WWW.DIGIPEN.EDU
//   [3]    = copyright line       styled smaller, grey
//   [4]    = empty
//   [5]    = TEAM                 gold header
//   [6]    = DEBUGACHU            white
//   [7]    = empty
//   [8]    = TEAM MEMBERS         gold header
//   [9-12] = team member names
//   [13]   = empty
//   [14]   = INSTRUCTORS          gold header
//   [15-17]= instructor names
//   [18]   = empty
//   [19]   = CREATED AT           gold header
//   [20]   = DigiPen Singapore
//   [21]   = empty
//   [22]   = PRESIDENT            gold header
//   [23]   = Claude Comair
//   [24]   = empty
//   [25]   = EXECUTIVES           gold header
//   [26-31]= executive names (one per line)
//   [32]   = empty
//   [33]   = SPECIAL THANKS       gold header
//   [34]   = All playtesters
//   [35]   = empty
//
// =========================================================
void drawCredits() {
    MenuBackground::draw();

    AEMtx33 scale, trans, world;
    AEMtx33Scale(&scale, (f32)AEGfxGetWindowWidth(), (f32)AEGfxGetWindowHeight());
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&world, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(overlayAlpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetTransform(world.m);
    AEGfxMeshDraw(overlayMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxSetTransparency(1.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    for (size_t i = 0; i < creditsData.size(); i++) {
        float yLine = yPos - static_cast<float>(i) * lineSpacing;

        if (yLine <= -450.0f || yLine >= 450.0f)
            continue;

        std::string const& currentLine = creditsData[i];
        if (currentLine.empty())
            continue;

        float textSize = 0.6f;
        float r = 1.0f, g = 1.0f, b = 1.0f;

        switch (i) {
        case 3: // copyright line
            r = 0.7f;
            g = 0.7f;
            b = 0.7f;
            textSize = 0.4f;
            break;
        case 5: // TEAM
            r = 1.0f;
            g = 0.84f;
            b = 0.0f;
            textSize = 0.9f;
            break;
        case 6: // Debugachu
            r = 1.0f;
            g = 1.0f;
            b = 1.0f;
            textSize = 0.8f;
            break;
        case 8:  // TEAM MEMBERS
        case 14: // INSTRUCTORS
        case 19: // CREATED AT
        case 22: // PRESIDENT
        case 25: // EXECUTIVES
        case 33: // SPECIAL THANKS
            r = 1.0f;
            g = 0.84f;
            b = 0.0f;
            textSize = 0.8f;
            break;
        default:
            textSize = 0.6f;
            break;
        }

        float textWidth = 0.0f, textHeight = 0.0f;
        AEGfxGetPrintSize(creditsFont, currentLine.c_str(), textSize, &textWidth, &textHeight);
        float xPos = -textWidth / 2.0f;
        float screenY = yLine / 450.0f;

        // Black outline for legibility over the background
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos - 0.002f, screenY - 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos + 0.002f, screenY - 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos - 0.002f, screenY + 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos + 0.002f, screenY + 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos, screenY, textSize, r, g, b, 1.0f);
    }

    buttonBack.draw();
    animManager.drawAll();
    g_debugSystem.drawAll();
}

// =========================================================
//
// freeCredits()
//
// - Frees the shared menu background simulation objects.
// - Calls freeAll() on the animation manager to release animation resources.
//
// =========================================================
void freeCredits() {
    MenuBackground::free();
    animManager.freeAll();
}

// =========================================================
//
// unloadCredits()
//
// - Unloads the shared menu background GPU assets.
// - Destroys the credits and button fonts.
// - Frees the overlay mesh.
// - Unloads the back button mesh and texture.
//
// =========================================================
void unloadCredits() {
    MenuBackground::unload();

    if (creditsFont) {
        AEGfxDestroyFont(creditsFont);
        creditsFont = 0;
    }
    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }
    if (overlayMesh) {
        AEGfxMeshFree(overlayMesh);
        overlayMesh = nullptr;
    }

    buttonBack.unload();
}