#include "States/LevelManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>

LevelManager levelManager;

void LevelManager::init() {
    level_editor_mode_ = false;
    current_level_ = 0;
    current_gameblock_ = GameBlock::None;
}

void LevelManager::initEditorUI() {
    // Setup Editor UI
    builder_button = Button(AEVec2{775.0f, 350.0f}, AEVec2{50.0f, 50.0f});
    builder_container = Button(AEVec2{0.0f, 0.0f}, container_scale_);
    // Set container position relative to button
    updateContainerPosition();
    // Set up meshes for button and container
    builder_button.SetupMesh();
    builder_container.SetupMesh();

    buttonPool.resize(static_cast<int>(GameBlock::None));
    updateInnerButtonPosition();

    // Setup brush preview
    circleMesh = CreateCircleMesh(20);

    savingRoot = Json::Value();
    readingRoot = Json::Value();
}

bool LevelManager::getLevelEditorMode() const { return level_editor_mode_; }

void LevelManager::setLevelEditorMode() { level_editor_mode_ = !level_editor_mode_; }

int LevelManager::getCurrentLevel() const { return current_level_; }

void LevelManager::SetCurrentLevel(int level) { current_level_ = level; }

GameBlock LevelManager::getCurrentGameBlock() const { return current_gameblock_; }

void LevelManager::setCurrentGameBlock(GameBlock block) { current_gameblock_ = block; }

bool LevelManager::getDisplayBuilderContainer() const { return display_builder_container_; }

void LevelManager::updateEditorButtonPosition() {
    // Update builder button position
    // If builder container is displayed, move button based on container scale
    if (display_builder_container_) {
        builder_button.transform_.pos_.x = builder_button.transform_.pos_.x - (container_scale_.x);
    } else {
        builder_button.transform_.pos_.x = 775.0f;
    }

    builder_button.UpdateTransform();
}

void LevelManager::updateContainerPosition() {
    // Update builder button position
    // And make sure builder container follows the button
    AEVec2 container_pos{};
    container_pos.x = (builder_button.transform_.pos_.x + builder_button.transform_.scale_.x / 2) +
                      (container_scale_.x / 2);

    container_pos.y = builder_button.transform_.pos_.y - (container_scale_.y / 2) +
                      (builder_button.transform_.scale_.y / 2);

    builder_container.transform_.pos_ = container_pos;

    builder_container.UpdateTransform();
}

void LevelManager::updateInnerButtonPosition() {
    // Update inner button position to follow the container
    AEVec2 containerPos = builder_container.transform_.pos_;
    AEVec2 containerScale = builder_container.transform_.scale_;
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

        buttonPool[i].transform_.scale_.x = buttonWidth;
        buttonPool[i].transform_.scale_.y = buttonHeight;
        buttonPool[i].transform_.pos_.x = x;
        buttonPool[i].transform_.pos_.y = y;

        if (buttonPool[i].graphics_.mesh_ == nullptr) {
            buttonPool[i].SetupMesh();
        }
        // If your SetupMesh needs to be re-run after transform changes, keep this.
        buttonPool[i].UpdateTransform();
    }
}

void LevelManager::updateLevelEditor() {
    if (!level_editor_mode_) {
        return;
    }

    // If Tab is pressed, toggle builder container
    if (AEInputCheckReleased(AEVK_TAB) || 0 == AESysDoesWindowExist()) {
        display_builder_container_ = !display_builder_container_;
        updateEditorButtonPosition();
        updateContainerPosition();
        updateInnerButtonPosition();
    }

    // Check if any button in the button pool is clicked
    for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
        if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
            if (buttonPool[i].OnClick()) {
                current_gameblock_ = static_cast<GameBlock>(i);
                std::cout << "Selected block: " << static_cast<int>(current_gameblock_) << "\n";
            }
        }
    }

    // Get scroll input for brush size adjustment
    s32 scroll_input{};
    AEInputMouseWheelDelta(&scroll_input);
    if (scroll_input > 0) {
        brush_radius_ += 5.0f; // Increase brush size
        if (brush_radius_ > 100.0f) {
            brush_radius_ = 100.0f; // Cap maximum brush size
        }
        std::cout << "Scroll up detected. Brush radius: " << brush_radius_ << "\n";
    } else if (scroll_input < 0) {
        brush_radius_ -= 5.0f; // Decrease brush size
        if (brush_radius_ < 20.0f) {
            brush_radius_ = 20.0f; // Cap minimum brush size
        }
        std::cout << "Scroll down detected. Brush radius: " << brush_radius_ << "\n";
    }
}

void LevelManager::renderLevelEditorUI() {

    if (!level_editor_mode_) {
        return;
    }

    // Render builder button
    builder_button.DrawColor(0.5f, 0.5f, 0.5f);

    // Render builder container and buttons within if display_builder_container_ is true
    if (display_builder_container_) {
        builder_container.DrawColor(1.f, 0.3f, 0.3f);
        // Render buttons in button pool
        for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
            buttonPool[i].DrawColor(0.0, 1.0f, 0.0f);
        }
    }
}

void LevelManager::freeLevelEditor() {
    builder_button.UnloadMesh();
    builder_container.UnloadMesh();
    AEGfxMeshFree(circleMesh);

    for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
        buttonPool[i].UnloadMesh();
    }
}

bool LevelManager::makeFilePath(int level) {
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

void LevelManager::saveMapInfo(int width, int height, int tilesize) {
    savingRoot["Map"]["width"] = width;
    savingRoot["Map"]["height"] = height;
    savingRoot["Map"]["tileSize"] = tilesize;
}
void LevelManager::saveTerrainInfo(std::vector<float> nodes, std::string terrainType) {
    Json::Value terrainJson(Json::arrayValue);
    for (size_t i = 0; i < nodes.size(); ++i) {
        terrainJson.append(nodes[i]);
    }
    savingRoot["Terrain"][terrainType] = terrainJson;
}
void LevelManager::saveStartEndInfo(std::vector<StartEnd> startPoints, StartEnd endPoint) {
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
    savingRoot["Objects"]["startPoints"] = startPointsJson_array;

    Json::Value endPointJson;
    endPointJson["posX"] = endPoint.transform_.pos_.x;
    endPointJson["posY"] = endPoint.transform_.pos_.y;
    endPointJson["scaleX"] = endPoint.transform_.scale_.x;
    endPointJson["scaleY"] = endPoint.transform_.scale_.y;
    endPointJson["rotation"] = endPoint.transform_.rotationRad_;
    endPointJson["type"] = static_cast<int>(endPoint.type_);
    endPointJson["direction"] = static_cast<int>(endPoint.direction_);
    savingRoot["Objects"]["endPoint"] = endPointJson;
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
        jsonWriter->write(savingRoot, &outfile);
        std::cout << "File saved successfully\n";
    } else {
        std::cout << "Failed to open file for writing\n";
    }

    // Clear root after writing to file to prevent accidental reuse
    savingRoot.clear();
}

bool LevelManager::getLevelData(int level) {
    // Clear previously read data to prevent accidental reuse
    readingRoot = Json::Value();

    std::string filePath = "Assets/Levels/Level_" + std::to_string(level) + "/Map.json";
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filePath << "\n";
        // Make directory and file if they don't exist
        if (makeFilePath(level) && makeLevelFile(level)) {
            std::cout << "Created missing directory and file for level " << level << "\n";
        } else {
            std::cout << "Failed to create missing directory and file for level " << level << "\n";
        }
        return false;
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;

    bool parsingSuccessful = Json::parseFromStream(builder, file, &readingRoot, &errs);
    if (!parsingSuccessful) {
        std::cout << "Failed to parse JSON: " << errs << "\n";
        return false;
    }

    if (readingRoot.isMember("None")) {
        std::cout << "File is empty\n";
        return false;
    }

    return true;
}

void LevelManager::checkLevelData() {
    // Loop through and check if level is playable by calling getLevelData for each
    for (int i = 1; i <= static_cast<int>(Level::None); ++i) {
        playableLevels[i - 1] = getLevelData(i);
        std::cout << "Level " << i << " playable: " << playableLevels[i - 1] << "\n";
    }
}

void LevelManager::parseMapInfo(int& width, int& height, int& tilesize) {
    if (readingRoot.isMember("Map")) {
        std::cout << "Parsing map info...\n";
        width = readingRoot["Map"]["width"].asInt();
        height = readingRoot["Map"]["height"].asInt();
        tilesize = readingRoot["Map"]["tileSize"].asInt();
    } else {
        std::cout << "Map info not found in JSON\n";
    }
}
void LevelManager::parseTerrainInfo(std::vector<float>& nodes, std::string terrainType) {
    if (!readingRoot.isMember("Terrain")) {
        std::cout << "Terrain info not found in JSON\n";
        return;
    }
    const Json::Value& terrain = readingRoot["Terrain"];
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

    if (readingRoot.isMember("Objects")) {
        const Json::Value& objects = readingRoot["Objects"];
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

void LevelManager::DrawBrushPreview(TerrainMaterial terrainType) {
    AEVec2 mousePos = GetMouseWorldPos();

    // Set up world matrix
    AEMtx33 scale_mtx, rot_mtx, trans_mtx, world_mtx;

    AEMtx33Scale(&scale_mtx, brush_radius_ * 2, brush_radius_ * 2);
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