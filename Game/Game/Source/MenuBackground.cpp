/*!
@file       MenuBackground.cpp
@author     Han Tianchou/H.tianchou@digipen.edu
@co_author  NIL

@date		March, 31, 2026

@brief      This source file contains the declaration of functions that

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

// ----------------------------------------------------------------------------
// Private state (only visible inside this translation unit)
// ----------------------------------------------------------------------------
namespace {

// ----------------------------------------------------------------------------
// Reference counter for Load/Unload
// ----------------------------------------------------------------------------
// Both MainMenu and Credits call load() and unload(). Without a guard,
// navigating MainMenu -> Credits -> MainMenu would:
//   1. load()   (MainMenu)   -- loads textures/meshes
//   2. unload() (MainMenu)   -- frees them
//   3. load()   (Credits)    -- loads again fine
//   4. unload() (Credits)    -- frees again fine
// BUT if the game state manager calls load() for Credits BEFORE calling
// unload() for MainMenu, assets would be double-loaded. The ref count
// makes Load/Unload idempotent: assets are loaded exactly once and freed
// exactly once no matter how many states share the background.
static int loadRefCount = 0;

// Map / level data
static int height = 45;
static int width = 80;
static int tileSize = 20;
static int portalLimit = 0;
static bool fileExist = false;

// Terrain
static Terrain* bgDirt = nullptr;
static Terrain* bgStone = nullptr;
static Terrain* bgMagic = nullptr;
static AEGfxTexture* pBgDirtTex = nullptr;
static AEGfxTexture* pBgStoneTex = nullptr;
static AEGfxTexture* pBgMagicTex = nullptr;

// Background tile
static TiledBackground bg;

// Simulation systems
static FluidSystem bgFluidSystem;
static StartEndPoint bgStartEndPoint;
static PortalSystem bgPortalSystem;
static VFXSystem bgVfxSystem;
static CollectibleSystem bgCollectibleSystem;
// ----------------------------------------------------------------------------
// bgSpawnWater
// Automatically trickles water from pipe start-points on a timer.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// MenuBackground public API
// ----------------------------------------------------------------------------

void MenuBackground::load(int backgroundLevel) {
    // Only load GPU assets once, no matter how many states call load()
    ++loadRefCount;
    if (loadRefCount > 1) {
        std::cout << "[MenuBackground] load() skipped (already loaded, refCount=" << loadRefCount
                  << ")\n";
        return;
    }

    if (levelManager.getLevelData(backgroundLevel)) {
        levelManager.parseMapInfo(width, height, tileSize, portalLimit);
        fileExist = true;
    } else {
        std::cout << "[MenuBackground] Failed to load level, using defaults.\n";
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

    std::cout << "[MenuBackground] load() complete.\n";
}

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

void MenuBackground::update(f32 deltaTime) {
    bgCollectibleSystem.update(deltaTime, bgFluidSystem.getParticlePool(FluidType::Water),
                               bgVfxSystem);
    bgStartEndPoint.update(deltaTime, bgFluidSystem.getParticlePool(FluidType::Water), bgVfxSystem);
    bgSpawnWater(deltaTime);
    bgFluidSystem.update(deltaTime, {bgDirt, bgStone});
    bgPortalSystem.update(
        deltaTime, bgFluidSystem.getParticlePool(FluidType::Water), // Fills the waterPool param
        bgVfxSystem                                                 // Allows VFX spawning
    );
    bgVfxSystem.update(deltaTime);
}

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

// ----------------------------------------------------------------------------
// DestroyDirtAtMouse
// Delegates dirt destruction + VFX burst to the internal systems.
// Returns true if dirt was actually removed so the caller can play audio.
// ----------------------------------------------------------------------------
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

void MenuBackground::unload() {
    // Only unload GPU assets when every state that loaded them has unloaded
    if (loadRefCount <= 0) {
        std::cout << "[MenuBackground] unload() called with refCount <= 0, ignoring.\n";
        return;
    }

    --loadRefCount;
    if (loadRefCount > 0) {
        std::cout << "[MenuBackground] unload() deferred (refCount=" << loadRefCount << ")\n";
        return;
    }

    // refCount reached 0 -- safe to free GPU assets now
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

    std::cout << "[MenuBackground] unload() complete.\n";
}