/*!
@file       ConfigManager.h
@author     Woo Guang Theng/guangtheng.woo@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declarations of the ConfigManager class,
            which loads JSON config files from a directory and provides typed accessors
            for retrieving values by file, section, and key.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// ==========================================
// Standard library
// ==========================================
#include <string>
#include <unordered_map>
#include <vector>

// ==========================================
// Third-party
// ==========================================
#include <AEEngine.h>
#include <json/json.h>

class ConfigManager {
public:
    // Scans the given directory and loads every .json file found.
    // Each file is keyed by its name without extension (e.g. "Level", "LevelSelector").
    void init(const std::string& fileDir);

    // ==========================================
    // Typed Value Accessors
    // ==========================================
    // Each accessor looks up file -> section -> key and returns the typed value.
    // If the key is not found, defaultVal is returned and a warning is printed.
    // ==========================================
    int getInt(const std::string& file, const std::string& section, const std::string& key,
               int defaultVal = 0) const;

    float getFloat(const std::string& file, const std::string& section, const std::string& key,
                   float defaultVal = 0.0f) const;

    bool getBool(const std::string& file, const std::string& section, const std::string& key,
                 bool defaultVal = false) const;

    std::string getString(const std::string& file, const std::string& section,
                          const std::string& key, const std::string& defaultVal = "") const;

    AEVec2 getAEVec2(const std::string& file, const std::string& section, const std::string& key,
                     const AEVec2& defaultVal = {}) const;

    // Fills out with all floats from a JSON array at file -> section -> key.
    void getFloatArray(const std::string& file, const std::string& section, const std::string& key,
                       std::vector<float>& out) const;

    // Returns the entire section as a Json::Value (empty value if not found).
    Json::Value getSection(const std::string& file, const std::string& section) const;

    // ==========================================
    // Checks for existence of files, sections, and keys
    // ==========================================
    bool hasFile(const std::string& file) const;
    bool hasSection(const std::string& file, const std::string& section) const;
    bool hasKey(const std::string& file, const std::string& section, const std::string& key) const;

    // Clears all loaded config data
    void cleanUp() { configs_.clear(); }

private:
    // Parses a single JSON file and stores it under fileName as the map key.
    bool loadFile(const std::string& filePath, const std::string& fileName);

    // Returns a pointer to the Json::Value at file -> section -> key, or nullptr if not found.
    const Json::Value* getValueRoot(const std::string& file, const std::string& section,
                                    const std::string& key) const;

    // Same as getValueRoot but also validates that the value is a JSON array.
    const Json::Value* getArrayRoot(const std::string& file, const std::string& section,
                                    const std::string& key) const;

    // Each file gets its own parsed root, keyed by filename without extension
    // e.g. "terrain", "audio", "levelEditor"
    std::unordered_map<std::string, Json::Value> configs_;
};

extern ConfigManager g_configManager;