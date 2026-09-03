#ifndef METEOR_MAYHEM_ENTITIES_H
#define METEOR_MAYHEM_ENTITIES_H

#include "config.h"
#include "raylib.h"

typedef enum WeaponType {
    WEAPON_LASER = 0,
    WEAPON_SHOTGUN = 1
} WeaponType;

typedef enum AsteroidSize {
    ASTEROID_SMALL = 0,
    ASTEROID_MEDIUM = 1,
    ASTEROID_LARGE = 2
} AsteroidSize;

typedef struct Player {
    Vector2 position;
    Vector2 velocity;
    float angle;
    float fireCooldown;
    float respawnInvincibility;
    float shieldCharge;
    float shieldTimer;
    int lives;
    WeaponType weapon;
} Player;

typedef struct Projectile {
    Vector2 position;
    Vector2 velocity;
    float angle;
    float radius;
    float lifetime;
    WeaponType weapon;
    bool active;
} Projectile;

typedef struct Asteroid {
    Vector2 position;
    Vector2 velocity;
    float radius;
    float rotation;
    float spin;
    AsteroidSize size;
    bool active;
} Asteroid;

typedef struct Alien {
    Vector2 position;
    Vector2 velocity;
    float angle;
    float radius;
    float stopTimer;
    float fireCooldown;
    bool stopped;
    bool active;
} Alien;

typedef struct AlienProjectile {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float radius;
    bool active;
} AlienProjectile;

void Player_StartRun(Player *player);
void Player_Respawn(Player *player);
void Player_Update(Player *player, float dt);
Vector2 Player_Forward(const Player *player);
bool Player_IsInvincible(const Player *player);

void Entities_Clear(Projectile *projectiles, Asteroid *asteroids,
                    Alien *aliens, AlienProjectile *alienProjectiles);
bool Projectile_Spawn(Projectile *projectiles, Vector2 position, Vector2 velocity,
                      float angle, float radius, float lifetime, WeaponType weapon);
void Projectiles_Update(Projectile *projectiles, float dt);

bool Asteroid_Spawn(Asteroid *asteroids, AsteroidSize size, float speedMultiplier);
bool Asteroid_SpawnFragment(Asteroid *asteroids, AsteroidSize size,
                            Vector2 position, float angleOffset, float speedMultiplier);
void Asteroids_Update(Asteroid *asteroids, float dt);

bool Alien_Spawn(Alien *aliens, float speedMultiplier);
bool AlienProjectile_Spawn(AlienProjectile *projectiles, Vector2 position,
                           Vector2 velocity);
void AlienProjectiles_Update(AlienProjectile *projectiles, float dt);

#endif
