#include "States/LevelManager.h"
#include "ConfigManager.h"
#include "Moss.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>

LevelManager levelManager;

void LevelManager::init() {
    levelEditorMode_ = static_cast<editorMode>(configManager.getInt(
        "LevelManager", "default", "levelEditorMode_", static_cast<int>(editorMode::None)));
    currentLevel_ = configManager.getInt("LevelManager", "default", "currentLevel_", 0);
    currentGameBlock_ = static_cast<GameBlock>(configManager.getInt(
        "LevelManager", "default", "currentGameBlock_", static_cast<int>(GameBlock::None)));

    // For level editor UI
    editorButtonStartPosX_ =
        configManager.getFloat("LevelManager", "default", "editorButtonStartPosX_", 775.0f);
    displayEditorContainer_ =
        configManager.getBool("LevelManager", "default", "displayEditorContainer_", true);
}

void LevelManager::initEditorUI() {
    // Setup Editor UI
    editorButton_.loadMesh();
    editorButton_.loadTexture("Assets/Textures/pale_blue_button.png");
    editorContainer_.loadMesh();
    editorContainer_.loadTexture("Assets/Textures/pink_button.png");

    editorButton_.initFromJson("level_manager_buttons", "editorButton_");
    editorContainer_.initFromJson("level_manager_buttons", "editorContainer_");
    // Set container position relative to button
    updateEditorButtonPosition();
    updateContainerPosition();

    editorButtonPool_.resize(static_cast<int>(GameBlock::None));
    for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
        editorButtonPool_[i].loadMesh();
        editorButtonPool_[i].loadTexture("Assets/Textures/pale_blue_button.png");
    }
    updateInnerButtonPosition();

    // Setup brush preview
    circleMesh = CreateCircleMesh(20, 0.5f);

    savingRoot_ = Json::Value();
    readingRoot_ = Json::Value();
}

editorMode LevelManager::getLevelEditorMode() const { return levelEditorMode_; }

void LevelManager::setLevelEditorMode(editorMode mode) { levelEditorMode_ = mode; }

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
    AEVec2 container_pos{};
    container_pos.x =
        (editorButton_.getTransform().pos_.x + editorButton_.getTransform().scale_.x / 2) +
        (editorContainer_.getTransform().scale_.x / 2);

    container_pos.y = editorButton_.getTransform().pos_.y -
                      (editorContainer_.getTransform().scale_.y / 2) +
                      (editorButton_.getTransform().scale_.y / 2);

    editorContainer_.setTransform(container_pos, editorContainer_.getTransform().scale_,
                                  editorContainer_.getTransform().rotationRad_);

    editorContainer_.updateTransform();
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
    if (levelEditorMode_ != editorMode::Edit) {
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
    s32 scroll_input{};
    AEInputMouseWheelDelta(&scroll_input);
    if (scroll_input > 0) {
        brushRadius_ += 5.0f; // Increase brush size
        if (brushRadius_ > 100.0f) {
            brushRadius_ = 100.0f; // Cap maximum brush size
        }
        std::cout << "Scroll up detected. Brush radius: " << brushRadius_ << "\n";
    } else if (scroll_input < 0) {
        brushRadius_ -= 5.0f; // Decrease brush size
        if (brushRadius_ < 20.0f) {
            brushRadius_ = 20.0f; // Cap minimum brush size
        }
        std::cout << "Scroll down detected. Brush radius: " << brushRadius_ << "\n";
    }
}

void LevelManager::renderLevelEditorUI(s8 font) {

    if (levelEditorMode_ != editorMode::Edit) {
        std::cout << "Not in edit mode, skipping render\n";
        return;
    }

    // Render builder button
    editorButton_.draw(font);

    // Render builder container and buttons within if displayEditorContainer_ is true
    if (displayEditorContainer_) {
        editorContainer_.draw(font);
        // Render buttons in button pool
        for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
            editorButtonPool_[i].draw(font);
        }
    }
}

void LevelManager::freeLevelEditor() {
    editorButton_.unload();
    editorContainer_.unload();
    AEGfxMeshFree(circleMesh);

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

void LevelManager::createLevelData(int level, int width, int height, int tilesize) {
    saveMapInfo(width, height, tilesize);
    writeToFile(level);
}

void LevelManager::saveMapInfo(int& width, int& height, int& tilesize) {
    savingRoot_["Map"]["width"] = width;
    savingRoot_["Map"]["height"] = height;
    savingRoot_["Map"]["tileSize"] = tilesize;
}
void LevelManager::saveTerrainInfo(std::vector<float>& nodes, const std::string& terrainType) {
    Json::Value terrainJson(Json::arrayValue);
    for (size_t i = 0; i < nodes.size(); ++i) {
        terrainJson.append(nodes[i]);
    }
    savingRoot_["Terrain"][terrainType] = terrainJson;
}

void LevelManager::saveStartEndInfo(std::vector<StartEnd>& startPoints, StartEnd& endPoint) {
    Json::Value startPointsJson_array(Json::arrayValue);
    for (const auto& startPoint : startPoints) {
        Json::Value startPointJson;
        startPointJson["posX"] = startPoint.transform_.pos_.x;
        startPointJson["posY"] = startPoint.transform_.pos_.y;
        startPointJson["scaleX"] = startPoint.transform_.scale_.x;
        startPointJson["scaleY"] = startPoint.transform_.scale_.y;
        startPointJson["rotation"] = startPoint.transform_.rotationRad_;
        startPointJson["type"] = static_cast<int>(startPoint.type_);
        startPointJson["direction"] = static_cast<int>(startPoint.direction_);
        startPointsJson_array.append(startPointJson);
    }
    savingRoot_["Objects"]["startPoints"] = startPointsJson_array;

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

    Json::Value collectiblesJson_array(Json::arrayValue);
    for (const auto& collectible : collectibles) {
        Json::Value collectibleJson;
        collectibleJson["posX"] = collectible.transform_.pos_.x;
        collectibleJson["posY"] = collectible.transform_.pos_.y;
        collectibleJson["type"] = static_cast<int>(collectible.type_);
        collectiblesJson_array.append(collectibleJson);
    }
    savingRoot_["Objects"]["collectibles"] = collectiblesJson_array;
}
void LevelManager::saveMossInfo(std::vector<Moss>& mosses) {
    Json::Value mossJson_array(Json::arrayValue);
    for (const auto& moss : mosses) {
        Json::Value mossJson;
        mossJson["posX"] = moss.transform_.pos_.x;
        mossJson["posY"] = moss.transform_.pos_.y;
        mossJson["type"] = static_cast<int>(moss.type_);
        mossJson["health"] = moss.currentHealth_;
        mossJson["maxHealth"] = moss.maxHealth_;
        mossJson["absorptionRate"] = moss.absorptionRate_;
        mossJson_array.append(mossJson);
    }
    savingRoot_["Objects"]["moss"] = mossJson_array;
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
    Json::Value portalsJson_array(Json::arrayValue);

    for (const auto& portal : portalSystem.GetPortals()) {
        Json::Value portalJson;
        portalJson["posX"] = portal->transform_.pos_.x;
        portalJson["posY"] = portal->transform_.pos_.y;
        portalJson["scaleX"] = portal->transform_.scale_.x;
        portalJson["scaleY"] = portal->transform_.scale_.y;
        portalJson["rotation"] = portal->transform_.rotationRad_;

        portalsJson_array.append(portalJson);
    }

    savingRoot_["Objects"]["portals"] = portalsJson_array;
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

void LevelManager::parseMapInfo(int& width, int& height, int& tilesize) {
    if (readingRoot_.isMember("Map")) {
        std::cout << "Parsing map info...\n";
        width = readingRoot_["Map"]["width"].asInt();
        height = readingRoot_["Map"]["height"].asInt();
        tilesize = readingRoot_["Map"]["tileSize"].asInt();
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
                f32 rotationRad_ = startPointsJson[i]["rotation"].asFloat();
                auto type = static_cast<StartEndType>(startPointsJson[i]["type"].asInt());
                auto dir = static_cast<GoalDirection>(startPointsJson[i]["direction"].asInt());
                startEndPointSystem.SetupPoint(pos, scale, rotationRad_, type, dir);
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
            f32 rotationRad_ = endPointJson["rotation"].asFloat();
            auto type = static_cast<StartEndType>(endPointJson["type"].asInt());
            auto dir = static_cast<GoalDirection>(endPointJson["direction"].asInt());
            startEndPointSystem.SetupPoint(pos, scale, rotationRad_, type, dir);
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

void LevelManager::drawBrushPreview(TerrainMaterial terrainType) {
    AEVec2 mousePos = GetMouseWorldPos();

    // Set up world matrix
    AEMtx33 scale_mtx, rot_mtx, trans_mtx, world_mtx;

    AEMtx33Scale(&scale_mtx, brushRadius_ * 2, brushRadius_ * 2);
    AEMtx33Rot(&rot_mtx, 0.0f);
    AEMtx33Trans(&trans_mtx, mousePos.x, mousePos.y);

    AEMtx33Concat(&world_mtx, &rot_mtx, &scale_mtx);
    AEMtx33Concat(&world_mtx, &trans_mtx, &world_mtx);

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
    }

    AEGfxSetTransform(world_mtx.m);
    AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
}