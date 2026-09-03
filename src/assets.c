#include "assets.h"

#include <string.h>

static bool TextureLoaded(Texture2D texture)
{
    return texture.id != 0;
}

bool Assets_Load(GameAssets *assets)
{
    memset(assets, 0, sizeof(*assets));

    // PNG is intentionally used here: it is supported by Raylib's default
    // image configuration on every target we build, unlike optional JPEG.
    assets->background = LoadTexture("assets/background.png");
    assets->playerShips[SHIP_RANGER] = LoadTexture("assets/player_ship_green.png");
    assets->playerShips[SHIP_STRIKER] = LoadTexture("assets/player_ship_red.png");
    assets->playerShips[SHIP_COMET] = LoadTexture("assets/player_ship_blue.png");
    assets->alienShip = LoadTexture("assets/alien_ship.png");
    assets->asteroid = LoadTexture("assets/asteroid.png");
    assets->laser = LoadTexture("assets/laser.png");
    assets->shotgunPellet = LoadTexture("assets/shotgun_pellet.png");
    assets->shieldAura = LoadTexture("assets/shield_aura.png");

    SetTextureFilter(assets->background, TEXTURE_FILTER_BILINEAR);
    for (int i = 0; i < SHIP_DESIGN_COUNT; ++i)
        SetTextureFilter(assets->playerShips[i], TEXTURE_FILTER_POINT);
    SetTextureFilter(assets->alienShip, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets->asteroid, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets->laser, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets->shotgunPellet, TEXTURE_FILTER_POINT);
    SetTextureFilter(assets->shieldAura, TEXTURE_FILTER_BILINEAR);

    assets->backgroundMusic = LoadMusicStream("sounds/bg_music.wav");
    assets->shootSound = LoadSound("sounds/shoot.wav");
    assets->hitSound = LoadSound("sounds/hit.wav");
    assets->backgroundMusic.looping = true;
    SetMusicVolume(assets->backgroundMusic, 0.35f);
    SetSoundVolume(assets->shootSound, 0.55f);
    SetSoundVolume(assets->hitSound, 0.70f);
    assets->musicEnabled = false;
    assets->sfxEnabled = true;

    bool shipsLoaded = true;
    for (int i = 0; i < SHIP_DESIGN_COUNT; ++i)
        shipsLoaded &= TextureLoaded(assets->playerShips[i]);

    return TextureLoaded(assets->background) && shipsLoaded &&
           TextureLoaded(assets->alienShip) &&
           TextureLoaded(assets->asteroid) &&
           TextureLoaded(assets->laser) &&
           TextureLoaded(assets->shotgunPellet) &&
           TextureLoaded(assets->shieldAura);
}

void Assets_ApplySettings(GameAssets *assets, const GameSettings *settings)
{
    if (settings->musicEnabled && !assets->musicEnabled)
        PlayMusicStream(assets->backgroundMusic);
    else if (!settings->musicEnabled && assets->musicEnabled)
        PauseMusicStream(assets->backgroundMusic);

    assets->musicEnabled = settings->musicEnabled;
    assets->sfxEnabled = settings->sfxEnabled;
}

void Assets_Update(GameAssets *assets)
{
    if (assets->musicEnabled) UpdateMusicStream(assets->backgroundMusic);
}

void Assets_PlayShot(GameAssets *assets, bool shotgun)
{
    if (!assets->sfxEnabled) return;
    SetSoundPitch(assets->shootSound, shotgun ? 0.72f : 1.15f);
    PlaySound(assets->shootSound);
}

void Assets_PlayHit(GameAssets *assets)
{
    if (!assets->sfxEnabled) return;
    SetSoundPitch(assets->hitSound, 1.0f);
    PlaySound(assets->hitSound);
}

void Assets_Unload(GameAssets *assets)
{
    UnloadMusicStream(assets->backgroundMusic);
    UnloadSound(assets->shootSound);
    UnloadSound(assets->hitSound);
    UnloadTexture(assets->background);
    for (int i = 0; i < SHIP_DESIGN_COUNT; ++i)
        UnloadTexture(assets->playerShips[i]);
    UnloadTexture(assets->alienShip);
    UnloadTexture(assets->asteroid);
    UnloadTexture(assets->laser);
    UnloadTexture(assets->shotgunPellet);
    UnloadTexture(assets->shieldAura);
}
