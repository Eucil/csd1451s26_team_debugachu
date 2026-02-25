#pragma once

#include <AEEngine.h>

class GameStateManager;

void LoadLevel3();

void InitializeLevel3();

void UpdateLevel3(GameStateManager& GSM, f32 deltaTime);

void DrawLevel3();

void FreeLevel3();

void UnloadLevel3();