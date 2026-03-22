#include "States/MainMenu.h"

#include <iostream>

#include <AEEngine.h>

// Background simulation includes
#include "AudioSystem.h"
#include "FluidSystem.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "States/LevelManager.h"
#include "Terrain.h"
#include "VFXSystem.h"

// UI includes
#include "Button.h"
#include "GameStateManager.h"
#include "States/Transition.h"

// Json file reading variables
static int height, width, tileSize;
static bool fileExist;

// Background simulation variables
static Terrain* bgDirt = nullptr;
static Terrain* bgStone = nullptr;
static Terrain* bgMagic = nullptr;
static AEGfxTexture* pBgDirtTex{ nullptr };
static AEGfxTexture* pBgStoneTex{ nullptr };
static AEGfxTexture* pBgMagicTex{ nullptr };

static FluidSystem bgFluidSystem;
static StartEndPoint bgStartEndPoint;
static PortalSystem bgPortalSystem;
static VFXSystem bgVfxSystem;
static CollectibleSystem bgCollectibleSystem;
static TransitionManager transitionManager;

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
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
	// Load background simulation level map
	//@todo add lvl99 to levelmanager
	if (levelManager.getLevelData(99)) {
		levelManager.parseMapInfo(width, height, tileSize);
		fileExist = true;
	}
	else {
		std::cout << "Failed to load level data\n";
		std::cout << "Using default values\n";
		width = 80;
		height = 45;
		tileSize = 20;
		fileExist = false;
	}
	// Load fonts
	titleFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 72);
	buttonFont = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 33);
	font = AEGfxCreateFont("Assets/Fonts/PressStart2P-Regular.ttf", 24);

	// Load background simulation textures
	Terrain::createMeshLibrary();
	Terrain::createColliderLibrary();

	pBgDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
	pBgStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
	pBgMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");

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

	bgCollectibleSystem.Load(font);
}

void InitializeMainMenu() {

	// Initialize simulation systems
	bgFluidSystem.Initialize();
	bgPortalSystem.Initialize();
	bgVfxSystem.Initialize(800, 20);
	bgCollectibleSystem.Initialize();

	// Setup terrain
	bgDirt =
		new Terrain(TerrainMaterial::Dirt, pBgDirtTex, { 0.0f, 0.0f }, height, width, tileSize, true);
	bgStone = new Terrain(TerrainMaterial::Stone, pBgStoneTex, { 0.0f, 0.0f }, height, width,
		tileSize, true);
	bgMagic = new Terrain(TerrainMaterial::Magic, pBgMagicTex, { 0.0f, 0.0f }, height, width,
		tileSize, false);

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

	if (fileExist) {
		levelManager.parseTerrainInfo(bgMagic->getNodes(), "Magic");
	}
	bgMagic->initCellsTransform();
	bgMagic->initCellsGraphics();
	bgMagic->initCellsCollider();
	bgMagic->updateTerrain();

	bgStartEndPoint.Initialize();
	if (fileExist) {
		levelManager.parseStartEndInfo(bgStartEndPoint);
		levelManager.parsePortalInfo(bgPortalSystem);
		levelManager.parseCollectibleInfo(bgCollectibleSystem);
	}

	for (auto& startPoint : bgStartEndPoint.startPoints_) {
		startPoint.infiniteWater_ = true;
		startPoint.releaseWater_ = true;
	}
	transitionManager.Initialize(&bgFluidSystem);

	// UI buttons
	startButton.initFromJson("main_menu_buttons", "Start");
	howToPlayButton.initFromJson("main_menu_buttons", "HowToPlay");
	settingsButton.initFromJson("main_menu_buttons", "Settings");
	creditsButton.initFromJson("main_menu_buttons", "Credits");
	quitButton.initFromJson("main_menu_buttons", "Quit");

	titleText.initFromJson("main_menu_texts", "Title");
}

static void BgSpawnWater(f32 deltaTime) {
	// STATE VARIABLES
	static bool isWaiting = true;
	static f32 stateTimer = 3.5f;    // Wait 3.5 seconds before the very first burst
	static f32 particleTimer = 0.0f; // Tracks how fast individual drops fall
	static int particlesSpawned = 0; // Explicitly counts how many dropped

	if (isWaiting) {
		stateTimer -= deltaTime;
		// When the wait is over, open the tap and reset the counter
		if (stateTimer <= 0.0f) {
			isWaiting = false;
			particlesSpawned = 0;
			particleTimer = 0.0f; // Force the first drop to spawn instantly
		}
	}
	else { // The tap is ON
		particleTimer -= deltaTime;

		while (particleTimer <= 0.0f && particlesSpawned < 10) {
			particleTimer += 0.05f; // Cooldown between drops
			particlesSpawned++;

			for (auto& startPoint : bgStartEndPoint.startPoints_) {
				if (startPoint.type_ == StartEndType::Pipe) {

					f32 randRadius = 8.0f;
					f32 xOffset = startPoint.transform_.pos_.x +
						AERandFloat() * startPoint.transform_.scale_.x -
						(startPoint.transform_.scale_.x / 2.f);

					f32 yOffset = startPoint.transform_.pos_.y -
						(startPoint.transform_.scale_.y / 2.f) - randRadius;

					bgFluidSystem.SpawnParticle(xOffset, yOffset, randRadius, FluidType::Water);
				}
			}
		}

		// Once we hit 10 particles, turn the tap OFF and wait 9 seconds
		if (particlesSpawned >= 10) {
			isWaiting = true;
			stateTimer = 9.0f;
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

	if (!transitionManager.IsTransitioning()) {
		// Mouse click handling for all buttons
		if (AEInputCheckReleased(AEVK_LBUTTON) || 0 == AESysDoesWindowExist()) {
			// Start button - goes to Level 1 (or you could make it go to a level select)
			if (startButton.checkMouseClick()) {
				std::cout << "Start button clicked - Going to Level Selector\n";
				transitionManager.StartTsunami(&GSM, StateId::LevelSelector);
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
				GSM.nextState_ = StateId::Settings;
			}

			// Credits button
			if (creditsButton.checkMouseClick()) {
				std::cout << "Credits button clicked\n";
				GSM.nextState_ = StateId::Credits;
			}

			// Quit button
			if (quitButton.checkMouseClick()) {
				std::cout << "Quit button clicked - Exiting game\n";
				GSM.nextState_ = StateId::Quit;
			}
		}
		if (AEInputCheckCurr(AEVK_LBUTTON)) {

			bool hitDirt = bgDirt->destroyAtMouse(20.0f);
			if (hitDirt) {
				bgVfxSystem.SpawnContinuous(VFXType::DirtBurst, GetMouseWorldPos(), deltaTime, 0.1f);
				g_audioSystem.playSound("dirt_break", "sfx", 0.25f, 1.0f);
			}
			else {
				bgVfxSystem.ResetSpawnTimer();
			}
		}
		else {
			bgVfxSystem.ResetSpawnTimer();
		}
		startButton.updateTransform();
		howToPlayButton.updateTransform();
		settingsButton.updateTransform();
		creditsButton.updateTransform();
		quitButton.updateTransform();
		bgCollectibleSystem.Update(deltaTime, bgFluidSystem.GetParticlePool(FluidType::Water));

		// Update the pipes to spawn water
		bgStartEndPoint.Update(deltaTime, bgFluidSystem.GetParticlePool(FluidType::Water));

		BgSpawnWater(deltaTime);
		// Update the fluid physics against the loaded terrain
		// @todo fix this
		bgFluidSystem.Update(deltaTime, { bgDirt, bgStone });

		bgPortalSystem.Update(deltaTime, bgFluidSystem.GetParticlePool(FluidType::Water));
		bgVfxSystem.Update(deltaTime);
	}
	else {
		f32 fastForwardTime = deltaTime * 4.0f;
		bgFluidSystem.Update(fastForwardTime, {});
	}
	transitionManager.Update(deltaTime);
}

void DrawMainMenu() {
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
	bgDirt->renderTerrain();
	bgStone->renderTerrain();
	bgMagic->renderTerrain();

	bgStartEndPoint.DrawColor();
	bgPortalSystem.DrawColor();

	bgVfxSystem.Draw();
	bgCollectibleSystem.Draw();

	// Draw all buttons with different colors
	startButton.draw(buttonFont);
	howToPlayButton.draw(buttonFont);
	settingsButton.draw(buttonFont);
	creditsButton.draw(buttonFont);
	quitButton.draw(buttonFont);

	// Draw game title
	titleText.draw(titleFont);

	bgFluidSystem.DrawColor();
}

void FreeMainMenu() {
	bgFluidSystem.Free();
	bgStartEndPoint.Free();
	bgPortalSystem.Free();
	bgVfxSystem.Free();
	bgCollectibleSystem.Free();

	delete bgDirt;
	bgDirt = nullptr;
	delete bgStone;
	bgStone = nullptr;
	delete bgMagic;
	bgMagic = nullptr;
}

void UnloadMainMenu() {

	// free terrain mesh
	Terrain::freeMeshLibrary();

	// unload background terrain textures
	/*

		if (pBgDirtTex) {
		AEGfxTextureUnload(pBgDirtTex);
		pBgDirtTex = nullptr;
	}
	if (pBgStoneTex) {
		AEGfxTextureUnload(pBgStoneTex);
		pBgStoneTex = nullptr;
	}
	if (pBgMagicTex) {
		AEGfxTextureUnload(pBgMagicTex);
		pBgMagicTex = nullptr;
	}
	*/

	/*
		if (titleFont) {
		AEGfxDestroyFont(titleFont);
		titleFont = 0;
	}
	if (buttonFont) {
		AEGfxDestroyFont(buttonFont);
		buttonFont = 0;
	}

	*/
	// Free all meshes
	startButton.unload();
	howToPlayButton.unload();
	settingsButton.unload();
	creditsButton.unload();
	quitButton.unload();

	// Free fonts
}