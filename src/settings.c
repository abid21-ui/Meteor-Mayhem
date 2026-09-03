#include "settings.h"

#include <stdio.h>
#include <string.h>

static const DifficultyProfile DIFFICULTIES[DIFFICULTY_COUNT] = {
    {
        .name = "ROOKIE",
        .initialSpawnInterval = 1.70f,
        .minimumSpawnInterval = 0.50f,
        .spawnAcceleration = 0.996f,
        .enemySpeedMultiplier = 0.90f,
        .shieldChargePerAlien = 45.0f,
        .scoreMultiplier = 1.00f
    },
    {
        .name = "PILOT",
        .initialSpawnInterval = 1.35f,
        .minimumSpawnInterval = 0.34f,
        .spawnAcceleration = 0.992f,
        .enemySpeedMultiplier = 1.00f,
        .shieldChargePerAlien = 35.0f,
        .scoreMultiplier = 1.25f
    },
    {
        .name = "ACE",
        .initialSpawnInterval = 1.05f,
        .minimumSpawnInterval = 0.22f,
        .spawnAcceleration = 0.986f,
        .enemySpeedMultiplier = 1.18f,
        .shieldChargePerAlien = 25.0f,
        .scoreMultiplier = 1.60f
    }
};

void Settings_Default(GameSettings *settings)
{
    settings->musicEnabled = true;
    settings->sfxEnabled = true;
    settings->shipDesign = SHIP_RANGER;
    settings->difficulty = DIFFICULTY_PILOT;
}

void Settings_Load(GameSettings *settings, const char *path)
{
    Settings_Default(settings);

    FILE *file = fopen(path, "r");
    if (file == NULL) return;

    char key[64];
    int value;
    while (fscanf(file, "%63s %d", key, &value) == 2)
    {
        if (strcmp(key, "music") == 0) settings->musicEnabled = value != 0;
        else if (strcmp(key, "sfx") == 0) settings->sfxEnabled = value != 0;
        else if (strcmp(key, "ship") == 0 && value >= 0 && value < SHIP_DESIGN_COUNT)
            settings->shipDesign = (ShipDesign)value;
        else if (strcmp(key, "difficulty") == 0 && value >= 0 && value < DIFFICULTY_COUNT)
            settings->difficulty = (DifficultyLevel)value;
    }

    fclose(file);
}

bool Settings_Save(const GameSettings *settings, const char *path)
{
    char temporaryPath[256];
    snprintf(temporaryPath, sizeof(temporaryPath), "%s.tmp", path);

    FILE *file = fopen(temporaryPath, "w");
    if (file == NULL) return false;

    fprintf(file, "music %d\n", settings->musicEnabled ? 1 : 0);
    fprintf(file, "sfx %d\n", settings->sfxEnabled ? 1 : 0);
    fprintf(file, "ship %d\n", (int)settings->shipDesign);
    fprintf(file, "difficulty %d\n", (int)settings->difficulty);
    if (fclose(file) != 0)
    {
        remove(temporaryPath);
        return false;
    }

    if (rename(temporaryPath, path) == 0) return true;
    remove(path);
    return rename(temporaryPath, path) == 0;
}

const char *Settings_ShipName(ShipDesign design)
{
    switch (design)
    {
        case SHIP_RANGER: return "RANGER";
        case SHIP_STRIKER: return "STRIKER";
        case SHIP_COMET: return "COMET";
        default: return "RANGER";
    }
}

const DifficultyProfile *Settings_DifficultyProfile(DifficultyLevel difficulty)
{
    if (difficulty < 0 || difficulty >= DIFFICULTY_COUNT)
        difficulty = DIFFICULTY_PILOT;
    return &DIFFICULTIES[difficulty];
}
