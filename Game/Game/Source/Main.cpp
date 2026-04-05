/*!
@file       Main.cpp
@author     Sean Lee Hong Wei/seanhongwei.lee@digipen.edu
@co_author  Woo Guang Theng/guangtheng.woo@digipen.edu,

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
// Standard library
#include <iostream>

// Third-party
#include <AEEngine.h>
#include <crtdbg.h>

// Project
#include "AudioSystem.h"
#include "Button.h"
#include "ConfigManager.h"
#include "DebugSystem.h"
#include "GameStateManager.h"
#include "LevelManager.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 0, 60, false, nullptr);

    AESysReset();

    AESysSetWindowTitle("Water The Plant");
    AESysSetWindowIcon("Assets/Textures/pink_flower_end.ico", 16, 16);

    // Initalise config manager
    g_configManager.init("Assets/GameData/FileConfigs");
    g_configManager.init("Assets/GameData/UI");
    Button::loadConfigFromJson("button_config", "Settings");

    GameStateManager GSM{};
    GSM.init(StateId::LogoScreen);

    // Audio system
    g_audioSystem.createGroup("sfx");
    g_audioSystem.createGroup("bgm");
    g_audioSystem.setGroupVolume("sfx", 0.5f);
    g_audioSystem.setGroupVolume("bgm", 0.5f);

    g_audioSystem.loadSound("wormhole_place", "Assets/Audio/wormhole_place.mp3");
    g_audioSystem.loadSound("dirt_break", "Assets/Audio/dirt_break.mp3");
    g_audioSystem.loadSound("click", "Assets/Audio/click.mp3");
    g_audioSystem.loadSound("hover", "Assets/Audio/button_hover2.ogg");
    g_audioSystem.loadMusic("main_music",
                            "Assets/Audio/A SINGLE ROSE - EasyListeningDow MSCLES1_17.mp3");
    g_audioSystem.loadSound("bell", "Assets/Audio/BELL-DESK_GEN-HDF-03247.mp3");
    g_audioSystem.loadSound("drip_water", "Assets/Audio/DripWater CTE01_55.4.mp3");
    g_audioSystem.loadSound("crank", "Assets/Audio/GENERATOR-SMALL_GEN-HDF-12831.mp3");
    g_audioSystem.loadSound("magic", "Assets/Audio/MagicCartoon CTE01_93.4.mp3");

    g_audioSystem.playMusic("main_music", "bgm", 1.0f, 1.0f);

    // Debug system
    s8 debugFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);
    g_debugSystem.load(debugFont);
    g_debugSystem.initFromJson("debug_system", "DebugOverlay");

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
    // Debug system
    g_debugSystem.unload();
    if (debugFont) {
        AEGfxDestroyFont(debugFont);
    }

    // Audio system
    g_audioSystem.stopGroup("sfx");
    g_audioSystem.stopGroup("bgm");
    g_audioSystem.unloadAllSounds();
    g_audioSystem.unloadAllMusic();
    g_audioSystem.unloadAllGroups();

    g_configManager.cleanUp();

    AESysExit();
}