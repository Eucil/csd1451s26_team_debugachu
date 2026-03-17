#pragma once

#include <AEEngine.h>

class GameStateManager;

void loadSettings();

void initializeSettings();

void updateSettings(GameStateManager& GSM, f32 deltaTime);

void drawSettings();

void freeSettings();

void unloadSettings();