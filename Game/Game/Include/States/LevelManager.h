/*!
@file       LevelManager.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This header file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

#include <filesystem>
#include <vector>

#include <AEEngine.h>
#include <json/json.h>

#include "Button.h"
#include "Collectible.h"
#include "Moss.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "Terrain.h"

// Level 1 to 8 are dev levels
// Level 9 to 12 are player levels
enum class Level {
    Level1,
    Level2,
    Level3,
    Level4,
    Level5,
    Level6,
    Level7,
    Level8,
    Level9,
    Level10,
    Level11,
    Level12,
    None,
    PlayerLevels = 8,
    Level99 = 99
};
enum class EditorMode { Edit, Delete, Create, None };
enum class GameBlock { Dirt, Stone, Magic, StartPoint, EndPoint, Collectible, Portal, Moss, None };

class LevelManager {
public:
    void init();
    void initEditorUI(s8 font);

    // Getters and setters
    //=================================================================
    EditorMode getLevelEditorMode() const;
    void setLevelEditorMode(EditorMode mode);
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
    void drawBrushPreview(TerrainMaterial terrainType, f32 radius_ = 0.0f);
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
    void createLevelData(int level, int width = 80, int height = 45, int tilesize = 20,
                         int portalLimit = 0);
    //=================================================================

    // Functions for saving/writing level data to JSON
    //=================================================================
    void saveMapInfo(int width, int height, int tilesize, int portalLimit);
    void saveTerrainInfo(std::vector<float>& nodes, const std::string& terrainType);
    void saveStartEndInfo(std::vector<StartEnd>& startPoints, StartEnd& endPoint);
    void saveCollectibleInfo(std::vector<Collectible>& collectibles);
    void saveMossInfo(std::vector<Moss>& mosses);
    void savePortalInfo(PortalSystem& portalSystem);
    void writeToFile(int level);
    //=================================================================

    // Functions for reading level data from JSON
    //=================================================================
    bool getLevelData(int level);
    void checkLevelData();
    int getHighScore(int level) const;
    bool saveLevelProgress(int level, int collectedCount);
    void parseMapInfo(int& width, int& height, int& tilesize, int& portalLimit);
    void parseTerrainInfo(std::vector<float>& nodes, std::string terrainType);
    void parseStartEndInfo(StartEndPoint& startEndPointSystem);
    void parseCollectibleInfo(CollectibleSystem& collectibleSystem);
    void parseMossInfo(MossSystem& mossSystem);
    void parsePortalInfo(PortalSystem& portalSystem);
    //=================================================================

    // Public variables
    //=================================================================
    // Array of bool to store which level is playable
    bool playableLevels_[static_cast<int>(Level::None)]{};
    // For preview placement
    f32 brushRadius_ = 20.0f;
    //=================================================================

private:
    EditorMode levelEditorMode_{EditorMode::None};
    int currentLevel_{0};
    int levelHighScores_[static_cast<int>(Level::None)] = {0};
    GameBlock currentGameBlock_{GameBlock::None};

    // For level editor UI
    float editorButtonStartPosX_ = {};
    bool displayEditorContainer_ = false;
    // Builder button is the parent container for builder container
    // Builder container is the parent container for all the buttons in the builder pool
    Button editorButton_;
    Button editorContainer_;
    std::vector<Button> editorButtonPool_;
    TextData editorControlsText_;

    // Brush mesh
    AEGfxVertexList* circleMesh_ = nullptr;

    // For saving and reading level data
    Json::Value savingRoot_{};
    Json::Value readingRoot_{};
};

extern LevelManager levelManager;