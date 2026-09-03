#ifndef METEOR_MAYHEM_SETTINGS_H
#define METEOR_MAYHEM_SETTINGS_H

#include <stdbool.h>

typedef enum ShipDesign {
    SHIP_RANGER = 0,
    SHIP_STRIKER,
    SHIP_COMET,
    SHIP_DESIGN_COUNT
} ShipDesign;

typedef enum DifficultyLevel {
    DIFFICULTY_ROOKIE = 0,
    DIFFICULTY_PILOT,
    DIFFICULTY_ACE,
    DIFFICULTY_COUNT
} DifficultyLevel;

typedef struct GameSettings {
    bool musicEnabled;
    bool sfxEnabled;
    ShipDesign shipDesign;
    DifficultyLevel difficulty;
} GameSettings;

typedef struct DifficultyProfile {
    const char *name;
    float initialSpawnInterval;
    float minimumSpawnInterval;
    float spawnAcceleration;
    float enemySpeedMultiplier;
    float shieldChargePerAlien;
    float scoreMultiplier;
} DifficultyProfile;

void Settings_Default(GameSettings *settings);
void Settings_Load(GameSettings *settings, const char *path);
bool Settings_Save(const GameSettings *settings, const char *path);
const char *Settings_ShipName(ShipDesign design);
const DifficultyProfile *Settings_DifficultyProfile(DifficultyLevel difficulty);

#endif
