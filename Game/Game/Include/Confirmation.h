/*!
@file       Confirmation.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file declares the ConfirmationTask enum and
            the ConfirmationSystem class, which manages confirmation
            dialogs with Yes/No buttons used across multiple game states.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// ============================
// Standard library
// ============================
#include <string>

// ============================
// Third-party
// ============================
#include <AEEngine.h>
#include <json/json.h>

// ============================
// Project
// ============================
#include "Button.h"
#include "Components.h"

// ============================
// Confirmation Task Enum
// ============================
enum class ConfirmationTask { Restart, MainMenu, Quit, Delete, No };

class ConfirmationSystem {
public:
    // ==========================================
    // Visibility Management
    // ==========================================
    void show();
    void hide();
    bool isShowing() const;

    // ==========================================
    // Lifecycle
    // ==========================================
    void load();
    void init(s8& buttonFont);
    void update();
    void draw();
    void unload();

    // ==========================================
    // Rendering
    // ==========================================
    void renderBackground();
    void setTransformFillScreen();
    void updateTransform();

    // =========================================
    // Initialization from JSON
    // ==========================================
    void initFromJson(const std::string& file, const std::string& section);

    // ==========================================
    // Input Handling
    // Returns true if the respective button was clicked while the confirmation dialog is showing.
    // Returns false otherwise.
    // ==========================================
    bool confirmationYesClicked();
    bool confirmationNoClicked();

    // ==========================================
    // Task Management
    // ==========================================
    void setTask(ConfirmationTask task);
    ConfirmationTask getTask() const;

private:
    // Variables to manage visibility and iframe after showing
    bool show_{false};
    bool justShown_{false};

    // Used to render black background
    Transform transform_;
    Graphics graphics_;

    // The current task that the confirmation is confirming
    ConfirmationTask task_{ConfirmationTask::No};

    // Buttons and text for the confirmation dialog
    Button buttonYes_;
    Button buttonNo_;
    TextData confirmationText_;
};
