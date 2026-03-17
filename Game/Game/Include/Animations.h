#include "AEEngine.h"

class UIFader {
private:
    f32 currentAlpha_ = 0.0f;
    f32 targetAlpha_ = 0.0f;
    f32 fadeSpeed_ = 5.0f;

public:
    UIFader(f32 speed = 5.0f) : fadeSpeed_(speed) {}

    void Update(f32 dt) {
        // Smoothly transition the current alpha toward the target alpha
        if (currentAlpha_ < targetAlpha_) {
            currentAlpha_ += fadeSpeed_ * dt;
            if (currentAlpha_ > targetAlpha_)
                currentAlpha_ = targetAlpha_;
        } else if (currentAlpha_ > targetAlpha_) {
            currentAlpha_ -= fadeSpeed_ * dt;
            if (currentAlpha_ < targetAlpha_)
                currentAlpha_ = targetAlpha_;
        }
    }

    void FadeIn() { targetAlpha_ = 1.0f; }
    void FadeOut() { targetAlpha_ = 0.0f; }

    f32 GetAlpha() const { return currentAlpha_; }
    bool IsVisible() const { return currentAlpha_ > 0.0f; }
};