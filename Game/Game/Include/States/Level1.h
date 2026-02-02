#pragma once

#include <AEEngine.h>

class GameStateManager;

void LoadLevel1();

void InitializeLevel1();

void UpdateLevel1(GameStateManager& GSM, f32 deltaTime);

void DrawLevel1();

void FreeLevel1();

void UnloadLevel1();