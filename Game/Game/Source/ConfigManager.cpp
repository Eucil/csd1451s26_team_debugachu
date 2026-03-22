#include "ConfigManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>

ConfigManager g_configManager;

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

bool ConfigManager::hasFile(const std::string& file) const {
    // Check if this file key exists
    if (configs_.count(file) == 0) {
        return false;
    }
    return true;
}

bool ConfigManager::hasSection(const std::string& file, const std::string& section) const {
    if (hasFile(file)) {
        // Check if this section key exists within the file's root
        if (configs_.at(file).isMember(section)) {
            return true;
        }
    }
    return false;
}

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

const Json::Value* ConfigManager::getValueRoot(const std::string& file, const std::string& section,
                                               const std::string& key) const {
    // If key can't be found, return nullptr
    if (!hasKey(file, section, key)) {
        return nullptr;
    }

    const Json::Value& root = configs_.at(file)[section][key];
    return &root;
}

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

AEVec2 ConfigManager::getAEVec2(const std::string& file, const std::string& section,
                                const std::string& key, const AEVec2& defaultVal) const {
    const Json::Value* arrayRoot = getArrayRoot(file, section, key);
    if (arrayRoot == nullptr || arrayRoot->size() != 2) {
        std::cout << "ConfigManager: getAEVec2 failed to find " << file << "." << section << "."
                  << key << " or array size is not 2, returning default value: (" << defaultVal.x
                  << ", " << defaultVal.y << ")\n";
        return defaultVal;
    }
    AEVec2 out;
    out.x = (*arrayRoot)[0].asFloat();
    out.y = (*arrayRoot)[1].asFloat();
    return out;
}

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

Json::Value ConfigManager::getSection(const std::string& file, const std::string& section) const {
    if (!hasSection(file, section)) {
        std::cout << "ConfigManager: getSection failed to find " << file << "." << section
                  << ", returning empty Json::Value\n";
        return Json::Value();
    }
    return configs_.at(file)[section];
}