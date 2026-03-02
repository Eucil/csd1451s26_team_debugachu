#pragma once

#include <AEEngine.h>

class GameStateManager;

void LoadLevelSelector();

void InitializeLevelSelector();

void UpdateLevelSelector(GameStateManager& GSM, f32 deltaTime);

void DrawLevelSelector();

void FreeLevelSelector();

void UnloadLevelSelector();