#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

#include "GameStateManager.h"
#include "Terrain.h"

static Terrain dirt(TerrainMaterial::Dirt, {0.0f, 0.0f}, 48, 96, 16);
static Terrain stone(TerrainMaterial::Stone, {0.0f, 0.0f}, 48, 96, 16);

void LoadLevel1() {
    // Todo
    std::cout << "Load level 1\n";

    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();
}

void InitializeLevel1() {
    // Todo
    std::cout << "Initialize level 1\n";

    dirt.initCellsTransform();
    dirt.initCellsGraphics();
    dirt.initCellsCollider();
    dirt.updateTerrain();

    stone.initCellsTransform();
    stone.initCellsGraphics();
    stone.initCellsCollider();
    stone.updateTerrain();
}

void UpdateLevel1(GameStateManager& GSM, f32 deltaTime) {
    // Todo
    std::cout << "Update level 1\n";

    // Press Q to go to main menu
    if (AEInputCheckTriggered(AEVK_Q) || 0 == AESysDoesWindowExist()) {
        std::cout << "Q triggered\n";
        GSM.nextState_ = StateId::MainMenu;
    }

    // Press R to restart
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered\n";
        GSM.nextState_ = StateId::Restart;
    }

    if (AEInputCheckCurr(AEVK_LBUTTON)) {
        dirt.destroyAtMouse(20.0f);
    }
}

void DrawLevel1() {
    // Todo
    std::cout << "Draw level 1\n";

    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    dirt.renderTerrain();
    stone.renderTerrain();
}

void FreeLevel1() {
    // Todo
    std::cout << "Free level 1\n";
}

void UnloadLevel1() {
    // Todo
    std::cout << "Unload level 1\n";

    Terrain::freeMeshLibrary();
}