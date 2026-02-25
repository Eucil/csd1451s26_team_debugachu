#pragma once

#include "UISystem.h"
#include <AEEngine.h>
#include <vector>

enum class Levels {
    Level_0, // No level ongoing
    Level_1,
    Level_2,
    Level_3,
};

enum class GameBlock { Dirt, Stone, StartPoint, EndPoint, Portal, None };

class LevelManager {
public:
    void init();

    bool getLevelEditorMode() const;
    void setLevelEditorMode(bool mode);
    Levels getCurrentLevel() const;
    void SetCurrentLevel(Levels level);
    bool getBuildMode() const;
    void setBuildMode(bool mode);
    GameBlock getCurrentGameBlock() const;
    void setCurrentGameBlock(GameBlock block);

    void updateEditorButtonPosition();
    void updateContainerPosition();
    void updateInnerButtonPosition();

    void updateLevelEditor();
    void renderLevelEditorUI();
    void freeLevelEditor();

private:
    bool level_editor_mode_;
    bool build_mode_;
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