/*!
@file       TileBackground.h
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

// Standard library
#include <cmath>
#include <string>

// Third-party
#include <AEEngine.h>

// Project
#include "Components.h"

struct TiledBackground {
    void loadFromJson(const std::string& file, const std::string& section);
    void draw() const;
    void unload();

    Graphics graphics_;
};
