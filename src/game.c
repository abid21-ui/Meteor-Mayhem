#include "game.h"

#include "config.h"
#include "raymath.h"

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const float PLAYER_RADIUS = 23.0f * GAMEPLAY_SCALE;


static const char *PilotDisplayName(const Game *game)
{
    return game->stats.pilotName[0] != '\0' ? game->stats.pilotName : "PILOT";
}

static bool OverdriveActive(const Game *game)
{
    return game->godMode || game->player.shieldTimer > 0.0f;
}

static void DrawCenteredText(const char *text, int y, int size, Color color)
{
    int width = MeasureText(text, size);
    DrawText(text, SCREEN_WIDTH / 2 - width / 2, y, size, color);
}

static void DrawTextureCentered(Texture2D texture, Vector2 position,
                                float width, float height, float rotation, Color tint)
{
    Rectangle source = { 0, 0, (float)texture.width, (float)texture.height };
    Rectangle destination = { position.x, position.y, width, height };
    Vector2 origin = { width / 2.0f, height / 2.0f };
    DrawTexturePro(texture, source, destination, origin, rotation, tint);
}

static void SaveStats(Game *game)
{
    Stats_Save(&game->stats, STATS_FILE);
}

static void SaveSettings(Game *game)
{
    Settings_Save(&game->settings, SETTINGS_FILE);
}

static void BeginPilotNameEntry(Game *game, bool fromSettings)
{
    game->namingFromSettings = fromSettings;
    strncpy(game->nameInput, game->stats.pilotName, PILOT_NAME_MAX);
    game->nameInput[PILOT_NAME_MAX] = '\0';
    game->nameInputLength = (int)strlen(game->nameInput);
    game->screen = SCREEN_PILOT_NAME;
}

static void TrimPilotName(char *name)
{
    char *start = name;
    while (*start == ' ') ++start;
    if (start != name) memmove(name, start, strlen(start) + 1);

    size_t length = strlen(name);
    while (length > 0 && name[length - 1] == ' ')
        name[--length] = '\0';
}

static void CommitRunHighScore(Game *game)
{
    if (!game->runActive) return;

    int *record = game->godModeUsedThisRun
        ? &game->stats.godModeHighScore
        : &game->stats.highScore;
    if (game->score > *record) *record = game->score;
}

static void StartRun(Game *game)
{
    // Restarting also closes the previous run and files its score in the
    // correct leaderboard.
    CommitRunHighScore(game);
    const DifficultyProfile *difficulty =
        Settings_DifficultyProfile(game->settings.difficulty);
    Entities_Clear(game->projectiles, game->asteroids,
                   game->aliens, game->alienProjectiles);
    Player_StartRun(&game->player);
    game->score = 0;
    game->spawnTimer = 0.0f;
    game->spawnInterval = difficulty->initialSpawnInterval;
    game->godMode = false;
    game->godModeUsedThisRun = false;
    game->godModeNoticeTimer = 0.0f;
    game->screen = SCREEN_PLAYING;
    game->runActive = true;
    game->stats.gamesPlayed++;
    SaveStats(game);
}

static void FinishRun(Game *game)
{
    CommitRunHighScore(game);
    game->runActive = false;
    SaveStats(game);
    game->screen = SCREEN_GAME_OVER;
}

static int AwardScore(Game *game, int basePoints)
{
    const DifficultyProfile *difficulty =
        Settings_DifficultyProfile(game->settings.difficulty);
    float multiplier = difficulty->scoreMultiplier;
    if (OverdriveActive(game)) multiplier *= SHIELD_SCORE_MULTIPLIER;
    int awarded = (int)lroundf((float)basePoints * multiplier);
    game->score += awarded;
    game->stats.totalPoints += (uint64_t)awarded;
    return awarded;
}

static float ShieldBarLevel(const Player *player)
{
    float level = player->shieldTimer > 0.0f
        ? player->shieldTimer / SHIELD_DURATION
        : player->shieldCharge / SHIELD_MAX_CHARGE;
    if (level < 0.0f) return 0.0f;
    if (level > 1.0f) return 1.0f;
    return level;
}

static int AsteroidPoints(AsteroidSize size)
{
    if (size == ASTEROID_SMALL) return 100;
    if (size == ASTEROID_MEDIUM) return 50;
    return 25;
}

static void SplitAsteroid(Game *game, AsteroidSize size, Vector2 position)
{
    if (size == ASTEROID_SMALL) return;
    AsteroidSize childSize = (AsteroidSize)(size - 1);
    float baseAngle = (float)GetRandomValue(0, 359);
    float speedMultiplier =
        Settings_DifficultyProfile(game->settings.difficulty)->enemySpeedMultiplier;
    Asteroid_SpawnFragment(game->asteroids, childSize, position,
                           baseAngle - 34.0f, speedMultiplier);
    Asteroid_SpawnFragment(game->asteroids, childSize, position,
                           baseAngle + 34.0f, speedMultiplier);
}

static void DestroyAsteroid(Game *game, int index, bool scoreKill)
{
    AsteroidSize size = game->asteroids[index].size;
    Vector2 position = game->asteroids[index].position;
    game->asteroids[index].active = false;
    SplitAsteroid(game, size, position);
    if (scoreKill)
    {
        game->stats.asteroidsSmashed++;
        AwardScore(game, AsteroidPoints(size));
    }
}

static void DestroyAlien(Game *game, int index, bool scoreKill)
{
    game->aliens[index].active = false;
    if (!scoreKill) return;

    game->stats.aliensSmashed++;
    AwardScore(game, 250);
    if (game->player.shieldTimer <= 0.0f)
    {
        game->player.shieldCharge +=
            Settings_DifficultyProfile(game->settings.difficulty)->shieldChargePerAlien;
        if (game->player.shieldCharge > SHIELD_MAX_CHARGE)
            game->player.shieldCharge = SHIELD_MAX_CHARGE;
    }
}

static void ClearRespawnArea(Game *game)
{
    const Vector2 center = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (game->asteroids[i].active &&
            Vector2Distance(game->asteroids[i].position, center) < 150.0f * GAMEPLAY_SCALE)
            game->asteroids[i].active = false;
    }
    for (int i = 0; i < MAX_ALIEN_PROJECTILES; ++i)
    {
        if (game->alienProjectiles[i].active &&
            Vector2Distance(game->alienProjectiles[i].position, center) < 180.0f * GAMEPLAY_SCALE)
            game->alienProjectiles[i].active = false;
    }
}

static bool DamagePlayer(Game *game)
{
    if (game->godMode) return false;
    if (Player_IsInvincible(&game->player)) return false;
    Assets_PlayHit(game->assets);
    game->player.lives--;
    if (game->player.lives <= 0)
    {
        FinishRun(game);
        return true;
    }

    Player_Respawn(&game->player);
    ClearRespawnArea(game);
    return true;
}

static void FireWeapon(Game *game)
{
    Player *player = &game->player;
    Vector2 forward = Player_Forward(player);
    float cooldownMultiplier = OverdriveActive(game) ? 0.5f : 1.0f;
    Vector2 muzzle = Vector2Add(
        player->position, Vector2Scale(forward, 35.0f * GAMEPLAY_SCALE));

    if (player->weapon == WEAPON_LASER)
    {
        Vector2 velocity = Vector2Add(player->velocity, Vector2Scale(forward, 720.0f));
        if (Projectile_Spawn(game->projectiles, muzzle, velocity,
                             player->angle, 5.0f * GAMEPLAY_SCALE, 1.25f, WEAPON_LASER))
        {
            player->fireCooldown = 0.20f * cooldownMultiplier;
            Assets_PlayShot(game->assets, false);
        }
        return;
    }

    bool fired = false;
    const int pelletCount = 7;
    const float spread = 36.0f;
    for (int i = 0; i < pelletCount; ++i)
    {
        float offset = -spread / 2.0f + spread * (float)i / (float)(pelletCount - 1);
        float angle = player->angle + offset;
        float radians = angle * DEG2RAD;
        Vector2 direction = { cosf(radians), sinf(radians) };
        Vector2 velocity = Vector2Add(player->velocity, Vector2Scale(direction, 535.0f));
        fired |= Projectile_Spawn(game->projectiles, muzzle, velocity,
                                  angle, 5.0f * GAMEPLAY_SCALE, 0.48f, WEAPON_SHOTGUN);
    }
    if (fired)
    {
        player->fireCooldown = 0.74f * cooldownMultiplier;
        Assets_PlayShot(game->assets, true);
    }
}

static void UpdateAliens(Game *game, float dt)
{
    const DifficultyProfile *difficulty =
        Settings_DifficultyProfile(game->settings.difficulty);
    for (int i = 0; i < MAX_ALIENS; ++i)
    {
        Alien *alien = &game->aliens[i];
        if (!alien->active) continue;

        Vector2 aim = Vector2Normalize(Vector2Subtract(game->player.position, alien->position));
        alien->angle = atan2f(aim.y, aim.x) * RAD2DEG;

        if (!alien->stopped)
        {
            alien->position = Vector2Add(alien->position, Vector2Scale(alien->velocity, dt));
            alien->stopTimer -= dt;
            if (alien->stopTimer <= 0.0f)
            {
                alien->stopped = true;
                alien->velocity = (Vector2){ 0.0f, 0.0f };
            }
        }
        else
        {
            alien->fireCooldown -= dt;
            if (alien->fireCooldown <= 0.0f)
            {
                AlienProjectile_Spawn(game->alienProjectiles, alien->position,
                                      Vector2Scale(aim, 285.0f * difficulty->enemySpeedMultiplier));
                alien->fireCooldown =
                    (1.15f + (float)GetRandomValue(0, 55) / 100.0f) /
                    difficulty->enemySpeedMultiplier;
            }
        }
    }
}

static void HandleProjectileHits(Game *game)
{
    for (int p = 0; p < MAX_PROJECTILES; ++p)
    {
        if (!game->projectiles[p].active) continue;
        bool consumed = false;

        for (int a = 0; a < MAX_ASTEROIDS; ++a)
        {
            if (!game->asteroids[a].active) continue;
            if (!CheckCollisionCircles(game->projectiles[p].position,
                                       game->projectiles[p].radius,
                                       game->asteroids[a].position,
                                       game->asteroids[a].radius)) continue;

            game->projectiles[p].active = false;
            DestroyAsteroid(game, a, true);
            Assets_PlayHit(game->assets);
            consumed = true;
            break;
        }
        if (consumed) continue;

        for (int a = 0; a < MAX_ALIENS; ++a)
        {
            if (!game->aliens[a].active) continue;
            if (!CheckCollisionCircles(game->projectiles[p].position,
                                       game->projectiles[p].radius,
                                       game->aliens[a].position,
                                       game->aliens[a].radius)) continue;

            game->projectiles[p].active = false;
            DestroyAlien(game, a, true);
            Assets_PlayHit(game->assets);
            break;
        }
    }
}

static void HandlePlayerCollisions(Game *game)
{
    Player *player = &game->player;

    for (int i = 0; i < MAX_ALIEN_PROJECTILES; ++i)
    {
        if (!game->alienProjectiles[i].active) continue;
        if (!CheckCollisionCircles(player->position, PLAYER_RADIUS,
                                   game->alienProjectiles[i].position,
                                   game->alienProjectiles[i].radius)) continue;

        game->alienProjectiles[i].active = false;
        if (player->shieldTimer <= 0.0f && DamagePlayer(game)) return;
    }

    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (!game->asteroids[i].active) continue;
        if (!CheckCollisionCircles(player->position, PLAYER_RADIUS,
                                   game->asteroids[i].position,
                                   game->asteroids[i].radius)) continue;

        if (OverdriveActive(game))
        {
            DestroyAsteroid(game, i, true);
            Assets_PlayHit(game->assets);
        }
        else if (DamagePlayer(game)) return;
    }

    for (int i = 0; i < MAX_ALIENS; ++i)
    {
        if (!game->aliens[i].active) continue;
        if (!CheckCollisionCircles(player->position, PLAYER_RADIUS,
                                   game->aliens[i].position,
                                   game->aliens[i].radius)) continue;

        if (OverdriveActive(game))
        {
            DestroyAlien(game, i, true);
            Assets_PlayHit(game->assets);
        }
        else if (DamagePlayer(game)) return;
    }
}

static void UpdatePlaying(Game *game, float dt)
{
    const DifficultyProfile *difficulty =
        Settings_DifficultyProfile(game->settings.difficulty);
    if (IsKeyPressed(KEY_R))
    {
        StartRun(game);
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        CommitRunHighScore(game);
        SaveStats(game);
        game->runActive = false;
        game->screen = SCREEN_MENU;
        return;
    }

    if (game->godModeNoticeTimer > 0.0f)
        game->godModeNoticeTimer -= dt;

    if (IsKeyPressed(KEY_G))
    {
        game->godMode = !game->godMode;
        game->godModeUsedThisRun = true;
        game->godModeNoticeTimer = 2.0f;
    }

    if (IsKeyPressed(KEY_TAB))
        game->player.weapon = game->player.weapon == WEAPON_LASER ? WEAPON_SHOTGUN : WEAPON_LASER;

    if (IsKeyPressed(KEY_S) && game->player.shieldCharge >= SHIELD_MAX_CHARGE &&
        game->player.shieldTimer <= 0.0f)
    {
        game->player.shieldCharge = 0.0f;
        game->player.shieldTimer = SHIELD_DURATION;
    }

    Player_Update(&game->player, dt);
    if (IsKeyDown(KEY_SPACE) && game->player.fireCooldown <= 0.0f) FireWeapon(game);

    game->spawnTimer += dt;
    if (game->spawnTimer >= game->spawnInterval)
    {
        if (GetRandomValue(0, 3) == 0)
            Alien_Spawn(game->aliens, difficulty->enemySpeedMultiplier);
        else
            Asteroid_Spawn(game->asteroids, (AsteroidSize)GetRandomValue(0, 2),
                           difficulty->enemySpeedMultiplier);
        game->spawnTimer = 0.0f;
        game->spawnInterval *= difficulty->spawnAcceleration;
        if (game->spawnInterval < difficulty->minimumSpawnInterval)
            game->spawnInterval = difficulty->minimumSpawnInterval;
    }

    Projectiles_Update(game->projectiles, dt);
    Asteroids_Update(game->asteroids, dt);
    UpdateAliens(game, dt);
    AlienProjectiles_Update(game->alienProjectiles, dt);
    HandleProjectileHits(game);
    HandlePlayerCollisions(game);
}

static void UpdateMenu(Game *game)
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        game->menuSelection = (game->menuSelection + 3) % 4;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        game->menuSelection = (game->menuSelection + 1) % 4;

    if (!IsKeyPressed(KEY_ENTER)) return;
    if (game->menuSelection == 0) StartRun(game);
    else if (game->menuSelection == 1) game->screen = SCREEN_SETTINGS;
    else if (game->menuSelection == 2) game->screen = SCREEN_STATS;
    else game->quitRequested = true;
}

static void ChangeSetting(Game *game, int direction)
{
    switch (game->settingsSelection)
    {
        case 0:
            game->settings.musicEnabled = !game->settings.musicEnabled;
            break;
        case 1:
            game->settings.sfxEnabled = !game->settings.sfxEnabled;
            break;
        case 3:
            game->settings.shipDesign = (ShipDesign)(
                ((int)game->settings.shipDesign + direction + SHIP_DESIGN_COUNT) %
                SHIP_DESIGN_COUNT);
            break;
        case 4:
            game->settings.difficulty = (DifficultyLevel)(
                ((int)game->settings.difficulty + direction + DIFFICULTY_COUNT) %
                DIFFICULTY_COUNT);
            break;
        default:
            break;
    }

    Assets_ApplySettings(game->assets, &game->settings);
    SaveSettings(game);
}

static void UpdateSettings(Game *game)
{
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
    {
        SaveSettings(game);
        game->screen = SCREEN_MENU;
        return;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        game->settingsSelection = (game->settingsSelection + 5) % 6;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        game->settingsSelection = (game->settingsSelection + 1) % 6;

    int direction = 0;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) direction = -1;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) direction = 1;

    if (direction != 0 && game->settingsSelection != 2 && game->settingsSelection < 5)
        ChangeSetting(game, direction);

    if (IsKeyPressed(KEY_ENTER))
    {
        if (game->settingsSelection == 5)
        {
            SaveSettings(game);
            game->screen = SCREEN_MENU;
        }
        else if (game->settingsSelection == 2)
        {
            BeginPilotNameEntry(game, true);
        }
        else
        {
            ChangeSetting(game, 1);
        }
    }
}

static void UpdatePilotName(Game *game)
{
    int character = GetCharPressed();
    while (character > 0)
    {
        bool allowed = (character >= 'A' && character <= 'Z') ||
                       (character >= 'a' && character <= 'z') ||
                       (character >= '0' && character <= '9') ||
                       character == ' ' || character == '-' || character == '_';
        if (allowed && game->nameInputLength < PILOT_NAME_MAX)
        {
            game->nameInput[game->nameInputLength++] = (char)character;
            game->nameInput[game->nameInputLength] = '\0';
        }
        character = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && game->nameInputLength > 0)
    {
        game->nameInput[--game->nameInputLength] = '\0';
    }

    if (game->namingFromSettings && IsKeyPressed(KEY_ESCAPE))
    {
        game->screen = SCREEN_SETTINGS;
        return;
    }

    if (!IsKeyPressed(KEY_ENTER)) return;
    TrimPilotName(game->nameInput);
    game->nameInputLength = (int)strlen(game->nameInput);
    if (game->nameInputLength == 0) return;

    strncpy(game->stats.pilotName, game->nameInput, PILOT_NAME_MAX);
    game->stats.pilotName[PILOT_NAME_MAX] = '\0';
    SaveStats(game);
    game->screen = game->namingFromSettings ? SCREEN_SETTINGS : SCREEN_MENU;
}

void Game_Init(Game *game, GameAssets *assets)
{
    memset(game, 0, sizeof(*game));
    game->assets = assets;
    Stats_Load(&game->stats, STATS_FILE);
    Settings_Load(&game->settings, SETTINGS_FILE);
    Assets_ApplySettings(game->assets, &game->settings);
    if (!Stats_HasProgress(&game->stats) && game->stats.pilotName[0] == '\0')
    {
        BeginPilotNameEntry(game, false);
    }
    else
    {
        if (game->stats.pilotName[0] == '\0')
        {
            strncpy(game->stats.pilotName, "PILOT", PILOT_NAME_MAX);
            game->stats.pilotName[PILOT_NAME_MAX] = '\0';
        }
        game->screen = SCREEN_MENU;
    }
    SetRandomSeed((unsigned int)time(NULL));
}

void Game_Update(Game *game)
{
    Assets_Update(game->assets);
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    switch (game->screen)
    {
        case SCREEN_MENU:
            UpdateMenu(game);
            break;
        case SCREEN_SETTINGS:
            UpdateSettings(game);
            break;
        case SCREEN_STATS:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_BACKSPACE))
                game->screen = SCREEN_MENU;
            break;
        case SCREEN_PLAYING:
            UpdatePlaying(game, dt);
            break;
        case SCREEN_GAME_OVER:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) StartRun(game);
            else if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) game->screen = SCREEN_MENU;
            else if (IsKeyPressed(KEY_S)) game->screen = SCREEN_STATS;
            break;
        case SCREEN_PILOT_NAME:
            UpdatePilotName(game);
            break;
    }
}

static void DrawBackdrop(const Game *game)
{
    DrawTexturePro(game->assets->background,
                   (Rectangle){ 0, 0, (float)game->assets->background.width,
                                (float)game->assets->background.height },
                   (Rectangle){ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT },
                   (Vector2){ 0, 0 }, 0.0f, WHITE);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 3, 8, 22, 55 });
}

static void DrawHud(const Game *game)
{
    const DifficultyProfile *difficulty =
        Settings_DifficultyProfile(game->settings.difficulty);
    DrawText(TextFormat("SCORE  %06d", game->score), 22, 18, 24, RAYWHITE);
    DrawText(TextFormat("%s  SCORE x%.2f", difficulty->name,
                        difficulty->scoreMultiplier),
             22, 48, 17, difficulty->scoreMultiplier > 1.25f ? ORANGE : SKYBLUE);
    DrawText(TextFormat("HIGH  %06d", game->stats.highScore),
             SCREEN_WIDTH / 2 - 86, 18, 22, GOLD);

    DrawTextureCentered(game->assets->playerShips[game->settings.shipDesign],
                        (Vector2){ SCREEN_WIDTH - 92.0f, 36.0f },
                        44.0f, 44.0f, 0.0f, WHITE);
    if (game->godMode)
    {
        // Draw the symbol ourselves so it works with raylib's built-in font,
        // which may not contain the Unicode infinity glyph.
        DrawText("x", SCREEN_WIDTH - 66, 23, 24, YELLOW);
        Vector2 previous = { 0.0f, 0.0f };
        for (int i = 0; i <= 48; ++i)
        {
            float t = 6.28318530718f * (float)i / 48.0f;
            float denominator = 1.0f + sinf(t) * sinf(t);
            Vector2 point = {
                SCREEN_WIDTH - 28.0f + 24.0f * cosf(t) / denominator,
                36.0f + 24.0f * sinf(t) * cosf(t) / denominator
            };
            if (i > 0) DrawLineEx(previous, point, 3.0f, YELLOW);
            previous = point;
        }
    }
    else
        DrawText(TextFormat("x%d", game->player.lives),
                 SCREEN_WIDTH - 62, 23, 26, RAYWHITE);

    const int barX = SCREEN_WIDTH - 286;
    const int barY = SCREEN_HEIGHT - 57;
    const int barWidth = 250;
    DrawText("ALIEN CORE", barX, barY - 23, 18, SKYBLUE);
    DrawRectangle(barX, barY, barWidth, 16, (Color){ 15, 31, 55, 230 });
    bool overdriveActive = OverdriveActive(game);
    float charge = game->godMode ? 1.0f : ShieldBarLevel(&game->player);
    DrawRectangle(barX + 2, barY + 2, (int)((barWidth - 4) * charge), 12,
                  overdriveActive || charge >= 1.0f ? YELLOW : SKYBLUE);
    DrawRectangleLines(barX, barY, barWidth, 16, LIGHTGRAY);

    if (game->godMode)
        DrawText("GOD MODE   PERMANENT OVERDRIVE",
                 barX - 18, barY - 47, 16, YELLOW);
    else if (overdriveActive)
        DrawText("OVERDRIVE!   2x FIRE   1.5x SCORE",
                 barX - 6, barY - 47, 16, YELLOW);
    else if (charge >= 1.0f)
        DrawText("SHIELD READY - PRESS S", barX + 8, barY - 47, 18, YELLOW);

    const char *weapon = game->player.weapon == WEAPON_LASER ? "LASER" : "SCATTERSHOT";
    DrawText(TextFormat("WEAPON: %s  [TAB]", weapon), 22, SCREEN_HEIGHT - 52, 20,
             game->player.weapon == WEAPON_LASER ? SKYBLUE : ORANGE);
    DrawText("TURN A/D or arrows   THRUST W/up   FIRE space   RESTART R   MENU Esc",
             22, SCREEN_HEIGHT - 26, 16, LIGHTGRAY);

    if (game->godModeNoticeTimer > 0.0f)
        DrawCenteredText(game->godMode ? "GOD MODE: ONLINE" : "GOD MODE: OFFLINE",
                         88, 24, game->godMode ? YELLOW : LIGHTGRAY);
}

static void DrawPlaying(const Game *game)
{
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        const Asteroid *asteroid = &game->asteroids[i];
        if (!asteroid->active) continue;
        float diameter = asteroid->radius * 2.18f;
        DrawTextureCentered(game->assets->asteroid, asteroid->position,
                            diameter, diameter, asteroid->rotation, WHITE);
    }

    for (int i = 0; i < MAX_ALIENS; ++i)
    {
        const Alien *alien = &game->aliens[i];
        if (!alien->active) continue;
        DrawTextureCentered(game->assets->alienShip, alien->position,
                            86.0f * GAMEPLAY_SCALE, 86.0f * GAMEPLAY_SCALE,
                            alien->angle - 90.0f, WHITE);
    }

    for (int i = 0; i < MAX_PROJECTILES; ++i)
    {
        const Projectile *projectile = &game->projectiles[i];
        if (!projectile->active) continue;
        if (projectile->weapon == WEAPON_LASER)
            DrawTextureCentered(game->assets->laser, projectile->position,
                                14.0f * GAMEPLAY_SCALE, 30.0f * GAMEPLAY_SCALE,
                                projectile->angle + 90.0f, WHITE);
        else
            DrawTextureCentered(game->assets->shotgunPellet, projectile->position,
                                13.0f * GAMEPLAY_SCALE, 13.0f * GAMEPLAY_SCALE,
                                projectile->angle + 90.0f, WHITE);
    }

    for (int i = 0; i < MAX_ALIEN_PROJECTILES; ++i)
    {
        if (!game->alienProjectiles[i].active) continue;
        DrawCircleV(game->alienProjectiles[i].position, 8.0f * GAMEPLAY_SCALE,
                    (Color){ 248, 55, 79, 100 });
        DrawCircleV(game->alienProjectiles[i].position, 4.5f * GAMEPLAY_SCALE,
                    (Color){ 255, 120, 82, 255 });
    }

    bool blinkOff = game->player.respawnInvincibility > 0.0f &&
                    ((int)(game->player.respawnInvincibility * 10.0f) % 2 == 0);
    if (!blinkOff)
        DrawTextureCentered(game->assets->playerShips[game->settings.shipDesign],
                            game->player.position,
                            68.0f * GAMEPLAY_SCALE, 68.0f * GAMEPLAY_SCALE,
                            game->player.angle + 90.0f, WHITE);

    if (OverdriveActive(game))
    {
        float pulse = (103.0f + sinf((float)GetTime() * 8.0f) * 5.0f) *
                      GAMEPLAY_SCALE;
        DrawTextureCentered(game->assets->shieldAura, game->player.position,
                            pulse, pulse, (float)GetTime() * 18.0f,
                            (Color){ 255, 255, 255, 220 });
    }

    DrawHud(game);
}

static void DrawMenu(const Game *game)
{
    const DifficultyProfile *difficulty =
        Settings_DifficultyProfile(game->settings.difficulty);
    DrawRectangle(SCREEN_WIDTH / 2 - 340, 85, 680, 550, (Color){ 5, 11, 28, 205 });
    DrawRectangleLines(SCREEN_WIDTH / 2 - 340, 85, 680, 550, (Color){ 61, 192, 230, 180 });
    DrawCenteredText("METEOR MAYHEM", 132, 58, RAYWHITE);
    DrawCenteredText(TextFormat("WELCOME BACK, %s %s", difficulty->name,
                                PilotDisplayName(game)),
                     205, 22, SKYBLUE);
    DrawCenteredText(TextFormat("THE %s IS FUELED AND WAITING FOR YOU.",
                                Settings_ShipName(game->settings.shipDesign)),
                     238, 17, LIGHTGRAY);
    DrawCenteredText(TextFormat("HIGH SCORE  %06d", game->stats.highScore), 280, 25, GOLD);

    const char *items[] = { "START GAME", "SETTINGS", "STATISTICS", "QUIT" };
    for (int i = 0; i < 4; ++i)
    {
        Color color = game->menuSelection == i ? YELLOW : LIGHTGRAY;
        const char *prefix = game->menuSelection == i ? ">  " : "   ";
        DrawCenteredText(TextFormat("%s%s", prefix, items[i]), 338 + i * 50, 28, color);
    }
    DrawCenteredText("UP/DOWN TO NAVIGATE   ENTER TO SELECT", 570, 18, GRAY);
}

static void DrawSettingsRow(const Game *game, int index, int y,
                            const char *label, const char *value)
{
    bool selected = game->settingsSelection == index;
    Color color = selected ? YELLOW : LIGHTGRAY;
    DrawText(selected ? ">" : " ", 345, y, 26, color);
    DrawText(label, 385, y, 24, color);
    int width = MeasureText(value, 24);
    DrawText(value, 885 - width, y, 24, selected ? RAYWHITE : GRAY);
}

static void DrawSettingsScreen(const Game *game)
{
    const DifficultyProfile *difficulty =
        Settings_DifficultyProfile(game->settings.difficulty);
    DrawRectangle(SCREEN_WIDTH / 2 - 440, 52, 880, 625, (Color){ 5, 11, 28, 225 });
    DrawRectangleLines(SCREEN_WIDTH / 2 - 440, 52, 880, 625, SKYBLUE);
    DrawCenteredText("SETTINGS", 80, 46, RAYWHITE);
    DrawCenteredText("LEFT / RIGHT TO CHANGE", 133, 17, SKYBLUE);

    DrawTextureCentered(game->assets->playerShips[game->settings.shipDesign],
                        (Vector2){ SCREEN_WIDTH / 2.0f, 205.0f },
                        96.0f, 96.0f, 0.0f, WHITE);
    DrawCenteredText(Settings_ShipName(game->settings.shipDesign), 254, 19, GOLD);

    DrawSettingsRow(game, 0, 300, "MUSIC",
                    game->settings.musicEnabled ? "<  ON  >" : "<  OFF  >");
    DrawSettingsRow(game, 1, 344, "SOUND EFFECTS",
                    game->settings.sfxEnabled ? "<  ON  >" : "<  OFF  >");
    DrawSettingsRow(game, 2, 388, "PILOT NAME",
                    TextFormat("[  %s  ]", PilotDisplayName(game)));
    DrawSettingsRow(game, 3, 432, "SHIP DESIGN",
                    TextFormat("<  %s  >", Settings_ShipName(game->settings.shipDesign)));
    DrawSettingsRow(game, 4, 476, "DIFFICULTY",
                    TextFormat("<  %s  >", difficulty->name));

    bool backSelected = game->settingsSelection == 5;
    DrawCenteredText(backSelected ? ">  BACK" : "BACK", 526, 25,
                     backSelected ? YELLOW : LIGHTGRAY);

    DrawCenteredText(TextFormat("Spawn %.2fs -> %.2fs  |  Enemy speed x%.2f",
                                difficulty->initialSpawnInterval,
                                difficulty->minimumSpawnInterval,
                                difficulty->enemySpeedMultiplier),
                     574, 16, LIGHTGRAY);
    DrawCenteredText(TextFormat("Alien core +%.0f%%  |  Score x%.2f",
                                difficulty->shieldChargePerAlien,
                                difficulty->scoreMultiplier),
                     602, 17, difficulty->scoreMultiplier > 1.25f ? ORANGE : SKYBLUE);
    DrawCenteredText("ENTER TO CHANGE/SELECT   ESC TO RETURN", 640, 15, GRAY);
}

static void DrawPilotNameScreen(const Game *game)
{
    DrawRectangle(SCREEN_WIDTH / 2 - 390, 120, 780, 470, (Color){ 5, 11, 28, 225 });
    DrawRectangleLines(SCREEN_WIDTH / 2 - 390, 120, 780, 470, SKYBLUE);
    DrawCenteredText(game->namingFromSettings ? "CHANGE PILOT NAME" : "WELCOME, NEW PILOT",
                     165, 44, RAYWHITE);
    DrawCenteredText(game->namingFromSettings
                         ? "WHAT SHOULD MISSION CONTROL CALL YOU?"
                         : "BEFORE LAUNCH, MISSION CONTROL NEEDS YOUR NAME.",
                     225, 18, SKYBLUE);

    const int boxX = SCREEN_WIDTH / 2 - 255;
    const int boxY = 295;
    const int boxWidth = 510;
    DrawRectangle(boxX, boxY, boxWidth, 72, (Color){ 12, 25, 48, 245 });
    DrawRectangleLines(boxX, boxY, boxWidth, 72, YELLOW);
    const char *shownName = game->nameInputLength > 0 ? game->nameInput : "TYPE YOUR NAME";
    Color nameColor = game->nameInputLength > 0 ? RAYWHITE : GRAY;
    DrawCenteredText(shownName, boxY + 20, 28, nameColor);

    bool cursorVisible = ((int)(GetTime() * 2.0) % 2) == 0;
    if (cursorVisible && game->nameInputLength > 0)
    {
        int textWidth = MeasureText(game->nameInput, 28);
        DrawRectangle(SCREEN_WIDTH / 2 + textWidth / 2 + 4, boxY + 19, 3, 34, YELLOW);
    }

    DrawCenteredText(TextFormat("%d / %d CHARACTERS", game->nameInputLength, PILOT_NAME_MAX),
                     390, 16, GRAY);
    DrawCenteredText("LETTERS, NUMBERS, SPACES, - AND _", 430, 16, LIGHTGRAY);
    DrawCenteredText("PRESS ENTER TO CONFIRM", 485, 22, YELLOW);
    if (game->namingFromSettings)
        DrawCenteredText("ESC TO CANCEL", 525, 16, GRAY);
}

static void DrawStatsScreen(const Game *game)
{
    DrawRectangle(SCREEN_WIDTH / 2 - 390, 72, 780, 575, (Color){ 5, 11, 28, 220 });
    DrawRectangleLines(SCREEN_WIDTH / 2 - 390, 72, 780, 575, SKYBLUE);
    DrawCenteredText("PILOT STATISTICS", 108, 46, RAYWHITE);
    DrawCenteredText(TextFormat("%s'S ALL-TIME COMBAT RECORD", PilotDisplayName(game)),
                     162, 18, SKYBLUE);

    const int left = SCREEN_WIDTH / 2 - 260;
    const int right = SCREEN_WIDTH / 2 + 260;
    const int startY = 215;
    const int gap = 54;
    const char *labels[] = {
        "REAL HIGH SCORE", "GOD-MODE HIGH SCORE", "GAMES PLAYED", "ASTEROIDS SMASHED",
        "ALIENS SMASHED", "TOTAL ACCUMULATED POINTS"
    };
    for (int i = 0; i < 6; ++i)
    {
        int y = startY + i * gap;
        const char *value = "0";
        if (i == 0) value = TextFormat("%d", game->stats.highScore);
        else if (i == 1) value = TextFormat("%d", game->stats.godModeHighScore);
        else if (i == 2) value = TextFormat("%llu", (unsigned long long)game->stats.gamesPlayed);
        else if (i == 3) value = TextFormat("%llu", (unsigned long long)game->stats.asteroidsSmashed);
        else if (i == 4) value = TextFormat("%llu", (unsigned long long)game->stats.aliensSmashed);
        else value = TextFormat("%llu", (unsigned long long)game->stats.totalPoints);

        DrawText(labels[i], left, y, 22, LIGHTGRAY);
        int valueWidth = MeasureText(value, 25);
        Color valueColor = i == 0 ? GOLD : (i == 1 ? ORANGE : RAYWHITE);
        DrawText(value, right - valueWidth, y - 2, 25, valueColor);
        DrawLine(left, y + 36, right, y + 36, (Color){ 70, 101, 132, 130 });
    }
    DrawCenteredText("ENTER / ESC / BACKSPACE TO RETURN", 602, 17, GRAY);
}

static void DrawGameOver(const Game *game)
{
    DrawRectangle(SCREEN_WIDTH / 2 - 330, 125, 660, 465, (Color){ 8, 7, 20, 220 });
    DrawRectangleLines(SCREEN_WIDTH / 2 - 330, 125, 660, 465, RED);
    DrawCenteredText("MISSION OVER", 172, 55, RED);
    DrawCenteredText(TextFormat("SCORE  %06d", game->score), 272, 32, RAYWHITE);
    DrawCenteredText(TextFormat("HIGH SCORE  %06d", game->stats.highScore), 320, 26, GOLD);
    DrawCenteredText("ENTER / R  RESTART", 410, 24, YELLOW);
    DrawCenteredText("S  STATISTICS", 450, 22, LIGHTGRAY);
    DrawCenteredText("M / ESC  MAIN MENU", 490, 22, LIGHTGRAY);
}

void Game_Draw(const Game *game)
{
    BeginDrawing();
    ClearBackground(BLACK);
    DrawBackdrop(game);

    switch (game->screen)
    {
        case SCREEN_MENU: DrawMenu(game); break;
        case SCREEN_PLAYING: DrawPlaying(game); break;
        case SCREEN_SETTINGS: DrawSettingsScreen(game); break;
        case SCREEN_STATS: DrawStatsScreen(game); break;
        case SCREEN_GAME_OVER: DrawGameOver(game); break;
        case SCREEN_PILOT_NAME: DrawPilotNameScreen(game); break;
    }

    EndDrawing();
}

void Game_Shutdown(Game *game)
{
    CommitRunHighScore(game);
    SaveStats(game);
    SaveSettings(game);
}

bool Game_ShouldQuit(const Game *game)
{
    return game->quitRequested;
}
