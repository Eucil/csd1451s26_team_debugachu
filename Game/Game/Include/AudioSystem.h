#pragma once

#include <string>
#include <unordered_map>

#include <AEEngine.h>

class AudioSystem {
public:
    // Unloads
    void unloadSound(const std::string& key) {
        AEAudioUnloadAudio(sounds_.at(key));
        sounds_.erase(key);
    }

    void unloadMusic(const std::string& key) {
        AEAudioUnloadAudio(music_.at(key));
        music_.erase(key);
    }

    void unloadGroup(const std::string& key) {
        AEAudioUnloadAudioGroup(groups_.at(key));
        groups_.erase(key);
        groupVolumes_.erase(key);
    }

    void unloadAllSounds() {
        for (auto& [key, audio] : sounds_) {
            AEAudioUnloadAudio(audio);
        }

        sounds_.clear();
    }

    void unloadAllMusic() {
        for (auto& [key, audio] : music_) {
            AEAudioUnloadAudio(audio);
        }

        music_.clear();
    }

    void unloadAllGroups() {
        for (auto& [key, group] : groups_) {
            AEAudioUnloadAudioGroup(group);
        }

        groups_.clear();
        groupVolumes_.clear();
    }

    // Groups
    void createGroup(const std::string& groupKey) {
        groups_[groupKey] = AEAudioCreateGroup();
        groupVolumes_[groupKey] = 100; // By default group volume is full (100%)
    }

    void pauseGroup(const std::string& groupKey) { AEAudioPauseGroup(groups_.at(groupKey)); }

    void resumeGroup(const std::string& groupKey) { AEAudioResumeGroup(groups_.at(groupKey)); }

    void stopGroup(const std::string& groupKey) { AEAudioStopGroup(groups_.at(groupKey)); }

    void setGroupVolume(const std::string& groupKey, f32 volume) {
        AEAudioSetGroupVolume(groups_.at(groupKey), volume);
    }

    void setGroupPitch(const std::string& groupKey, f32 pitch) {
        AEAudioSetGroupPitch(groups_.at(groupKey), pitch);
    }

    // Group Volume
    void adjustGroupVolume(const std::string& groupKey, f32 delta) {
        if (groups_.find(groupKey) == groups_.end())
            return;

        // Clamp volume to [0, 100]
        groupVolumes_[groupKey] = static_cast<s32>(AEClamp(static_cast<f32>(groupVolumes_[groupKey]) + delta, 0.0f, 100.0f));
        AEAudioSetGroupVolume(groups_.at(groupKey), groupVolumes_[groupKey] / 100.0f);
    }

    s32 getGroupVolume(const std::string& groupKey) const {
        auto it = groupVolumes_.find(groupKey);
        if (it == groupVolumes_.end()) {
            return 0;
        }

        return it->second;
    }

    // Load sound / music
    void loadSound(const std::string& key, const std::string& filepath) {
        sounds_[key] = AEAudioLoadSound(filepath.c_str());
    }

    void loadMusic(const std::string& key, const std::string& filepath) {
        music_[key] = AEAudioLoadMusic(filepath.c_str());
    }

    // Play sound / music
    void playSound(const std::string& audioKey, const std::string& groupKey, f32 volume = 1.0f,
                   f32 pitch = 1.0f, int loops = 0) {
        // Checks if audio exists
        if (sounds_.find(audioKey) == sounds_.end())
            return;
        // Checks if group exists
        if (groups_.find(groupKey) == groups_.end())
            return;
        AEAudioPlay(sounds_.at(audioKey), groups_.at(groupKey), volume, pitch, loops);
    }

    void playMusic(const std::string& audioKey, const std::string& groupKey, f32 volume = 1.0f,
                   f32 pitch = 1.0f, int loops = -1) {
        // Checks if music exists
        if (music_.find(audioKey) == music_.end())
            return;
        // Checks if group exists
        if (groups_.find(groupKey) == groups_.end())
            return;
        AEAudioPlay(music_.at(audioKey), groups_.at(groupKey), volume, pitch, loops);
    }

    AEAudioGroup getGroup(const std::string& groupKey) const { return groups_.at(groupKey); }

private:
    std::unordered_map<std::string, AEAudio> sounds_;
    std::unordered_map<std::string, AEAudio> music_;
    std::unordered_map<std::string, AEAudioGroup> groups_;

    // Store as s32 instead of f32 to avoid floating point inaccuracy
    std::unordered_map<std::string, s32> groupVolumes_;
};

extern AudioSystem g_audioSystem;
