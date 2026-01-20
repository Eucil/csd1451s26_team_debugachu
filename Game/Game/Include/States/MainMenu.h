#pragma once

class GameStateManager;

void LoadMainMenu();

void InitializeMainMenu();

void UpdateMainMenu(GameStateManager& GSM, float deltaTime);

void DrawMainMenu();

void FreeMainMenu();

void UnloadMainMenu();