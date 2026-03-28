#pragma once
#include <string>

#include <AEEngine.h>

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
    TextData statsPerfectText_;
    TextData statsPartialText_;
    TextData noNextLevelText_;

    std::string collectiblesFormat_;

    // For background panel
    Transform transform_;
    Graphics graphics_;

    s8 font_;

    // Win screen data
    int collectiblesCollected_{0};
    int totalCollectibles_{0};
    int currentLevel_{0};
    int nextLevel_{0};
    bool hasNextLevel_{false}; // true when a playable next level exists

public:
    void Load(s8 font);

    // currentLevel should be the 1-based level number currently being played
    // (i.e. the same value stored in levelManager.getCurrentLevel())
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