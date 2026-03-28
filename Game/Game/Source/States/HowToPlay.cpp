#include "States/HowToPlay.h"

#include <cstdio>
#include <cstring>

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

// Navigation buttons
static Button nextButton;
static Button backButton;
static Button exitButton;

// Full-screen semi-transparent overlay mesh (created once, reused)
static AEGfxVertexList* overlayMesh = nullptr;

std::vector<std::string> howToPlayData;

const int LINES_PER_PAGE = 7;

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
/*

*/

// Draw the page indicator dots (e.g. "● ● ○ ○")
static void DrawPageIndicator() {
    int totalPages = (howToPlayData.size() + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
    if (totalPages == 0)
        totalPages = 1;
    // Build a string like "1 / 4"
    char buf[16];
    sprintf_s(buf, sizeof(buf), "%d / %d", currentPage, totalPages);

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

    // Load json data
    Json::Value htpSection = g_configManager.getSection("how_to_play", "HowToPlayInfo");
    if (!htpSection.isNull() && htpSection.isMember("Lines") && htpSection["Lines"].isArray()) {
        const Json::Value& linesArray = htpSection["Lines"];
        howToPlayData.clear();
        for (const Json::Value& line : linesArray) {
            howToPlayData.push_back(line.asString());
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
    int totalPages = (howToPlayData.size() + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
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

    if (nextButton.checkMouseClick()) {
        if (currentPage < totalPages)
            ++currentPage;
        else
            GSM.nextState_ = StateId::MainMenu;
    }

    if (backButton.checkMouseClick()) {
        if (currentPage > 1)
            --currentPage;
    }

    if (exitButton.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
    }

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

    size_t startIndex = (currentPage - 1) * LINES_PER_PAGE;
    size_t endIndex = startIndex + LINES_PER_PAGE;
    if (endIndex > howToPlayData.size()) {
        endIndex = howToPlayData.size();
    }

    f32 startY = 0.50f;
    f32 stepY = 0.15f;
    int displayLine = 0;

    for (size_t i = startIndex; i < endIndex; i++) {
        std::string currentLine = howToPlayData[i];

        if (currentLine.empty()) {
            displayLine++;
            continue;
        }

        f32 screenY = startY - (displayLine * stepY);
        displayLine++;

        f32 textSize = 0.6f;
        f32 r = 1.0f, g = 1.0f, b = 1.0f;
        s8 currentFont = bodyFont;

        // Auto-detect headers and style them
        if (currentLine == "CONTROLS" || currentLine == "OBJECTIVE" || currentLine == "PORTALS" ||
            currentLine == "TIPS") {
            r = 1.0f;
            g = 0.84f;
            b = 0.0f;
            textSize = 0.8f;
            currentFont = titleFont;
        }

        DrawCenteredText(currentFont, currentLine.c_str(), screenY, textSize, r, g, b);
    }

    // 6. Page indicator (e.g. "2 / 4")
    DrawPageIndicator();

    //// 7. Navigation hint at very bottom
    // DrawCenteredText(bodyFont, "[ < ] [ > ] or ARROW KEYS to navigate", -0.55f, 0.5f, 0.5f, 0.5f,
    //                  0.5f);

    // 8. Navigation buttons
    nextButton.draw();
    if (currentPage > 1) {
        backButton.draw();
    }
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