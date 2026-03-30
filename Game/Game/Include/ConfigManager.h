#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <AEEngine.h>
#include <json/json.h>

class ConfigManager {
public:
    void init(const std::string& fileDir);

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
    void getFloatArray(const std::string& file, const std::string& section, const std::string& key,
                       std::vector<float>& out) const;
    Json::Value getSection(const std::string& file, const std::string& section) const;

    bool hasFile(const std::string& file) const;
    bool hasSection(const std::string& file, const std::string& section) const;
    bool hasKey(const std::string& file, const std::string& section, const std::string& key) const;

    void cleanUp() { configs_.clear(); }

private:
    bool loadFile(const std::string& filePath, const std::string& fileName);
    const Json::Value* getValueRoot(const std::string& file, const std::string& section,
                                    const std::string& key) const;
    const Json::Value* getArrayRoot(const std::string& file, const std::string& section,
                                    const std::string& key) const;

    // Each file gets its own parsed root, keyed by filename without extension
    // e.g. "terrain", "audio", "levelEditor"
    std::unordered_map<std::string, Json::Value> configs_;
};

extern ConfigManager g_configManager;