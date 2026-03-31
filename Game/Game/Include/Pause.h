/*!
@file       Pause.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/

#pragma once

#include <string>

#include <AEEngine.h>
#include <json/json.h>

#include "Components.h"

class PauseSystem {
public:
    void pause();
    void resume();

    bool isPaused() const;

    void renderBackground();

    void setTransformFillScreen();

    void updateTransform();

    void loadMesh();

    void initFromJson(const std::string& file, const std::string& section);

    void unload();

private:
    bool pause_{false}; // true means paused, false mean not paused

    Transform transform_;
    Graphics graphics_;
};
