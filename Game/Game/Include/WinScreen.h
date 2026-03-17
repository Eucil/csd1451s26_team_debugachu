#pragma once
#include "AEEngine.h"
#include "Button.h"
#include "GameStateManager.h"

class WinScreen {
private:
    bool isVisible_{false};

    // UI Elements
    Button nextLevelButton_;
    Button restartButton_;
    Button mainMenuButton_;

    TextData titleText_;
    TextData collectiblesText_;
    TextData statsText_;

    // For background panel
    Transform transform_;
    Graphics graphics_;

    s8 font_;

    // Win screen data
    int collectiblesCollected_{0};
    int totalCollectibles_{0};
    int currentLevel_{0};
    int nextLevel_{0};

public:
    void Load(s8 font);
    void Show(int collected, int total, int currentLevel);
    void Hide();
    bool IsVisible() const { return isVisible_; }

    void Update(GameStateManager& GSM);
    void Draw();
    void Free();
    void Unload();

    void SetCollectibles(int collected, int total) {
        collectiblesCollected_ = collected;
        totalCollectibles_ = total;
    }
};
