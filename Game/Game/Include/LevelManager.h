/*!
@file       LevelManager.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This header file declares the LevelManager class, which manages
            level loading, saving, editor UI, and playability tracking
            for both dev levels (1-8) and player-created levels (9+).

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// ============================
// Standard library
// ============================
#include <filesystem>
#include <vector>

// ============================
// Third-party
// ============================
#include <AEEngine.h>
#include <json/json.h>

// ============================
// Project
// ============================
#include "Button.h"
#include "Collectible.h"
#include "Moss.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "Terrain.h"

// Level 1 to 8 are dev levels
// Level 9 to 16 are player levels
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
    Level13,
    Level14,
    Level15,
    Level16,
    None,
    PlayerLevels = 8,
    Level99 = 99
};

// Whether the editor is in placement (Edit), deletion (Delete),
// creation (Create), or normal play (None) mode.
enum class EditorMode { Edit, Delete, Create, None };

// Types of game blocks that can be placed in the level editor
enum class GameBlock { Dirt, Stone, Magic, StartPoint, EndPoint, Collectible, Portal, Moss, None };

class LevelManager {
public:
    // Reads initial state (editor mode, current level, game block) from config JSON.
    void init();
    void initEditorUI(s8 font);
    void freeLevelEditor();

    // ==========================================
    // Getters & Setters
    // ==========================================
    EditorMode getLevelEditorMode() const;
    void setLevelEditorMode(EditorMode mode);
    int getCurrentLevel() const;
    void setCurrentLevel(int level);
    GameBlock getCurrentGameBlock() const;
    void setCurrentGameBlock(GameBlock block);
    bool getDisplayBuilderContainer() const;

    // ==========================================
    // Editor UI Update & Render
    // ==========================================
    void updateEditorButtonPosition();
    void updateContainerPosition();
    void updateInnerButtonPosition();
    void updateLevelEditor();
    void renderLevelEditorUI();
    void drawBrushPreview(TerrainMaterial terrainType, f32 radius_ = 0.0f);

    // ==========================================
    // File / Directory Management
    // ==========================================
    bool makeLevelFilePath(int level);
    bool makeLevelFile(int level);

    // ==========================================
    // Level Data — Create & Delete
    // ==========================================
    void deleteLevelData(int level);
    void createLevelData(int level, int width = 80, int height = 45, int tilesize = 20,
                         int portalLimit = 0);

    // ==========================================
    // Level Data — Save to JSON
    // ==========================================
    // Each save function populates savingRoot_. Call writeToFile() afterward
    // to flush everything to disk in one operation.
    void saveMapInfo(int width, int height, int tilesize, int portalLimit);
    void saveTerrainInfo(std::vector<float>& nodes, const std::string& terrainType);
    void saveStartEndInfo(std::vector<StartEnd>& startPoints, StartEnd& endPoint);
    void saveCollectibleInfo(std::vector<Collectible>& collectibles);
    void saveMossInfo(std::vector<Moss>& mosses);
    void savePortalInfo(PortalSystem& portalSystem);
    void writeToFile(int level);

    // ==========================================
    // Level Data — Load from JSON
    // ==========================================
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

    // ==========================================
    // Public Variables
    // ==========================================
    // Array of bool to store which level is playable
    bool playableLevels_[static_cast<int>(Level::None)]{};
    // For preview placement
    f32 brushRadius_ = 20.0f;

private:
    EditorMode levelEditorMode_{EditorMode::None};
    int currentLevel_{0};
    int levelHighScores_[static_cast<int>(Level::None)] = {0};
    GameBlock currentGameBlock_{GameBlock::None};

    // ==========================================
    // Editor UI
    // ==========================================
    float editorButtonStartPosX_ = {};
    bool displayEditorContainer_ = false;

    // editorButton_   — the toggle button that opens/closes the block selector panel.
    // editorContainer_ — the panel that holds the block selector buttons.
    // editorButtonPool_ — one button per GameBlock type inside the container.
    Button editorButton_;
    Button editorContainer_;
    std::vector<Button> editorButtonPool_;
    TextData editorControlsText_;

    // Brush mesh
    AEGfxVertexList* circleMesh_ = nullptr;

    // ==========================================
    // JSON Saving & Reading
    // ==========================================
    Json::Value savingRoot_{};
    Json::Value readingRoot_{};
};

extern LevelManager levelManager;