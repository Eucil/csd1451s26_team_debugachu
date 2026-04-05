/*!
@file       Settings.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the definitions of functions that makes
            the Settings game state,
            handling load, initialize, update, draw, free, and unload.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/Settings.h"

// Third-party
#include <AEEngine.h>

// Project
#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "DebugSystem.h"
#include "GameStateManager.h"
#include "LevelManager.h"
#include "MenuBackground.h"

static Button buttonIncreaseSfxVolume;
static Button buttonDecreaseSfxVolume;
static Button buttonIncreaseBgmVolume;
static Button buttonDecreaseBgmVolume;
static Button buttonBack;

static s8 titleFont;
static s8 buttonFont;

static TextData headerText;
static TextData sfxVolumeText;
static TextData bgmVolumeText;
static TextData sfxVolumeAmountText;
static TextData bgmVolumeAmountText;

// Animations
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

// =========================================================
//
// loadSettings
//
// Loads fonts, button meshes and textures, and the destructible
// terrain background. Also registers and initialises all
// animation managers for this state.
//
// =========================================================
void loadSettings() {

    animManager.clear();
    animManager.add(&screenFader);
    animManager.add(&someOtherCoolAnimation);
    animManager.initializeAll();

    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 48);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    buttonIncreaseSfxVolume.loadMesh();
    buttonIncreaseSfxVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseSfxVolume.loadMesh();
    buttonDecreaseSfxVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonIncreaseBgmVolume.loadMesh();
    buttonIncreaseBgmVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseBgmVolume.loadMesh();
    buttonDecreaseBgmVolume.loadTexture("Assets/Textures/brown_square_24_24.png");

    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_rectangle_40_24.png");

    // Load the destructible terrain background
    MenuBackground::load(100);
}

// =========================================================
//
// initializeSettings
//
// Initialises all buttons and text fields from JSON config,
// assigns fonts, and sets up the shared background simulation.
//
// =========================================================
void initializeSettings() {
    buttonIncreaseSfxVolume.initFromJson("settings_buttons", "IncreaseSfxVolume");
    buttonIncreaseSfxVolume.setTextFont(buttonFont);
    buttonDecreaseSfxVolume.initFromJson("settings_buttons", "DecreaseSfxVolume");
    buttonDecreaseSfxVolume.setTextFont(buttonFont);
    buttonIncreaseBgmVolume.initFromJson("settings_buttons", "IncreaseBgmVolume");
    buttonIncreaseBgmVolume.setTextFont(buttonFont);
    buttonDecreaseBgmVolume.initFromJson("settings_buttons", "DecreaseBgmVolume");
    buttonDecreaseBgmVolume.setTextFont(buttonFont);

    buttonBack.initFromJson("settings_buttons", "Back");
    buttonBack.setTextFont(buttonFont);

    headerText.initFromJson("settings_text", "Header");
    headerText.font_ = titleFont;

    sfxVolumeText.initFromJson("settings_text", "SfxVolume");
    sfxVolumeText.font_ = buttonFont;
    bgmVolumeText.initFromJson("settings_text", "BgmVolume");
    bgmVolumeText.font_ = buttonFont;
    sfxVolumeAmountText.initFromJson("settings_text", "SfxVolumeAmount");
    sfxVolumeAmountText.font_ = buttonFont;
    bgmVolumeAmountText.initFromJson("settings_text", "BgmVolumeAmount");
    bgmVolumeAmountText.font_ = buttonFont;

    // Initialize the shared background simulation
    MenuBackground::initialize();
}

// =========================================================
//
// updateSettings
//
// Handles button input to adjust SFX and BGM volumes, navigates
// back to the main menu, updates volume display text, processes
// dirt destruction on click, and ticks the background simulation.
// Delegates to the debug system when the debug menu is open.
//
// =========================================================
void updateSettings(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        // Check for button presses to change audio group
        if (buttonIncreaseSfxVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("sfx", +10);
        }
        if (buttonDecreaseSfxVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("sfx", -10);
        }
        if (buttonIncreaseBgmVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("bgm", +10);
        }
        if (buttonDecreaseBgmVolume.checkMouseClick()) {
            g_audioSystem.adjustGroupVolume("bgm", -10);
        }

        // Check for button press to go back to main menu
        if (buttonBack.checkMouseClick()) {
            screenFader.startFadeOut(&GSM, StateId::MainMenu);
        }

        // Get volume amounts
        sfxVolumeAmountText.content_ = std::to_string(g_audioSystem.getGroupVolume("sfx")) + "%";
        bgmVolumeAmountText.content_ = std::to_string(g_audioSystem.getGroupVolume("bgm")) + "%";

        buttonIncreaseSfxVolume.updateTransform();
        buttonDecreaseSfxVolume.updateTransform();
        buttonIncreaseBgmVolume.updateTransform();
        buttonDecreaseBgmVolume.updateTransform();

        buttonBack.updateTransform();

        if (AEInputCheckCurr(AEVK_LBUTTON)) {
            bool hitDirt = MenuBackground::destroyDirtAtMouse(20.0f);
            if (hitDirt) {
                g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
            }
        }

        // Update the shared background (fluid, portals, etc. keep running)
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
// drawSettings
//
// Draws the live background, all volume control buttons,
// the back button, header and volume label text, then the
// animation and debug overlays.
//
// =========================================================
void drawSettings() {

    // Draw the shared live background first
    MenuBackground::draw();

    buttonIncreaseSfxVolume.draw();
    buttonDecreaseSfxVolume.draw();
    buttonIncreaseBgmVolume.draw();
    buttonDecreaseBgmVolume.draw();

    buttonBack.draw();

    headerText.draw(true);
    sfxVolumeText.draw();
    bgmVolumeText.draw();
    sfxVolumeAmountText.draw();
    bgmVolumeAmountText.draw();

    animManager.drawAll();

    g_debugSystem.drawAll();
}

// =========================================================
//
// freeSettings
//
// Clears the debug scene, frees the background simulation,
// and releases all animation resources.
//
// =========================================================
void freeSettings() {
    g_debugSystem.clearScene();
    MenuBackground::free();

    animManager.freeAll();
}

// =========================================================
//
// unloadSettings
//
// Destroys fonts, unloads all button meshes and textures,
// and unloads the destructible background.
//
// =========================================================
void unloadSettings() {
    // Unload fonts
    if (titleFont) {
        AEGfxDestroyFont(titleFont);
        titleFont = 0;
    }
    if (buttonFont) {
        AEGfxDestroyFont(buttonFont);
        buttonFont = 0;
    }

    buttonIncreaseSfxVolume.unload();
    buttonDecreaseSfxVolume.unload();
    buttonIncreaseBgmVolume.unload();
    buttonDecreaseBgmVolume.unload();

    buttonBack.unload();

    // Unload destructible background
    MenuBackground::unload();
}