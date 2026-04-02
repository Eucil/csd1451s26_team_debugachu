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

#include <iostream>

#include <AEEngine.h>

#include "AudioSystem.h"
#include "FluidSystem.h"
#include "PortalSystem.h"
#include "StartEndPoint.h"
#include "States/LevelManager.h"
#include "Terrain.h"
#include "Utils.h"
#include "VFXSystem.h"


// ----------------------------------------------------------------------------
// Private state (only visible inside this translation unit)
// ----------------------------------------------------------------------------
namespace {

// ----------------------------------------------------------------------------
// Reference counter for Load/Unload
// ----------------------------------------------------------------------------
// Both MainMenu and Credits call Load() and Unload(). Without a guard,
// navigating MainMenu -> Credits -> MainMenu would:
//   1. Load()   (MainMenu)   -- loads textures/meshes
//   2. Unload() (MainMenu)   -- frees them
//   3. Load()   (Credits)    -- loads again fine
//   4. Unload() (Credits)    -- frees again fine
// BUT if the game state manager calls Load() for Credits BEFORE calling
// Unload() for MainMenu, assets would be double-loaded. The ref count
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
// ----------------------------------------------------------------------------
// BgSpawnWater
// Automatically trickles water from pipe start-points on a timer.
// ----------------------------------------------------------------------------
static void BgSpawnWater(f32 deltaTime) {
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

                    bgFluidSystem.SpawnParticle(xOffset, yOffset, randRadius, FluidType::Water);
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

void MenuBackground::Load() {
    // Only load GPU assets once, no matter how many states call Load()
    ++loadRefCount;
    if (loadRefCount > 1) {
        std::cout << "[MenuBackground] Load() skipped (already loaded, refCount=" << loadRefCount
                  << ")\n";
        return;
    }

    if (levelManager.getLevelData(99)) {
        levelManager.parseMapInfo(width, height, tileSize, portalLimit);
        fileExist = true;
    } else {
        std::cout << "[MenuBackground] Failed to load level 99, using defaults.\n";
        width = 80;
        height = 45;
        tileSize = 20;
        fileExist = false;
    }

    Terrain::createMeshLibrary();
    Terrain::createColliderLibrary();

    pBgDirtTex  = AEGfxTextureLoad("Assets/Textures/terrain_dirt.png");
    pBgStoneTex = AEGfxTextureLoad("Assets/Textures/terrain_stone.png");
    pBgMagicTex = AEGfxTextureLoad("Assets/Textures/terrain_magic.png");
    bg.loadFromJson("background", "Background");

    std::cout << "[MenuBackground] Load() complete.\n";
}

void MenuBackground::Initialize() {
    bgFluidSystem.Initialize();
    bgPortalSystem.Initialize(portalLimit);
    bgVfxSystem.Initialize(800, 20);
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

    bgStartEndPoint.Initialize();
    if (fileExist) {
        levelManager.parseStartEndInfo(bgStartEndPoint);
        levelManager.parsePortalInfo(bgPortalSystem);
    }

    for (auto& startPoint : bgStartEndPoint.startPoints_) {
        startPoint.infiniteWater_ = true;
        startPoint.releaseWater_ = true;
    }
}

void MenuBackground::Update(f32 deltaTime) {
    bgStartEndPoint.Update(deltaTime, bgFluidSystem.GetParticlePool(FluidType::Water));
    BgSpawnWater(deltaTime);
    bgFluidSystem.Update(deltaTime, {bgDirt, bgStone});
    bgPortalSystem.Update(deltaTime, bgFluidSystem.GetParticlePool(FluidType::Water));
    bgVfxSystem.Update(deltaTime);
}

void MenuBackground::Draw() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    bg.draw();

    bgDirt->renderTerrain();
    bgStone->renderTerrain();
    bgMagic->renderTerrain();

    bgStartEndPoint.DrawTexture(0);
    bgPortalSystem.Draw();

    bgVfxSystem.Draw();
    bgFluidSystem.DrawColor();
}

// ----------------------------------------------------------------------------
// DestroyDirtAtMouse
// Delegates dirt destruction + VFX burst to the internal systems.
// Returns true if dirt was actually removed so the caller can play audio.
// ----------------------------------------------------------------------------
bool MenuBackground::DestroyDirtAtMouse(f32 radius) {
    if (bgDirt == nullptr)
        return false;

    bool hitDirt = bgDirt->destroyAtMouse(radius);

    if (hitDirt) {
        bgVfxSystem.SpawnContinuous(VFXType::DirtBurst, GetMouseWorldPos(), 0.016f, 0.1f);
    } else {
        bgVfxSystem.ResetSpawnTimer();
    }

    return hitDirt;
}

void MenuBackground::Free() {
    bgFluidSystem.Free();
    bgStartEndPoint.Free();
    bgPortalSystem.Free();
    bgVfxSystem.Free();
    delete bgDirt;
    bgDirt = nullptr;
    delete bgStone;
    bgStone = nullptr;
    delete bgMagic;
    bgMagic = nullptr;
}

void MenuBackground::Unload() {
    // Only unload GPU assets when every state that loaded them has unloaded
    if (loadRefCount <= 0) {
        std::cout << "[MenuBackground] Unload() called with refCount <= 0, ignoring.\n";
        return;
    }

    --loadRefCount;
    if (loadRefCount > 0) {
        std::cout << "[MenuBackground] Unload() deferred (refCount=" << loadRefCount << ")\n";
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

    std::cout << "[MenuBackground] Unload() complete.\n";
}