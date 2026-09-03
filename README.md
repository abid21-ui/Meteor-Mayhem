# Meteor Mayhem: Arcade Reforged v2.3

A modular C/Raylib revival of the original **Meteor Mayhem** prototype.

## Features

- Momentum-based Asteroids-style movement with thrust acceleration, drift, and screen wrapping.
- Three-life heart system with a short safe respawn window.
- Gameplay ships, enemies, projectiles, shields, asteroids, and matching hitboxes
  are scaled to 1.5x for clearer action.
- Large brown alien battleships travel to a fully visible interior position,
  then stop, aim, and fire at the player.
- Asteroids in three sizes that split into smaller fragments.
- Two switchable player weapons:
  - **Laser:** fast fire rate, long range, one accurate bolt.
  - **Scattershot:** seven pellets, wide spread, short range, slower fire rate.
- Alien-core shield power-up:
  - Each destroyed alien adds 25-45% charge, based on difficulty.
  - Press `S` when the bar is full for eight seconds of invincibility.
  - Alien kills cannot recharge the shield while it is active.
  - Shield expiry grants 1.25 seconds of blinking invincibility frames.
  - Shielded collisions destroy asteroids and alien ships on contact.
  - Shield overdrive awards 1.5x points while active.
- Persistent high score and lifetime statistics.
- A persistent Settings screen with:
  - Independent music and sound-effect toggles.
  - Three selectable player ships: **Ranger**, **Striker**, and **Comet**.
  - Three difficulty levels with distinct risk/reward tuning.
- Background music, firing sound, and impact sound.

## Difficulty and rewards

| Difficulty | Initial / minimum spawn delay | Enemy speed | Shield charge per alien | Score multiplier |
| --- | ---: | ---: | ---: | ---: |
| Rookie | 1.70s / 0.50s | 0.90x | 45% | 1.00x |
| Pilot | 1.35s / 0.34s | 1.00x | 35% | 1.25x |
| Ace | 1.05s / 0.22s | 1.18x | 25% | 1.60x |

The spawn delay also decreases more aggressively during a run on higher
difficulties. Shield-overdrive scoring stacks with the difficulty multiplier,
so an Ace pilot can earn up to 2.40x points while the shield is active.

## Controls

| Action | Key |
| --- | --- |
| Rotate | `A` / `D` or left / right arrows |
| Thrust | `W` or up arrow |
| Fire | `Space` |
| Switch weapon | `Tab` |
| Activate full shield | `S` |
| Restart the current run | `R` |
| Return to menu | `Esc` |
| Navigate menus | Arrow keys or `W` / `S` |
| Change a setting | Left / right arrows or `A` / `D` |
| Select | `Enter` |

The main menu contains **Start Game**, **Settings**, **Statistics**, and **Quit**.
The statistics screen records high score, games played, asteroids smashed,
aliens smashed, and total accumulated points. Progress is stored locally in
`meteor_stats.dat`; options are stored in `meteor_settings.dat`.

## Build with CMake (recommended)

Requirements:

- A C11 compiler
- CMake 3.20 or newer
- Git if Raylib is not already installed

From the project directory:

```bash
cmake -S . -B build
cmake --build build --config Release
```

The build first looks for an installed Raylib 6.x. If it is unavailable, CMake
downloads Raylib 6.0 and builds it with the game. The `assets` and `sounds`
directories are copied beside the executable automatically.

### Windows with Code::Blocks MinGW

Open PowerShell in the project folder and run:

```powershell
cmake --fresh -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
.\build\meteor_mayhem.exe
```

Run the executable from the project folder as shown so both asset directories
are available. The build also copies them beside the executable.

Run on Linux/macOS:

```bash
./build/meteor_mayhem
```

For a multi-configuration Windows generator, the executable is commonly at:

```text
build/Release/meteor_mayhem.exe
```

## Build with Make

If Raylib is already installed and visible to your compiler:

```bash
make
make run
```

Run the game from the project directory so it can find `assets/` and `sounds/`.

## Project layout

```text
meteor-mayhem/
├── assets/           Generated and recovered visual assets
├── sounds/           Music and sound effects
├── include/
│   ├── assets.h      Asset and audio interface
│   ├── config.h      Tunable constants
│   ├── entities.h    Player, projectile, asteroid, and alien types
│   ├── game.h        Game state and public game interface
│   ├── settings.h    Settings and difficulty profiles
│   └── stats.h       Persistent statistics interface
├── src/
│   ├── assets.c      Asset loading and sound playback
│   ├── entities.c    Physics, spawning, and entity lifecycles
│   ├── game.c        Screens, combat, scoring, collisions, and rendering
│   ├── main.c        Window/audio setup and main loop
│   ├── settings.c    Persistent settings and difficulty balancing
│   └── stats.c       Safe statistics loading and saving
├── CMakeLists.txt
└── Makefile
```

## Balancing constants

The easiest gameplay values to adjust are in `include/config.h`. Difficulty
profiles live at the top of `src/settings.c`. Weapon cadence, projectile range,
movement acceleration, and score values are kept close to their relevant
functions in `src/game.c` and `src/entities.c`.
