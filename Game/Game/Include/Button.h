#pragma once

#include <string>

#include <AEEngine.h>
#include <json/json.h>

#include "Components.h"

struct TextData {
    std::string content_{};
    float x_{0.f}, y_{0.f};
    float scale_{1.f};
    float lineSpacing_{1.f};
    float r_{1.f}, g_{1.f}, b_{1.f}, a_{1.f};
    s8 font_{0};

    void draw(bool center = false);
    void initFromJson(const std::string& file, const std::string& section);
    void setTransform(const f32& x, const f32& y, const f32& scale, const f32& r, const f32& g,
                      const f32& b, const f32& a);
};

class Button {
public:
    void loadTexture(const char* path);
    void loadMesh();
    void initFromJson(const std::string& file, const std::string& section);
    void setTransform(const AEVec2& pos, const AEVec2& scale, f32 rotationRad = 0.f);
    Transform getTransform() const;
    void setText(const std::string& content, const f32& x, const f32& y, const f32& scale,
                 const f32& r, const f32& g, const f32& b, const f32& a);
    void setTextFont(s8 font);
    void setRGBA(const f32& r, const f32& g, const f32& b, const f32& a);
    void updateTransform();
    void draw();
    void unload();
    bool checkMouseClick() const;
    bool isHovered() const;
    AEGfxVertexList* getMesh() const { return graphics_.mesh_; }
    AEGfxTexture* getTexture() const { return graphics_.texture_; }

private:
    Transform transform_;
    Graphics graphics_;
    TextData text_;
};