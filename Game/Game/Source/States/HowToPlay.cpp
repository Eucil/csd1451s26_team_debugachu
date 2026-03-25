#include "States/HowToPlay.h"

#include <cstdio>
#include <cstring>

#include <AEEngine.h>

#include "AudioSystem.h"
#include "Button.h"
#include "GameStateManager.h"
#include "MenuBackground.h"
#include "Utils.h"

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
#define HOW_TO_PLAY_TOTAL_PAGES 4

// ----------------------------------------------------------------------------
// Static state
// ----------------------------------------------------------------------------
static int currentPage = 1; // 1-indexed current page
static s8 titleFont = 0;    // large font for page titles
static s8 bodyFont = 0;     // smaller font for body text

// Navigation buttons
static Button nextButton;
static Button backButton;
static Button exitButton;

// One texture slot per page for the illustration image
static AEGfxTexture* pageTextures[HOW_TO_PLAY_TOTAL_PAGES] = {nullptr};

// Full-screen semi-transparent overlay mesh (created once, reused)
static AEGfxVertexList* overlayMesh = nullptr;

// ----------------------------------------------------------------------------
// Page data -- edit these to match your game content
// ----------------------------------------------------------------------------
struct PageData {
    const char* title;
    const char* lines[6]; // up to 6 body lines, NULL to stop
    int lineCount;
    const char* texturePath; // nullptr if no image for this page
};

static const PageData pages[HOW_TO_PLAY_TOTAL_PAGES] = {
    // Page 1 - Controls
    {"CONTROLS",
     {"LEFT CLICK  - Dig / Destroy dirt", "Navigate to Level Selector",
      "and choose a level to play", nullptr, nullptr, nullptr},
     3,
     nullptr},
    // Page 2 - Objective
    {"OBJECTIVE",
     {"Guide water from the SOURCE", "to the GOAL by digging", "through the terrain.",
      "Fill the goal to win!", nullptr, nullptr},
     4,
     nullptr},
    // Page 3 - Portals
    {"PORTALS",
     {"Place PORTALS to teleport water", "through walls and obstacles.",
      "Right Click to place a portal.", "Linked portals share a color.", nullptr, nullptr},
     4,
     nullptr},
    // Page 4 - Tips
    {"TIPS",
     {"Water flows with gravity.", "Stone terrain cannot be dug.", "Avoid Moss to conserve water",
      "Press R to restart a level.", nullptr},
     5,
     nullptr}};

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

// Draw current page illustration if it has one
static void DrawPageTexture(int pageIndex) {
    if (pageTextures[pageIndex] == nullptr)
        return;

    // Draw the image centered in the upper half of the screen
    static AEGfxVertexList* imgMesh = nullptr;
    if (imgMesh == nullptr) {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
                    -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f,
                    0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        imgMesh = AEGfxMeshEnd();
    }

    AEMtx33 scale, trans, world;
    AEMtx33Scale(&scale, 600.0f, 300.0f); // image display size
    AEMtx33Trans(&trans, 0.0f, 150.0f);   // upper-center of screen
    AEMtx33Concat(&world, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransform(world.m);
    AEGfxTextureSet(pageTextures[pageIndex], 0.0f, 0.0f);
    AEGfxMeshDraw(imgMesh, AE_GFX_MDM_TRIANGLES);
}

// Draw the page indicator dots (e.g. "● ● ○ ○")
static void DrawPageIndicator() {
    // Build a string like "1 / 4"
    char buf[16];
    sprintf_s(buf, sizeof(buf), "%d / %d", currentPage, HOW_TO_PLAY_TOTAL_PAGES);

    f32 tw = 0.0f, th = 0.0f;
    AEGfxGetPrintSize(bodyFont, buf, 0.8f, &tw, &th);
    f32 xPos = -tw / 2.0f;
    AEGfxPrint(bodyFont, buf, xPos, -0.88f, 0.8f, 0.6f, 0.6f, 0.6f, 1.0f);
}

// ----------------------------------------------------------------------------
// State functions
// ----------------------------------------------------------------------------

void LoadHowToPlay() {
    // Shared background (same as MainMenu and Credits)
    MenuBackground::Load();

    // Load fonts
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    bodyFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 28);

    // Load per-page illustration textures (set path in pages[] above to enable)
    for (int i = 0; i < HOW_TO_PLAY_TOTAL_PAGES; ++i) {
        if (pages[i].texturePath != nullptr) {
            pageTextures[i] = AEGfxTextureLoad(pages[i].texturePath);
        }
    }

    // Load navigation button assets
    nextButton.loadMesh();
    nextButton.loadTexture("Assets/Textures/brown_button.png");
    backButton.loadMesh();
    backButton.loadTexture("Assets/Textures/brown_button.png");
    exitButton.loadMesh();
    exitButton.loadTexture("Assets/Textures/brown_button.png");
}

void InitializeHowToPlay() {
    MenuBackground::Initialize();

    // Reset to first page every time we enter
    currentPage = 1;

    // Next button - bottom right
    nextButton.setTextFont(bodyFont);
    nextButton.setTransform({300.0f, -300.0f}, {220.0f, 60.0f});
    nextButton.setText("NEXT >", 0.f, 0.f, 0.65f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Back button - bottom left
    backButton.setTextFont(bodyFont);
    backButton.setTransform({-300.0f, -300.0f}, {220.0f, 60.0f});
    backButton.setText("< BACK", 0.f, 0.f, 0.65f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Exit button - bottom center
    exitButton.setTextFont(bodyFont);
    exitButton.setTransform({0.0f, -300.0f}, {220.0f, 60.0f});
    exitButton.setText("MENU", 0.f, 0.f, 0.65f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void UpdateHowToPlay(GameStateManager& GSM, f32 deltaTime) {

    // Keyboard navigation
    if (AEInputCheckTriggered(AEVK_RIGHT) || AEInputCheckTriggered(AEVK_SPACE)) {
        if (currentPage < HOW_TO_PLAY_TOTAL_PAGES)
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

    // Mouse click on Next button
    if (nextButton.checkMouseClick()) {
        if (currentPage < HOW_TO_PLAY_TOTAL_PAGES)
            ++currentPage;
        else
            GSM.nextState_ = StateId::MainMenu;
    }

    // Mouse click on Back button
    if (backButton.checkMouseClick()) {
        if (currentPage > 1)
            --currentPage;
        else
            GSM.nextState_ = StateId::MainMenu;
    }

    // Exit to main menu
    if (exitButton.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
    }

    // Allow dirt destruction just like the main menu
    if (AEInputCheckCurr(AEVK_LBUTTON)) {
        bool hitDirt = MenuBackground::DestroyDirtAtMouse(20.0f);
        if (hitDirt) {
            g_audioSystem.playSound("dirt_break", "sfx", 0.25f, 1.0f);
        }
    }

    // Update button transforms (recalculates centered text positions)
    nextButton.updateTransform();
    backButton.updateTransform();
    exitButton.updateTransform();

    // Keep background alive
    MenuBackground::Update(deltaTime);
}

void DrawHowToPlay() {
    // 1. Live background (terrain + fluid + portals)
    MenuBackground::Draw();

    // 2. Dark overlay to make text readable
    DrawOverlay(0.55f);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // 3. Page illustration (if this page has one)
    DrawPageTexture(currentPage - 1);

    // 4. Page title  (gold, large, near top)
    DrawCenteredText(titleFont, pages[currentPage - 1].title, 0.75f, 1.0f, 1.0f, 0.84f, 0.0f);

    // 5. Body lines  (white, medium, spaced downward from center)
    const PageData& page = pages[currentPage - 1];
    f32 lineStartY = 0.25f;
    f32 lineStep = 0.16f;

    for (int i = 0; i < page.lineCount; ++i) {
        if (page.lines[i] == nullptr)
            break;
        f32 screenY = lineStartY - i * lineStep;
        DrawCenteredText(bodyFont, page.lines[i], screenY, 0.75f, 1.0f, 1.0f, 1.0f);
    }

    // 6. Page indicator (e.g. "2 / 4")
    DrawPageIndicator();

    //// 7. Navigation hint at very bottom
    // DrawCenteredText(bodyFont, "[ < ] [ > ] or ARROW KEYS to navigate", -0.55f, 0.5f, 0.5f, 0.5f,
    //                  0.5f);

    // 8. Navigation buttons
    nextButton.draw();
    backButton.draw();
    exitButton.draw();
}

void FreeHowToPlay() {
    MenuBackground::Free();

    nextButton.unload();
    backButton.unload();
    exitButton.unload();

    if (overlayMesh) {
        AEGfxMeshFree(overlayMesh);
        overlayMesh = nullptr;
    }

    for (int i = 0; i < HOW_TO_PLAY_TOTAL_PAGES; ++i) {
        if (pageTextures[i]) {
            AEGfxTextureUnload(pageTextures[i]);
            pageTextures[i] = nullptr;
        }
    }
}

void UnloadHowToPlay() {
    MenuBackground::Unload();

    if (titleFont) {
        AEGfxDestroyFont(titleFont);
        titleFont = 0;
    }
    if (bodyFont) {
        AEGfxDestroyFont(bodyFont);
        bodyFont = 0;
    }
}