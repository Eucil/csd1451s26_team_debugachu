#include "States/LevelManager.h"
#include <iostream>

LevelManager levelManager;

void LevelManager::init() {
    level_editor_mode_ = true;
    build_mode_ = true;
    current_level_ = Levels::Level_0;
    current_gameblock_ = GameBlock::None;

    builder_button = Button(AEVec2{775.0f, 350.0f}, AEVec2{50.0f, 50.0f});
    builder_container = Button(AEVec2{0.0f, 0.0f}, container_scale_);
    // Set container position relative to button
    updateContainerPosition();
    // Set up meshes for button and container
    builder_button.SetupMesh();
    builder_container.SetupMesh();

    buttonPool.resize(static_cast<int>(GameBlock::None));
    updateInnerButtonPosition();
}

bool LevelManager::getLevelEditorMode() const { return level_editor_mode_; }

void LevelManager::setLevelEditorMode(bool mode) { level_editor_mode_ = mode; }

Levels LevelManager::getCurrentLevel() const { return current_level_; }

void LevelManager::SetCurrentLevel(Levels level) { current_level_ = level; }

bool LevelManager::getBuildMode() const { return build_mode_; }

void LevelManager::setBuildMode(bool mode) { build_mode_ = mode; }

GameBlock LevelManager::getCurrentGameBlock() const { return current_gameblock_; }

void LevelManager::setCurrentGameBlock(GameBlock block) { current_gameblock_ = block; }

void LevelManager::updateEditorButtonPosition() {
    // Update builder button position
    // If builder container is displayed, move button based on container scale
    if (display_builder_container_) {
        builder_button.transform_.pos_.x = builder_button.transform_.pos_.x - (container_scale_.x);
    } else {
        builder_button.transform_.pos_.x = 775.0f;
    }

    // std::cout << "Button pos: (" << builder_button.transform_.pos_.x << ", "
    //           << builder_button.transform_.pos_.y << ")\n";

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

    // std::cout << "Container pos: (" << container_pos.x << ", " << container_pos.y << ")\n";

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

    // If builder button is clicked, toggle builder container
    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
        if (builder_button.OnClick()) {
            display_builder_container_ = !display_builder_container_;
            updateEditorButtonPosition();
            updateContainerPosition();
            updateInnerButtonPosition();
        }
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
    for (int i = 0; i < static_cast<int>(GameBlock::None); ++i) {
        buttonPool[i].UnloadMesh();
    }
}