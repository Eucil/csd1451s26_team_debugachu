#include "States/Credits.h"

#include <cstdio>
#include <cstring>
#include <string>

#include <AEEngine.h>

#include "AudioSystem.h"
#include "Button.h"
#include "ConfigManager.h"
#include "GameStateManager.h"
#include "MenuBackground.h" // <-- shared background
#include "Utils.h"

// ----------------------------------------------------------------------------
// Credits text list (NULL-terminated)
// ----------------------------------------------------------------------------
/*
static const char* credits[] = {
    "",
    "",
    "WWW.DIGIPEN.EDU",
    "All content copyright 2026 DigiPen Institute of Technology Singapore. All Rights Reserved",
    "",
    "TEAM",
    "Debugachu",
    "",
    "DEVELOPED BY",
    "Woo Guang Theng",
    "Sean Lee Hong Wei",
    "Chia Hanxin",
    "Han Tianchou",
    "",
    "FACULTY AND ADVISORS",
    "Gerald Wong",
    "Soroor Malekmohammadi Faradounbeh",
    "Tommy Tan",
    "",
    "CREATED AT",
    "DigiPen Institute of Technology Singapore",
    "",
    "PRESIDENT",
    "Claude Comair",
    "",
    "EXECUTIVES",
    "Chu Jason Yeu Tat       Michael Gats",
    "Tan Chek Ming       Prasanna Kumar Ghali",
    "Mandy Wong       Johnny Deek",
    "",
    "SPECIAL THANKS",
    "All playtesters",
    "",
    NULL};
*/

// ----------------------------------------------------------------------------
// Static state
// ----------------------------------------------------------------------------
static f32 yPos = 0.0f;
static s8 creditsFont = 0;
static s8 buttonFont = 0;
static Button buttonBack;
static float lineSpacing = 80.0f;
static float scrollSpeed = -200.0f; // negative = scroll up
std::vector<std::string> creditsData;

void LoadCredits() {
    // Load the destructible terrain background
    MenuBackground::Load();

    // Load credits font
    creditsFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 36);

    // Load button font
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    Json::Value Lines = g_configManager.getSection("credits", "CreditsInfo");

    if (!Lines.isNull() && Lines.isMember("Lines") && Lines["Lines"].isArray()) {
        const Json::Value& linesArray = Lines["Lines"];

        // Clear old data just in case
        creditsData.clear();

        // Loop through the linearray and read into creditsData instead
        for (Json::Value const& line : linesArray) {
            std::string str = line.asString();
            creditsData.push_back(str);
        }
    }

    // Load back button
    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
}

void InitializeCredits() {
    // Initialize the shared background simulation
    MenuBackground::Initialize();

    // Reset credits scroll position
    yPos = -450.0f;
    printf("Credits: Initialized with yPos = %f\n", yPos);

    buttonBack.initFromJson("credits_buttons", "Back");
    buttonBack.setTextFont(buttonFont);
}

void UpdateCredits(GameStateManager& GSM, f32 deltaTime) {
    // Scroll credits upward
    yPos -= scrollSpeed * deltaTime;

    // Input to skip or return
    if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_SPACE)) {
        GSM.nextState_ = StateId::MainMenu;
    }

    if (buttonBack.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
    }
    buttonBack.updateTransform();

    // Auto-return when all lines have scrolled past
    float lastLineY = yPos - (creditsData.size() - 1) * lineSpacing;
    if (lastLineY > 450.0f) {
        printf("Credits: Finished scrolling, returning to MainMenu\n");
        GSM.nextState_ = StateId::MainMenu;
    }

    // Left-click held: destroy dirt + VFX + audio (same as MainMenu)
    if (AEInputCheckCurr(AEVK_LBUTTON)) {
        bool hitDirt = MenuBackground::DestroyDirtAtMouse(20.0f);
        if (hitDirt) {
            g_audioSystem.playSound("dirt_break", "sfx", 0.25f, 1.0f);
        }
    }

    // Update the shared background (fluid, portals, etc. keep running)
    MenuBackground::Update(deltaTime);
}

void DrawCredits() {
    // Draw the shared live background first
    MenuBackground::Draw();

    // Semi-transparent dark overlay so text is readable
    static AEGfxVertexList* overlayMesh = nullptr;
    if (overlayMesh == nullptr) {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFF000000, 0.0f, 1.0f, 0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f,
                    -0.5f, 0.5f, 0xFF000000, 0.0f, 0.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFF000000, 1.0f, 1.0f, 0.5f, 0.5f, 0xFF000000, 1.0f, 0.0f, -0.5f,
                    0.5f, 0xFF000000, 0.0f, 0.0f);
        overlayMesh = AEGfxMeshEnd();
    }

    AEMtx33 scale, trans, world;
    AEMtx33Scale(&scale, (f32)AEGfxGetWindowWidth(), (f32)AEGfxGetWindowHeight());
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&world, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
    AEGfxSetTransform(world.m);
    AEGfxMeshDraw(overlayMesh, AE_GFX_MDM_TRIANGLES);

    AEGfxSetTransparency(1.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // ----------------------------------------------------------------
    // Index-based styling table.
    // Using array indices avoids strcmp on strings that contain
    // multi-byte UTF-8 characters (like (c)), which can cause
    // mismatches depending on the compiler's codepage setting.
    //
    // Index map (matches the credits[] array order):
    //   0,1  = empty lines
    //   2    = WWW.DIGIPEN.EDU
    //   3    = copyright line   <-- contains (c), styled smaller/grey
    //   4    = empty
    //   5    = TEAM             <-- red header
    //   6    = Debugachu        <-- gold
    //   7    = empty
    //   8    = DEVELOPED BY     <-- gold header
    //   9-12 = dev names
    //   13   = empty
    //   14   = FACULTY AND ADVISORS <-- gold header
    //   15-17= faculty names
    //   18   = empty
    //   19   = CREATED AT       <-- gold header
    //   20   = DigiPen SG
    //   21   = empty
    //   22   = PRESIDENT        <-- gold header
    //   23   = Claude Comair
    //   24   = empty
    //   25   = EXECUTIVES       <-- gold header
    //   26-28= executive names
    //   29   = empty
    //   30   = SPECIAL THANKS   <-- gold header
    //   31   = All playtesters
    //   32   = empty
    // ----------------------------------------------------------------
    for (size_t i = 0; i < creditsData.size(); i++) {
        float yLine = yPos - i * lineSpacing;

        if (yLine <= -450.0f || yLine >= 450.0f)
            continue;

        std::string currentLine = creditsData[i];
        if (currentLine.size() == 0)
            continue;

        // Default style
        float textSize = 0.6f;
        float r = 1.0f, g = 1.0f, b = 1.0f;

        // Style by index -- no strcmp needed
        switch (i) {
        case 3: // copyright line
            r = 0.7f;
            g = 0.7f;
            b = 0.7f;
            textSize = 0.4f;
            break;
        case 5: // TEAM
            r = 1.0f;
            g = 0.0f;
            b = 0.0f;
            textSize = 0.9f;
            break;
        case 6: // Debugachu
            r = 1.0f;
            g = 0.84f;
            b = 0.0f;
            textSize = 0.8f;
            break;
        case 8:  // DEVELOPED BY
        case 14: // FACULTY AND ADVISORS
        case 19: // CREATED AT
        case 22: // PRESIDENT
        case 25: // EXECUTIVES
        case 30: // SPECIAL THANKS
            r = 1.0f;
            g = 0.84f;
            b = 0.0f;
            textSize = 0.8f;
            break;
        default:
            textSize = 0.6f;
            break;
        }

        // Auto-center: measure text width and compute xPos
        float textWidth = 0.0f, textHeight = 0.0f;
        AEGfxGetPrintSize(creditsFont, currentLine.c_str(), textSize, &textWidth, &textHeight);
        float xPos = -textWidth / 2.0f;

        float screenY = yLine / 450.0f;

        // Black outline for readability
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos - 0.002f, screenY - 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos + 0.002f, screenY - 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos - 0.002f, screenY + 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos + 0.002f, screenY + 0.002f, textSize, 0.f,
                   0.f, 0.f, 1.f);

        // Main colored text
        AEGfxPrint(creditsFont, currentLine.c_str(), xPos, screenY, textSize, r, g, b, 1.0f);
    }

    // Draw back button on top
    buttonBack.draw();
}
void FreeCredits() {
    // Free the shared background simulation objects
    MenuBackground::Free();
}

void UnloadCredits() {
    // Unload shared GPU assets
    MenuBackground::Unload();

    if (creditsFont) {
        AEGfxDestroyFont(creditsFont);
        creditsFont = 0;
    }

    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }

    buttonBack.unload();
}