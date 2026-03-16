#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

// Background simulation includes
#include "FluidSystem.h"
#include "StartEndPoint.h"
#include "Terrain.h"
#include "States/LevelManager.h"

// UI includes
#include "Button.h"
#include "GameStateManager.h"


// Json file reading variables
static int height, width, tileSize;
static bool fileExist;

// Background simulation variables
static Terrain* bgDirt = nullptr;
static Terrain* bgStone = nullptr;
static AEGfxTexture* pBgDirtTex{nullptr};
static AEGfxTexture* pBgStoneTex{nullptr};

static FluidSystem bgFluidSystem;
static StartEndPoint bgStartEndPoint;

// Auto spawn fluid without player input
static f32 autoSpawnTimer = 0.0f;


// TC added start
static Button startButton;
static Button howToPlayButton;
static Button settingsButton;
static Button creditsButton;
static Button quitButton;

static TextData titleText;

static s8 titleFont;
static s8 buttonFont;
// TC added end
static s8 font;

void LoadMainMenu() {
    // Load background simulation level map
    //@todo add lvl99 to levelmanager
    if (levelManager.getLevelData(99)) {
        levelManager.parseMapInfo(width, height, tileSize); 
        fileExist = true;
    } else {
        std::cout << "Failed to load level data\n";
        std::cout << "Using default values\n";
        width = 80;
        height = 45;
        tileSize = 20;
        fileExist = false;
    }

    // Load background simulation textures
    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pBgDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pBgStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");

    // Setup buttons - position them vertically
    startButton.loadMesh();
    startButton.loadTexture("Assets/Textures/brown_button.png");
    howToPlayButton.loadMesh();
    howToPlayButton.loadTexture("Assets/Textures/brown_button.png");
    settingsButton.loadMesh();
    settingsButton.loadTexture("Assets/Textures/brown_button.png");
    creditsButton.loadMesh();
    creditsButton.loadTexture("Assets/Textures/brown_button.png");
    quitButton.loadMesh();
    quitButton.loadTexture("Assets/Textures/brown_button.png");

    // Load fonts
    titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 72);
    buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 36);
}

void InitializeMainMenu() {

    // Initialize simulation systems
    bgFluidSystem.Initialize();

    // Setup terrain
    bgDirt = new Terrain(TerrainMaterial::Dirt, pBgDirtTex, {0.0f, 0.0f}, height, width,
                          tileSize, true);
    bgStone = new Terrain(TerrainMaterial::Stone, pBgStoneTex, {0.0f, 0.0f}, height, width,
                          tileSize, true);
    // Initialise JSON data into the background terrain cells
    if (fileExist) {
        levelManager.parseTerrainInfo(bgDirt->getNodes(), "Dirt");
    }
    bgDirt->initCellsTransform();
    bgDirt->initCellsGraphics();
    bgDirt->initCellsCollider();
    bgDirt->updateTerrain();
    if (fileExist) {
        levelManager.parseTerrainInfo(bgStone->getNodes(), "Stone");
    }
    bgStone->initCellsTransform();
    bgStone->initCellsGraphics();
    bgStone->initCellsCollider();
    bgStone->updateTerrain();

    bgStartEndPoint.Initialize();
    if (fileExist) {
        levelManager.parseStartEndInfo(bgStartEndPoint);
    }

    for (auto& startPoint : bgStartEndPoint.startPoints_) {
        startPoint.infinite_water_ = true;
        startPoint.release_water_ = true;
    }

    // UI buttons
    startButton.initFromJson("main_menu_buttons", "Start");
    howToPlayButton.initFromJson("main_menu_buttons", "HowToPlay");
    settingsButton.initFromJson("main_menu_buttons", "Settings");
    creditsButton.initFromJson("main_menu_buttons", "Credits");
    quitButton.initFromJson("main_menu_buttons", "Quit");

    titleText.initFromJson("main_menu_texts", "Title");
}

static void BgSpawnWater(f32 deltaTime) {
    static f32 global_spawn_timer = 0.0f;
    global_spawn_timer -= deltaTime;

    // Only spawn if enough time has passed
    if (global_spawn_timer <= 0.0f) {
        global_spawn_timer = 0.025f; // Keep the same flow rate as your main game

        for (auto& startPoint : bgStartEndPoint.startPoints_) {
            // Only process if the pipe is currently turned ON by our toggle timer
            if (startPoint.release_water_ && startPoint.type_ == StartEndType::Pipe) {

                f32 randRadius = 5.0f;
                f32 x_offset = startPoint.transform_.pos_.x +
                               AERandFloat() * startPoint.transform_.scale_.x -
                               (startPoint.transform_.scale_.x / 2.f);

                AEVec2 position = {x_offset, startPoint.transform_.pos_.y -
                                                 (startPoint.transform_.scale_.y / 2.f) -
                                                 randRadius};

                // Spawn the particle into our background fluid system
                bgFluidSystem.SpawnParticle(position.x, position.y, randRadius, FluidType::Water);
            }
        }
    }
}

void UpdateMainMenu(GameStateManager& GSM, f32 deltaTime) {
    (void)deltaTime; // Unused parameter, but required by function signature
    // Keep keyboard shortcuts for development/testing
    if (AEInputCheckTriggered(AEVK_R) || 0 == AESysDoesWindowExist()) {
        std::cout << "R triggered - Restart\n";
        GSM.nextState_ = StateId::Restart;
    }

    // Mouse click handling for all buttons
    if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {

        // Start button - goes to Level 1 (or you could make it go to a level select)
        if (startButton.checkMouseClick()) {
            std::cout << "Start button clicked - Going to Level Selector\n";
            GSM.nextState_ = StateId::LevelSelector;
        }

        // How To Play button
        if (howToPlayButton.checkMouseClick()) {
            std::cout << "How To Play button clicked\n";
            // TODO: Implement how to play screen or state
            // For now, just print or you could set a new state
            // GSM.nextState_ = StateId::HowToPlay;
        }

        // Settings button
        if (settingsButton.checkMouseClick()) {
            std::cout << "Settings button clicked\n";
            // TODO: Implement settings screen or state
            // GSM.nextState_ = StateId::Settings;
        }

        // Credits button
        if (creditsButton.checkMouseClick()) {
            std::cout << "Credits button clicked\n";
            // TODO: Implement credits screen or state
            // GSM.nextState_ = StateId::Credits;
        }

        // Quit button
        if (quitButton.checkMouseClick()) {
            std::cout << "Quit button clicked - Exiting game\n";
            GSM.nextState_ = StateId::Quit;
        }
    }

    startButton.updateTransform();
    howToPlayButton.updateTransform();
    settingsButton.updateTransform();
    creditsButton.updateTransform();
    quitButton.updateTransform();
    // Update the pipes to spawn water
    bgStartEndPoint.Update(deltaTime, bgFluidSystem.GetParticlePool(FluidType::Water));

    
    static f32 waterToggleTimer = 3.0f; // Time in seconds before switching
    static bool waterIsFlowing = true;

    waterToggleTimer -= deltaTime;
    if (waterToggleTimer <= 0.0f) {
        waterIsFlowing = !waterIsFlowing; 
        waterToggleTimer = 3.0f;         

        // Apply the new state to all background pipes
        for (auto& startPoint : bgStartEndPoint.startPoints_) {
            startPoint.release_water_ = waterIsFlowing;
        }
    }
    BgSpawnWater(deltaTime);
    // Update the fluid physics against the loaded terrain
    // @todo fix this
    bgFluidSystem.Update(deltaTime, *bgDirt);
    bgFluidSystem.Update(deltaTime, *bgStone);

}

void DrawMainMenu() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    bgDirt->renderTerrain();
    bgStone->renderTerrain();
    bgStartEndPoint.DrawColor(); 
    bgFluidSystem.DrawColor();

    // Draw all buttons with different colors
    startButton.draw(buttonFont);
    howToPlayButton.draw(buttonFont);
    settingsButton.draw(buttonFont);
    creditsButton.draw(buttonFont);
    quitButton.draw(buttonFont);

    // Draw game title
    titleText.draw(titleFont);
}

void FreeMainMenu() {
    bgFluidSystem.Free();
    bgStartEndPoint.Free();

    delete bgDirt;
    bgDirt = nullptr;
    delete bgStone;
    bgStone = nullptr;
}

void UnloadMainMenu() {
    // Free all meshes
    startButton.unload();
    howToPlayButton.unload();
    settingsButton.unload();
    creditsButton.unload();
    quitButton.unload();

    // Free fonts
    AEGfxDestroyFont(titleFont);
    AEGfxDestroyFont(buttonFont);
}