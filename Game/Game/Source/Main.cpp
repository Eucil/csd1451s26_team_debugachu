#include <iostream>

#include <AEEngine.h>
#include <crtdbg.h>

#include "GameStateManager.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, nullptr);

    AESysSetWindowTitle("Game!");

    AESysReset();

    printf("Hello, World\n");

    GameStateManager GSM;
    GSM.init(StateId::Level2);

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

    AESysExit();
}