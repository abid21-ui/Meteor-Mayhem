#include "entities.h"

#include "raymath.h"

#include <math.h>
#include <string.h>

static float RandomFloat(float minimum, float maximum)
{
    float amount = (float)GetRandomValue(0, 10000) / 10000.0f;
    return minimum + (maximum - minimum) * amount;
}

static Vector2 RandomEdgePosition(float margin)
{
    switch (GetRandomValue(0, 3))
    {
        case 0: return (Vector2){ RandomFloat(0, SCREEN_WIDTH), -margin };
        case 1: return (Vector2){ RandomFloat(0, SCREEN_WIDTH), SCREEN_HEIGHT + margin };
        case 2: return (Vector2){ -margin, RandomFloat(0, SCREEN_HEIGHT) };
        default: return (Vector2){ SCREEN_WIDTH + margin, RandomFloat(0, SCREEN_HEIGHT) };
    }
}

void Player_StartRun(Player *player)
{
    memset(player, 0, sizeof(*player));
    player->lives = STARTING_LIVES;
    player->weapon = WEAPON_LASER;
    Player_Respawn(player);
}

void Player_Respawn(Player *player)
{
    player->position = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    player->velocity = (Vector2){ 0.0f, 0.0f };
    player->angle = -90.0f;
    player->fireCooldown = 0.0f;
    player->respawnInvincibility = 2.0f;
    player->shieldTimer = 0.0f;
}

Vector2 Player_Forward(const Player *player)
{
    float radians = player->angle * DEG2RAD;
    return (Vector2){ cosf(radians), sinf(radians) };
}

bool Player_IsInvincible(const Player *player)
{
    return player->shieldTimer > 0.0f || player->respawnInvincibility > 0.0f;
}

void Player_Update(Player *player, float dt)
{
    const float rotationSpeed = 220.0f;
    const float acceleration = 310.0f;
    const float maximumSpeed = 360.0f;

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player->angle -= rotationSpeed * dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player->angle += rotationSpeed * dt;

    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
    {
        player->velocity = Vector2Add(
            player->velocity,
            Vector2Scale(Player_Forward(player), acceleration * dt));
    }

    float speed = Vector2Length(player->velocity);
    if (speed > maximumSpeed)
        player->velocity = Vector2Scale(Vector2Normalize(player->velocity), maximumSpeed);

    player->velocity = Vector2Scale(player->velocity, powf(0.992f, dt * 60.0f));
    player->position = Vector2Add(player->position, Vector2Scale(player->velocity, dt));

    const float margin = 26.0f * GAMEPLAY_SCALE;
    if (player->position.x < -margin) player->position.x = SCREEN_WIDTH + margin;
    if (player->position.x > SCREEN_WIDTH + margin) player->position.x = -margin;
    if (player->position.y < -margin) player->position.y = SCREEN_HEIGHT + margin;
    if (player->position.y > SCREEN_HEIGHT + margin) player->position.y = -margin;

    if (player->fireCooldown > 0.0f) player->fireCooldown -= dt;
    if (player->respawnInvincibility > 0.0f) player->respawnInvincibility -= dt;
    if (player->shieldTimer > 0.0f)
    {
        player->shieldTimer -= dt;
        if (player->shieldTimer <= 0.0f)
        {
            player->shieldTimer = 0.0f;
            if (player->respawnInvincibility < SHIELD_END_INVINCIBILITY)
                player->respawnInvincibility = SHIELD_END_INVINCIBILITY;
        }
    }
}

void Entities_Clear(Projectile *projectiles, Asteroid *asteroids,
                    Alien *aliens, AlienProjectile *alienProjectiles)
{
    memset(projectiles, 0, sizeof(Projectile) * MAX_PROJECTILES);
    memset(asteroids, 0, sizeof(Asteroid) * MAX_ASTEROIDS);
    memset(aliens, 0, sizeof(Alien) * MAX_ALIENS);
    memset(alienProjectiles, 0, sizeof(AlienProjectile) * MAX_ALIEN_PROJECTILES);
}

bool Projectile_Spawn(Projectile *projectiles, Vector2 position, Vector2 velocity,
                      float angle, float radius, float lifetime, WeaponType weapon)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i)
    {
        if (projectiles[i].active) continue;
        projectiles[i] = (Projectile){
            .position = position,
            .velocity = velocity,
            .angle = angle,
            .radius = radius,
            .lifetime = lifetime,
            .weapon = weapon,
            .active = true
        };
        return true;
    }
    return false;
}

void Projectiles_Update(Projectile *projectiles, float dt)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i)
    {
        if (!projectiles[i].active) continue;
        projectiles[i].position = Vector2Add(
            projectiles[i].position, Vector2Scale(projectiles[i].velocity, dt));
        projectiles[i].lifetime -= dt;
        if (projectiles[i].lifetime <= 0.0f) projectiles[i].active = false;
    }
}

static float AsteroidRadius(AsteroidSize size)
{
    if (size == ASTEROID_SMALL) return 18.0f * GAMEPLAY_SCALE;
    if (size == ASTEROID_MEDIUM) return 33.0f * GAMEPLAY_SCALE;
    return 58.0f * GAMEPLAY_SCALE;
}

static float AsteroidSpeed(AsteroidSize size)
{
    if (size == ASTEROID_SMALL) return RandomFloat(130.0f, 180.0f);
    if (size == ASTEROID_MEDIUM) return RandomFloat(90.0f, 135.0f);
    return RandomFloat(60.0f, 95.0f);
}

bool Asteroid_Spawn(Asteroid *asteroids, AsteroidSize size, float speedMultiplier)
{
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (asteroids[i].active) continue;
        float radius = AsteroidRadius(size);
        Vector2 position = RandomEdgePosition(radius + 8.0f);
        Vector2 target = {
            SCREEN_WIDTH / 2.0f + RandomFloat(-260.0f, 260.0f),
            SCREEN_HEIGHT / 2.0f + RandomFloat(-180.0f, 180.0f)
        };
        Vector2 direction = Vector2Normalize(Vector2Subtract(target, position));
        asteroids[i] = (Asteroid){
            .position = position,
            .velocity = Vector2Scale(direction, AsteroidSpeed(size) * speedMultiplier),
            .radius = radius,
            .rotation = RandomFloat(0.0f, 360.0f),
            .spin = RandomFloat(-42.0f, 42.0f),
            .size = size,
            .active = true
        };
        return true;
    }
    return false;
}

bool Asteroid_SpawnFragment(Asteroid *asteroids, AsteroidSize size,
                            Vector2 position, float angleOffset, float speedMultiplier)
{
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (asteroids[i].active) continue;
        float radians = angleOffset * DEG2RAD;
        Vector2 direction = { cosf(radians), sinf(radians) };
        asteroids[i] = (Asteroid){
            .position = position,
            .velocity = Vector2Scale(direction, AsteroidSpeed(size) * speedMultiplier),
            .radius = AsteroidRadius(size),
            .rotation = RandomFloat(0.0f, 360.0f),
            .spin = RandomFloat(-70.0f, 70.0f),
            .size = size,
            .active = true
        };
        return true;
    }
    return false;
}

void Asteroids_Update(Asteroid *asteroids, float dt)
{
    const float margin = 120.0f * GAMEPLAY_SCALE;
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (!asteroids[i].active) continue;
        asteroids[i].position = Vector2Add(
            asteroids[i].position, Vector2Scale(asteroids[i].velocity, dt));
        asteroids[i].rotation += asteroids[i].spin * dt;
        if (asteroids[i].position.x < -margin || asteroids[i].position.x > SCREEN_WIDTH + margin ||
            asteroids[i].position.y < -margin || asteroids[i].position.y > SCREEN_HEIGHT + margin)
            asteroids[i].active = false;
    }
}

bool Alien_Spawn(Alien *aliens, float speedMultiplier)
{
    for (int i = 0; i < MAX_ALIENS; ++i)
    {
        if (aliens[i].active) continue;

        const float spawnMargin = 54.0f * GAMEPLAY_SCALE;
        const float stopInset = 110.0f;
        int edge = GetRandomValue(0, 3);
        Vector2 position = { 0.0f, 0.0f };
        Vector2 target = { 0.0f, 0.0f };

        if (edge == 0)
        {
            float x = RandomFloat(stopInset, SCREEN_WIDTH - stopInset);
            position = (Vector2){ x, -spawnMargin };
            target = (Vector2){ x, stopInset };
        }
        else if (edge == 1)
        {
            float x = RandomFloat(stopInset, SCREEN_WIDTH - stopInset);
            position = (Vector2){ x, SCREEN_HEIGHT + spawnMargin };
            target = (Vector2){ x, SCREEN_HEIGHT - stopInset };
        }
        else if (edge == 2)
        {
            float y = RandomFloat(stopInset, SCREEN_HEIGHT - stopInset);
            position = (Vector2){ -spawnMargin, y };
            target = (Vector2){ stopInset, y };
        }
        else
        {
            float y = RandomFloat(stopInset, SCREEN_HEIGHT - stopInset);
            position = (Vector2){ SCREEN_WIDTH + spawnMargin, y };
            target = (Vector2){ SCREEN_WIDTH - stopInset, y };
        }

        Vector2 direction = Vector2Normalize(Vector2Subtract(target, position));
        float speed = 125.0f * speedMultiplier;
        float travelTime = Vector2Distance(position, target) / speed;
        aliens[i] = (Alien){
            .position = position,
            .velocity = Vector2Scale(direction, speed),
            .angle = atan2f(direction.y, direction.x) * RAD2DEG,
            .radius = 38.0f * GAMEPLAY_SCALE,
            .stopTimer = travelTime,
            .fireCooldown = RandomFloat(0.8f, 1.6f) / speedMultiplier,
            .stopped = false,
            .active = true
        };
        return true;
    }
    return false;
}

bool AlienProjectile_Spawn(AlienProjectile *projectiles, Vector2 position,
                           Vector2 velocity)
{
    for (int i = 0; i < MAX_ALIEN_PROJECTILES; ++i)
    {
        if (projectiles[i].active) continue;
        projectiles[i] = (AlienProjectile){
            .position = position,
            .velocity = velocity,
            .lifetime = 4.0f,
            .radius = 6.0f * GAMEPLAY_SCALE,
            .active = true
        };
        return true;
    }
    return false;
}

void AlienProjectiles_Update(AlienProjectile *projectiles, float dt)
{
    for (int i = 0; i < MAX_ALIEN_PROJECTILES; ++i)
    {
        if (!projectiles[i].active) continue;
        projectiles[i].position = Vector2Add(
            projectiles[i].position, Vector2Scale(projectiles[i].velocity, dt));
        projectiles[i].lifetime -= dt;
        if (projectiles[i].lifetime <= 0.0f) projectiles[i].active = false;
    }
}
