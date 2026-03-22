#include <iostream>

#include <AEEngine.h>
#include <crtdbg.h>

#include "AudioSystem.h"
#include "ConfigManager.h"
#include "GameStateManager.h"
#include "States/LevelManager.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, nullptr);

    AESysSetWindowTitle("Water The Plant");

    AESysReset();

    // Initalise config manager
    g_configManager.init("Assets/GameData/FileConfigs");
    g_configManager.init("Assets/GameData/UI");

    GameStateManager GSM;
    GSM.init(StateId::MainMenu);

    // Audio system
    g_audioSystem.createGroup("sfx");
    g_audioSystem.createGroup("bgm");

    g_audioSystem.loadSound("faucet_squeak", "Assets/Audio/faucet_squeak.mp3");
    g_audioSystem.loadSound("wormhole_place", "Assets/Audio/wormhole_place.mp3");
    g_audioSystem.loadSound("dirt_break", "Assets/Audio/dirt_break.mp3");
    g_audioSystem.loadSound("ding", "Assets/Audio/ding.mp3");
    g_audioSystem.loadSound("click", "Assets/Audio/click.mp3");
    g_audioSystem.loadSound("pop", "Assets/Audio/pop.mp3");

    // Game Loop
    while (GSM.currentState_ != StateId::Quit) {
        if (GSM.currentState_ == StateId::Restart) {
            GSM.currentState_ = GSM.previousState_;
            GSM.nextState_ = GSM.previousState_;
        } else {
            GSM.update(GSM.nextState_);

            GSM.callLoad();
        }

        GSM.callInitialize();

        while (GSM.nextState_ == GSM.currentState_) {
            AESysFrameStart();

            f32 deltaTime = (f32)AEFrameRateControllerGetFrameTime();

            // Click ESCAPE to exit game
            if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist()) {
                std::cout << "ESCAPE triggered\n";
                GSM.nextState_ = StateId::Quit;
            }

            GSM.callUpdate(deltaTime);
            GSM.callDraw();

            AESysFrameEnd();
        }

        GSM.callFree();

        if (GSM.nextState_ != StateId::Restart) {
            GSM.callUnload();
        }

        GSM.previousState_ = GSM.currentState_;
        GSM.currentState_ = GSM.nextState_;
    }
    // Audio system
    g_audioSystem.stopGroup("sfx");
    g_audioSystem.stopGroup("bgm");
    g_audioSystem.unloadAllSounds();
    g_audioSystem.unloadAllMusic();
    g_audioSystem.unloadAllGroups();

    AESysExit();
}