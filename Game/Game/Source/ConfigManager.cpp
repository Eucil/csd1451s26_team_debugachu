/*!
@file       ConfigManager.cpp
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the definition of functions that make
            ConfigManager class, which scans a directory for JSON config files,
            parses them with JsonCpp, and exposes typed accessors for retrieving
            values by file, section, and key.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "ConfigManager.h"

// ==========================================
// Standard library
// ==========================================
#include <filesystem>
#include <fstream>
#include <iostream>

// ==========================================
// Global instance of ConfigManager
// ==========================================
ConfigManager g_configManager;

// =========================================================
//
// ConfigManager::init(const std::string& fileDir)
//
// - Scans fileDir for .json files and loads each one.
// - The filename without extension becomes the key in configs_.
// - e.g. "Level.json" is accessible as configs_["Level"].
//
// =========================================================
void ConfigManager::init(const std::string& fileDir) {

    // Check if directory exists
    if (!std::filesystem::exists(fileDir)) {
        std::cerr << "Directory not found: " << fileDir << "\n";
        return;
    }

    // Automatically load every .json file found in the directory
    for (const auto& fileInput : std::filesystem::directory_iterator(fileDir)) {
        // Skip non-json files
        if (fileInput.path().extension() != ".json")
            continue;

        // Extract filename without extension to use as key in configs_ map
        std::string fileName =
            fileInput.path().stem().string(); // e.g. "terrain" from "terrain.json"
        loadFile(fileInput.path().string(), fileName);
    }
}

// =========================================================
//
// ConfigManager::loadFile(const std::string& filePath, const std::string& fileName)
//
// - Opens filePath, parses it as JSON, and stores the result
// - in configs_ under fileName.
// - Returns false if the file cannot be opened or parsed.
//
// =========================================================
bool ConfigManager::loadFile(const std::string& filePath, const std::string& fileName) {
    std::ifstream file(filePath);
    // Check if file can be opened
    if (!file.is_open()) {
        std::cerr << "ConfigManager could not open: " << filePath << "\n";
        return false;
    }

    Json::Value readingRoot;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errors;

    // Parse JSON from file into readingRoot
    if (!Json::parseFromStream(builder, file, &readingRoot, &errors)) {
        std::cerr << "ConfigManager parse error in " << fileName << ": " << errors << "\n";
        return false;
    }

    // Store the reading root in the configs_ map with the fileName as key
    configs_[fileName] = readingRoot;
    std::cout << "ConfigManager loaded: " << fileName << ".json\n";
    return true;
}

// =========================================================
//
// ConfigManager::hasFile(const std::string& file)
//
// - Returns true if a config with the given file key has been loaded.
// - Else returns false.
//
// =========================================================
bool ConfigManager::hasFile(const std::string& file) const {
    // Check if this file key exists
    if (configs_.count(file) == 0) {
        return false;
    }
    return true;
}

// =========================================================
//
// ConfigManager::hasSection(const std::string& file, const std::string& section)
//
// - Returns true if the given section exists within the loaded file.
// - Else returns false.
//
// =========================================================
bool ConfigManager::hasSection(const std::string& file, const std::string& section) const {
    if (hasFile(file)) {
        // Check if this section key exists within the file's root
        if (configs_.at(file).isMember(section)) {
            return true;
        }
    }
    return false;
}

// =========================================================
//
// ConfigManager::hasKey(const std::string& file, const std::string& section,
//                       const std::string& key)
//
// - Returns true if the given key exists within file -> section.
// - Else returns false.
//
// =========================================================
bool ConfigManager::hasKey(const std::string& file, const std::string& section,
                           const std::string& key) const {
    if (hasSection(file, section)) {
        // Check if this key exists within the section
        if (configs_.at(file)[section].isMember(key)) {
            return true;
        }
    }
    return false;
}

// =========================================================
//
// ConfigManager::getValueRoot(const std::string& file, const std::string& section,
//                             const std::string& key)
//
// - Internal helper that navigates to file -> section -> key
// - and returns a pointer to the Json::Value, or nullptr if not found.
//
// =========================================================
const Json::Value* ConfigManager::getValueRoot(const std::string& file, const std::string& section,
                                               const std::string& key) const {
    // If key can't be found, return nullptr
    if (!hasKey(file, section, key)) {
        return nullptr;
    }

    const Json::Value& root = configs_.at(file)[section][key];
    return &root;
}

// =========================================================
//
// ConfigManager::getArrayRoot(const std::string& file, const std::string& section,
//                             const std::string& key)
//
// - Same as getValueRoot but additionally validates that the value
// - is a JSON array. Returns nullptr if not found or not an array.
//
// =========================================================
const Json::Value* ConfigManager::getArrayRoot(const std::string& file, const std::string& section,
                                               const std::string& key) const {
    // If key can't be found, return nullptr
    if (!hasKey(file, section, key)) {
        return nullptr;
    }

    const Json::Value& root = configs_.at(file)[section][key];

    // Check if root is an array
    if (!root.isArray()) {
        return nullptr;
    }
    return &root;
}

// =========================================================
//
// ConfigManager::getInt(const std::string& file, const std::string& section,
//                       const std::string& key, int defaultVal)
//
// - Returns the int value at file -> section -> key.
// - Prints a warning and returns defaultVal if not found.
//
// =========================================================
int ConfigManager::getInt(const std::string& file, const std::string& section,
                          const std::string& key, int defaultVal) const {
    const Json::Value* root = getValueRoot(file, section, key);
    if (root == nullptr) {
        std::cout << "ConfigManager: getInt failed to find " << file << "." << section << "." << key
                  << ", returning default value: " << defaultVal << "\n";
        return defaultVal;
    }
    return root->asInt();
}

// =========================================================
//
// ConfigManager::getFloat(const std::string& file, const std::string& section,
//                         const std::string& key, float defaultVal)
//
// - Returns the float value at file -> section -> key.
// - Prints a warning and returns defaultVal if not found.
//
// =========================================================
float ConfigManager::getFloat(const std::string& file, const std::string& section,
                              const std::string& key, float defaultVal) const {
    const Json::Value* root = getValueRoot(file, section, key);
    if (root == nullptr) {
        std::cout << "ConfigManager: getFloat failed to find " << file << "." << section << "."
                  << key << ", returning default value: " << defaultVal << "\n";
        return defaultVal;
    }
    return root->asFloat();
}

// =========================================================
//
// ConfigManager::getBool(const std::string& file, const std::string& section,
//                        const std::string& key, bool defaultVal)
//
// - Returns the bool value at file -> section -> key.
// - Prints a warning and returns defaultVal if not found.
//
// =========================================================
bool ConfigManager::getBool(const std::string& file, const std::string& section,
                            const std::string& key, bool defaultVal) const {
    const Json::Value* root = getValueRoot(file, section, key);
    if (root == nullptr) {
        std::cout << "ConfigManager: getBool failed to find " << file << "." << section << "."
                  << key << ", returning default value: " << defaultVal << "\n";
        return defaultVal;
    }
    return root->asBool();
}

// =========================================================
//
// ConfigManager::getString(const std::string& file, const std::string& section,
//                          const std::string& key, const std::string& defaultVal)
//
// - Returns the string value at file -> section -> key.
// - Prints a warning and returns defaultVal if not found.
//
// =========================================================
std::string ConfigManager::getString(const std::string& file, const std::string& section,
                                     const std::string& key, const std::string& defaultVal) const {
    const Json::Value* root = getValueRoot(file, section, key);
    if (root == nullptr) {
        std::cout << "ConfigManager: getString failed to find " << file << "." << section << "."
                  << key << ", returning default value: " << defaultVal << "\n";
        return defaultVal;
    }
    return root->asString();
}

// =========================================================
//
// ConfigManager::getAEVec2(const std::string& file, const std::string& section,
//                          const std::string& key, const AEVec2& defaultVal)
//
// - Returns the AEVec2 value at file -> section -> key,
// - parsed from a two-element JSON array [x, y].
// - Prints a warning and returns defaultVal if not found or wrong size.
//
// =========================================================
AEVec2 ConfigManager::getAEVec2(const std::string& file, const std::string& section,
                                const std::string& key, const AEVec2& defaultVal) const {
    const Json::Value* arrayRoot = getArrayRoot(file, section, key);
    if (arrayRoot == nullptr || arrayRoot->size() != 2) {
        std::cout << "ConfigManager: getAEVec2 failed to find " << file << "." << section << "."
                  << key << " or array size is not 2, returning default value: (" << defaultVal.x
                  << ", " << defaultVal.y << ")\n";
        return defaultVal;
    }
    AEVec2 out{};
    out.x = (*arrayRoot)[0].asFloat();
    out.y = (*arrayRoot)[1].asFloat();
    return out;
}

// =========================================================
//
// ConfigManager::getFloatArray(const std::string& file, const std::string& section,
//                              const std::string& key, std::vector<float>& out)
//
// - Fills out with all float values from the JSON array
// - at file -> section -> key. Does nothing if not found.
//
// =========================================================
void ConfigManager::getFloatArray(const std::string& file, const std::string& section,
                                  const std::string& key, std::vector<float>& out) const {
    const Json::Value* arrayRoot = getArrayRoot(file, section, key);
    if (arrayRoot == nullptr) {
        return;
    }

    Json::ArrayIndex count = arrayRoot->size();
    for (Json::ArrayIndex i = 0; i < count; ++i) {
        out[i] = (*arrayRoot)[i].asFloat();
    }
}

// =========================================================
//
// ConfigManager::getSection(const std::string& file, const std::string& section)
//
// - Returns the entire section as a Json::Value.
// - Returns an empty Json::Value and prints a warning if not found.
//
// =========================================================
Json::Value ConfigManager::getSection(const std::string& file, const std::string& section) const {
    if (!hasSection(file, section)) {
        std::cout << "ConfigManager: getSection failed to find " << file << "." << section
                  << ", returning empty Json::Value\n";
        return Json::Value();
    }
    return configs_.at(file)[section];
}