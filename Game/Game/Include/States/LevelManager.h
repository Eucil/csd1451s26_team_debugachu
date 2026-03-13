#pragma once

#include "StartEndPoint.h"
#include "Terrain.h"
#include "UISystem.h"
#include <AEEngine.h>
#include <filesystem>
#include <json/json.h>
#include <vector>

enum class Level { Level1, Level2, Level3, Level4, Level5, Level6, Level7, Level8, None };
enum class editorMode { Edit, Delete, Create, None };
enum class GameBlock { Dirt, Stone, StartPoint, EndPoint, None };

class LevelManager {
public:
    void init();
    void initEditorUI();

    // Getters and setters
    //=================================================================
    editorMode getLevelEditorMode() const;
    void setLevelEditorMode(editorMode mode);
    int getCurrentLevel() const;
    void SetCurrentLevel(int level);
    GameBlock getCurrentGameBlock() const;
    void setCurrentGameBlock(GameBlock block);
    bool getDisplayBuilderContainer() const;
    //=================================================================

    // Update & Render Functions
    //=================================================================
    void updateEditorButtonPosition();
    void updateContainerPosition();
    void updateInnerButtonPosition();
    void updateLevelEditor();
    void renderLevelEditorUI();
    void drawBrushPreview(TerrainMaterial terrainType);
    //=================================================================

    // Free functions
    //=================================================================
    void freeLevelEditor();
    //=================================================================

    // Create file/directory functions
    //=================================================================
    bool makeFilePath(int level);
    bool makeLevelFile(int level);
    //=================================================================

    // Functions for deleting and create level data
    //=================================================================
    void deleteLevelData(int level);
    void createLevelData(int level, int width = 80, int height = 45, int tilesize = 20);
    //=================================================================

    // Functions for saving/writing level data to JSON
    //=================================================================
    void saveMapInfo(int width, int height, int tilesize);
    void saveTerrainInfo(std::vector<float> nodes, std::string terrainType);
    void saveStartEndInfo(std::vector<StartEnd> startPoints, StartEnd endPoint);
    void writeToFile(int level);
    //=================================================================

    // Functions for reading level data from JSON
    //=================================================================
    bool getLevelData(int level);
    void checkLevelData();
    void parseMapInfo(int& width, int& height, int& tilesize);
    void parseTerrainInfo(std::vector<float>& nodes, std::string terrainType);
    void parseStartEndInfo(StartEndPoint& startEndPointSystem);
    //=================================================================

    // Public variables
    //=================================================================
    // Array of bool to store which level is playable
    bool playableLevels[static_cast<int>(Level::None)]{};
    // For preview placement
    f32 brush_radius_ = 20.0f;
    //=================================================================

private:
    editorMode level_editor_mode_{editorMode::None};
    int current_level_{0};
    GameBlock current_gameblock_{GameBlock::None};

    // For level editor UI
    AEVec2 container_scale_ = {};
    bool display_builder_container_ = false;
    // Builder button is the parent container for builder container
    // Builder container is the parent container for all the buttons in the builder pool
    Button builder_button;
    Button builder_container;
    std::vector<Button> buttonPool;

    // Brush mesh
    AEGfxVertexList* circleMesh = nullptr;

    // For saving and reading level data
    Json::Value savingRoot{};
    Json::Value readingRoot{};
};

extern LevelManager levelManager;