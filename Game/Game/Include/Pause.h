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
