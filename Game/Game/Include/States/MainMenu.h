#pragma once

#include <AEEngine.h>

class GameStateManager;

void LoadMainMenu();

void InitializeMainMenu();

void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime);

void DrawMainMenu();

void FreeMainMenu();

void UnloadMainMenu();