#include "States/Credits.h"
#include "Button.h"
#include "GameStateManager.h"
#include "Utils.h"
#include <cstdio>
#include <cstring>


// Helper function to calculate centered X position
static float GetCenteredX(const char* text, float textSize) {
    if (!text || strlen(text) == 0)
        return 0.0f;

    float charWidth = textSize * 0.12f;
    float textWidth = strlen(text) * charWidth;
    return -textWidth / 2.0f;
}

// Credits list - NULL terminated
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

// Static variables
static f32 yPos = 0.0f;             // Starting Y position
static s8 font;                     // Font for credits
static Button backButton;           // Back button to return to main menu
static float lineSpacing = 80.0f;   // Spacing between lines
static float scrollSpeed = -200.0f; // Scroll speed in pixels per second
static int numLines = 0;            // Number of credit lines

// Simple dirt background
static AEGfxTexture* dirtTexture = nullptr;
static AEGfxVertexList* screenQuadMesh = nullptr;

void LoadCredits() {
    // Load font
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 36);

    // Calculate number of credit lines
    numLines = 0;
    while (credits[numLines] != NULL) {
        numLines++;
    }
    printf("Credits: Loaded %d lines\n", numLines);

    // Reset scroll position
    yPos = 950.0f;

    // Load dirt texture (same as Level 1)
    dirtTexture = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");

    // Create a simple quad mesh for full-screen rendering
    AEGfxMeshStart();
    // Two triangles forming a full-screen quad (with texture coordinates)
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f,
                0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    screenQuadMesh = AEGfxMeshEnd();

    // Initialize back button
    backButton.loadMesh();
    backButton.loadTexture("Assets/Textures/brown_button.png");
    backButton.initFromJson("main_menu_buttons", "Back");
    backButton.setTransform({-650.0f, -350.0f}, {150.0f, 50.0f});
    backButton.setText("Back", -0.875f, -0.8f, 0.75f, 1.0f, 1.0f, 1.0f, 1.0f);
    backButton.setTextFont(font);
}

void InitializeCredits() {
    // Reset scroll position
    yPos = -450.0f;
    printf("Credits: Initialized with yPos = %f\n", yPos);
}

void UpdateCredits(GameStateManager& GSM, f32 deltaTime) {
    // Scroll credits UPWARD
    yPos -= scrollSpeed * deltaTime;

    // Check for input to skip or go back
    if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_SPACE)) {
        GSM.nextState_ = StateId::MainMenu;
    }

    // Check back button
    if (backButton.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
    }
    backButton.updateTransform();

    // Auto return to main menu when credits finish
    float lastLineY = yPos - (numLines - 1) * lineSpacing;
    if (lastLineY > 450.0f) {
        printf("Credits: Finished scrolling, lastLineY=%f\n", lastLineY);
        GSM.nextState_ = StateId::MainMenu;
    }
}

void DrawCredits() {
    // Draw full-screen dirt background
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetTransparency(1.0f);

    // Create a full-screen quad transform
    AEMtx33 scale, trans, world;
    AEMtx33Scale(&scale, 1600.0f, 900.0f); // Screen size
    AEMtx33Trans(&trans, 0.0f, 0.0f);      // Center of screen
    AEMtx33Concat(&world, &trans, &scale);

    // Draw the dirt texture stretched across the whole screen
    AEGfxSetTransform(world.m);
    AEGfxTextureSet(dirtTexture, 0, 0); // Use full texture (no tiling)
    AEGfxMeshDraw(screenQuadMesh, AE_GFX_MDM_TRIANGLES);

    // Add a semi-transparent dark overlay to make text readable
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.4f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

    // Same full-screen quad for overlay
    AEGfxSetTransform(world.m);
    AEGfxMeshDraw(screenQuadMesh, AE_GFX_MDM_TRIANGLES);

    // Reset for text rendering
    AEGfxSetTransparency(1.0f);

    // Draw credits text
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    int linesDrawn = 0;
    for (int i = 0; i < numLines; i++) {
        float yLine = yPos - i * lineSpacing;

        if (yLine > -450.0f && yLine < 450.0f) {
            const char* currentLine = credits[i];

            if (strlen(currentLine) == 0) {
                linesDrawn++;
                continue;
            }

            float textSize = 0.6f;
            float r = 1.0f, g = 1.0f, b = 1.0f;
            float xPos = 0.0f;

            // Text positioning code (keep your existing one)
            if (strcmp(currentLine, "WWW.DIGIPEN.EDU") == 0) {
                textSize = 0.6f;
                xPos = -0.22f;
            } else if (strcmp(currentLine, "All content copyright 2025 DigiPen Institute of "
                                           "Technology Singapore. All Rights Reserved") == 0) {
                r = 0.7f;
                g = 0.7f;
                b = 0.7f;
                textSize = 0.4f;
                xPos = -0.80f;
            } else if (strcmp(currentLine, "TEAM") == 0) {
                r = 1.0f;
                g = 0.0f;
                b = 0.0f;
                textSize = 0.9f;
                xPos = -0.10f;
            } else if (strcmp(currentLine, "Debugachu") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f;
                textSize = 0.8f;
                xPos = -0.175f;
            } else if (strcmp(currentLine, "DEVELOPED BY") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f;
                textSize = 0.8f;
                xPos = -0.224f;
            } else if (strcmp(currentLine, "Woo Guang Theng") == 0) {
                textSize = 0.6f;
                xPos = -0.212f;
            } else if (strcmp(currentLine, "Sean Lee Hong Wei") == 0) {
                textSize = 0.6f;
                xPos = -0.235f;
            } else if (strcmp(currentLine, "Chia Hanxin") == 0) {
                textSize = 0.6f;
                xPos = -0.16f;
            } else if (strcmp(currentLine, "Han Tianchou") == 0) {
                textSize = 0.6f;
                xPos = -0.17f;
            } else if (strcmp(currentLine, "FACULTY AND ADVISORS") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f;
                textSize = 0.8f;
                xPos = -0.35f;
            } else if (strcmp(currentLine, "Gerald Wong") == 0) {
                textSize = 0.6f;
                xPos = -0.16f;
            } else if (strcmp(currentLine, "Soroor Malekmohammadi Faradounbeh") == 0) {
                textSize = 0.6f;
                xPos = -0.435f;
            } else if (strcmp(currentLine, "Tommy Tan") == 0) {
                textSize = 0.6f;
                xPos = -0.13f;
            } else if (strcmp(currentLine, "CREATED AT") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f;
                textSize = 0.8f;
                xPos = -0.183f;
            } else if (strcmp(currentLine, "DigiPen Institute of Technology Singapore") == 0) {
                textSize = 0.6f;
                xPos = -0.52f;
            } else if (strcmp(currentLine, "PRESIDENT") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f;
                textSize = 0.8f;
                xPos = -0.16f;
            } else if (strcmp(currentLine, "Claude Comair") == 0) {
                textSize = 0.6f;
                xPos = -0.18f;
            } else if (strcmp(currentLine, "EXECUTIVES") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f;
                textSize = 0.8f;
                xPos = -0.175f;
            } else if (strcmp(currentLine, "Chu Jason Yeu Tat       Michael Gats") == 0) {
                textSize = 0.6f;
                xPos = -0.48f;
            } else if (strcmp(currentLine, "Tan Chek Ming       Prasanna Kumar Ghali") == 0) {
                textSize = 0.6f;
                xPos = -0.52f;
            } else if (strcmp(currentLine, "Mandy Wong       Johnny Deek") == 0) {
                textSize = 0.6f;
                xPos = -0.35f;
            } else if (strcmp(currentLine, "SPECIAL THANKS") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f;
                textSize = 0.8f;
                xPos = -0.23f;
            } else if (strcmp(currentLine, "All playtesters") == 0) {
                textSize = 0.6f;
                xPos = -0.17f;
            }

            float screenY = yLine / 450.0f;

            // Draw text with black outline for readability on dirt
            AEGfxPrint(font, currentLine, xPos - 0.002f, screenY - 0.002f, textSize, 0.0f, 0.0f,
                       0.0f, 1.0f);
            AEGfxPrint(font, currentLine, xPos + 0.002f, screenY - 0.002f, textSize, 0.0f, 0.0f,
                       0.0f, 1.0f);
            AEGfxPrint(font, currentLine, xPos - 0.002f, screenY + 0.002f, textSize, 0.0f, 0.0f,
                       0.0f, 1.0f);
            AEGfxPrint(font, currentLine, xPos + 0.002f, screenY + 0.002f, textSize, 0.0f, 0.0f,
                       0.0f, 1.0f);

            // Draw main text
            AEGfxPrint(font, currentLine, xPos, screenY, textSize, r, g, b, 1.0f);
            linesDrawn++;
        }
    }

    // Draw back button
    backButton.draw();
}

void FreeCredits() {
    if (dirtTexture) {
        AEGfxTextureUnload(dirtTexture);
        dirtTexture = nullptr;
    }

    if (screenQuadMesh) {
        AEGfxMeshFree(screenQuadMesh);
        screenQuadMesh = nullptr;
    }

    backButton.unload();
}

void UnloadCredits() { AEGfxDestroyFont(font); }