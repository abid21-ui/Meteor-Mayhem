#ifndef METEOR_MAYHEM_GAME_H
#define METEOR_MAYHEM_GAME_H

#include "assets.h"
#include "entities.h"
#include "stats.h"

typedef enum GameScreen {
    SCREEN_MENU = 0,
    SCREEN_PLAYING,
    SCREEN_SETTINGS,
    SCREEN_STATS,
    SCREEN_GAME_OVER,
    SCREEN_PILOT_NAME
} GameScreen;

typedef struct Game {
    GameAssets *assets;
    LifetimeStats stats;
    GameSettings settings;
    Player player;
    Projectile projectiles[MAX_PROJECTILES];
    Asteroid asteroids[MAX_ASTEROIDS];
    Alien aliens[MAX_ALIENS];
    AlienProjectile alienProjectiles[MAX_ALIEN_PROJECTILES];
    GameScreen screen;
    int menuSelection;
    int settingsSelection;
    char nameInput[PILOT_NAME_MAX + 1];
    int nameInputLength;
    int score;
    float spawnTimer;
    float spawnInterval;
    float godModeNoticeTimer;
    bool quitRequested;
    bool runActive;
    bool namingFromSettings;
    bool godMode;
    bool godModeUsedThisRun;
} Game;

void Game_Init(Game *game, GameAssets *assets);
void Game_Update(Game *game);
void Game_Draw(const Game *game);
void Game_Shutdown(Game *game);
bool Game_ShouldQuit(const Game *game);

#endif
