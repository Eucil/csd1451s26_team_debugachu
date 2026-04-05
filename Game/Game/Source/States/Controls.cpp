/*!
@file       Controls.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu

@date       March, 31, 2026

@brief      This source file contains the definitions of functions that
            implement the Controls screen state, which displays a multi-page
            how-to-play guide. Pages are loaded from JSON and illustrated with
            sprite icons and animated collectibles. The state shares the live
            menu background with MainMenu and Credits.

            Functions defined:
                - loadControls,       loads fonts, pages, sprites, and buttons
                - initializeControls, resets page index and initializes animations
                - updateControls,     handles input, navigation, and background
                - drawControls,       renders background, overlay, text, and sprites
                - freeControls,       releases meshes, textures, and animations
                - unloadControls,     unloads fonts and button assets

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#include "States/Controls.h"

// Standard library
#include <sstream>
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
//            PAGE DATA
// ==========================================
struct HTPPage {
    TextData title;
    TextData body;
};

// ==========================================
//            SPRITE ILLUSTRATION SYSTEM
// ==========================================

// Each entry attaches a sprite to a specific page number.
// screenX/Y are in normalised [-1, 1] space. sizeX/Y are world units.
struct PageSprite {
    AEGfxTexture* tex{nullptr};
    AEGfxVertexList* mesh{nullptr};
    int page{0};
    f32 screenX{0.6f};
    f32 screenY{0.0f};
    f32 sizeX{80.0f};
    f32 sizeY{80.0f};
};

// ==========================================
//            STATIC STATE
// ==========================================
static int currentPage = 1;
static s8 titleFont = 0;
static s8 bodyFont = 0;
static s8 buttonFont = 0;

static Button buttonNext;
static Button buttonPrevious;
static Button buttonBack;

static std::vector<HTPPage> pages;
static AEGfxVertexList* overlayMesh = nullptr;

static f32 s_overlayAlpha = 0.55f;
static f32 s_bodyLineSpacing = 0.15f;
static f32 s_pageIndY = -0.88f;
static f32 s_pageIndScale = 0.8f;
static f32 s_pageIndR = 0.6f;
static f32 s_pageIndG = 0.6f;
static f32 s_pageIndB = 0.6f;

static std::vector<PageSprite> s_sprites;
static AEGfxVertexList* s_fullUvMesh = nullptr;
static AEGfxVertexList* s_flowerMesh = nullptr;
static AEGfxVertexList* s_portalMesh = nullptr;
static AEGfxTexture* s_pipeTex = nullptr;
static AEGfxTexture* s_portalTex = nullptr;
static AEGfxTexture* s_flowerTex = nullptr;
static AEGfxTexture* s_magicTex = nullptr;
static AEGfxVertexList* s_magicMesh = nullptr;
static AEGfxTexture* s_mossTex = nullptr;
static AEGfxVertexList* s_mossMesh = nullptr;

// Collectible icon meshes for page 5 (colour-drawn, no texture)
static AEGfxVertexList* s_starMesh = nullptr;
static AEGfxVertexList* s_gemMesh = nullptr;
static AEGfxVertexList* s_leafMesh = nullptr;
static float s_collectRot = 0.0f;   // shared rotation angle (radians)
static float s_collectPulse = 0.0f; // shared pulse timer

// ==========================================
//            ANIMATIONS
// ==========================================
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader uiFadeIn_;

// ==========================================
//            STATIC FUNCTION DECLARATIONS
// ==========================================
static AEGfxVertexList* makeUvMesh(float u0, float u1);
static void createCollectibleMeshes();
static void drawCollectibleIcons();
static void ensureOverlayMesh();
static void drawOverlay(f32 alpha);
static void drawCenteredText(s8 font, const char* text, f32 screenY, f32 size, f32 r, f32 g, f32 b);
static void drawPageIndicator();

// ==========================================
//            CONTROLS STATE
// ==========================================

// =========================================================
//
// loadControls()
//
// - Loads the shared menu background assets.
// - Creates title, body, and button fonts.
// - Reads each page's title and body from the how_to_play JSON config.
// - Reads display settings (overlay alpha, line spacing, indicator style).
// - Loads sprite illustration textures and builds UV-baked quad meshes.
// - Builds collectible icon meshes for page 5.
// - Registers sprite entries with page number and screen position.
// - Loads navigation button meshes and textures.
//
// =========================================================
void loadControls() {
    MenuBackground::load();

    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    bodyFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    pages.clear();
    Json::Value pagesArray = g_configManager.getSection("how_to_play", "Pages");
    if (!pagesArray.isNull() && pagesArray.isArray()) {
        for (const Json::Value& p : pagesArray) {
            HTPPage page;
            const Json::Value& t = p["title"];
            page.title.content_ = t["content"].asString();
            page.title.x_ = t["x"].asFloat();
            page.title.y_ = t["y"].asFloat();
            page.title.scale_ = t["scale"].asFloat();
            page.title.r_ = t["red"].asFloat();
            page.title.g_ = t["green"].asFloat();
            page.title.b_ = t["blue"].asFloat();
            page.title.a_ = t["alpha"].asFloat();
            page.title.font_ = titleFont;

            const Json::Value& b = p["body"];
            page.body.content_ = b["content"].asString();
            page.body.x_ = b["x"].asFloat();
            page.body.y_ = b["y"].asFloat();
            page.body.scale_ = b["scale"].asFloat();
            page.body.r_ = b["red"].asFloat();
            page.body.g_ = b["green"].asFloat();
            page.body.b_ = b["blue"].asFloat();
            page.body.a_ = b["alpha"].asFloat();
            page.body.font_ = bodyFont;

            pages.push_back(page);
        }
    }

    Json::Value ds = g_configManager.getSection("how_to_play", "DisplaySettings");
    if (!ds.isNull()) {
        s_overlayAlpha = ds["overlayAlpha"].asFloat();
        s_bodyLineSpacing = ds["bodyLineSpacing"].asFloat();
        s_pageIndY = ds["pageIndicatorY"].asFloat();
        s_pageIndScale = ds["pageIndicatorScale"].asFloat();
        s_pageIndR = ds["pageIndicatorRed"].asFloat();
        s_pageIndG = ds["pageIndicatorGreen"].asFloat();
        s_pageIndB = ds["pageIndicatorBlue"].asFloat();
    }

    s_pipeTex = AEGfxTextureLoad("Assets/Textures/overgrown_pipe_end.png");
    s_portalTex = AEGfxTextureLoad("Assets/Textures/wormhole.png");
    s_flowerTex = AEGfxTextureLoad("Assets/Textures/pink_flower_sprite_sheet.png");
    s_magicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");
    s_mossTex = AEGfxTextureLoad("Assets/Textures/moss_sprite_sheet.png");

    s_fullUvMesh = makeUvMesh(0.0f, 1.0f);
    s_flowerMesh = makeUvMesh(0.0f, 1.0f / 4.0f);
    s_portalMesh = makeUvMesh(0.0f, 1.0f);
    s_magicMesh = makeUvMesh(0.0f, 1.0f);
    s_mossMesh = makeUvMesh(0.0f, 1.0f / 3.0f); // frame 0 (idle) of moss sheet

    createCollectibleMeshes();

    // {tex, mesh, page, screenX, screenY, sizeX, sizeY}
    // Portal in-game is 30w x 60h -- mirrored as 1:2 ratio here
    s_sprites.clear();
    s_sprites.push_back({s_pipeTex, s_fullUvMesh, 2, -0.42f, 0.35f, 80.0f, 80.0f});    // pipe
    s_sprites.push_back({s_portalTex, s_portalMesh, 2, 0.42f, 0.10f, 60.0f, 120.0f});  // portal
    s_sprites.push_back({s_flowerTex, s_flowerMesh, 2, -0.42f, -0.10f, 80.0f, 80.0f}); // flower
    s_sprites.push_back({s_magicTex, s_magicMesh, 3, 0.52f, 0.05f, 80.0f, 80.0f});     // magic
    s_sprites.push_back({s_portalTex, s_portalMesh, 3, -0.52f, 0.20f, 60.0f, 120.0f}); // portal
    s_sprites.push_back({s_mossTex, s_mossMesh, 4, 0.42f, -0.05f, 80.0f, 80.0f});      // moss

    buttonNext.loadMesh();
    buttonNext.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonPrevious.loadMesh();
    buttonPrevious.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
}

// =========================================================
//
// initializeControls()
//
// - Initializes the shared menu background simulation.
// - Resets the current page index to 1 on every entry.
// - Clears and re-registers animations, then initializes all.
// - Reads navigation button transforms from JSON config.
// - Assigns the button font to each navigation button.
//
// =========================================================
void initializeControls() {
    MenuBackground::initialize();

    currentPage = 1;

    animManager.clear();
    animManager.add(&screenFader);
    animManager.add(&uiFadeIn_);
    animManager.initializeAll();

    buttonNext.initFromJson("how_to_play_buttons", "Next");
    buttonNext.setTextFont(buttonFont);
    buttonPrevious.initFromJson("how_to_play_buttons", "Previous");
    buttonPrevious.setTextFont(buttonFont);
    buttonBack.initFromJson("how_to_play_buttons", "Back");
    buttonBack.setTextFont(buttonFont);
}

// =========================================================
//
// updateControls()
//
// - Handles right-arrow and space to advance pages or exit to MainMenu.
// - Handles left-arrow to go back a page.
// - Handles escape to exit to MainMenu immediately.
// - Mirrors keyboard navigation on next, previous, and back button clicks.
// - Destroys dirt at the mouse cursor when left mouse is held.
// - Updates navigation button transforms each frame.
// - Advances collectible icon rotation and pulse animation timers.
// - Updates the shared menu background simulation.
// - Updates all registered animations.
// - Delegates to the debug system when it is open.
//
// =========================================================
void updateControls(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        int totalPages = static_cast<int>(pages.size());
        if (totalPages == 0)
            totalPages = 1;

        if (AEInputCheckTriggered(AEVK_RIGHT) || AEInputCheckTriggered(AEVK_SPACE)) {
            if (currentPage < totalPages)
                ++currentPage;
            else
                screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }
        if (AEInputCheckTriggered(AEVK_LEFT)) {
            if (currentPage > 1)
                --currentPage;
        }
        if (AEInputCheckTriggered(AEVK_ESCAPE)) {
            screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }

        if (buttonNext.checkMouseClick()) {
            if (currentPage < totalPages)
                ++currentPage;
            else
                screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }
        if (buttonPrevious.checkMouseClick()) {
            if (currentPage > 1)
                --currentPage;
        }
        if (buttonBack.checkMouseClick()) {
            screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }

        if (AEInputCheckCurr(AEVK_LBUTTON)) {
            bool hitDirt = MenuBackground::destroyDirtAtMouse(20.0f);
            if (hitDirt) {
                g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
            }
        }

        buttonNext.updateTransform();
        buttonPrevious.updateTransform();
        buttonBack.updateTransform();

        s_collectRot += deltaTime * 0.5f;
        s_collectPulse += deltaTime * 3.0f;

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
// drawControls()
//
// - Draws the shared live menu background.
// - Draws the semi-transparent dark overlay.
// - Draws the title and body text for the current page, centered.
// - Draws the page indicator (e.g. "2 / 5").
// - Draws animated collectible icons on page 5.
// - Draws sprite illustrations registered for the current page.
// - Draws navigation buttons, animation overlays, and debug overlay.
//
// =========================================================
void drawControls() {
    MenuBackground::draw();
    drawOverlay(s_overlayAlpha);

    if (currentPage >= 1 && currentPage <= static_cast<int>(pages.size())) {
        const HTPPage& page = pages[static_cast<size_t>(currentPage) - 1u];

        drawCenteredText(titleFont, page.title.content_.c_str(), page.title.y_, page.title.scale_,
                         page.title.r_, page.title.g_, page.title.b_);

        std::istringstream stream(page.body.content_);
        std::string line;
        f32 lineY = page.body.y_;
        while (std::getline(stream, line)) {
            drawCenteredText(bodyFont, line.c_str(), lineY, page.body.scale_, page.body.r_,
                             page.body.g_, page.body.b_);
            lineY -= s_bodyLineSpacing;
        }
    }

    drawPageIndicator();

    if (currentPage == 5) {
        drawCollectibleIcons();
    }

    for (const auto& sp : s_sprites) {
        if (sp.page != currentPage || !sp.tex || !sp.mesh)
            continue;
        f32 worldX = sp.screenX * static_cast<f32>(AEGfxGetWindowWidth()) * 0.5f;
        f32 worldY = sp.screenY * static_cast<f32>(AEGfxGetWindowHeight()) * 0.5f;
        AEMtx33 S, T, W;
        AEMtx33Scale(&S, sp.sizeX, sp.sizeY);
        AEMtx33Trans(&T, worldX, worldY);
        AEMtx33Concat(&W, &T, &S);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransform(W.m);
        AEGfxTextureSet(sp.tex, 0.0f, 0.0f);
        AEGfxMeshDraw(sp.mesh, AE_GFX_MDM_TRIANGLES);
    }

    buttonNext.draw();
    if (currentPage > 1) {
        buttonPrevious.draw();
    }
    buttonBack.draw();
    animManager.drawAll();
    g_debugSystem.drawAll();
}

// =========================================================
//
// freeControls()
//
// - Frees the shared menu background simulation objects.
// - Frees all registered animation resources.
// - Frees the overlay mesh and all sprite meshes and textures.
// - Frees the collectible icon meshes and clears the sprite list.
//
// =========================================================
void freeControls() {
    MenuBackground::free();
    animManager.freeAll();

    if (overlayMesh) {
        AEGfxMeshFree(overlayMesh);
        overlayMesh = nullptr;
    }
    if (s_fullUvMesh) {
        AEGfxMeshFree(s_fullUvMesh);
        s_fullUvMesh = nullptr;
    }
    if (s_flowerMesh) {
        AEGfxMeshFree(s_flowerMesh);
        s_flowerMesh = nullptr;
    }
    if (s_portalMesh) {
        AEGfxMeshFree(s_portalMesh);
        s_portalMesh = nullptr;
    }
    if (s_magicMesh) {
        AEGfxMeshFree(s_magicMesh);
        s_magicMesh = nullptr;
    }
    if (s_mossMesh) {
        AEGfxMeshFree(s_mossMesh);
        s_mossMesh = nullptr;
    }
    if (s_starMesh) {
        AEGfxMeshFree(s_starMesh);
        s_starMesh = nullptr;
    }
    if (s_gemMesh) {
        AEGfxMeshFree(s_gemMesh);
        s_gemMesh = nullptr;
    }
    if (s_leafMesh) {
        AEGfxMeshFree(s_leafMesh);
        s_leafMesh = nullptr;
    }

    if (s_pipeTex) {
        AEGfxTextureUnload(s_pipeTex);
        s_pipeTex = nullptr;
    }
    if (s_portalTex) {
        AEGfxTextureUnload(s_portalTex);
        s_portalTex = nullptr;
    }
    if (s_flowerTex) {
        AEGfxTextureUnload(s_flowerTex);
        s_flowerTex = nullptr;
    }
    if (s_magicTex) {
        AEGfxTextureUnload(s_magicTex);
        s_magicTex = nullptr;
    }
    if (s_mossTex) {
        AEGfxTextureUnload(s_mossTex);
        s_mossTex = nullptr;
    }

    s_sprites.clear();
}

// =========================================================
//
// unloadControls()
//
// - Unloads the shared menu background GPU assets.
// - Destroys the title, body, and button fonts.
// - Unloads the navigation button meshes and textures.
//
// =========================================================
void unloadControls() {
    MenuBackground::unload();

    if (titleFont) {
        AEGfxDestroyFont(titleFont);
        titleFont = 0;
    }
    if (bodyFont) {
        AEGfxDestroyFont(bodyFont);
        bodyFont = 0;
    }
    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }

    buttonNext.unload();
    buttonPrevious.unload();
    buttonBack.unload();
}

// ==========================================
//            STATIC HELPERS
// ==========================================

// =========================================================
//
// makeUvMesh()
//
// - Builds a unit quad with UVs mapped from u0 to u1 on the U axis.
// - Used to sample a specific region of a sprite sheet without offsets.
//
// =========================================================
static AEGfxVertexList* makeUvMesh(float u0, float u1) {
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, u0, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f, -0.5f, 0.5f,
                0xFFFFFFFF, u0, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, u1, 0.0f, -0.5f, 0.5f,
                0xFFFFFFFF, u0, 0.0f);
    return AEGfxMeshEnd();
}

// =========================================================
//
// createCollectibleMeshes()
//
// - Builds the star mesh as a 5-pointed shape using outer and inner radii,
//   matching the geometry used in CollectibleSystem::CreateMeshes().
// - Builds the gem mesh as a diamond using two triangles.
// - Builds the leaf mesh as a teardrop using 12 fan segments with
//   a sinusoidally varying radius per segment.
//
// =========================================================
static void createCollectibleMeshes() {
    constexpr int kStarPoints = 5;
    constexpr float kOuterRadius = 0.5f;
    constexpr float kInnerRadius = 0.25f;

    AEGfxMeshStart();
    for (int i = 0; i < kStarPoints; i++) {
        float a1 = (i * 2.0f * 3.14159f / kStarPoints) - 3.14159f / 2.0f;
        float a2 = ((i + 1) * 2.0f * 3.14159f / kStarPoints) - 3.14159f / 2.0f;
        float mid = (a1 + a2) / 2.0f;
        float x1 = cosf(a1) * kOuterRadius, y1 = sinf(a1) * kOuterRadius;
        float x2 = cosf(a2) * kOuterRadius, y2 = sinf(a2) * kOuterRadius;
        float xi = cosf(mid) * kInnerRadius, yi = sinf(mid) * kInnerRadius;
        AEGfxTriAdd(0, 0, 0xFFFFFF00, 0.5f, 0.5f, x1, y1, 0xFFFFFF00, x1 + 0.5f, y1 + 0.5f, xi, yi,
                    0xFFFFFF00, xi + 0.5f, yi + 0.5f);
        AEGfxTriAdd(0, 0, 0xFFFFFF00, 0.5f, 0.5f, xi, yi, 0xFFFFFF00, xi + 0.5f, yi + 0.5f, x2, y2,
                    0xFFFFFF00, x2 + 0.5f, y2 + 0.5f);
    }
    s_starMesh = AEGfxMeshEnd();

    AEGfxMeshStart();
    AEGfxTriAdd(0.0f, 0.5f, 0xFFFF00FF, 0.5f, 1.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f, 0.0f,
                0xFFFF00FF, 1.0f, 0.5f);
    AEGfxTriAdd(0.0f, -0.5f, 0xFFFF00FF, 0.5f, 0.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f,
                0.0f, 0xFFFF00FF, 1.0f, 0.5f);
    s_gemMesh = AEGfxMeshEnd();

    constexpr int kLeafSeg = 12;
    AEGfxMeshStart();
    for (int i = 0; i < kLeafSeg; i++) {
        float a1 = (i * 2.0f * 3.14159f / kLeafSeg);
        float a2 = ((i + 1) * 2.0f * 3.14159f / kLeafSeg);
        float r1 = 0.3f + 0.2f * sinf(a1 * 2.0f);
        float r2 = 0.3f + 0.2f * sinf(a2 * 2.0f);
        float x1 = cosf(a1) * r1, y1 = sinf(a1) * r1;
        float x2 = cosf(a2) * r2, y2 = sinf(a2) * r2;
        AEGfxTriAdd(0, 0, 0xFF00FF00, 0.5f, 0.5f, x1, y1, 0xFF00FF00, x1 + 0.5f, y1 + 0.5f, x2, y2,
                    0xFF00FF00, x2 + 0.5f, y2 + 0.5f);
    }
    s_leafMesh = AEGfxMeshEnd();
}

// =========================================================
//
// drawCollectibleIcons()
//
// - Guards against null meshes and returns early if any are missing.
// - Computes a pulse scale from s_collectPulse using the same formula
//   as CollectibleSystem::Update (sinf * 0.1 + 1.0).
// - Draws each icon with a per-icon rotation speed applied to s_collectRot,
//   giving each a distinct spin rate.
// - Applies Scale * Rotate * Translate so each icon spins around its centre.
//
// =========================================================
static void drawCollectibleIcons() {
    if (!s_starMesh || !s_gemMesh || !s_leafMesh)
        return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    float winW = static_cast<float>(AEGfxGetWindowWidth());
    float winH = static_cast<float>(AEGfxGetWindowHeight());
    float pulse = sinf(s_collectPulse) * 0.1f + 1.0f;
    float iconSize = 60.0f * pulse;

    float rotSpeeds[] = {1.0f, 1.5f, 2.0f};

    struct IconDef {
        AEGfxVertexList* mesh;
        float r, g, b, offsetX;
    };
    IconDef icons[] = {
        {s_starMesh, 1.0f, 1.0f, 0.0f, -0.35f},
        {s_gemMesh, 1.0f, 0.0f, 1.0f, 0.0f},
        {s_leafMesh, 0.0f, 1.0f, 0.0f, 0.35f},
    };

    for (int i = 0; i < 3; ++i) {
        const IconDef& ic = icons[i];
        float worldX = ic.offsetX * winW * 0.5f;
        float worldY = -0.20f * winH * 0.5f;
        float rot = s_collectRot * rotSpeeds[i];
        AEMtx33 S, R, T, W;
        AEMtx33Scale(&S, iconSize, iconSize);
        AEMtx33Rot(&R, rot);
        AEMtx33Trans(&T, worldX, worldY);
        AEMtx33Concat(&W, &R, &S);
        AEMtx33Concat(&W, &T, &W);
        AEGfxSetColorToMultiply(ic.r, ic.g, ic.b, 1.0f);
        AEGfxSetTransform(W.m);
        AEGfxMeshDraw(ic.mesh, AE_GFX_MDM_TRIANGLES);
    }
}

// =========================================================
//
// ensureOverlayMesh()
//
// - Returns immediately if the overlay mesh already exists.
// - Builds a fullscreen unit quad with solid black vertex colours.
//
// =========================================================
static void ensureOverlayMesh() {
    if (overlayMesh != nullptr)
        return;
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.0f, 1.0f, 0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFF000000, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f, 0.5f, 0.5f, 0xFF000000, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFF000000, 0.0f, 0.0f);
    overlayMesh = AEGfxMeshEnd();
}

// =========================================================
//
// drawOverlay()
//
// - Ensures the overlay mesh exists before drawing.
// - Scales the mesh to fill the entire window.
// - Draws a solid black quad at the given alpha transparency.
// - Resets transparency to 1.0 after drawing.
//
// =========================================================
static void drawOverlay(f32 alpha) {
    ensureOverlayMesh();

    AEMtx33 scale, trans, world;
    AEMtx33Scale(&scale, (f32)AEGfxGetWindowWidth(), (f32)AEGfxGetWindowHeight());
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&world, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(alpha);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetTransform(world.m);
    AEGfxMeshDraw(overlayMesh, AE_GFX_MDM_TRIANGLES);
    AEGfxSetTransparency(1.0f);
}

// =========================================================
//
// drawCenteredText()
//
// - Returns early if the text pointer is null or the string is empty.
// - Measures rendered text width to compute a centered x position.
// - Draws four offset copies in black to create a legible outline.
// - Draws the main text in the specified colour on top.
//
// =========================================================
static void drawCenteredText(s8 font, const char* text, f32 screenY, f32 size, f32 r, f32 g,
                             f32 b) {
    if (!text || strlen(text) == 0)
        return;

    f32 tw = 0.0f, th = 0.0f;
    AEGfxGetPrintSize(font, text, size, &tw, &th);
    f32 xPos = -tw / 2.0f;

    AEGfxPrint(font, text, xPos - 0.002f, screenY - 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos + 0.002f, screenY - 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos - 0.002f, screenY + 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos + 0.002f, screenY + 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos, screenY, size, r, g, b, 1.0f);
}

// =========================================================
//
// drawPageIndicator()
//
// - Formats the current and total page count as "X / Y".
// - Measures the string width to compute a centered x position.
// - Draws the indicator using the settings loaded from JSON.
//
// =========================================================
static void drawPageIndicator() {
    int totalPages = static_cast<int>(pages.size());
    if (totalPages == 0)
        totalPages = 1;

    char buf[16];
    sprintf_s(buf, sizeof(buf), "%d / %d", currentPage, totalPages);

    f32 tw = 0.0f, th = 0.0f;
    AEGfxGetPrintSize(bodyFont, buf, s_pageIndScale, &tw, &th);
    f32 xPos = -tw / 2.0f;
    AEGfxPrint(bodyFont, buf, xPos, s_pageIndY, s_pageIndScale, s_pageIndR, s_pageIndG, s_pageIndB,
               1.0f);
}