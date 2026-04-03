/*!
@file       Controls.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Chia Hanxin/c.hanxin@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#include "States/Controls.h"

#include <cstdio>
#include <cstring>
#include <sstream>

#include <AEEngine.h>

#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "ConfigManager.h"
#include "DebugSystem.h"
#include "GameStateManager.h"
#include "MenuBackground.h"
#include "Utils.h"

// ----------------------------------------------------------------------------
// Static state
// ----------------------------------------------------------------------------
static int currentPage = 1; // 1-indexed current page
static s8 titleFont = 0;    // large font for page titles
static s8 bodyFont = 0;     // smaller font for body text
static s8 buttonFont = 0;   // font for buttons

// Navigation buttons
static Button buttonNext;
static Button buttonPrevious;
static Button buttonBack;

// Animations
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

// Full-screen semi-transparent overlay mesh (created once, reused)
static AEGfxVertexList* overlayMesh = nullptr;

// Each page has a title (header) and a body (newline-separated lines)
struct HTPPage {
    TextData title;
    TextData body;
};
static std::vector<HTPPage> pages;

// Display settings loaded from JSON
static f32 s_overlayAlpha = 0.55f;
static f32 s_bodyLineSpacing = 0.15f;
static f32 s_pageIndY = -0.88f;
static f32 s_pageIndScale = 0.8f;
static f32 s_pageIndR = 0.6f, s_pageIndG = 0.6f, s_pageIndB = 0.6f;

// ----------------------------------------------------------------------------
// Sprite illustration system
// ----------------------------------------------------------------------------
struct PageSprite {
    AEGfxTexture* tex{nullptr};
    AEGfxVertexList* mesh{nullptr};
    int page{0};
    f32 screenX{0.6f};
    f32 screenY{0.0f};
    f32 sizeX{80.0f}; // width  in world units
    f32 sizeY{80.0f}; // height in world units
};

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

// Collectible meshes (colour-drawn, no texture) for page 5
static AEGfxVertexList* s_starMesh = nullptr;
static AEGfxVertexList* s_gemMesh = nullptr;
static AEGfxVertexList* s_leafMesh = nullptr;
static float s_collectRot = 0.0f;   // shared rotation angle (radians)
static float s_collectPulse = 0.0f; // shared pulse timer

static AEGfxVertexList* MakeUvMesh(float u0, float u1) {
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, u0, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f, -0.5f, 0.5f,
                0xFFFFFFFF, u0, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, u1, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, u1, 0.0f, -0.5f, 0.5f,
                0xFFFFFFFF, u0, 0.0f);
    return AEGfxMeshEnd();
}

static void CreateCollectibleMeshes() {
    // Star (5-pointed) -- matches Collectible.cpp
    AEGfxMeshStart();
    constexpr int kStarPoints = 5;
    constexpr float kOuterRadius = 0.5f;
    constexpr float kInnerRadius = 0.25f;
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
    // Gem (diamond)
    AEGfxMeshStart();
    AEGfxTriAdd(0.0f, 0.5f, 0xFFFF00FF, 0.5f, 1.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f, 0.0f,
                0xFFFF00FF, 1.0f, 0.5f);
    AEGfxTriAdd(0.0f, -0.5f, 0xFFFF00FF, 0.5f, 0.0f, -0.5f, 0.0f, 0xFFFF00FF, 0.0f, 0.5f, 0.5f,
                0.0f, 0xFFFF00FF, 1.0f, 0.5f);
    s_gemMesh = AEGfxMeshEnd();
    // Leaf (teardrop)
    AEGfxMeshStart();
    constexpr int kLeafSeg = 12;
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

static void DrawCollectibleIcons() {
    if (!s_starMesh || !s_gemMesh || !s_leafMesh)
        return;
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    float winW = static_cast<float>(AEGfxGetWindowWidth());
    float winH = static_cast<float>(AEGfxGetWindowHeight());
    // Pulse scale matching Collectible.cpp: sinf(pulseTimer)*0.1+1.0, base scale 30
    float pulse = sinf(s_collectPulse) * 0.1f + 1.0f;
    float iconSize = 60.0f * pulse;
    // Each collectible spins at a slightly different speed (matching rotationSpeed_ variance)
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
        const auto& ic = icons[i];
        float worldX = ic.offsetX * winW * 0.5f;
        float worldY = -0.20f * winH * 0.5f;
        float rot = s_collectRot * rotSpeeds[i];
        AEMtx33 S, R, T, W;
        AEMtx33Scale(&S, iconSize, iconSize);
        AEMtx33Rot(&R, rot);
        AEMtx33Trans(&T, worldX, worldY);
        AEMtx33Concat(&W, &R, &S); // rotate then scale
        AEMtx33Concat(&W, &T, &W); // then translate
        AEGfxSetColorToMultiply(ic.r, ic.g, ic.b, 1.0f);
        AEGfxSetTransform(W.m);
        AEGfxMeshDraw(ic.mesh, AE_GFX_MDM_TRIANGLES);
    }
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

// Build and cache the full-screen overlay quad
static void EnsureOverlayMesh() {
    if (overlayMesh != nullptr)
        return;
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.0f, 1.0f, 0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFF000000, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f, 0.5f, 0.5f, 0xFF000000, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFF000000, 0.0f, 0.0f);
    overlayMesh = AEGfxMeshEnd();
}

// Draw the dark overlay over the live background
static void DrawOverlay(f32 alpha) {
    EnsureOverlayMesh();

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

// Draw text centered horizontally at a given screen-space y (-1..+1)
static void DrawCenteredText(s8 font, const char* text, f32 screenY, f32 size, f32 r, f32 g,
                             f32 b) {
    if (!text || strlen(text) == 0)
        return;

    f32 tw = 0.0f, th = 0.0f;
    AEGfxGetPrintSize(font, text, size, &tw, &th);
    f32 xPos = -tw / 2.0f;

    // Black outline for legibility over the live background
    AEGfxPrint(font, text, xPos - 0.002f, screenY - 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos + 0.002f, screenY - 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos - 0.002f, screenY + 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos + 0.002f, screenY + 0.002f, size, 0.f, 0.f, 0.f, 1.f);
    AEGfxPrint(font, text, xPos, screenY, size, r, g, b, 1.0f);
}

// Draw the page indicator (e.g. "2 / 4")
static void DrawPageIndicator() {
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

// ----------------------------------------------------------------------------
// State functions
// ----------------------------------------------------------------------------

void LoadControls() {
    // Shared background (same as MainMenu and Credits)
    MenuBackground::Load();

    // Load fonts
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    bodyFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // Load pages
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

    // Load display settings
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

    // Load sprite illustrations
    s_pipeTex = AEGfxTextureLoad("Assets/Textures/overgrown_pipe_end.png");
    s_portalTex = AEGfxTextureLoad("Assets/Textures/wormhole.png");
    s_flowerTex = AEGfxTextureLoad("Assets/Textures/pink_flower_sprite_sheet.png");

    s_fullUvMesh = MakeUvMesh(0.0f, 1.0f);
    s_flowerMesh = MakeUvMesh(0.0f, 1.0f / 4.0f);
    s_portalMesh = MakeUvMesh(0.0f, 1.0f);

    s_magicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");
    s_magicMesh = MakeUvMesh(0.0f, 1.0f);
    s_mossTex = AEGfxTextureLoad("Assets/Textures/moss_sprite_sheet.png");
    s_mossMesh = MakeUvMesh(0.0f, 1.0f / 3.0f); // frame 0 (idle) of moss sheet
    CreateCollectibleMeshes();

    // {tex, mesh, page, screenX, screenY, sizeX, sizeY}
    // Portal in-game is 30w x 60h -- mirror that 1:2 ratio here scaled up
    s_sprites.clear();
    s_sprites.push_back({s_pipeTex, s_fullUvMesh, 2, -0.42f, 0.35f, 80.0f, 80.0f});    // pipe
    s_sprites.push_back({s_portalTex, s_portalMesh, 2, 0.42f, 0.10f, 60.0f, 120.0f});  // portal 1:2
    s_sprites.push_back({s_flowerTex, s_flowerMesh, 2, -0.42f, -0.10f, 80.0f, 80.0f}); // flower
    // Page 3: magic terrain tile (left) + portal (right)
    s_sprites.push_back({s_magicTex, s_magicMesh, 3, 0.52f, 0.05f, 80.0f, 80.0f}); // magic terrain
    s_sprites.push_back({s_portalTex, s_portalMesh, 3, -0.52f, 0.20f, 60.0f, 120.0f}); // portal 1:2
    // Page 4: moss (idle frame 0)
    s_sprites.push_back({s_mossTex, s_mossMesh, 4, 0.42f, -0.05f, 80.0f, 80.0f}); // moss

    // Load navigation button assets
    buttonNext.loadMesh();
    buttonNext.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonPrevious.loadMesh();
    buttonPrevious.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
}

void InitializeControls() {
    MenuBackground::Initialize();

    // Animations
    animManager.Clear();
    animManager.Add(&screenFader);
    animManager.Add(&someOtherCoolAnimation);
    animManager.InitializeAll();
    // Reset to first page every time we enter
    currentPage = 1;

    buttonNext.initFromJson("how_to_play_buttons", "Next");
    buttonNext.setTextFont(buttonFont);

    buttonPrevious.initFromJson("how_to_play_buttons", "Previous");
    buttonPrevious.setTextFont(buttonFont);

    buttonBack.initFromJson("how_to_play_buttons", "Back");
    buttonBack.setTextFont(buttonFont);
}

void UpdateControls(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        int totalPages = static_cast<int>(pages.size());
        if (totalPages == 0)
            totalPages = 1;

        // Keyboard navigation
        if (AEInputCheckTriggered(AEVK_RIGHT) || AEInputCheckTriggered(AEVK_SPACE)) {
            if (currentPage < totalPages)
                ++currentPage;
            else
                screenFader.StartFadeOut(&GSM, StateId::MainMenu);
        }
        if (AEInputCheckTriggered(AEVK_LEFT)) {
            if (currentPage > 1)
                --currentPage;
        }
        if (AEInputCheckTriggered(AEVK_ESCAPE)) {
            screenFader.StartFadeOut(&GSM, StateId::MainMenu);
        }

        if (buttonNext.checkMouseClick()) {
            if (currentPage < totalPages)
                ++currentPage;
            else
                screenFader.StartFadeOut(&GSM, StateId::MainMenu);
        }

        if (buttonPrevious.checkMouseClick()) {
            if (currentPage > 1)
                --currentPage;
        }

        if (buttonBack.checkMouseClick()) {
            screenFader.StartFadeOut(&GSM, StateId::MainMenu);
        }

        if (AEInputCheckCurr(AEVK_LBUTTON)) {
            bool hitDirt = MenuBackground::DestroyDirtAtMouse(20.0f);
            if (hitDirt) {
                g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
            }
        }

        // Update button transforms (recalculates centered text positions)
        buttonNext.updateTransform();
        buttonPrevious.updateTransform();
        buttonBack.updateTransform();

        // Advance collectible icon animation
        s_collectRot += deltaTime * 0.5f;
        s_collectPulse += deltaTime * 3.0f;

        // Keep background alive
        MenuBackground::Update(deltaTime);
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }
    // Animations
    animManager.UpdateAll(deltaTime);
}

void DrawControls() {
    // 1. Live background (terrain + fluid + portals)
    MenuBackground::Draw();

    // 2. Dark overlay to make text readable
    DrawOverlay(s_overlayAlpha);

    // 3. Draw title and body for the current page
    if (currentPage >= 1 && currentPage <= static_cast<int>(pages.size())) {
        const HTPPage& page = pages[currentPage - 1];

        DrawCenteredText(titleFont, page.title.content_.c_str(), page.title.y_, page.title.scale_,
                         page.title.r_, page.title.g_, page.title.b_);

        std::istringstream stream(page.body.content_);
        std::string line;
        f32 lineY = page.body.y_;
        while (std::getline(stream, line)) {
            DrawCenteredText(bodyFont, line.c_str(), lineY, page.body.scale_, page.body.r_,
                             page.body.g_, page.body.b_);
            lineY -= s_bodyLineSpacing;
        }
    }

    // 4. Page indicator (e.g. "2 / 4")
    DrawPageIndicator();

    // 5a. Collectible icons on page 5
    if (currentPage == 5) {
        DrawCollectibleIcons();
    }

    // 5b. Sprite illustrations for current page
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

    // 6. Navigation buttons
    buttonNext.draw();
    if (currentPage > 1) {
        buttonPrevious.draw();
    }
    buttonBack.draw();
    animManager.DrawAll();
    g_debugSystem.drawAll();
}

void FreeControls() {
    MenuBackground::Free();
    animManager.FreeAll();
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
    if (s_magicMesh) {
        AEGfxMeshFree(s_magicMesh);
        s_magicMesh = nullptr;
    }
    if (s_magicTex) {
        AEGfxTextureUnload(s_magicTex);
        s_magicTex = nullptr;
    }
    if (s_mossMesh) {
        AEGfxMeshFree(s_mossMesh);
        s_mossMesh = nullptr;
    }
    if (s_mossTex) {
        AEGfxTextureUnload(s_mossTex);
        s_mossTex = nullptr;
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
    s_sprites.clear();
}

void UnloadControls() {
    MenuBackground::Unload();

    // Unload fonts
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

    // Unload buttons
    buttonNext.unload();
    buttonPrevious.unload();
    buttonBack.unload();
}
