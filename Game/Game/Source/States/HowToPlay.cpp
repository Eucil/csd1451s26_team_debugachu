#include "States/HowToPlay.h"

#include <cstdio>
#include <cstring>
#include <sstream>

#include <AEEngine.h>

#include "AudioSystem.h"
#include "Button.h"
#include "ConfigManager.h"
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

void LoadHowToPlay() {
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

    // Load navigation button assets
    buttonNext.loadMesh();
    buttonNext.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonPrevious.loadMesh();
    buttonPrevious.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
}

void InitializeHowToPlay() {
    MenuBackground::Initialize();

    // Reset to first page every time we enter
    currentPage = 1;

    buttonNext.initFromJson("how_to_play_buttons", "Next");
    buttonNext.setTextFont(buttonFont);

    buttonPrevious.initFromJson("how_to_play_buttons", "Previous");
    buttonPrevious.setTextFont(buttonFont);

    buttonBack.initFromJson("how_to_play_buttons", "Back");
    buttonBack.setTextFont(buttonFont);
}

void UpdateHowToPlay(GameStateManager& GSM, f32 deltaTime) {
    int totalPages = static_cast<int>(pages.size());
    if (totalPages == 0)
        totalPages = 1;

    // Keyboard navigation
    if (AEInputCheckTriggered(AEVK_RIGHT) || AEInputCheckTriggered(AEVK_SPACE)) {
        if (currentPage < totalPages)
            ++currentPage;
        else
            GSM.nextState_ = StateId::MainMenu;
    }
    if (AEInputCheckTriggered(AEVK_LEFT)) {
        if (currentPage > 1)
            --currentPage;
    }
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        GSM.nextState_ = StateId::MainMenu;
    }

    if (buttonNext.checkMouseClick()) {
        if (currentPage < totalPages)
            ++currentPage;
        else
            GSM.nextState_ = StateId::MainMenu;
    }

    if (buttonPrevious.checkMouseClick()) {
        if (currentPage > 1)
            --currentPage;
    }

    if (buttonBack.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
    }

    if (AEInputCheckCurr(AEVK_LBUTTON)) {
        bool hitDirt = MenuBackground::DestroyDirtAtMouse(20.0f);
        if (hitDirt) {
            g_audioSystem.playSound("dirt_break", "sfx", 0.25f, 1.0f);
        }
    }

    // Update button transforms (recalculates centered text positions)
    buttonNext.updateTransform();
    buttonPrevious.updateTransform();
    buttonBack.updateTransform();

    // Keep background alive
    MenuBackground::Update(deltaTime);
}

void DrawHowToPlay() {
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

    // 5. Navigation buttons
    buttonNext.draw();
    if (currentPage > 1) {
        buttonPrevious.draw();
    }
    buttonBack.draw();
}

void FreeHowToPlay() {
    MenuBackground::Free();

    if (overlayMesh) {
        AEGfxMeshFree(overlayMesh);
        overlayMesh = nullptr;
    }
}

void UnloadHowToPlay() {
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
