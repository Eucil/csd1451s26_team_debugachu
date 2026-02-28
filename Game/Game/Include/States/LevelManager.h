#pragma once

#include "UISystem.h"
#include <AEEngine.h>
#include <filesystem>
#include <vector>

enum class Levels {
    Level_0, // No level ongoing
    Level_1,
    Level_2,
    Level_3,
};

enum class GameBlock { Dirt, Stone, StartPoint, EndPoint, None };

class LevelManager {
public:
    void init();

    bool getLevelEditorMode() const;
    void setLevelEditorMode(bool mode);
    Levels getCurrentLevel() const;
    void SetCurrentLevel(Levels level);
    GameBlock getCurrentGameBlock() const;
    void setCurrentGameBlock(GameBlock block);
    bool getDisplayBuilderContainer() const;

    void updateEditorButtonPosition();
    void updateContainerPosition();
    void updateInnerButtonPosition();

    void updateLevelEditor();
    void renderLevelEditorUI();
    void freeLevelEditor();

    bool makeFilePath();
    bool makeLevelFile();
    bool getLevelData();

private:
    bool level_editor_mode_;
    Levels current_level_;
    GameBlock current_gameblock_;

    AEVec2 container_scale_ = {300.0f, 300.0f};
    bool display_builder_container_ = false;
    // Builder button is the parent container for builder container
    // Builder container is the parent container for all the buttons in the builder pool
    Button builder_button;
    Button builder_container;
    std::vector<Button> buttonPool;
};

extern LevelManager levelManager;