#include <AEEngine.h>

#include <crtdbg.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    bool gameRunning = true;

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, nullptr);

    AESysSetWindowTitle("Game!");

    AESysReset();

    printf("Hello, World\n");

    // Game Loop
    while (gameRunning) {
        AESysFrameStart();

        if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
            gameRunning = false;

        AESysFrameEnd();
    }

    AESysExit();
}