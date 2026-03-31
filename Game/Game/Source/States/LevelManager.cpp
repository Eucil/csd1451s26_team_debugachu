/*!
@file       LevelManager.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  Sean Lee Hong Wei/seanhongwei.lee@digipen.edu,
            Han Tianchou/H.tianchou@digipen.edu

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "States/LevelManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <json/json.h>

#include "ConfigManager.h"
#include "Moss.h"
#include "Utils.h"

LevelManager levelManager;

void LevelManager::init() {
    levelEditorMode_ = static_cast<EditorMode>(g_configManager.getInt(
        "LevelManager", "default", "levelEditorMode_", static_cast<int>(EditorMode::None)));
    currentLevel_ = g_configManager.getInt("LevelManager", "default", "currentLevel_", 0);
    currentGameBlock_ = static_cast<GameBlock>(g_configManager.getInt(
        "LevelManager", "default", "currentGameBlock_", static_cast<int>(GameBlock::None)));

    displayEditorContainer_ =
        g_configManager.getBool("LevelManager", "default", "displayEditorContainer_", true);
}

void LevelManager::initEditorUI(s8 font) {
    // Setup Editor UI
    editorButton_.loadMesh();
    editorButton_.loadTexture("Assets/Textures/brown_rectangle_40_24.png");
    editorContainer_.loadMesh();
    editorContainer_.loadTexture("Assets/Textures/brown_square_80_80.png");

    editorButton_.initFromJson("level_manager_buttons", "editorButton_");
    editorButton_.setTextFont(font);
    editorButtonStartPosX_ = editorButton_.getTransform().pos_.x;
    editorContainer_.initFromJson("level_manager_buttons", "editorContainer_");
    editorContainer_.setTextFont(font);
    editorControlsText_.initFromJson("level_manager_buttons", "editorControlsText_");
    editorControlsText_.font_ = font;
    // Set container position relative to button
    updateEditorButtonPosition();
    updateContainerPosition();

    static const char* blockNames[] = {"Dirt", "Stone", "Magic",  "Start",
                                       "End",  "Item",  "Portal", "Moss"};
    static const char* blockTextures[] = {
        "Assets/Textures/terrain_dirt.png",
        "Assets/Textures/terrain_stone.png",
        "Assets/Textures/terrain_magic.png",
        "Assets/Textures/overgrown_pipe_end.png",
        "Assets/Textures/pink_flower_sprite_sheet.png",
        "Assets/Textures/white_square.png",
        "Assets/Textures/wormhole.png",
        "Assets/Textures/white_square.png",
    };

    editorButtonPool_.resize(static_cast<int>(GameBlock::None));
    for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
        editorButtonPool_[i].loadMesh();
        editorButtonPool_[i].loadTexture(blockTextures[i]);
        editorButtonPool_[i].setTextFont(font);
        editorButtonPool_[i].setText(blockNames[i], 0.f, 0.f, 0.5f, 1.f, 1.f, 1.f, 1.f);
    }
    updateInnerButtonPosition();

    // Setup brush preview
    circleMesh_ = CreateCircleMesh(20, 0.5f);

    savingRoot_ = Json::Value();
    readingRoot_ = Json::Value();
}

EditorMode LevelManager::getLevelEditorMode() const { return levelEditorMode_; }

void LevelManager::setLevelEditorMode(EditorMode mode) { levelEditorMode_ = mode; }

int LevelManager::getCurrentLevel() const { return currentLevel_; }

void LevelManager::SetCurrentLevel(int level) { currentLevel_ = level; }

GameBlock LevelManager::getCurrentGameBlock() const { return currentGameBlock_; }

void LevelManager::setCurrentGameBlock(GameBlock block) { currentGameBlock_ = block; }

bool LevelManager::getDisplayBuilderContainer() const { return displayEditorContainer_; }

void LevelManager::updateEditorButtonPosition() {
    // Update builder button position
    // If builder container is displayed, move button based on container scale
    if (displayEditorContainer_) {
        AEVec2 newPos = {editorButton_.getTransform().pos_.x -
                             editorContainer_.getTransform().scale_.x,
                         editorButton_.getTransform().pos_.y};
        editorButton_.setTransform(newPos, editorButton_.getTransform().scale_,
                                   editorButton_.getTransform().rotationRad_);
    } else {
        editorButton_.setTransform({editorButtonStartPosX_, editorButton_.getTransform().pos_.y},
                                   editorButton_.getTransform().scale_,
                                   editorButton_.getTransform().rotationRad_);
    }

    editorButton_.updateTransform();
}

void LevelManager::updateContainerPosition() {
    // Update builder button position
    // And make sure builder container follows the button
    AEVec2 containerPos{};
    containerPos.x =
        (editorButton_.getTransform().pos_.x + editorButton_.getTransform().scale_.x / 2) +
        (editorContainer_.getTransform().scale_.x / 2);

    containerPos.y = editorButton_.getTransform().pos_.y -
                     (editorContainer_.getTransform().scale_.y / 2) +
                     (editorButton_.getTransform().scale_.y / 2);

    editorContainer_.setTransform(containerPos, editorContainer_.getTransform().scale_,
                                  editorContainer_.getTransform().rotationRad_);

    editorContainer_.updateTransform();

    // Position controls text centered below the container
    f32 halfWinW = AEGfxGetWindowWidth() / 2.0f;
    f32 halfWinH = AEGfxGetWindowHeight() / 2.0f;
    editorControlsText_.x_ =
        (containerPos.x - editorContainer_.getTransform().scale_.x / 2.0f) / halfWinW;
    editorControlsText_.y_ =
        (containerPos.y - editorContainer_.getTransform().scale_.y / 2.0f - 30.0f) / halfWinH;
}

void LevelManager::updateInnerButtonPosition() {
    // Update inner button position to follow the container
    AEVec2 containerPos = editorContainer_.getTransform().pos_;
    AEVec2 containerScale = editorContainer_.getTransform().scale_;
    float padding = 15.0f;

    int totalButtons = static_cast<int>(GameBlock::None);

    int columns = 3;
    if (columns > totalButtons)
        columns = totalButtons;                        // avoid empty columns
    int rows = (totalButtons + columns - 1) / columns; // ceil division

    // Available space inside container after padding on edges and between items
    float availableWidth = containerScale.x - (columns + 1) * padding;
    float availableHeight = containerScale.y - (rows + 1) * padding;

    // Button size (fit to grid)
    float buttonWidth = availableWidth / columns;
    float buttonHeight = availableHeight / rows;

    // Top-left corner of the inner area (where the first padding starts)
    float left = containerPos.x - (containerScale.x / 2.0f) + padding;
    float top = containerPos.y + (containerScale.y / 2.0f) - padding;

    for (int i = 0; i < totalButtons; ++i) {
        int r = i / columns; // row index 0..rows-1
        int c = i % columns; // col index 0..columns-1

        // center of the button in world coords
        float x = left + c * (buttonWidth + padding) + buttonWidth * 0.5f;
        float y = top - r * (buttonHeight + padding) - buttonHeight * 0.5f;

        editorButtonPool_[i].setTransform({x, y}, {buttonWidth, buttonHeight});

        // If your SetupMesh needs to be re-run after transform changes, keep this.
        editorButtonPool_[i].updateTransform();
    }
}

void LevelManager::updateLevelEditor() {
    if (levelEditorMode_ != EditorMode::Edit) {
        return;
    }

    // If Tab is pressed, toggle builder container
    if (AEInputCheckReleased(AEVK_TAB) || 0 == AESysDoesWindowExist()) {
        displayEditorContainer_ = !displayEditorContainer_;
        updateEditorButtonPosition();
        updateContainerPosition();
        updateInnerButtonPosition();
    }

    // Check if any button in the button pool is clicked
    for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
        if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
            if (editorButtonPool_[i].checkMouseClick()) {
                currentGameBlock_ = static_cast<GameBlock>(i);
                std::cout << "Selected block: " << static_cast<int>(currentGameBlock_) << "\n";
            }
        }
    }

    // Get scroll input for brush size adjustment
    s32 scrollInput{};
    AEInputMouseWheelDelta(&scrollInput);
    if (scrollInput > 0) {
        brushRadius_ += 5.0f; // Increase brush size
        if (brushRadius_ > 100.0f) {
            brushRadius_ = 100.0f; // Cap maximum brush size
        }
        std::cout << "Scroll up detected. Brush radius: " << brushRadius_ << "\n";
    } else if (scrollInput < 0) {
        brushRadius_ -= 5.0f; // Decrease brush size
        if (brushRadius_ < 20.0f) {
            brushRadius_ = 20.0f; // Cap minimum brush size
        }
        std::cout << "Scroll down detected. Brush radius: " << brushRadius_ << "\n";
    }
}

void LevelManager::renderLevelEditorUI() {

    if (levelEditorMode_ != EditorMode::Edit) {
        std::cout << "Not in edit mode, skipping render\n";
        return;
    }

    // Render builder button
    editorButton_.draw();

    // Render builder container and buttons within if displayEditorContainer_ is true
    if (displayEditorContainer_) {
        editorContainer_.draw();

        // Render buttons in button pool
        // These arrays let you tint the blocks:
        static const f32 blockBaseR[] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f};
        static const f32 blockBaseG[] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.6f};
        static const f32 blockBaseB[] = {1.f, 1.f, 1.f, 1.f, 1.f, 0.1f, 1.f, 0.f};

        // Collectible(5) = yellow (1,1,0)
        // Moss(7) = green (0,0.6,0)
        for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
            f32 mul = (i == static_cast<int>(currentGameBlock_)) ? 1.f : 0.4f;
            editorButtonPool_[i].setRGBA(blockBaseR[i] * mul, blockBaseG[i] * mul,
                                         blockBaseB[i] * mul, 1.f);
            editorButtonPool_[i].draw();
        }

        editorControlsText_.draw(false);
    }
}

void LevelManager::freeLevelEditor() {
    editorButton_.unload();
    editorContainer_.unload();
    AEGfxMeshFree(circleMesh_);

    for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
        editorButtonPool_[i].unload();
    }
}

bool LevelManager::makeLevelFilePath(int level) {
    std::string levelPath = "Assets/Levels/Level_" + std::to_string(level);
    std::filesystem::path filepath(levelPath);

    if (std::filesystem::exists(filepath)) {
        std::cout << "Path exists\n";
    } else {
        std::cout << "Path does not exist\n";
        std::cout << "Creating directory...\n";
        if (std::filesystem::create_directories(filepath)) {
            std::cout << "Directory created successfully\n";
        } else {
            std::cout << "Failed to create directory\n";
            return false;
        }
    }

    return true;
}

bool LevelManager::makeLevelFile(int level) {
    // check if file exists, if not create file
    std::string levelPath = "Assets/Levels/Level_" + std::to_string(level) + "/Map.json";
    std::filesystem::path filePath(levelPath);

    if (std::filesystem::exists(filePath)) {
        std::cout << "File exists\n";
    } else {
        std::cout << "File does not exist\n";
        std::cout << "Creating file...\n";
        std::ofstream outfile(filePath); // Creates the file
        if (outfile) {
            std::cout << "File created successfully\n";

            // Write some default value to file
            Json::Value root;
            root["None"];

            // Builder is use for file configuration
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  "; // pretty print
            // Writer is used to write the json value to file
            std::unique_ptr<Json::StreamWriter> jsonWriter(builder.newStreamWriter());
            jsonWriter->write(root, &outfile);
        } else {
            std::cout << "Failed to create file\n";
            return false;
        }
    }
    return true;
}

void LevelManager::deleteLevelData(int level) {
    std::string levelPath = "Assets/Levels/Level_" + std::to_string(level) + "/Map.json";
    std::filesystem::path filePath(levelPath);

    // Set file to default value
    savingRoot_ = Json::Value();
    savingRoot_["None"];

    // Builder is use for file configuration
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    builder["emitUTF8"] = true; // Writer is used to write the json value to file
    builder["sortKeys"] = false;
    std::unique_ptr<Json::StreamWriter> jsonWriter(builder.newStreamWriter());
    std::ofstream outfile(filePath);
    if (outfile) {
        jsonWriter->write(savingRoot_, &outfile);
        std::cout << "File saved successfully\n";
    } else {
        std::cout << "Failed to open file for writing\n";
    }

    // Clear root after writing to file to prevent accidental reuse
    savingRoot_.clear();
}

void LevelManager::createLevelData(int level, int width, int height, int tilesize,
                                   int portalLimit) {
    saveMapInfo(width, height, tilesize, portalLimit);
    writeToFile(level);
}

void LevelManager::saveMapInfo(int width, int height, int tilesize, int portalLimit) {
    savingRoot_["Map"]["width"] = width;
    savingRoot_["Map"]["height"] = height;
    savingRoot_["Map"]["tileSize"] = tilesize;
    savingRoot_["Map"]["portalLimit"] = portalLimit;
}
void LevelManager::saveTerrainInfo(std::vector<float>& nodes, const std::string& terrainType) {
    Json::Value terrainJson(Json::arrayValue);
    for (size_t i = 0; i < nodes.size(); ++i) {
        terrainJson.append(nodes[i]);
    }
    savingRoot_["Terrain"][terrainType] = terrainJson;
}

void LevelManager::saveStartEndInfo(std::vector<StartEnd>& startPoints, StartEnd& endPoint) {
    Json::Value startPointsJsonArray(Json::arrayValue);
    for (const auto& startPoint : startPoints) {
        Json::Value startPointJson;
        startPointJson["posX"] = startPoint.transform_.pos_.x;
        startPointJson["posY"] = startPoint.transform_.pos_.y;
        startPointJson["scaleX"] = startPoint.transform_.scale_.x;
        startPointJson["scaleY"] = startPoint.transform_.scale_.y;
        startPointJson["rotation"] = startPoint.transform_.rotationRad_;
        startPointJson["type"] = static_cast<int>(startPoint.type_);
        startPointJson["direction"] = static_cast<int>(startPoint.direction_);
        startPointsJsonArray.append(startPointJson);
    }
    savingRoot_["Objects"]["startPoints"] = startPointsJsonArray;

    Json::Value endPointJson;
    endPointJson["posX"] = endPoint.transform_.pos_.x;
    endPointJson["posY"] = endPoint.transform_.pos_.y;
    endPointJson["scaleX"] = endPoint.transform_.scale_.x;
    endPointJson["scaleY"] = endPoint.transform_.scale_.y;
    endPointJson["rotation"] = endPoint.transform_.rotationRad_;
    endPointJson["type"] = static_cast<int>(endPoint.type_);
    endPointJson["direction"] = static_cast<int>(endPoint.direction_);
    savingRoot_["Objects"]["endPoint"] = endPointJson;
}

void LevelManager::saveCollectibleInfo(std::vector<Collectible>& collectibles) {

    Json::Value collectiblesJsonArray(Json::arrayValue);
    for (const auto& collectible : collectibles) {
        Json::Value collectibleJson;
        collectibleJson["posX"] = collectible.transform_.pos_.x;
        collectibleJson["posY"] = collectible.transform_.pos_.y;
        collectibleJson["type"] = static_cast<int>(collectible.type_);
        collectiblesJsonArray.append(collectibleJson);
    }
    savingRoot_["Objects"]["collectibles"] = collectiblesJsonArray;
}
void LevelManager::saveMossInfo(std::vector<Moss>& mosses) {
    Json::Value mossJsonArray(Json::arrayValue);
    for (const auto& moss : mosses) {
        Json::Value mossJson;
        mossJson["posX"] = moss.transform_.pos_.x;
        mossJson["posY"] = moss.transform_.pos_.y;
        mossJson["type"] = static_cast<int>(moss.type_);
        mossJson["health"] = moss.currentHealth_;
        mossJson["maxHealth"] = moss.maxHealth_;
        mossJson["absorptionRate"] = moss.absorptionRate_;
        mossJsonArray.append(mossJson);
    }
    savingRoot_["Objects"]["moss"] = mossJsonArray;
}

void LevelManager::parseMossInfo(MossSystem& mossSystem) {
    if (readingRoot_.isMember("Objects")) {
        const Json::Value& objects = readingRoot_["Objects"];
        if (objects.isMember("moss")) {
            const Json::Value& mosses = objects["moss"];
            for (Json::ArrayIndex i = 0; i < mosses.size(); ++i) {
                AEVec2 pos{mosses[i]["posX"].asFloat(), mosses[i]["posY"].asFloat()};
                MossType type = static_cast<MossType>(mosses[i]["type"].asInt());
                mossSystem.LoadLevelMoss(pos, type);

                // Optional: restore health if saved
                // You'd need to modify LoadLevelMoss to accept more parameters
            }
        }
    }
}

void LevelManager::savePortalInfo(PortalSystem& portalSystem) {
    Json::Value portalsJsonArray(Json::arrayValue);

    for (const auto& portal : portalSystem.GetPortals()) {
        Json::Value portalJson;
        portalJson["posX"] = portal->transform_.pos_.x;
        portalJson["posY"] = portal->transform_.pos_.y;
        portalJson["scaleX"] = portal->transform_.scale_.x;
        portalJson["scaleY"] = portal->transform_.scale_.y;
        portalJson["rotation"] = portal->transform_.rotationRad_;

        portalsJsonArray.append(portalJson);
    }

    savingRoot_["Objects"]["portals"] = portalsJsonArray;
}

void LevelManager::writeToFile(int level) {
    std::string levelPath = "Assets/Levels/Level_" + std::to_string(level) + "/Map.json";
    std::filesystem::path filePath(levelPath);
    if (!std::filesystem::exists(filePath)) {
        std::cout << "File does not exist\n";
        return;
    }

    // Builder is use for file configuration
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    builder["emitUTF8"] = true; // Writer is used to write the json value to file
    builder["sortKeys"] = false;
    std::unique_ptr<Json::StreamWriter> jsonWriter(builder.newStreamWriter());
    std::ofstream outfile(filePath);
    if (outfile) {
        jsonWriter->write(savingRoot_, &outfile);
        std::cout << "File saved successfully\n";
    } else {
        std::cout << "Failed to open file for writing\n";
    }

    // Clear root after writing to file to prevent accidental reuse
    savingRoot_.clear();
}

bool LevelManager::getLevelData(int level) {
    // Clear previously read data to prevent accidental reuse
    readingRoot_ = Json::Value();

    std::string filePath = "Assets/Levels/Level_" + std::to_string(level) + "/Map.json";
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filePath << "\n";
        // Make directory and file if they don't exist
        if (makeLevelFilePath(level) && makeLevelFile(level)) {
            std::cout << "Created missing directory and file for level " << level << "\n";
        } else {
            std::cout << "Failed to create missing directory and file for level " << level << "\n";
        }
        return false;
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;



    bool parsingSuccessful = Json::parseFromStream(builder, file, &readingRoot_, &errs);
    
    if (readingRoot_.isMember("Map") && readingRoot_["Map"].isMember("highScore")) {
        levelHighScores_[level - 1] = readingRoot_["Map"]["highScore"].asInt();
    } else {
        levelHighScores_[level - 1] = 0; // Default to 0 if no score exists yet
    }

    if (!parsingSuccessful) {
        std::cout << "Failed to parse JSON: " << errs << "\n";
        return false;
    }

    if (readingRoot_.isMember("None")) {
        std::cout << "File is empty\n";
        return false;
    }


    return true;
}

void LevelManager::checkLevelData() {
    // Loop through and check if level is playable by calling getLevelData for each
    for (int i = 1; i <= static_cast<int>(Level::None); ++i) {
        playableLevels_[i - 1] = getLevelData(i);
        std::cout << "Level " << i << " playable: " << playableLevels_[i - 1] << "\n";
    }
}

int LevelManager::getHighScore(int level) const {
    if (level > 0 && level <= static_cast<int>(Level::None)) {
        return levelHighScores_[level - 1];
    }
    return 0;
}

bool LevelManager::saveLevelProgress(int level, int collectedCount) {
    std::string levelPath = "Assets/Levels/Level_" + std::to_string(level) + "/Map.json";
    std::ifstream inFile(levelPath);
    if (!inFile.is_open())
        return false;

    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    if (!Json::parseFromStream(readerBuilder, inFile, &root, &errs)) {
        inFile.close();
        return false;
    }
    inFile.close();

    int existingCount = root["Map"]["highScore"].asInt();

    // Only overwrite if it's a new record
    if (collectedCount > existingCount) {
        root["Map"]["highScore"] = collectedCount;
        levelHighScores_[level - 1] = collectedCount; // Update the cached array

        std::ofstream outFile(levelPath, std::ios::trunc);
        if (!outFile.is_open())
            return false;

        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(writerBuilder.newStreamWriter());
        writer->write(root, &outFile);
        outFile.close();

        return true;
    }
    return false;
}

void LevelManager::parseMapInfo(int& width, int& height, int& tilesize, int& portalLimit) {
    if (readingRoot_.isMember("Map")) {
        std::cout << "Parsing map info...\n";
        width = readingRoot_["Map"]["width"].asInt();
        height = readingRoot_["Map"]["height"].asInt();
        tilesize = readingRoot_["Map"]["tileSize"].asInt();
        portalLimit = readingRoot_["Map"]["portalLimit"].asInt();
    } else {
        std::cout << "Map info not found in JSON\n";
    }
}
void LevelManager::parseTerrainInfo(std::vector<float>& nodes, std::string terrainType) {
    if (!readingRoot_.isMember("Terrain")) {
        std::cout << "Terrain info not found in JSON\n";
        return;
    }
    const Json::Value& terrain = readingRoot_["Terrain"];
    if (!terrain.isMember(terrainType)) {
        std::cout << "Terrain type '" << terrainType << "' not found in JSON\n";
        return;
    }
    const Json::Value& nodeArray = terrain[terrainType];
    if (!nodeArray.isArray()) {
        std::cout << "Terrain[" << terrainType << "] is not an array\n";
        return;
    }

    // Debug: sizes
    std::cout << "Terrain '" << terrainType << "': nodeArray size = " << nodeArray.size()
              << ", nodes.size() = " << nodes.size() << "\n";

    Json::ArrayIndex count = nodeArray.size();
    for (Json::ArrayIndex i = 0; i < count; ++i) {
        nodes[i] = nodeArray[i].asFloat();
    }
}
void LevelManager::parseStartEndInfo(StartEndPoint& startEndPointSystem) {

    if (readingRoot_.isMember("Objects")) {
        const Json::Value& objects = readingRoot_["Objects"];
        // Parse start points
        if (objects.isMember("startPoints")) {
            std::cout << "Parsing start points info...\n";
            const Json::Value& startPointsJson = objects["startPoints"];
            for (Json::ArrayIndex i = 0; i < startPointsJson.size(); ++i) {
                AEVec2 pos{startPointsJson[i]["posX"].asFloat(),
                           startPointsJson[i]["posY"].asFloat()};
                AEVec2 scale{startPointsJson[i]["scaleX"].asFloat(),
                             startPointsJson[i]["scaleY"].asFloat()};
                f32 rotationRad = startPointsJson[i]["rotation"].asFloat();
                auto type = static_cast<StartEndType>(startPointsJson[i]["type"].asInt());
                auto dir = static_cast<GoalDirection>(startPointsJson[i]["direction"].asInt());
                startEndPointSystem.SetupPoint(pos, scale, rotationRad, type, dir);
            }
        } else {
            std::cout << "Start points not found in JSON\n";
        }
        // Parse end point
        if (objects.isMember("endPoint")) {
            std::cout << "Parsing end point info...\n";
            const Json::Value& endPointJson = objects["endPoint"];
            AEVec2 pos{endPointJson["posX"].asFloat(), endPointJson["posY"].asFloat()};
            AEVec2 scale{endPointJson["scaleX"].asFloat(), endPointJson["scaleY"].asFloat()};
            f32 rotationRad = endPointJson["rotation"].asFloat();
            auto type = static_cast<StartEndType>(endPointJson["type"].asInt());
            auto dir = static_cast<GoalDirection>(endPointJson["direction"].asInt());
            startEndPointSystem.SetupPoint(pos, scale, rotationRad, type, dir);
        } else {
            std::cout << "End point not found in JSON\n";
        }
    } else {
        std::cout << "Objects info not found in JSON\n";
    }
}

void LevelManager::parseCollectibleInfo(CollectibleSystem& collectibleSystem) {
    if (readingRoot_.isMember("Objects")) {
        const Json::Value& objects = readingRoot_["Objects"];
        // Parse collectibles
        if (objects.isMember("collectibles")) {
            const Json::Value& collectibles = objects["collectibles"];
            std::cout << "Parsing collectibles info...\n";

            for (Json::ArrayIndex i = 0; i < collectibles.size(); ++i) {
                AEVec2 pos{collectibles[i]["posX"].asFloat(), collectibles[i]["posY"].asFloat()};
                CollectibleType type =
                    static_cast<CollectibleType>(collectibles[i]["type"].asInt());
                collectibleSystem.LoadLevelCollectibles(pos, type);
            }
        } else {
            std::cout << "Collectibles info not found in JSON\n";
        }
    }
}

void LevelManager::parsePortalInfo(PortalSystem& portalSystem) {
    if (readingRoot_.isMember("Objects")) {
        const Json::Value& objects = readingRoot_["Objects"];

        if (objects.isMember("portals")) {
            std::cout << "Parsing portals info...\n";
            const Json::Value& portalsJson = objects["portals"];

            for (Json::ArrayIndex i = 0; i < portalsJson.size(); ++i) {
                AEVec2 pos = {portalsJson[i]["posX"].asFloat(), portalsJson[i]["posY"].asFloat()};
                AEVec2 scale = {portalsJson[i]["scaleX"].asFloat(),
                                portalsJson[i]["scaleY"].asFloat()};

                // Convert stored Radians back to Degrees for SetupPortal
                f32 rotationRad = portalsJson[i]["rotation"].asFloat();
                f32 rotationDeg = AERadToDeg(rotationRad);

                portalSystem.SetupPortal(pos, scale, rotationDeg);
            }
        } else {
            std::cout << "Portals not found in JSON\n";
        }
    }
}

void LevelManager::drawBrushPreview(TerrainMaterial terrainType, f32 radius_) {
    AEVec2 mousePos = GetMouseWorldPos();

    // Set up world matrix
    AEMtx33 scaleMtx, rotMtx, transMtx, worldMtx;

    // If no radius_ is passed in, use brushRadius_
    if (radius_ != 0.0f) {
        AEMtx33Scale(&scaleMtx, radius_ * 2, radius_ * 2);
    } else {
        AEMtx33Scale(&scaleMtx, brushRadius_ * 2, brushRadius_ * 2);
    }
    AEMtx33Rot(&rotMtx, 0.0f);
    AEMtx33Trans(&transMtx, mousePos.x, mousePos.y);

    AEMtx33Concat(&worldMtx, &rotMtx, &scaleMtx);
    AEMtx33Concat(&worldMtx, &transMtx, &worldMtx);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(0.5f);
    switch (terrainType) {
    case TerrainMaterial::Dirt:
        AEGfxSetColorToMultiply(0.545f, 0.271f, 0.075f, 1.0f); // Brown color for dirt preview
        break;
    case TerrainMaterial::Stone:
        AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 1.0f); // Gray color for stone preview
        break;
    case TerrainMaterial::Magic:
        AEGfxSetColorToMultiply(0.5f, 0.0f, 0.5f, 1.0f); // Purple color for magic preview
        break;
    }

    AEGfxSetTransform(worldMtx.m);
    AEGfxMeshDraw(circleMesh_, AE_GFX_MDM_TRIANGLES);
}