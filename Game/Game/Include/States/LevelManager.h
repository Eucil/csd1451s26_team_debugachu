#pragma once

#include "StartEndPoint.h"
#include "Terrain.h"
#include "UISystem.h"
#include <AEEngine.h>
#include <filesystem>
#include <json/json.h>
#include <vector>

enum class GameBlock { Dirt, Stone, StartPoint, EndPoint, None };

class LevelManager {
public:
    void init();

    bool getLevelEditorMode() const;
    void setLevelEditorMode();
    int getCurrentLevel() const;
    void SetCurrentLevel(int level);
    GameBlock getCurrentGameBlock() const;
    void setCurrentGameBlock(GameBlock block);
    bool getDisplayBuilderContainer() const;

    void updateEditorButtonPosition();
    void updateContainerPosition();
    void updateInnerButtonPosition();

    void updateLevelEditor();
    void renderLevelEditorUI();
    void freeLevelEditor();

    bool makeFilePath(int level);
    bool makeLevelFile(int level);

    // Functions for saving level data to JSON
    void saveMapInfo(int width, int height, int tilesize);
    void saveTerrainInfo(std::vector<float> nodes, std::string terrainType);
    void saveStartEndInfo(std::vector<StartEnd> startPoints, StartEnd endPoint);
    void writeToFile(int level);
    // Functions for reading level data from JSON
    bool getLevelData(int level);
    void parseMapInfo(int& width, int& height, int& tilesize);
    void parseTerrainInfo(std::vector<float>& nodes, std::string terrainType);
    void parseStartEndInfo(StartEndPoint& startEndPointSystem);

    // Number of levels
    std::vector<int> level_vec_;

private:
    bool level_editor_mode_{false};
    int current_level_{0};
    GameBlock current_gameblock_{GameBlock::None};

    AEVec2 container_scale_ = {300.0f, 300.0f};
    bool display_builder_container_ = false;
    // Builder button is the parent container for builder container
    // Builder container is the parent container for all the buttons in the builder pool
    Button builder_button;
    Button builder_container;
    std::vector<Button> buttonPool;

    Json::Value savingRoot{};
    Json::Value readingRoot{};
};

extern LevelManager levelManager;