#pragma once

class PauseSystem {
public:
    void pause();
    void resume();

    bool isPaused() const;

private:
    bool pause_{false}; // true means paused, false mean not paused
};
