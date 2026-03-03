#pragma once

#include <AEEngine.h>

class GameStateManager;

void LoadLevel();

void InitializeLevel();

void UpdateLevel(GameStateManager& GSM, f32 deltaTime);

void DrawLevel();

void FreeLevel();

void UnloadLevel();