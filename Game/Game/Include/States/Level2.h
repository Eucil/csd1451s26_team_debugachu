#pragma once

#include <AEEngine.h>

class GameStateManager;

void LoadLevel2();

void InitializeLevel2();

void UpdateLevel2(GameStateManager& GSM, f32 deltaTime);

void DrawLevel2();

void FreeLevel2();

void UnloadLevel2();
