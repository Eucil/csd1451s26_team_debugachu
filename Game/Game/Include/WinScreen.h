/*!
@file       WinScreen.h
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu

@date       March, 31, 2026

@brief      This header file contains the declaration of the WinScreen class,
            which manages the victory overlay shown when the player completes
            a level. It displays the result, collectibles gathered, and
            navigation buttons for next level, restart, and main menu.

            The Next Level button is automatically greyed out and disabled
            when no further playable level exists beyond the current one.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "Button.h"
#include "GameStateManager.h"

// ==========================================
// WIN SCREEN
// ==========================================
class WinScreen {
private:
    bool isVisible_{false};

    // Navigation buttons
    Button nextLevelButton_; // Greyed out when no next level exists
    Button restartButton_;
    Button mainMenuButton_;

    // Text elements
    TextData titleText_;
    TextData collectiblesText_;
    TextData statsPerfectText_;
    TextData statsPartialText_;
    TextData noNextLevelText_;

    // Format string for collectiblesText_ (loaded from JSON, filled at runtime)
    std::string collectiblesFormat_;

    // Full-screen semi-transparent background panel
    Transform transform_;
    Graphics graphics_;

    s8 font_{0}; // Owned by Level.cpp; do not destroy here

    // Result data set by show() and used by update() and draw()
    int collectiblesCollected_{0};
    int totalCollectibles_{0};
    int currentLevel_{0};
    int nextLevel_{0};
    bool hasNextLevel_{false};

public:
    // Call once in Level::load() to create GPU resources and read JSON config.
    void load(s8 font);

    // Call when the win condition triggers.
    // currentLevel is the 1-based level number from levelManager.getCurrentLevel().
    void show(int collected, int total, int currentLevel);

    // Call after a button click to hide the overlay.
    void hide();

    // Returns true while the win overlay is on screen.
    bool isVisible() const { return isVisible_; }

    // Call every frame in Level::update() to handle button clicks.
    void update(GameStateManager& GSM);

    // Call every frame in Level::draw() to render the overlay.
    void draw();

    // Call in Level::free() to reset state without freeing GPU resources.
    void free();

    // Call in Level::unload() to release GPU resources.
    // Does not destroy the font since it is owned by Level.cpp.
    void unload();

    // Convenience setter to update counts without calling show().
    void setCollectibles(int collected, int total) {
        collectiblesCollected_ = collected;
        totalCollectibles_ = total;
    }
};