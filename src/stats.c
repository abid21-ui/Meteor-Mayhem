#include "stats.h"

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void Stats_Load(LifetimeStats *stats, const char *path)
{
    memset(stats, 0, sizeof(*stats));

    FILE *file = fopen(path, "r");
    if (file == NULL) return;

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        uint64_t value;
        if (sscanf(line, "high_score %" SCNu64, &value) == 1)
            stats->highScore = value > INT_MAX ? INT_MAX : (int)value;
        else if (sscanf(line, "god_mode_high_score %" SCNu64, &value) == 1)
            stats->godModeHighScore = value > INT_MAX ? INT_MAX : (int)value;
        else if (sscanf(line, "games_played %" SCNu64, &value) == 1)
            stats->gamesPlayed = value;
        else if (sscanf(line, "asteroids_smashed %" SCNu64, &value) == 1)
            stats->asteroidsSmashed = value;
        else if (sscanf(line, "aliens_smashed %" SCNu64, &value) == 1)
            stats->aliensSmashed = value;
        else if (sscanf(line, "total_points %" SCNu64, &value) == 1)
            stats->totalPoints = value;
        else if (strncmp(line, "pilot_name ", 11) == 0)
        {
            char *name = line + 11;
            name[strcspn(name, "\r\n")] = '\0';
            strncpy(stats->pilotName, name, PILOT_NAME_MAX);
            stats->pilotName[PILOT_NAME_MAX] = '\0';
        }
    }

    fclose(file);
}

bool Stats_Save(const LifetimeStats *stats, const char *path)
{
    char temporaryPath[256];
    snprintf(temporaryPath, sizeof(temporaryPath), "%s.tmp", path);

    FILE *file = fopen(temporaryPath, "w");
    if (file == NULL) return false;

    fprintf(file, "pilot_name %s\n", stats->pilotName);
    fprintf(file, "high_score %d\n", stats->highScore);
    fprintf(file, "god_mode_high_score %d\n", stats->godModeHighScore);
    fprintf(file, "games_played %" PRIu64 "\n", stats->gamesPlayed);
    fprintf(file, "asteroids_smashed %" PRIu64 "\n", stats->asteroidsSmashed);
    fprintf(file, "aliens_smashed %" PRIu64 "\n", stats->aliensSmashed);
    fprintf(file, "total_points %" PRIu64 "\n", stats->totalPoints);

    if (fclose(file) != 0) return false;

    if (rename(temporaryPath, path) == 0) return true;

    // Windows cannot replace an existing file with rename(). Keep this as a
    // fallback; POSIX systems take the atomic path above.
    if (remove(path) != 0) return false;
    return rename(temporaryPath, path) == 0;
}

bool Stats_HasProgress(const LifetimeStats *stats)
{
    return stats->highScore > 0 || stats->godModeHighScore > 0 || stats->gamesPlayed > 0 ||
           stats->asteroidsSmashed > 0 || stats->aliensSmashed > 0 ||
           stats->totalPoints > 0;
}
