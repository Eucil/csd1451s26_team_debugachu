/*!
@file       MainMenu.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Chia Hanxin/c.hanxin@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/MainMenu.h"

// Standard library
#include <iostream>

// Third-party
#include <AEEngine.h>

// Project
#include "Animations.h"
#include "AudioSystem.h"
#include "Button.h"
#include "Confirmation.h"
#include "DebugSystem.h"
#include "GameStateManager.h"
#include "MenuBackground.h"


// ----------------------------------------------------------------------------
// UI-only state  (background lives in MenuBackground)
// ----------------------------------------------------------------------------
static Button startButton;
static Button howToPlayButton;
static Button settingsButton;
static Button creditsButton;
static Button quitButton;

// Text/Font
static TextData titleText;

static s8 titleFont = 0;
static s8 buttonFont = 0;

// Animations
static AnimationManager animManager;
static ScreenFaderManager screenFader;
static UIFader someOtherCoolAnimation;

static ConfirmationSystem confirmationSystem;

void loadMainMenu() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Load the shared animated background (terrain, fluid, portals, etc.)
    MenuBackground::load();

    // Load fonts for UI
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 72);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    // Load button assets
    startButton.loadMesh();
    startButton.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    howToPlayButton.loadMesh();
    howToPlayButton.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    settingsButton.loadMesh();
    settingsButton.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    creditsButton.loadMesh();
    creditsButton.loadTexture("Assets/Textures/brown_rectangle_80_24.png");
    quitButton.loadMesh();
    quitButton.loadTexture("Assets/Textures/brown_rectangle_80_24.png");

    confirmationSystem.load();
    confirmationSystem.hide();
}

void initializeMainMenu() {
    // Initialize the shared background simulation
    MenuBackground::initialize();

    // Initialize UI buttons from JSON
    startButton.initFromJson("main_menu_buttons", "Start");
    startButton.setTextFont(buttonFont);
    howToPlayButton.initFromJson("main_menu_buttons", "Controls");
    howToPlayButton.setTextFont(buttonFont);
    settingsButton.initFromJson("main_menu_buttons", "Settings");
    settingsButton.setTextFont(buttonFont);
    creditsButton.initFromJson("main_menu_buttons", "Credits");
    creditsButton.setTextFont(buttonFont);
    quitButton.initFromJson("main_menu_buttons", "Quit");
    quitButton.setTextFont(buttonFont);

    // Text/Fonts
    titleText.initFromJson("main_menu_texts", "Title");
    titleText.font_ = titleFont;

    // Animations
    animManager.clear();
    animManager.add(&screenFader);
    animManager.add(&someOtherCoolAnimation);
    animManager.initializeAll();

    // Confirmation System
    confirmationSystem.init(buttonFont);
}

void updateMainMenu(GameStateManager& GSM, f32 deltaTime) {
    if (!g_debugSystem.isOpen()) {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.open();
        }

        //(void)deltaTime; // Unused parameter, but required by function signature
        // Keep keyboard shortcuts for development/testing
        if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
            std::cout << "R triggered - Restart\n";
            GSM.nextState_ = StateId::Restart;
        }

        // Button click handling
        if (!confirmationSystem.isShowing()) {

            if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
                if (startButton.checkMouseClick()) {
                    std::cout << "Start button clicked - Going to Level Selector\n";

                    screenFader.startFadeOut(&GSM, StateId::LevelSelector);
                }
                if (howToPlayButton.checkMouseClick()) {
                    screenFader.startFadeOut(&GSM, StateId::Controls);
                }

                // Settings button
                if (settingsButton.checkMouseClick()) {
                    std::cout << "Settings button clicked\n";
                    screenFader.startFadeOut(&GSM, StateId::Settings);
                }

                // Credits button
                if (creditsButton.checkMouseClick()) {
                    std::cout << "Credits button clicked\n";
                    screenFader.startFadeOut(&GSM, StateId::Credits);
                }
                if (quitButton.checkMouseClick()) {
                    confirmationSystem.show();
                    confirmationSystem.setTask(ConfirmationTask::Quit);
                }
            }
        }

        if (confirmationSystem.confirmationYesClicked()) {
            if (confirmationSystem.getTask() == ConfirmationTask::Quit) {
                screenFader.startFadeOut(&GSM, StateId::Quit);
            }
        }
        if (confirmationSystem.confirmationNoClicked()) {
            confirmationSystem.hide();
            confirmationSystem.setTask(ConfirmationTask::No);
        }

        // ------------------------------------------------------------
        // Left-click held: destroy dirt + VFX + audio
        // ------------------------------------------------------------
        if (AEInputCheckCurr(AEVK_LBUTTON)) {
            bool hitDirt = MenuBackground::destroyDirtAtMouse(20.0f);
            if (hitDirt) {
                g_audioSystem.playSound("dirt_break", "sfx", 0.5f, 1.0f);
            }
        }

        // Update button transforms
        startButton.updateTransform();
        howToPlayButton.updateTransform();
        settingsButton.updateTransform();
        creditsButton.updateTransform();
        quitButton.updateTransform();

        // Update shared background (fluid, portals, collectibles, VFX)
        MenuBackground::update(deltaTime);
    } else {
        if (AEInputCheckTriggered(AEVK_Z)) {
            g_debugSystem.close();
        }
        g_debugSystem.update();
    }

    animManager.updateAll(deltaTime);

    confirmationSystem.update();
}

void drawMainMenu() {
    // Draw shared background (terrain, fluid, portals, collectibles, VFX)
    MenuBackground::draw();

    // Draw UI on top
    if (confirmationSystem.isShowing()) {
        startButton.draw(false);
        howToPlayButton.draw(false);
        settingsButton.draw(false);
        creditsButton.draw(false);
        quitButton.draw(false);
    } else {
        startButton.draw();
        howToPlayButton.draw();
        settingsButton.draw();
        creditsButton.draw();
        quitButton.draw();
    }

    // Draw game title
    titleText.draw();

    confirmationSystem.draw();

    animManager.drawAll();
    g_debugSystem.drawAll();
}

void freeMainMenu() {

    animManager.freeAll();
    MenuBackground::free();
}

void unloadMainMenu() {
    // Unload shared GPU assets
    MenuBackground::unload();

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

    confirmationSystem.unload();
}