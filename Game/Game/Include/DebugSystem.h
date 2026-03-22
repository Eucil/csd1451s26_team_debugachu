#pragma once

#include <string>

#include <AEEngine.h>
#include <json/json.h>

#include "Button.h"
#include "Components.h"
#include "GameStateManager.h"

struct DebugOptions {
    bool renderColliders{false};
    bool showFps{false};
    bool unlimitedWater{false};
    bool showCollisionCount{false};
    bool showVfxParticleCount{false};
    bool showWaterParticleCount{false};
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

    DebugOptions options_;

    void update();
    void draw();

private:
    void updateTransform();
    void renderBackground();

    bool open_{false};

    s8 font_{0};

    Transform transform_;
    Graphics graphics_;

    Button buttonClose_;
};

extern DebugSystem g_debugSystem;
