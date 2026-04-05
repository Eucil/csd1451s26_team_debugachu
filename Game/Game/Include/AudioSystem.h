/*!
@file       AudioSystem.h
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This header file contains the declaration of the AudioSystem class,
            which wraps AEEngine audio functionality for sounds, music, and groups.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#pragma once

// Standard library
#include <string>
#include <unordered_map>

// Third-party
#include <AEEngine.h>

class AudioSystem {
public:
    // Unloads
    void unloadSound(const std::string& key);
    void unloadMusic(const std::string& key);
    void unloadGroup(const std::string& key);
    void unloadAllSounds();
    void unloadAllMusic();
    void unloadAllGroups();

    // Groups
    void createGroup(const std::string& groupKey);
    void pauseGroup(const std::string& groupKey);
    void resumeGroup(const std::string& groupKey);
    void stopGroup(const std::string& groupKey);
    void setGroupVolume(const std::string& groupKey, f32 volume);
    void setGroupPitch(const std::string& groupKey, f32 pitch);

    // Group Volume
    void adjustGroupVolume(const std::string& groupKey, f32 delta);
    s32  getGroupVolume(const std::string& groupKey) const;

    // Load sound / music
    void loadSound(const std::string& key, const std::string& filepath);
    void loadMusic(const std::string& key, const std::string& filepath);

    // Play sound / music
    void playSound(const std::string& audioKey, const std::string& groupKey, f32 volume = 1.0f,
                   f32 pitch = 1.0f, int loops = 0);
    void playMusic(const std::string& audioKey, const std::string& groupKey, f32 volume = 1.0f,
                   f32 pitch = 1.0f, int loops = -1);

    AEAudioGroup getGroup(const std::string& groupKey) const;

private:
    std::unordered_map<std::string, AEAudio> sounds_;
    std::unordered_map<std::string, AEAudio> music_;
    std::unordered_map<std::string, AEAudioGroup> groups_;

    // Store as s32 instead of f32 to avoid floating point inaccuracy
    std::unordered_map<std::string, s32> groupVolumes_;
};

extern AudioSystem g_audioSystem;
