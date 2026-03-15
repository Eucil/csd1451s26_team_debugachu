#pragma once

#include <string>

#include <AEEngine.h>
#include <json/json.h>

#include "Components.h"

struct TextData {
    std::string content_;
    float x_{0.f}, y_{0.f};
    float scale_{1.f};
    float r_{1.f}, g_{1.f}, b_{1.f}, a_{1.f};
};

class NewButton {
public:
    void loadTexture(const char* path);
    void loadMesh();
    void initFromJson(const std::string& file, const std::string& section);
    void updateTransform();
    void draw(s8 font);
    void unload();
    bool checkMouseClick() const;

private:
    Transform transform_;
    Graphics graphics_;
    TextData text_;
};