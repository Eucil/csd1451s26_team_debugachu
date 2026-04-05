/*!
@file       MenuBackground.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date       March, 31, 2026

@brief      This source file contains the definitions of functions in the
            MenuBackground namespace, which provides a shared animated
            background used by the MainMenu, Credits, and Controls states.
            A reference counter ensures assets are loaded and freed exactly
            once regardless of how many states share the background.

@copyright  Copyright (C) 2026 DigiPen Institute of Technology.
            Reproduction or disclosure of this file or its contents
            without the prior written consent of DigiPen Institute of
            Technology is prohibited.
*//*______________________________________________________________________*/
#include "MenuBackground.h"

// Standard library
#include <iostream>

// Third-party
#include <AEEngine.h>

// Project
#include "AudioSystem.h"
#include "DebugSystem.h"
#include "FluidSystem.h"
#include "LevelManager.h"
#include "MouseUtils.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "Terrain.h"
#include "TileBackground.h"
#include "VFXSystem.h"

// ==========================================
//            PRIVATE STATE
// ==========================================
namespace {

// Reference counter — ensures assets are loaded exactly once even when
// multiple states call load() before either calls unload().
static int loadRefCount = 0;

static int height = 45;
static int width = 80;
static int tileSize = 20;
static int portalLimit = 0;
static bool fileExist = false;

static Terrain* bgDirt = nullptr;
static Terrain* bgStone = nullptr;
static Terrain* bgMagic = nullptr;
static AEGfxTexture* pBgDirtTex = nullptr;
static AEGfxTexture* pBgStoneTex = nullptr;
static AEGfxTexture* pBgMagicTex = nullptr;

static TiledBackground bg;

static FluidSystem bgFluidSystem;
static StartEndPoint bgStartEndPoint;
static PortalSystem bgPortalSystem;
static VFXSystem bgVfxSystem;
static CollectibleSystem bgCollectibleSystem;

// =========================================================
//
// bgSpawnWater()
//
// - Waits for an initial delay before spawning water.
// - Spawns up to 10 particles at each pipe start point on a timer.
// - After all particles are spawned, waits before repeating.
//
// =========================================================
static void bgSpawnWater(f32 deltaTime) {
    static bool isWaiting = true;
    static f32 stateTimer = 3.5f;
    static f32 particleTimer = 0.0f;
    static int particlesSpawned = 0;

    if (isWaiting) {
        stateTimer -= deltaTime;
        if (stateTimer <= 0.0f) {
            isWaiting = false;
            particlesSpawned = 0;
            particleTimer = 0.0f;
        }
    } else {
        particleTimer -= deltaTime;
        while (particleTimer <= 0.0f && particlesSpawned < 10) {
            particleTimer += 0.05f;
            particlesSpawned++;
            for (auto& startPoint : bgStartEndPoint.startPoints_) {
                if (startPoint.type_ == StartEndType::Pipe) {
                    f32 randRadius = 8.0f;
                    f32 xOffset = startPoint.transform_.pos_.x +
                                  AERandFloat() * startPoint.transform_.scale_.x -
                                  (startPoint.transform_.scale_.x / 2.f);
                    f32 yOffset = startPoint.transform_.pos_.y -
                                  (startPoint.transform_.scale_.y / 2.f) - randRadius;
                    bgFluidSystem.spawnParticle(xOffset, yOffset, randRadius, FluidType::Water);
                }
            }
        }
        if (particlesSpawned >= 10) {
            isWaiting = true;
            stateTimer = 9.0f;
        }
    }
}

} // anonymous namespace

// ==========================================
//            MENU BACKGROUND PUBLIC API
// ==========================================

// =========================================================
//
// MenuBackground::load()
//
// - Increments the reference counter and returns early if assets
//   are already loaded by another state.
// - Attempts to load level data for the background level; falls back
//   to default dimensions if the file does not exist.
// - Creates the terrain mesh and collider libraries.
// - Loads the dirt, stone, and magic terrain textures.
// - Loads the tiled background from JSON config.
//
// =========================================================
void MenuBackground::load(int backgroundLevel) {
    ++loadRefCount;
    if (loadRefCount > 1)
        return;

    if (levelManager.getLevelData(backgroundLevel)) {
        levelManager.parseMapInfo(width, height, tileSize, portalLimit);
        fileExist = true;
    } else {
        width = 80;
        height = 45;
        tileSize = 20;
        fileExist = false;
    }

    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pBgDirtTex = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pBgStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
    pBgMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");
    bg.loadFromJson("background", "Background");
}

// =========================================================
//
// MenuBackground::initialize()
//
// - Initializes the fluid, portal, and VFX systems.
// - Allocates and initializes the three terrain layers (dirt, stone, magic).
// - Parses terrain data from the level file if it exists.
// - Initializes all terrain cells (transform, graphics, colliders).
// - Parses start/end point and portal data from the level file if it exists.
// - Sets all pipe start points to infinite water release mode.
// - Registers all systems with the debug system.
//
// =========================================================
void MenuBackground::initialize() {
    bgFluidSystem.initialize();
    bgPortalSystem.initialize(portalLimit);
    bgVfxSystem.initialize(800, 20);

    bgDirt =
        new Terrain(TerrainMaterial::Dirt, pBgDirtTex, {0.0f, 0.0f}, height, width, tileSize, true);
    bgStone = new Terrain(TerrainMaterial::Stone, pBgStoneTex, {0.0f, 0.0f}, height, width,
                          tileSize, true);
    bgMagic = new Terrain(TerrainMaterial::Magic, pBgMagicTex, {0.0f, 0.0f}, height, width,
                          tileSize, false);

    if (fileExist)
        levelManager.parseTerrainInfo(bgDirt->getNodes(), "Dirt");
    bgDirt->initCellsTransform();
    bgDirt->initCellsGraphics();
    bgDirt->initCellsCollider();
    bgDirt->updateTerrain();

    if (fileExist)
        levelManager.parseTerrainInfo(bgStone->getNodes(), "Stone");
    bgStone->initCellsTransform();
    bgStone->initCellsGraphics();
    bgStone->initCellsCollider();
    bgStone->updateTerrain();

    if (fileExist)
        levelManager.parseTerrainInfo(bgMagic->getNodes(), "Magic");
    bgMagic->initCellsTransform();
    bgMagic->initCellsGraphics();
    bgMagic->initCellsCollider();
    bgMagic->updateTerrain();

    bgStartEndPoint.initialize();
    if (fileExist) {
        levelManager.parseStartEndInfo(bgStartEndPoint);
        levelManager.parsePortalInfo(bgPortalSystem);
    }

    for (auto& startPoint : bgStartEndPoint.startPoints_) {
        startPoint.infiniteWater_ = true;
        startPoint.releaseWater_ = true;
    }

    g_debugSystem.setScene(bgDirt, bgStone, bgMagic, &bgFluidSystem, nullptr, &bgPortalSystem,
                           &bgStartEndPoint, &bgVfxSystem);
}

// =========================================================
//
// MenuBackground::update()
//
// - Updates collectibles, start/end points, and water spawning.
// - Updates the fluid simulation against the terrain layers.
// - Updates the portal system with the current water particle pool.
// - Updates the VFX system.
//
// =========================================================
void MenuBackground::update(f32 deltaTime) {
    bgCollectibleSystem.update(deltaTime, bgFluidSystem.getParticlePool(FluidType::Water),
                               bgVfxSystem);
    bgStartEndPoint.update(deltaTime, bgFluidSystem.getParticlePool(FluidType::Water), bgVfxSystem);
    bgSpawnWater(deltaTime);
    bgFluidSystem.update(deltaTime, {bgDirt, bgStone});
    bgPortalSystem.update(deltaTime, bgFluidSystem.getParticlePool(FluidType::Water), bgVfxSystem);
    bgVfxSystem.update(deltaTime);
}

// =========================================================
//
// MenuBackground::draw()
//
// - Sets the background colour to black.
// - Draws the tiled background, then terrain layers in order.
// - Draws start/end points, portals, VFX, and fluid particles.
//
// =========================================================
void MenuBackground::draw() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    bg.draw();

    bgDirt->renderTerrain();
    bgStone->renderTerrain();
    bgMagic->renderTerrain();

    bgStartEndPoint.drawTexture(0);
    bgPortalSystem.draw();
    bgVfxSystem.draw();
    bgFluidSystem.drawColor();
}

// =========================================================
//
// MenuBackground::destroyDirtAtMouse()
//
// - Returns false immediately if the dirt terrain pointer is null.
// - Attempts to destroy dirt at the current mouse world position.
// - If dirt was destroyed, spawns a continuous dirt burst VFX.
// - If no dirt was hit, resets the VFX spawn timer.
// - Returns true if dirt was destroyed, false otherwise.
//
// =========================================================
bool MenuBackground::destroyDirtAtMouse(f32 radius) {
    if (bgDirt == nullptr)
        return false;

    bool hitDirt = bgDirt->destroyAtMouse(radius);

    if (hitDirt) {
        bgVfxSystem.spawnContinuous(VFXType::DirtBurst, getMouseWorldPos(), 0.016f, 0.1f);
    } else {
        bgVfxSystem.resetSpawnTimer();
    }

    return hitDirt;
}

// =========================================================
//
// MenuBackground::free()
//
// - Clears the debug system scene registration.
// - Frees the fluid, start/end point, portal, and VFX systems.
// - Deletes all three terrain layers and nulls their pointers.
//
// =========================================================
void MenuBackground::free() {
    g_debugSystem.clearScene();
    bgFluidSystem.free();
    bgStartEndPoint.free();
    bgPortalSystem.free();
    bgVfxSystem.free();
    delete bgDirt;
    bgDirt = nullptr;
    delete bgStone;
    bgStone = nullptr;
    delete bgMagic;
    bgMagic = nullptr;
}

// =========================================================
//
// MenuBackground::unload()
//
// - Returns early if the reference counter is already zero.
// - Decrements the reference counter and defers unloading if other
//   states still hold a reference.
// - When the counter reaches zero, frees the terrain mesh library
//   and unloads all terrain textures and the tiled background.
//
// =========================================================
void MenuBackground::unload() {
    if (loadRefCount <= 0)
        return;

    --loadRefCount;
    if (loadRefCount > 0)
        return;

    Terrain::freeMeshLibrary();

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
    bg.unload();
}