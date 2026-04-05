/*!
@file       AudioSystem.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the definitions of the AudioSystem class,
            which wraps AEEngine audio functionality for sounds, music, and groups.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "AudioSystem.h"

// Standard library
#include <string>
#include <unordered_map>

// Third-party
#include <AEEngine.h>

AudioSystem g_audioSystem;

// =========================================================
//
// unloadSound
//
// Unloads a single sound asset from memory and removes
// its entry from the sounds map.
//
// =========================================================
void AudioSystem::unloadSound(const std::string& key) {
    AEAudioUnloadAudio(sounds_.at(key));
    sounds_.erase(key);
}

// =========================================================
//
// unloadMusic
//
// Unloads a single music asset from memory and removes
// its entry from the music map.
//
// =========================================================
void AudioSystem::unloadMusic(const std::string& key) {
    AEAudioUnloadAudio(music_.at(key));
    music_.erase(key);
}

// =========================================================
//
// unloadGroup
//
// Unloads an audio group and removes its entries from
// the groups and group volumes maps.
//
// =========================================================
void AudioSystem::unloadGroup(const std::string& key) {
    AEAudioUnloadAudioGroup(groups_.at(key));
    groups_.erase(key);
    groupVolumes_.erase(key);
}

// =========================================================
//
// unloadAllSounds
//
// Unloads every sound asset currently held and clears
// the sounds map.
//
// =========================================================
void AudioSystem::unloadAllSounds() {
    for (auto& [key, audio] : sounds_) {
        AEAudioUnloadAudio(audio);
    }
    sounds_.clear();
}

// =========================================================
//
// unloadAllMusic
//
// Unloads every music asset currently held and clears
// the music map.
//
// =========================================================
void AudioSystem::unloadAllMusic() {
    for (auto& [key, audio] : music_) {
        AEAudioUnloadAudio(audio);
    }
    music_.clear();
}

// =========================================================
//
// unloadAllGroups
//
// Unloads every audio group currently held and clears
// both the groups and group volumes maps.
//
// =========================================================
void AudioSystem::unloadAllGroups() {
    for (auto& [key, group] : groups_) {
        AEAudioUnloadAudioGroup(group);
    }
    groups_.clear();
    groupVolumes_.clear();
}

// =========================================================
//
// createGroup
//
// Creates a new audio group and registers it under the
// given key. Default volume is set to 50%.
//
// =========================================================
void AudioSystem::createGroup(const std::string& groupKey) {
    groups_[groupKey] = AEAudioCreateGroup();
    groupVolumes_[groupKey] = 50;
}

// =========================================================
//
// pauseGroup
//
// Pauses all audio currently playing in the given group.
//
// =========================================================
void AudioSystem::pauseGroup(const std::string& groupKey) {
    AEAudioPauseGroup(groups_.at(groupKey));
}

// =========================================================
//
// resumeGroup
//
// Resumes all paused audio in the given group.
//
// =========================================================
void AudioSystem::resumeGroup(const std::string& groupKey) {
    AEAudioResumeGroup(groups_.at(groupKey));
}

// =========================================================
//
// stopGroup
//
// Stops all audio playing in the given group.
//
// =========================================================
void AudioSystem::stopGroup(const std::string& groupKey) {
    AEAudioStopGroup(groups_.at(groupKey));
}

// =========================================================
//
// setGroupVolume
//
// Sets the volume of the given group to the specified value.
//
// =========================================================
void AudioSystem::setGroupVolume(const std::string& groupKey, f32 volume) {
    AEAudioSetGroupVolume(groups_.at(groupKey), volume);
}

// =========================================================
//
// setGroupPitch
//
// Sets the pitch of the given group to the specified value.
//
// =========================================================
void AudioSystem::setGroupPitch(const std::string& groupKey, f32 pitch) {
    AEAudioSetGroupPitch(groups_.at(groupKey), pitch);
}

// =========================================================
//
// adjustGroupVolume
//
// Adjusts the volume of the given group by a delta, clamped
// to [0, 100]. Stored as an integer to avoid floating point
// inaccuracy, then normalised to [0.0, 1.0] for AEEngine.
//
// =========================================================
void AudioSystem::adjustGroupVolume(const std::string& groupKey, f32 delta) {
    if (groups_.find(groupKey) == groups_.end())
        return;

    groupVolumes_[groupKey] = static_cast<s32>(
        AEClamp(static_cast<f32>(groupVolumes_[groupKey]) + delta, 0.0f, 100.0f));
    AEAudioSetGroupVolume(groups_.at(groupKey), groupVolumes_[groupKey] / 100.0f);
}

// =========================================================
//
// getGroupVolume
//
// Returns the current volume of the given group as an
// integer in [0, 100]. Returns 0 if the group is not found.
//
// =========================================================
s32 AudioSystem::getGroupVolume(const std::string& groupKey) const {
    auto it = groupVolumes_.find(groupKey);
    if (it == groupVolumes_.end())
        return 0;
    return it->second;
}

// =========================================================
//
// loadSound
//
// Loads a sound asset from the given filepath and stores
// it under the given key.
//
// =========================================================
void AudioSystem::loadSound(const std::string& key, const std::string& filepath) {
    sounds_[key] = AEAudioLoadSound(filepath.c_str());
}

// =========================================================
//
// loadMusic
//
// Loads a music asset from the given filepath and stores
// it under the given key.
//
// =========================================================
void AudioSystem::loadMusic(const std::string& key, const std::string& filepath) {
    music_[key] = AEAudioLoadMusic(filepath.c_str());
}

// =========================================================
//
// playSound
//
// Plays the sound registered under audioKey through the
// group registered under groupKey. No-ops silently if
// either key is not found.
//
// =========================================================
void AudioSystem::playSound(const std::string& audioKey, const std::string& groupKey,
                            f32 volume, f32 pitch, int loops) {
    if (sounds_.find(audioKey) == sounds_.end())
        return;
    if (groups_.find(groupKey) == groups_.end())
        return;
    AEAudioPlay(sounds_.at(audioKey), groups_.at(groupKey), volume, pitch, loops);
}

// =========================================================
//
// playMusic
//
// Plays the music registered under audioKey through the
// group registered under groupKey. Loops indefinitely by
// default. No-ops silently if either key is not found.
//
// =========================================================
void AudioSystem::playMusic(const std::string& audioKey, const std::string& groupKey,
                            f32 volume, f32 pitch, int loops) {
    if (music_.find(audioKey) == music_.end())
        return;
    if (groups_.find(groupKey) == groups_.end())
        return;
    AEAudioPlay(music_.at(audioKey), groups_.at(groupKey), volume, pitch, loops);
}

// =========================================================
//
// getGroup
//
// Returns the AEAudioGroup handle registered under the
// given key.
//
// =========================================================
AEAudioGroup AudioSystem::getGroup(const std::string& groupKey) const {
    return groups_.at(groupKey);
}
