/*!
@file       DebugSystem.h
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
#include <unordered_map>
#include <vector>

#include <AEEngine.h>
#include <json/json.h>

#include "Button.h"
#include "Components.h"
#include "GameStateManager.h"

class Terrain;
class FluidSystem;

struct DebugToggle {
    std::string key_;
    std::string hudFormat_;
    Button checkbox_;
    TextData label_;
};

class DebugSystem {
public:
    void load(s8 font);
    void initFromJson(const std::string& file, const std::string& section);
    void unload();

    void open();
    void close();
    void toggle();

    bool isOpen() const;

    std::unordered_map<std::string, bool> options_;
    std::unordered_map<std::string, float> hudValues_;

    void update();
    void draw();
    void drawHUD();
    void drawColliders(Terrain& terrain);
    void drawFluidColliders(FluidSystem& fluidSystem);

private:
    void updateTransform();
    void renderBackground();

    bool open_{false};

    s8 font_{0};

    Transform transform_;
    Graphics graphics_;
    AEGfxVertexList* hudMesh_{nullptr};
    AEGfxVertexList* wireRectMesh_{nullptr};
    AEGfxVertexList* wireCircleMesh_{nullptr};
    AEGfxVertexList* wireLineMesh_{nullptr};
    TextData headerText_;

    Button buttonClose_;
    std::vector<DebugToggle> toggles_;
};

extern DebugSystem g_debugSystem;
