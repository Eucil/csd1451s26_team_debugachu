#include "States/Credits.h"
#include "Button.h"
#include "GameStateManager.h"
#include "Utils.h"
#include <cstdio>
#include <cstring>

// Helper function to calculate centered X position (keeping for reference, but we'll use manual
// positions)
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
    "All content copyright 2025 DigiPen Institute of Technology Singapore. All Rights Reserved",
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

static struct Particle {
    AEVec2 pos;
    AEVec2 vel;
    float rotation;
    float scale;
    float lifeTime;
    float maxLifeTime;
    bool active;
} particlePool[100];

// Texture handles
static AEGfxTexture* bubbleTexture = nullptr;
static AEGfxVertexList* particleMesh = nullptr;

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

    // Create simple meshes
    particleMesh = CreateCircleMesh(8, 5.0f); // Small circle for particles

    // Initialize back button
    backButton.loadMesh();
    backButton.loadTexture("Assets/Textures/brown_button.png");
    backButton.initFromJson("main_menu_buttons", "Back");
    backButton.setTransform({-650.0f, -350.0f}, {150.0f, 50.0f});
    backButton.setText("Back", -0.875f, -0.8f, 0.75f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Initialize background elements
    for (int i = 0; i < 100; i++) {
        particlePool[i].active = false;
    }
}

void InitializeCredits() {
    // Reset scroll position
    yPos = -450.0f;
    printf("Credits: Initialized with yPos = %f\n", yPos);

    // Spawn some initial particles
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 100; j++) {
            if (!particlePool[j].active) {
                particlePool[j].active = true;
                particlePool[j].pos = {(float)(rand() % 1600) - 800.0f,
                                       (float)(rand() % 900) - 450.0f};
                particlePool[j].vel = {(float)(rand() % 40 - 20), (float)(rand() % 20 + 10)};
                particlePool[j].rotation = (float)(rand() % 360);
                particlePool[j].scale = (float)(rand() % 5 + 2);
                particlePool[j].lifeTime = 0.0f;
                particlePool[j].maxLifeTime = (float)(rand() % 5 + 3);
                break;
            }
        }
    }
}

void UpdateCredits(GameStateManager& GSM, f32 deltaTime) {
    // Scroll credits UPWARD (negative speed)
    yPos -= scrollSpeed * deltaTime; // scrollSpeed is -200, so this is yPos += 200*dt

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

    // Update background particles
    for (int i = 0; i < 100; i++) {
        if (particlePool[i].active) {
            particlePool[i].pos.x += particlePool[i].vel.x * deltaTime;
            particlePool[i].pos.y += particlePool[i].vel.y * deltaTime;
            particlePool[i].rotation += 50.0f * deltaTime;
            particlePool[i].lifeTime += deltaTime;

            // Reset if out of bounds or dead
            if (particlePool[i].pos.y > 450.0f || particlePool[i].pos.x < -800.0f ||
                particlePool[i].pos.x > 800.0f ||
                particlePool[i].lifeTime > particlePool[i].maxLifeTime) {

                particlePool[i].active = false;

                // Spawn new particle
                for (int j = 0; j < 100; j++) {
                    if (!particlePool[j].active) {
                        particlePool[j].active = true;
                        particlePool[j].pos = {(float)(rand() % 1600) - 800.0f, -450.0f};
                        particlePool[j].vel = {(float)(rand() % 40 - 20),
                                               (float)(rand() % 20 + 10)};
                        particlePool[j].rotation = 0.0f;
                        particlePool[j].scale = (float)(rand() % 5 + 2);
                        particlePool[j].lifeTime = 0.0f;
                        particlePool[j].maxLifeTime = (float)(rand() % 5 + 3);
                        break;
                    }
                }
            }
        }
    }
}

void DrawCredits() {
    // Clear background to dark blue
    AEGfxSetBackgroundColor(0.05f, 0.2f, 0.3f); // Dark blue color

    // Draw background particles
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    for (int i = 0; i < 100; i++) {
        if (particlePool[i].active) {
            // Set transform
            AEMtx33 scale, rot, trans, world;
            AEMtx33Scale(&scale, particlePool[i].scale, particlePool[i].scale);
            AEMtx33Rot(&rot, AEDegToRad(particlePool[i].rotation));
            AEMtx33Trans(&trans, particlePool[i].pos.x, particlePool[i].pos.y);

            AEMtx33Concat(&world, &rot, &scale);
            AEMtx33Concat(&world, &trans, &world);

            AEGfxSetTransform(world.m);

            // Fade out as they age
            float alpha = 1.0f - (particlePool[i].lifeTime / particlePool[i].maxLifeTime);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);
            AEGfxSetTransparency(alpha);

            if (bubbleTexture) {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxTextureSet(bubbleTexture, 0, 0);
            }

            AEGfxMeshDraw(particleMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Draw credits text
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    int linesDrawn = 0;
    for (int i = 0; i < numLines; i++) {
        float yLine = yPos - i * lineSpacing;

        // Only draw if on screen
        if (yLine > -450.0f && yLine < 450.0f) {
            const char* currentLine = credits[i];

            // Skip truly empty strings (but keep the line position)
            if (strlen(currentLine) == 0) {
                linesDrawn++; // Still count it for spacing
                continue;
            }

            // Determine text color and size based on content
            float textSize = 0.6f;
            float r = 1.0f, g = 1.0f, b = 1.0f;
            float xPos = 0.0f; // Default X position (center)

            if (strcmp(currentLine, "WWW.DIGIPEN.EDU") == 0) {
                textSize = 0.6f;
                xPos = -0.22f; // Adjust this value
            } else if (strcmp(currentLine, "All content copyright 2025 DigiPen Institute of "
                                           "Technology Singapore. All Rights Reserved") == 0) {
                r = 0.7f;
                g = 0.7f;
                b = 0.7f; // Gray
                textSize = 0.4f;
                xPos = -0.80f; // Long line needs more negative
            } else if (strcmp(currentLine, "TEAM") == 0) {
                r = 1.0f;
                g = 0.0f;
                b = 0.0f; // Red
                textSize = 0.9f;
                xPos = -0.10f; // Short line, adjust this
            } else if (strcmp(currentLine, "Debugachu") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f; // Gold
                textSize = 0.8f;
                xPos = -0.175f; // Adjust this
            } else if (strcmp(currentLine, "DEVELOPED BY") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f; // Gold
                textSize = 0.8f;
                xPos = -0.224f; // Adjust this
            } else if (strcmp(currentLine, "Woo Guang Theng") == 0) {
                textSize = 0.6f;
                xPos = -0.212f; // Adjust this
            } else if (strcmp(currentLine, "Sean Lee Hong Wei") == 0) {
                textSize = 0.6f;
                xPos = -0.235f; // Adjust this
            } else if (strcmp(currentLine, "Chia Hanxin") == 0) {
                textSize = 0.6f;
                xPos = -0.16f; // Adjust this
            } else if (strcmp(currentLine, "Han Tianchou") == 0) {
                textSize = 0.6f;
                xPos = -0.17f; // Adjust this
            } else if (strcmp(currentLine, "FACULTY AND ADVISORS") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f; // Gold
                textSize = 0.8f;
                xPos = -0.35f; // Adjust this
            } else if (strcmp(currentLine, "Gerald Wong") == 0) {
                textSize = 0.6f;
                xPos = -0.16f; // Adjust this
            } else if (strcmp(currentLine, "Soroor Malekmohammadi Faradounbeh") == 0) {
                textSize = 0.6f;
                xPos = -0.435f; // Very long line, needs more negative
            } else if (strcmp(currentLine, "Tommy Tan") == 0) {
                textSize = 0.6f;
                xPos = -0.13f; // Adjust this
            } else if (strcmp(currentLine, "CREATED AT") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f; // Gold
                textSize = 0.8f;
                xPos = -0.183f; // Adjust this
            } else if (strcmp(currentLine, "DigiPen Institute of Technology Singapore") == 0) {
                textSize = 0.6f;
                xPos = -0.52f; // Long line
            } else if (strcmp(currentLine, "PRESIDENT") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f; // Gold
                textSize = 0.8f;
                xPos = -0.16f; // Adjust this
            } else if (strcmp(currentLine, "Claude Comair") == 0) {
                textSize = 0.6f;
                xPos = -0.18f; // Adjust this
            } else if (strcmp(currentLine, "EXECUTIVES") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f; // Gold
                textSize = 0.8f;
                xPos = -0.175f; // Adjust this
            } else if (strcmp(currentLine, "Chu Jason Yeu Tat       Michael Gats") == 0) {
                textSize = 0.6f;
                xPos = -0.48f; // Long line with spaces
            } else if (strcmp(currentLine, "Tan Chek Ming       Prasanna Kumar Ghali") == 0) {
                textSize = 0.6f;
                xPos = -0.52f; // Very long line
            } else if (strcmp(currentLine, "Mandy Wong       Johnny Deek") == 0) {
                textSize = 0.6f;
                xPos = -0.35f; // Long line
            } else if (strcmp(currentLine, "SPECIAL THANKS") == 0) {
                r = 1.0f;
                g = 0.84f;
                b = 0.0f; // Gold
                textSize = 0.8f;
                xPos = -0.23f; // Adjust this
            } else if (strcmp(currentLine, "All playtesters") == 0) {
                textSize = 0.6f;
                xPos = -0.17f; // Adjust this
            }

            // Convert world Y to screen Y
            float screenY = yLine / 450.0f;

            // Debug print for first few lines
            static bool debugPrinted = false;
            if (!debugPrinted && i < 10) {
                printf("Line %d: '%s' len=%zu textSize=%.2f xPos=%.3f\n", i, currentLine,
                       strlen(currentLine), textSize, xPos);
                if (i == 9)
                    debugPrinted = true;
            }

            // Draw the text with manual X position
            AEGfxPrint(font, currentLine, xPos, screenY, textSize, r, g, b, 1.0f);
            linesDrawn++;
        }
    }

    // Draw back button
    backButton.draw(font);
}

void FreeCredits() {
    // Free textures
    if (bubbleTexture) {
        AEGfxTextureUnload(bubbleTexture);
        bubbleTexture = nullptr;
    }

    // Free meshes
    if (particleMesh) {
        AEGfxMeshFree(particleMesh);
        particleMesh = nullptr;
    }

    // Free button
    backButton.unload();
}

void UnloadCredits() { AEGfxDestroyFont(font); }