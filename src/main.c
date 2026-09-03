#include "assets.h"
#include "config.h"
#include "game.h"
#include "raylib.h"

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Meteor Mayhem");
    InitAudioDevice();
    SetTargetFPS(60);

    GameAssets assets;
    if (!Assets_Load(&assets))
    {
        TraceLog(LOG_ERROR, "One or more required textures could not be loaded.");
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    Game game;
    Game_Init(&game, &assets);

    while (!WindowShouldClose() && !Game_ShouldQuit(&game))
    {
        Game_Update(&game);
        Game_Draw(&game);
    }

    Game_Shutdown(&game);
    Assets_Unload(&assets);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
