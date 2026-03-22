#include "States/Settings.h"

#include <AEEngine.h>

#include "AudioSystem.h"
#include "Button.h"
#include "GameStateManager.h"
#include "States/LevelManager.h"

static Button buttonIncreaseSfxVolume;
static Button buttonDecreaseSfxVolume;
static Button buttonIncreaseBgmVolume;
static Button buttonDecreaseBgmVolume;
static Button buttonBack;

static s8 font;

static TextData headerText;
static TextData sfxVolumeText;
static TextData bgmVolumeText;
static TextData sfxVolumeAmountText;
static TextData bgmVolumeAmountText;

void loadSettings() {
    font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

    buttonIncreaseSfxVolume.loadMesh();
    buttonIncreaseSfxVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseSfxVolume.loadMesh();
    buttonDecreaseSfxVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonIncreaseBgmVolume.loadMesh();
    buttonIncreaseBgmVolume.loadTexture("Assets/Textures/brown_square_24_24.png");
    buttonDecreaseBgmVolume.loadMesh();
    buttonDecreaseBgmVolume.loadTexture("Assets/Textures/brown_square_24_24.png");

    buttonBack.loadMesh();
    buttonBack.loadTexture("Assets/Textures/brown_square_24_24.png");
}

void initializeSettings() {
    buttonIncreaseSfxVolume.initFromJson("settings_buttons", "IncreaseSfxVolume");
    buttonIncreaseSfxVolume.setTextFont(font);
    buttonDecreaseSfxVolume.initFromJson("settings_buttons", "DecreaseSfxVolume");
    buttonDecreaseSfxVolume.setTextFont(font);
    buttonIncreaseBgmVolume.initFromJson("settings_buttons", "IncreaseBgmVolume");
    buttonIncreaseBgmVolume.setTextFont(font);
    buttonDecreaseBgmVolume.initFromJson("settings_buttons", "DecreaseBgmVolume");
    buttonDecreaseBgmVolume.setTextFont(font);

    buttonBack.initFromJson("settings_buttons", "Back");
    buttonBack.setTextFont(font);

    headerText.initFromJson("settings_text", "Header");
    headerText.font_ = font;
    sfxVolumeText.initFromJson("settings_text", "SfxVolume");
    sfxVolumeText.font_ = font;
    bgmVolumeText.initFromJson("settings_text", "BgmVolume");
    bgmVolumeText.font_ = font;
    sfxVolumeAmountText.initFromJson("settings_text", "SfxVolumeAmount");
    sfxVolumeAmountText.font_ = font;
    bgmVolumeAmountText.initFromJson("settings_text", "BgmVolumeAmount");
    bgmVolumeAmountText.font_ = font;
}

void updateSettings(GameStateManager& GSM, f32 deltaTime) {
    // Check for button presses to change audio group
    if (buttonIncreaseSfxVolume.checkMouseClick()) {
        g_audioSystem.adjustGroupVolume("sfx", +10);
    }
    if (buttonDecreaseSfxVolume.checkMouseClick()) {
        g_audioSystem.adjustGroupVolume("sfx", -10);
    }
    if (buttonIncreaseBgmVolume.checkMouseClick()) {
        g_audioSystem.adjustGroupVolume("bgm", +10);
    }
    if (buttonDecreaseBgmVolume.checkMouseClick()) {
        g_audioSystem.adjustGroupVolume("bgm", -10);
    }

    // Check for button press to go back to main menu
    if (buttonBack.checkMouseClick()) {
        GSM.nextState_ = StateId::MainMenu;
    }

    // Get volume amounts
    sfxVolumeAmountText.content_ = std::to_string(g_audioSystem.getGroupVolume("sfx")) + "%";
    bgmVolumeAmountText.content_ = std::to_string(g_audioSystem.getGroupVolume("bgm")) + "%";

    buttonIncreaseSfxVolume.updateTransform();
    buttonDecreaseSfxVolume.updateTransform();
    buttonIncreaseBgmVolume.updateTransform();
    buttonDecreaseBgmVolume.updateTransform();

    buttonBack.updateTransform();
}

void drawSettings() {
    buttonIncreaseSfxVolume.draw();
    buttonDecreaseSfxVolume.draw();
    buttonIncreaseBgmVolume.draw();
    buttonDecreaseBgmVolume.draw();

    buttonBack.draw();

    headerText.draw();
    sfxVolumeText.draw();
    bgmVolumeText.draw();
    sfxVolumeAmountText.draw();
    bgmVolumeAmountText.draw();
}

void freeSettings() {}

void unloadSettings() {
    AEGfxDestroyFont(font);

    buttonIncreaseSfxVolume.unload();
    buttonDecreaseSfxVolume.unload();
    buttonIncreaseBgmVolume.unload();
    buttonDecreaseBgmVolume.unload();

    buttonBack.unload();
}