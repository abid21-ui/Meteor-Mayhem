#ifndef METEOR_MAYHEM_STATS_H
#define METEOR_MAYHEM_STATS_H

#include <stdbool.h>
#include <stdint.h>

#define PILOT_NAME_MAX 16

typedef struct LifetimeStats {
    char pilotName[PILOT_NAME_MAX + 1];
    int highScore;
    int godModeHighScore;
    uint64_t gamesPlayed;
    uint64_t asteroidsSmashed;
    uint64_t aliensSmashed;
    uint64_t totalPoints;
} LifetimeStats;

void Stats_Load(LifetimeStats *stats, const char *path);
bool Stats_Save(const LifetimeStats *stats, const char *path);
bool Stats_HasProgress(const LifetimeStats *stats);

#endif
