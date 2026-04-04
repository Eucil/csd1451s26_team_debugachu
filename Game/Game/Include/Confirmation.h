/*!
@file       Confirmation.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

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
#include <json/json.h>

// Project
#include "Button.h"
#include "Components.h"

enum class ConfirmationTask { Restart, MainMenu, Quit, Delete, No };

class ConfirmationSystem {
public:
    void show();
    void hide();

    bool isShowing() const;

    void load();

    void init(s8& buttonFont);

    void update();

    void draw();

    void renderBackground();

    void setTransformFillScreen();

    void updateTransform();

    void initFromJson(const std::string& file, const std::string& section);

    void unload();

    bool confirmationYesClicked();
    bool confirmationNoClicked();

    void setTask(ConfirmationTask task);
    ConfirmationTask getTask() const;

private:
    bool show_{false}; // true means display confirmation screen
    bool justShown_{false};

    // Used to render black background
    Transform transform_;
    Graphics graphics_;

    ConfirmationTask task_{ConfirmationTask::No}; // The task that the confirmation is for. Default
                                                  // to No (i.e. not showing)
    Button buttonYes_;
    Button buttonNo_;
    TextData confirmationText_;
};
