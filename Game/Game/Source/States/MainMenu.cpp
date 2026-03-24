#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "AudioSystem.h"
#include "MenuBackground.h" // shared background
#include "Utils.h"

// UI includes
#include "Button.h"
#include "GameStateManager.h"
#include "States/Transition.h"

// ----------------------------------------------------------------------------
// UI-only state  (background lives in MenuBackground)
// ----------------------------------------------------------------------------
static Button startButton;
static Button howToPlayButton;
static Button settingsButton;
static Button creditsButton;
static Button quitButton;
static TextData titleText;

static s8 titleFont = 0;
static s8 buttonFont = 0;

void LoadMainMenu() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Load the shared animated background (terrain, fluid, portals, etc.)
    MenuBackground::Load();

    // Load fonts for UI
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 72);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 33);

    // Load button assets
    startButton.loadMesh();
    startButton.loadTexture("Assets/Textures/brown_button.png");
    howToPlayButton.loadMesh();
    howToPlayButton.loadTexture("Assets/Textures/brown_button.png");
    settingsButton.loadMesh();
    settingsButton.loadTexture("Assets/Textures/brown_button.png");
    creditsButton.loadMesh();
    creditsButton.loadTexture("Assets/Textures/brown_button.png");
    quitButton.loadMesh();
    quitButton.loadTexture("Assets/Textures/brown_button.png");
}

void InitializeMainMenu() {
    // Initialize the shared background simulation
    MenuBackground::Initialize();

    // Initialize UI buttons from JSON
    startButton.initFromJson("main_menu_buttons", "Start");
    startButton.setTextFont(buttonFont);
    howToPlayButton.initFromJson("main_menu_buttons", "HowToPlay");
    howToPlayButton.setTextFont(buttonFont);
    settingsButton.initFromJson("main_menu_buttons", "Settings");
    settingsButton.setTextFont(buttonFont);
    creditsButton.initFromJson("main_menu_buttons", "Credits");
    creditsButton.setTextFont(buttonFont);
    quitButton.initFromJson("main_menu_buttons", "Quit");
    quitButton.setTextFont(buttonFont);

    titleText.initFromJson("main_menu_texts", "Title");
    titleText.font_ = titleFont;
}

void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime) {
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered - Restart\n";
        GSM.nextState_ = StateId::Restart;
    }

    // Button click handling
    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        if (startButton.checkMouseClick()) {
            std::cout << "Start button clicked - Going to Level Selector\n";
            GSM.nextState_ = StateId::LevelSelector;
        }
        if (howToPlayButton.checkMouseClick()) {
            std::cout << "How To Play button clicked\n";
            GSM.nextState_ = StateId::HowToPlay;
        }
        if (settingsButton.checkMouseClick()) {
            std::cout << "Settings button clicked\n";
            GSM.nextState_ = StateId::Settings;
        }
        if (creditsButton.checkMouseClick()) {
            std::cout << "Credits button clicked\n";
            GSM.nextState_ = StateId::Credits;
        }
        if (quitButton.checkMouseClick()) {
            std::cout << "Quit button clicked - Exiting game\n";
            GSM.nextState_ = StateId::Quit;
        }
    }

    // ------------------------------------------------------------
    // Left-click held: destroy dirt + VFX + audio
    // DestroyDirtAtMouse handles both the terrain destruction and
    // spawning the visual VFX burst. We only need to play the sound.
    // ------------------------------------------------------------
    if (AEInputCheckCurr(AEVK_LBUTTON)) {
        bool hitDirt = MenuBackground::DestroyDirtAtMouse(20.0f);
        if (hitDirt) {
            g_audioSystem.playSound("dirt_break", "sfx", 0.25f, 1.0f);
        }
    }

    // Update button transforms
    startButton.updateTransform();
    howToPlayButton.updateTransform();
    settingsButton.updateTransform();
    creditsButton.updateTransform();
    quitButton.updateTransform();

    // Update shared background (fluid, portals, collectibles, VFX)
    MenuBackground::Update(deltaTime);
}

void DrawMainMenu() {
    // Draw shared background (terrain, fluid, portals, collectibles, VFX)
    MenuBackground::Draw();

    // Draw UI on top
    startButton.draw();
    howToPlayButton.draw();
    settingsButton.draw();
    creditsButton.draw();
    quitButton.draw();

    titleText.draw();
}

void FreeMainMenu() { MenuBackground::Free(); }

void UnloadMainMenu() {
    // Unload shared GPU assets
    MenuBackground::Unload();

    // Unload button assets
    startButton.unload();
    howToPlayButton.unload();
    settingsButton.unload();
    creditsButton.unload();
    quitButton.unload();

    if (titleFont) {
        AEGfxDestroyFont(titleFont);
        titleFont = 0;
    }
    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }
}