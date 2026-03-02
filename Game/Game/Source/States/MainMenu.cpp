#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"
#include "UISystem.h"

static Button level1Button;
static Button level2Button;
// TC added start
static Button startButton;
static Button howToPlayButton;
static Button settingsButton;
static Button creditsButton;
static Button quitButton;

static Text startText;
static Text howToPlayText;
static Text settingsText;
static Text creditsText;
static Text quitText;

static s8 titleFont;
static s8 buttonFont;
// TC added end
static Text level1Text;
static Text level2Text;
static s8 font;

// void LoadMainMenu() {
//     // Todo
//     // std::cout << "Load main menu\n";
//
//     // Setup buttons
//     level1Button = Button(AEVec2{0.0f, 200.0f}, AEVec2{400.0f, 200.0f});
//     level2Button = Button(AEVec2{0.0f, -100.0f}, AEVec2{400.0f, 200.0f});
//
//     level1Button.SetupMesh();
//     level2Button.SetupMesh();
//
//     // Setup texts
//     level1Text = Text(-0.2f, 0.4f, "Level 1");
//     level2Text = Text(-0.2f, -0.3f, "Level 2");
//
//     font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
// }
void LoadMainMenu() {
    // Setup buttons - position them vertically
    // Start button
    startButton = Button(AEVec2{0.0f, 300.0f}, AEVec2{400.0f, 80.0f});

    // How To Play button
    howToPlayButton = Button(AEVec2{0.0f, 180.0f}, AEVec2{400.0f, 80.0f});

    // Settings button
    settingsButton = Button(AEVec2{0.0f, 60.0f}, AEVec2{400.0f, 80.0f});

    // Credits button
    creditsButton = Button(AEVec2{0.0f, -60.0f}, AEVec2{400.0f, 80.0f});

    // Quit button
    quitButton = Button(AEVec2{0.0f, -180.0f}, AEVec2{400.0f, 80.0f});

    // level1Button = Button(AEVec2{0.0f, 200.0f}, AEVec2{400.0f, 200.0f});
    // level2Button = Button(AEVec2{0.0f, -100.0f}, AEVec2{400.0f, 200.0f});

    // Setup all button meshes
    startButton.SetupMesh();
    howToPlayButton.SetupMesh();
    settingsButton.SetupMesh();
    creditsButton.SetupMesh();
    quitButton.SetupMesh();

    // Setup texts for each button
    startText = Text(-0.15f, 0.64f, "START");           // For button at y=300
    howToPlayText = Text(-0.23f, 0.38f, "HOW TO PLAY"); // For button at y=180
    settingsText = Text(-0.15f, 0.13f, "SETTINGS");     // For button at y=60
    creditsText = Text(-0.15f, -0.13f, "CREDITS");      // For button at y=-60
    quitText = Text(-0.12f, -0.38f, "QUIT");            // For button at y=-180

    // Load fonts
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 72);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 36);

    // Keep the original font for compatibility
    // font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
}

void InitializeMainMenu() {
    // Todo
    // std::cout << "Initialize main menu\n";
}

// void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime) {
//     // Todo
//     // std::cout << "Update main menu\n";
//
//     // Press 1 to go to level 1
//     if (AEInputCheckTriggered(AEVK_1) || 0 == AESysDoesWindowExist()) {
//         std::cout << "1 triggered\n";
//         GSM.nextState_ = StateId::Level1;
//     }
//
//     // Press 2 to go to level 2
//     if (AEInputCheckTriggered(AEVK_2) || 0 == AESysDoesWindowExist()) {
//         std::cout << "2 triggered\n";
//         GSM.nextState_ = StateId::Level2;
//     }
//
//     // Press R to restart
//     if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
//         std::cout << "R triggered\n";
//         GSM.nextState_ = StateId::Restart;
//     }
//
//     if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
//         if (level1Button.OnClick()) {
//             std::cout << "Start button clicked\n";
//             GSM.nextState_ = StateId::Level1;
//         }
//         if (level2Button.OnClick()) {
//             std::cout << "Exit button clicked\n";
//             GSM.nextState_ = StateId::Level2;
//         }
//     }
// }
void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime) {
    // Keep keyboard shortcuts for development/testing
    if (AEInputCheckTriggered(AEVK_1) || 0 == AESysDoesWindowExist()) {
        std::cout << "1 triggered - Going to Level 1\n";
        GSM.nextState_ = StateId::Level1;
    }

    if (AEInputCheckTriggered(AEVK_2) || 0 == AESysDoesWindowExist()) {
        std::cout << "2 triggered - Going to Level 2\n";
        GSM.nextState_ = StateId::Level2;
    }

    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered - Restart\n";
        GSM.nextState_ = StateId::Restart;
    }

    // Mouse click handling for all buttons
    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {

        // Start button - goes to Level 1 (or you could make it go to a level select)
        if (startButton.OnClick()) {
            std::cout << "Start button clicked - Going to Level 1\n";
            GSM.nextState_ = StateId::Level1;
        }

        // How To Play button
        if (howToPlayButton.OnClick()) {
            std::cout << "How To Play button clicked\n";
            // TODO: Implement how to play screen or state
            // For now, just print or you could set a new state
            // GSM.nextState_ = StateId::HowToPlay;
        }

        // Settings button
        if (settingsButton.OnClick()) {
            std::cout << "Settings button clicked\n";
            // TODO: Implement settings screen or state
            // GSM.nextState_ = StateId::Settings;
        }

        // Credits button
        if (creditsButton.OnClick()) {
            std::cout << "Credits button clicked\n";
            // TODO: Implement credits screen or state
            // GSM.nextState_ = StateId::Credits;
        }

        // Quit button
        if (quitButton.OnClick()) {
            std::cout << "Quit button clicked - Exiting game\n";
            GSM.nextState_ = StateId::Quit;
        }
    }
}

// void DrawMainMenu() {
//     // Todo
//     // std::cout << "Draw main menu\n";
//     AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
//
//     level1Button.DrawColor(0.0f, 1.0f, 0.0f); // Green
//     level2Button.DrawColor(1.0f, 0.0f, 0.0f); // Red
//
//     const char* level1Str = level1Text.text_.c_str();
//     AEGfxPrint(font, level1Str, level1Text.pos_x_, level1Text.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
//     const char* level2Str = level2Text.text_.c_str();
//     AEGfxPrint(font, level2Str, level2Text.pos_x_, level2Text.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
// }
void DrawMainMenu() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Draw all buttons with different colors
    startButton.DrawColor(0.0f, 0.8f, 0.0f);     // Green for Start
    howToPlayButton.DrawColor(0.0f, 0.5f, 1.0f); // Light blue for How To Play
    settingsButton.DrawColor(0.8f, 0.8f, 0.0f);  // Yellow for Settings
    creditsButton.DrawColor(0.8f, 0.0f, 0.8f);   // Purple for Credits
    quitButton.DrawColor(0.8f, 0.0f, 0.0f);      // Red for Quit

    // Optional: Draw a game title
    const char* titleStr = "WATER THE PLANT";
    AEGfxPrint(titleFont, titleStr, -0.8f, 0.8f, 1.f, 1.f, 1.f, 1.f, 1.f);

    // Draw all button texts
    const char* startStr = startText.text_.c_str();
    AEGfxPrint(buttonFont, startStr, startText.pos_x_, startText.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);

    const char* howToPlayStr = howToPlayText.text_.c_str();
    AEGfxPrint(buttonFont, howToPlayStr, howToPlayText.pos_x_, howToPlayText.pos_y_, 1.f, 1.f, 1.f,
               1.f, 1.f);

    const char* settingsStr = settingsText.text_.c_str();
    AEGfxPrint(buttonFont, settingsStr, settingsText.pos_x_, settingsText.pos_y_, 1.f, 1.f, 1.f,
               1.f, 1.f);

    const char* creditsStr = creditsText.text_.c_str();
    AEGfxPrint(buttonFont, creditsStr, creditsText.pos_x_, creditsText.pos_y_, 1.f, 1.f, 1.f, 1.f,
               1.f);

    const char* quitStr = quitText.text_.c_str();
    AEGfxPrint(buttonFont, quitStr, quitText.pos_x_, quitText.pos_y_, 1.f, 1.f, 1.f, 1.f, 1.f);
}
// void FreeMainMenu() {
//     // Todo
//     // std::cout << "Free main menu\n";
// }
void FreeMainMenu() {
    // Free button meshes
    /*startButton.UnloadMesh();
    howToPlayButton.UnloadMesh();
    settingsButton.UnloadMesh();
    creditsButton.UnloadMesh();
    quitButton.UnloadMesh();*/
}

// void UnloadMainMenu() {
//     // Todo
//     // std::cout << "Unload main menu\n";
//     level1Button.UnloadMesh();
//     level2Button.UnloadMesh();
//     AEGfxDestroyFont(font);
// }
void UnloadMainMenu() {
    // Free all meshes
    startButton.UnloadMesh();
    howToPlayButton.UnloadMesh();
    settingsButton.UnloadMesh();
    creditsButton.UnloadMesh();
    quitButton.UnloadMesh();

    // Free fonts
    AEGfxDestroyFont(titleFont);
    AEGfxDestroyFont(buttonFont);
    // AEGfxDestroyFont(font); //
}