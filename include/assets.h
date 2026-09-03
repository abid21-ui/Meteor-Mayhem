#ifndef METEOR_MAYHEM_ASSETS_H
#define METEOR_MAYHEM_ASSETS_H

#include "raylib.h"
#include "settings.h"

typedef struct GameAssets {
    Texture2D background;
    Texture2D playerShips[SHIP_DESIGN_COUNT];
    Texture2D alienShip;
    Texture2D asteroid;
    Texture2D laser;
    Texture2D shotgunPellet;
    Texture2D shieldAura;
    Music backgroundMusic;
    Sound shootSound;
    Sound hitSound;
    bool musicEnabled;
    bool sfxEnabled;
} GameAssets;

bool Assets_Load(GameAssets *assets);
void Assets_ApplySettings(GameAssets *assets, const GameSettings *settings);
void Assets_Update(GameAssets *assets);
void Assets_PlayShot(GameAssets *assets, bool shotgun);
void Assets_PlayHit(GameAssets *assets);
void Assets_Unload(GameAssets *assets);

#endif
