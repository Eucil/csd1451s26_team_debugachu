#pragma once
#include <AEEngine.h>

class GameStateManager;

void LoadCredits();
void InitializeCredits();
void UpdateCredits(GameStateManager& GSM, f32 deltaTime);
void DrawCredits();
void FreeCredits();
void UnloadCredits();