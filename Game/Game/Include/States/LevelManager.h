#pragma once

#include "Button.h"
#include "StartEndPoint.h"
#include "Terrain.h"

#include <AEEngine.h>
#include <filesystem>
#include <json/json.h>
#include <vector>

enum class Level { Level1, Level2, Level3, Level4, Level5, Level6, Level7, Level8, None };
enum class editorMode { Edit, Delete, Create, None };
enum class GameBlock { Dirt, Stone, Magic, StartPoint, EndPoint, None };

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
    void renderLevelEditorUI(s8 font);
    void drawBrushPreview(TerrainMaterial terrainType);
    //=================================================================

    // Free functions
    //=================================================================
    void freeLevelEditor();
    //=================================================================

    // Create file/directory functions
    //=================================================================
    bool makeLevelFilePath(int level);
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
    bool playableLevels_[static_cast<int>(Level::None)]{};
    // For preview placement
    f32 brushRadius_ = 20.0f;
    //=================================================================

private:
    editorMode levelEditorMode_{editorMode::None};
    int currentLevel_{0};
    GameBlock currentGameBlock_{GameBlock::None};

    // For level editor UI
    float editorButtonStartPosX_ = {};
    bool displayEditorContainer_ = false;
    // Builder button is the parent container for builder container
    // Builder container is the parent container for all the buttons in the builder pool
    Button editorButton_;
    Button editorContainer_;
    std::vector<Button> editorButtonPool_;

    // Brush mesh
    AEGfxVertexList* circleMesh = nullptr;

    // For saving and reading level data
    Json::Value savingRoot_{};
    Json::Value readingRoot_{};
};

extern LevelManager levelManager;