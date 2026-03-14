#include "Pause.h"

void PauseSystem::pause() { pause_ = true; }

void PauseSystem::resume() { pause_ = false; }

bool PauseSystem::isPaused() const { return pause_; }